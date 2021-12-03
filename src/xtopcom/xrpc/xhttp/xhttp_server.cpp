// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrpc/xhttp/xhttp_server.h"

#include "xbasic/xmemory.hpp"
#include "xmetrics/xmetrics.h"
#include "xrpc/xratelimit/xratelimit_data.h"
#include "xrpc/xratelimit/xratelimit_data_queue.h"

#include <iostream>
#include <regex>
NS_BEG2(top, xrpc)

HttpServer xhttp_server::m_server;
unique_ptr<xrpc_service<xedge_http_method>> xhttp_server::m_rpc_service = nullptr;
bool xhttp_server::m_is_running = false;

using namespace top::xChainRPC;
xhttp_server::xhttp_server(shared_ptr<xrpc_edge_vhost> edge_vhost,
                           common::xip2_t xip2,
                           bool archive_flag,
                           observer_ptr<store::xstore_face_t> store,
                           observer_ptr<base::xvblockstore_t> block_store,
                           observer_ptr<base::xvtxstore_t> txstore,
                           observer_ptr<elect::ElectMain> elect_main,
                           observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor) {
#if defined XDISABLE_RATELIMIT
    m_enable_ratelimit = false;
    xinfo("m_enable_ratelimit is false");
#endif
    if (m_rpc_service == nullptr) {
        m_rpc_service = top::make_unique<xrpc_service<xedge_http_method>>(edge_vhost, xip2, archive_flag, store, block_store, txstore, elect_main, election_cache_data_accessor);
    } else {
        m_rpc_service->reset_edge_method_mgr(edge_vhost, xip2);
    }
}

void xhttp_server::start(uint16_t nPort, uint32_t nThreadNum) {
    if (m_is_running) {
        xdbg("rpc_service http_server already started");
        return;
    }
    m_is_running = true;
    m_server.config.port = nPort;
    m_server.config.reuse_address = true;
    m_server.config.thread_pool_size = nThreadNum;

    m_server.resource["/"]["POST"] = std::bind(&xhttp_server::start_service, this, std::placeholders::_1, std::placeholders::_2);
    m_server.resource["/"]["OPTIONS"] = [](shared_ptr<HttpServer::Response> response, shared_ptr<HttpServer::Request>) {
        *response << "HTTP/1.1 200 OK\r\n";
        *response << "Access-Control-Allow-Origin:*\r\n";
        *response << "Access-Control-Request-Method:POST\r\n";
        *response << "Access-Control-Allow-Headers:Origin,X-Requested-With,Content-Type,Accept\r\n";
        *response << "Access-Control-Request-Headers:Content-type\r\n";
    };
    m_server.on_error = [](std::shared_ptr<HttpServer::Request>, const SimpleWeb::error_code & ec) {
        // Handle errors here
        // Note that connection timeouts will also call this handle with ec set to SimpleWeb::errc::operation_canceled
        xkinfo_rpc("on_error:%s,%d", ec.message().c_str(), ec.value());
    };
    m_server.io_service = m_rpc_service->m_io_service;
    m_server_thread = std::thread([self = shared_from_this()]() {
        // Start server
        self->m_server.start();
    });

    m_server_thread.detach();
    xdbg("rpc_service http_server started: %d", nPort);

    if (m_enable_ratelimit) {
        m_config.SetRatePerSecond(10 * 1000);
        m_config.SetBurstSize(12 * 1000);
        m_config.SetAllowBorrowSize(0);
        m_config.SetRequestToken(1000);
        m_ratelimit = top::make_unique<RatelimitServer>(m_config);
        m_ratelimit->RegistRequestOut([self = shared_from_this()](RatelimitData * data) {
            if (data == nullptr) {
                xdbg("rpc_service http_server null request");
                return;
            }
            asio::ip::address_v4 addr_v4(dynamic_cast<RatelimitDataHttp *>(data)->ip_);
            asio::ip::address addr(addr_v4);
            auto ip_s = addr.to_string();
            m_rpc_service->execute(dynamic_cast<RatelimitDataHttp *>(data)->response_, dynamic_cast<RatelimitDataHttp *>(data)->content_, ip_s);
            delete data;
        });
        m_ratelimit->RegistResponseOut([self = shared_from_this()](RatelimitData * data) {
            if (data == nullptr) {
                xdbg("rpc_service http_server null response");
                return;
            }
            RatelimitDataHttp * data_http = dynamic_cast<RatelimitDataHttp *>(data);
            if (data && data->err_ == static_cast<int>(RatelimitDispatch::CheckResult::Refused)) {
                std::string sequence_id = RatelimitServerHelper::GetSequenceId(data_http->content_);
                char info[256] = {0};
                snprintf(info, 256, "{\"errmsg\":\"request too often.\",\"errno\":110,\"sequence_id\":\"%s\"}", sequence_id.c_str());
                XMETRICS_PACKET_ALARM("rpc_ratelimit_alarm",
                                      "errmsg", "request too often",
                                      "sequence_id", sequence_id);
                data_http->response_->write(info);
            }
            delete data_http;
        });
    }
}

void xhttp_server::start_service(std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response,
                                 std::shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Request> request) {
    auto content = request->content.string();
    XMETRICS_COUNTER_INCREMENT("rpc_http_request", 1);
    asio::ip::address addr = request->remote_endpoint->address();
    if (m_enable_ratelimit) {
        // get ip
        uint ip{0};
        if (addr.is_v6()) {
            ip = addr.to_v6().to_v4().to_uint();
        } else {
            ip = addr.to_v4().to_uint();
        }

        // get account
        std::string account = RatelimitServerHelper::GetAccountAddress(content);
        RatelimitDataHttp * data = new RatelimitDataHttp();
        data->type_ = RatelimitData::TypeInOut::kIn;
        data->ip_ = ip;
        data->account_ = account;
        data->content_ = content;
        data->response_ = response;
        m_ratelimit->RequestIn(data);
    } else {
        // ipv4 expressed in ipv6 format: ::ffff:192.168.20.9
        auto ip_s = addr.to_string().substr(7);
        m_rpc_service->execute(response, content, ip_s);
    }
}

xhttp_server::~xhttp_server() {
    std::call_once(xrpc_once_flag::m_once_flag, [this]() { m_rpc_service->cancel_token_timeout(); });

    // m_server_thread.join();
}

NS_END2
