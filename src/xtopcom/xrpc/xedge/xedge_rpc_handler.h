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
#include "simplewebserver/server_http.hpp"
#include "simplewebsocketserver/server_ws.hpp"
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_msg_define.h"
#include "xedge_rpc_vhost.h"
#include "xedge_rpc_session.hpp"
#include "xedge_rpc_handler_base.hpp"

NS_BEG2(top, xrpc)
using http_response_t = SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response;
using WsServer = SimpleWeb::SocketServer<SimpleWeb::WS>;
using ws_connection_t = WsServer::Connection;


class xedge_http_handler : public xedge_handler_base<http_response_t> {
public:
    explicit xedge_http_handler(shared_ptr<xrpc_edge_vhost> edge_vhost, std::shared_ptr<asio::io_service> ioc,
                                observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
     :xedge_handler_base<http_response_t>(edge_vhost, ioc, election_cache_data_accessor) {}
    enum_xrpc_type type() override { return enum_xrpc_type::enum_xrpc_http_type; }
};

class xedge_ws_handler : public xedge_handler_base<ws_connection_t> {
public:
    explicit xedge_ws_handler(shared_ptr<xrpc_edge_vhost> edge_vhost, std::shared_ptr<asio::io_service> ioc,
                              observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor)
     :xedge_handler_base<ws_connection_t>(edge_vhost, ioc, election_cache_data_accessor) {}
    enum_xrpc_type type() override { return enum_xrpc_type::enum_xrpc_ws_type; }
};


NS_END2
