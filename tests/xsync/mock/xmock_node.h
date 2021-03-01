// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstore/xstore.h"
#include "xmbus/xmessage_bus.h"
#include "xvnetwork/xaddress.h"
#include "xmock_transport.h"
#include "xmock_vnet.hpp"
#include "xmock_vhost.hpp"
#include "xchain_timer/xchain_timer_face.h"

#include "xbase/xvledger.h"
#include "xblockstore/xblockstore_face.h"
#include "xsync/xsync_object.h"

namespace top { namespace mock {

class xmock_node_t {
public:
    xmock_node_t();
    ~xmock_node_t();

    void create_sync();

    void start();
    void stop();
    void update_chain_timer(const time::xchain_time_st & chain_current_time);

public:
    bool m_is_start{false};
    std::string m_vnode_id;
    top::vnetwork::xvnode_address_t m_addr;
    xvip2_t m_xip;
    std::string m_public_key;
    std::string m_private_key;
    xobject_ptr_t<base::xvnodesrv_t> m_nodesvr{};
    xobject_ptr_t<base::xvcertauth_t> m_certauth{};

    std::shared_ptr<xmock_vhost_t> m_vhost{nullptr};
    std::shared_ptr<xmock_vnet_t> m_vnet{nullptr};
    std::unique_ptr<mbus::xmessage_bus_face_t> m_mbus;
    xobject_ptr_t<store::xstore_face_t> m_store;
    xobject_ptr_t<base::xvblockstore_t> m_blockstore;
    xobject_ptr_t<time::xchain_time_face_t> m_chain_timer;

    xobject_ptr_t<base::xiothread_t> m_sync_thread{};
    std::vector<xobject_ptr_t<base::xiothread_t>> m_sync_account_thread_pool{};
    std::vector<xobject_ptr_t<base::xiothread_t>> m_sync_handler_thread_pool{};
    std::shared_ptr<sync::xsync_object_t> m_sync_obj{};
};

}
}
