// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#ifndef ASIO_STANDALONE
#define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#define USE_STANDALONE_ASIO
#endif
#include <type_traits>
#include "simplewebserver/server_http.hpp"
#include "simplewebsocketserver/server_ws.hpp"
#include "xvnetwork/xaddress.h"
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_msg_define.h"
#include "xrpc/xerror/xrpc_error_json.h"
#include "xrpc/xerror/xrpc_error_code.h"
#include "xrpc/xrpc_method.h"

NS_BEG2(top, xrpc)
using vnetwork::xvnode_address_t;
using std::error_code;
using http_response_t = SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using ws_connection_t = WsServer::Connection;

#define TIME_OUT 10

template<class T>
class xedge_handler_base;

template<class T>
class xforward_session_base : public std::enable_shared_from_this<xforward_session_base<T>>
{
public:
    xforward_session_base(const std::vector<shared_ptr<xrpc_msg_request_t>>& edge_msg_ptr_list, xedge_handler_base<T>* edge_handler_ptr, shared_ptr<T>& response, std::shared_ptr<asio::io_service>& ioc, const unordered_set<xvnode_address_t>& addr_set)
        :m_edge_msg_ptr_list(edge_msg_ptr_list), m_edge_handler_ptr(edge_handler_ptr), m_response(response), m_ioc(ioc), m_destination_address(addr_set) {}
    virtual ~xforward_session_base() {}
    virtual void write_response(const string&) = 0;
    void set_timeout(long seconds) noexcept;
    void cancel_timeout() noexcept;
public:
    std::vector<std::shared_ptr<xrpc_msg_request_t>>    m_edge_msg_ptr_list;
    xedge_handler_base<T>*                              m_edge_handler_ptr;
    shared_ptr<T>                                       m_response;
    unique_ptr<asio::steady_timer>                      m_timer;
    shared_ptr<asio::io_service>&                       m_ioc;
    int16_t                                             m_retry_num{ 1 };
    unordered_set<xvnode_address_t>                     m_destination_address{};
};

template<class T>
void xforward_session_base<T>::set_timeout(long seconds) noexcept
{
    if (seconds == 0) {
        m_timer = nullptr;
        return;
    }
    if (m_timer == nullptr) {
        m_timer = std::unique_ptr<asio::steady_timer>(new asio::steady_timer(*m_ioc));
    }
    m_timer->expires_from_now(std::chrono::seconds(seconds));
    auto self(this->shared_from_this());
    m_timer->async_wait([self](const error_code &ec) {
        if (!ec) {
            //  std::unique_lock<std::mutex> lock(session_mgr->m_mutex);
            if (self->m_retry_num-- > 0) {
                self->m_edge_handler_ptr->edge_send_msg(self->m_edge_msg_ptr_list, "", "");
                self->set_timeout(TIME_OUT);
            } else {
                xrpc_error_json error_json(static_cast<uint32_t>(enum_xrpc_error_code::rpc_shard_exec_error), RPC_TIMEOUT_MSG, self->m_edge_msg_ptr_list.front()->m_client_id);
                self->write_response(error_json.write());
            }
        }
    });
}
template<class T>
void xforward_session_base<T>::cancel_timeout() noexcept
{
    if (m_timer) {
        error_code ec;
        m_timer->cancel(ec);
    }
}

template<typename>
class forward_session;

template<>
class forward_session<http_response_t> : public xforward_session_base<http_response_t> {
public:
    forward_session(const std::vector<shared_ptr<xrpc_msg_request_t>>& edge_msg_ptr_list, xedge_handler_base<http_response_t>* edge_handler_ptr, shared_ptr<http_response_t>& response, std::shared_ptr<asio::io_service>& ioc, const unordered_set<xvnode_address_t>& addr_set)
        :xforward_session_base<http_response_t>(edge_msg_ptr_list, edge_handler_ptr, response, ioc, addr_set) {}
    ~forward_session() {}
    void write_response(const string& response) override
    {
        m_response->write(response);
    }
};

template<>
class forward_session<ws_connection_t> : public xforward_session_base<ws_connection_t> {
public:
    forward_session(const std::vector<shared_ptr<xrpc_msg_request_t>>& edge_msg_ptr_list, xedge_handler_base<ws_connection_t>* edge_handler_ptr, shared_ptr<ws_connection_t>& response, std::shared_ptr<asio::io_service>& ioc, const unordered_set<xvnode_address_t>& addr_set)
        :xforward_session_base<ws_connection_t>(edge_msg_ptr_list, edge_handler_ptr, response, ioc, addr_set) {}
    ~forward_session() {}
    void write_response(const string& response) override
    {
        m_response->send(response, [](const error_code &ec) {
            if (ec) {
                xkinfo_rpc("Server: Error sending message. Error: %d, error message:%s", ec.value(), ec.message().c_str());
            }
        });
    }
};

NS_END2
