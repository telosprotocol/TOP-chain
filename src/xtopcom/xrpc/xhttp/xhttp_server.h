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
#include <string>
#include <thread>
#include "xrpc/xrpc_define.h"
#include "xrpc/xtraffic_controller.h"
#include "xrpc/xrule_manager.h"
#include "xrpc/xedge/xedge_method_manager.hpp"
#include "xrpc/prerequest/xpre_request_handler.h"
#include "simplewebserver/server_http.hpp"
#include "xrpc/xrpc_service.hpp"
#include "xrpc/xratelimit/xratelimit_server.h"
#include "xdata/xtransaction_cache.h"

NS_BEG2(top, xrpc)
using std::thread;
using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using top::xChainRPC::RatelimitConfig;
using top::xChainRPC::RatelimitServer;
class xhttp_server : public std::enable_shared_from_this<xhttp_server> {
public:
    xhttp_server(shared_ptr<xrpc_edge_vhost> edge_vhost, common::xip2_t xip2, bool archive_flag = false, observer_ptr<store::xstore_face_t> store = nullptr,
                 observer_ptr<base::xvblockstore_t> block_store = nullptr, observer_ptr<elect::ElectMain> elect_main = nullptr,
                 observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor = nullptr,
                 observer_ptr<data::xtransaction_cache_t> const & transaction_cache = nullptr);
    void start(uint16_t nPort, uint32_t nThreadNum = 1);
    void start_service(shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Response> response, shared_ptr<SimpleWeb::ServerBase<SimpleWeb::HTTP>::Request> request);
    ~xhttp_server();
    xedge_http_method* get_edge_method() { return m_rpc_service->m_edge_method_mgr_ptr.get(); }
private:
    static HttpServer                                   m_server;
    static unique_ptr<xrpc_service<xedge_http_method>>  m_rpc_service;
    static bool                                         m_is_running;
    thread                                          m_server_thread;
    RatelimitConfig                                 m_config;
    unique_ptr<RatelimitServer>                     m_ratelimit{ nullptr };
    bool                                            m_enable_ratelimit{ true };
};

NS_END2
