// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xrpc/xws/xws_server.h"

#include "xbasic/xmemory.hpp"
#include "xmetrics/xmetrics.h"
#include "xrpc/xratelimit/xratelimit_data.h"
#include "xrpc/xratelimit/xratelimit_data_queue.h"

using namespace top::xChainRPC;
NS_BEG2(top, xrpc)

WsServer xws_server::m_server;
unique_ptr<xrpc_service<xedge_ws_method>> xws_server::m_rpc_service = nullptr;
bool xws_server::m_is_running = false;

xws_server::xws_server(shared_ptr<xrpc_edge_vhost> edge_vhost,
                       common::xip2_t xip2,
                       bool archive_flag,
                       observer_ptr<base::xvblockstore_t> block_store,
                       observer_ptr<base::xvtxstore_t> txstore,
                       observer_ptr<elect::ElectMain> elect_main,
                       observer_ptr<top::election::cache::xdata_accessor_face_t> const & election_cache_data_accessor) {
#if defined RPC_DISABLE_RATELIMIT
    m_enable_ratelimit = false;
#endif
    if (m_rpc_service == nullptr) {
        m_rpc_service = top::make_unique<xrpc_service<xedge_ws_method>>(edge_vhost, xip2, archive_flag, block_store, txstore, elect_main, election_cache_data_accessor);
    } else {
        m_rpc_service->reset_edge_method_mgr(edge_vhost, xip2);
    }
}
void xws_server::start(uint16_t nPort, uint32_t nThreadNum) {
    if (m_is_running) {
        xdbg("rpc_service ws_server already started");
        return;
    }
    m_is_running = true;
    m_server.config.port = nPort;
    m_server.config.thread_pool_size = nThreadNum;

    auto & service = m_server.endpoint["/"];
    service.on_message = std::bind(&xws_server::start_service, this, _1, _2);

    service.on_open = [](shared_ptr<WsServer::Connection> connection) {
        XMETRICS_GAUGE(metrics::rpc_ws_connect, 1);
        xinfo_rpc("Server: Opened connection %p", connection.get());
    };

    // See RFC 6455 7.4.1. for status codes
    service.on_close = [](shared_ptr<WsServer::Connection> connection, int status, const string & /*reason*/) {
        XMETRICS_GAUGE(metrics::rpc_ws_close, 1);
        xinfo_rpc("Server: Closed connection %p with status code %d", connection.get(), status);
    };

    service.on_error = [](shared_ptr<WsServer::Connection> connection, const SimpleWeb::error_code & ec) {
        xinfo_rpc("Server: Error in connection %p.Error:%d,error message: %s", connection.get(), ec, ec.message().c_str());
    };

    m_server.io_service = m_rpc_service->m_io_service;
    auto self = shared_from_this();
    m_server_thread = std::thread([self]() {
        // Start server
        self->m_server.start();
    });
    m_server_thread.detach();
    xdbg("rpc_service ws_server started %d", nPort);

    if (m_enable_ratelimit) {
        m_config.SetRatePerSecond(10 * 1000);
        m_config.SetBurstSize(12 * 1000);
        m_config.SetAllowBorrowSize(0);
        m_config.SetRequestToken(1000);
        m_ratelimit = top::make_unique<RatelimitServer>(m_config);
        auto self = shared_from_this();
        m_ratelimit->RegistRequestOut([self](RatelimitData * data) {
            if (data == nullptr) {
                xdbg("rpc_service ws_server null request");
                return;
            }
            asio::ip::address_v4 addr_v4(dynamic_cast<RatelimitDataWs *>(data)->ip_);
            asio::ip::address addr(addr_v4);
            auto ip_s = addr.to_string();
            m_rpc_service->execute(dynamic_cast<RatelimitDataWs *>(data)->connection_, dynamic_cast<RatelimitDataWs *>(data)->content_, ip_s);
            delete data;
        });
        m_ratelimit->RegistResponseOut([self](RatelimitData * data) {
            if (data == nullptr) {
                xdbg("rpc_service ws_server null response");
                return;
            }
            RatelimitDataWs * data_http = dynamic_cast<RatelimitDataWs *>(data);
            if (data && data->err_ == static_cast<int>(RatelimitDispatch::CheckResult::Refused)) {
                std::string sequence_id = RatelimitServerHelper::GetSequenceId(data_http->content_);
                char info[256] = {0};
                snprintf(info, 256, "{\"errmsg\":\"request too often.\",\"errno\":110,\"sequence_id\":\"%s\"}", sequence_id.c_str());
                XMETRICS_PACKET_ALARM("rpc_ratelimit_alarm",
                                      "errmsg", "request too often",
                                      "sequence_id", sequence_id);
                data_http->connection_->send(info);
            }
            delete data_http;
        });
    }
}

void xws_server::start_service(shared_ptr<WsServer::Connection> connection, shared_ptr<WsServer::InMessage> in_message) {
    auto content = in_message->string();
    XMETRICS_GAUGE(metrics::rpc_ws_request, 1);
    asio::ip::address addr = connection->remote_endpoint.address();
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
        RatelimitDataWs * data = new RatelimitDataWs();
        data->type_ = RatelimitData::TypeInOut::kIn;
        data->ip_ = ip;
        data->account_ = account;
        data->content_ = content;
        data->connection_ = connection;
        m_ratelimit->RequestIn(data);
    } else {
        // ipv4 expressed in ipv6 format: ::ffff:192.168.20.9
        auto ip_s = addr.to_string().substr(7);
        m_rpc_service->execute(connection, content, ip_s);
    }
}

xws_server::~xws_server() {
    std::call_once(xrpc_once_flag::m_once_flag, [this]() { m_rpc_service->cancel_token_timeout(); });
    // m_server_thread.join();
}

xedge_ws_method * xws_server::get_edge_method() {
    return m_rpc_service->m_edge_method_mgr_ptr.get();
}

NS_END2
