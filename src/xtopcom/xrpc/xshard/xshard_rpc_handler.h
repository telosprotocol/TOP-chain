// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#ifndef ASIO_STANDALONE
#    define ASIO_STANDALONE
#endif
#ifndef USE_STANDALONE_ASIO
#    define USE_STANDALONE_ASIO
#endif
#include "xbasic/xmemory.hpp"
#include "xbasic/xobject_ptr.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xrpc/xrpc_define.h"
#include "xrpc/xrpc_msg_define.h"
#include "xrpc/xrule_manager.h"
#include "xtxpool_service/xtxpool_service_face.h"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvhost_face.h"

NS_BEG2(top, xrpc)
using vnetwork::xmessage_t;
using vnetwork::xvhost_face_t;
using vnetwork::xvnetwork_driver_face_t;
using vnetwork::xvnode_address_t;

class xshard_rpc_handler : public std::enable_shared_from_this<xshard_rpc_handler> {
public:
    explicit xshard_rpc_handler(std::shared_ptr<xvnetwork_driver_face_t>        shard_host,
                                xtxpool_service::xtxpool_proxy_face_ptr const & txpool_service,
                                observer_ptr<top::base::xiothread_t>            thread);
    void on_message(const xvnode_address_t & edge_sender, xmessage_t const & message, std::uint64_t const timer_height);
    void shard_process_request(const xrpc_msg_request_t & edge_msg, const xvnode_address_t & edge_sender, const uint64_t msghash);
    void process_msg(const xrpc_msg_request_t & edge_msg, xjson_proc_t & json_proc);
    void start();
    void stop();

public:
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> m_shard_vhost;
    unique_ptr<xfilter_manager>                        m_rule_mgr_ptr;
    xtxpool_service::xtxpool_proxy_face_ptr            m_txpool_service;
    observer_ptr<top::base::xiothread_t>               m_thread;
};

NS_END2
