// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xrpc/xhttp/xhttp_server.h"
#include "xrpc/xws/xws_server.h"
#include "xrpc/xshard/xshard_rpc_handler.h"
#include "xrpc/xcluster/xcluster_rpc_handler.h"
#include "xrpc/xarc/xarc_rpc_handler.h"
#include "xbase/xobject_ptr.h"
#include "xtxstore/xtxstore_face.h"

NS_BEG2(top, xrpc)
class xrpc_init
{
public:
    xrpc_init(std::shared_ptr<xvnetwork_driver_face_t> vhost,
              const common::xnode_type_t node_type,
              observer_ptr<xrouter_face_t> router_ptr,
              const uint16_t http_port,
              const uint16_t ws_port,
              xtxpool_service_v2::xtxpool_proxy_face_ptr const &  txpool_service,
              observer_ptr<store::xstore_face_t> const & store,
              observer_ptr<base::xvblockstore_t> const & block_store,
              observer_ptr<base::xvtxstore_t> const & txstore,
              observer_ptr<elect::ElectMain> elect_main,
              observer_ptr<election::cache::xdata_accessor_face_t> const & election_cache_data_accessor);

    void stop();

    static void init_rpc_cb_thread();

private:
    shared_ptr<xshard_rpc_handler>   m_shard_handler{ nullptr };
    shared_ptr<xcluster_rpc_handler> m_cluster_handler{ nullptr };
    shared_ptr<xarc_rpc_handler>     m_arc_handler{ nullptr };
    shared_ptr<xrpc_edge_vhost>      m_edge_handler{ nullptr };
    static top::base::xiothread_t*   m_thread;
};

class rpc_message_para_t : public top::base::xobject_t {
public:
    rpc_message_para_t(vnetwork::xvnode_address_t const & sender, vnetwork::xmessage_t const & message, uint64_t const timer_height = 0):
                       m_sender(sender),
                       m_message(message),
                       m_timer_height(timer_height)
{}

private:
    ~rpc_message_para_t() override {}

public:
    vnetwork::xvnode_address_t m_sender;
    vnetwork::xmessage_t       m_message;
    uint64_t                   m_timer_height;
};

NS_END2
