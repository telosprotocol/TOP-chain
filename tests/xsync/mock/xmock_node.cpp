// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xmemory.hpp"
#include "xmock_node.h"
#include "xmbus/xevent_behind.h"
#include "xconfig/xconfig_register.h"
#include "xloader/xconfig_onchain_loader.h"
#include "xmbus/xevent_role.h"

namespace top { namespace mock {

using namespace base;
using namespace vnetwork;
using namespace store;
using namespace data;
using namespace sync;

///////////

xmock_node_t::xmock_node_t() {

#if 0
    static bool s_is_loaded = false;

    if (!s_is_loaded) {
        auto& config_center = top::config::xconfig_register_t::get_instance();
        auto loader = std::make_shared<loader::xconfig_onchain_loader_t>(make_observer(m_infra.store_ptr), make_observer(infra.mbus_ptr.get()), make_observer(m_infra.chain_timer_ptr.get()));
        config_center.add_loader(loader);
        config_center.init_static_config();
        config_center.load();

        s_is_loaded = true;
    }
#endif
}

xmock_node_t::~xmock_node_t() {
    stop();
    // destroy timer thread
    m_blockstore->close();
}

void xmock_node_t::create_sync() {
    m_sync_thread = make_object_ptr<base::xiothread_t>();

    std::vector<observer_ptr<base::xiothread_t>> sync_account_thread_pool;
    for (uint32_t i = 0; i < 2; i++) {
        xobject_ptr_t<base::xiothread_t> thread = make_object_ptr<base::xiothread_t>();
        m_sync_account_thread_pool.push_back(thread);
        sync_account_thread_pool.push_back(make_observer(thread));
    }

    std::vector<observer_ptr<base::xiothread_t>> sync_handler_thread_pool;
    for (uint32_t i = 0; i < 2; i++) {
        xobject_ptr_t<base::xiothread_t> thread = make_object_ptr<base::xiothread_t>();
        m_sync_handler_thread_pool.push_back(thread);
        sync_handler_thread_pool.push_back(make_observer(thread));
    }

    m_sync_obj = std::make_shared<sync::xsync_object_t>(make_observer(m_mbus), make_observer(m_store), make_observer(m_vhost), m_blockstore, m_nodesvr, m_certauth,
                                    make_observer(m_sync_thread), sync_account_thread_pool, sync_handler_thread_pool);
}

void xmock_node_t::start() {

    if (m_is_start)
        return;

    m_sync_obj->start();
    m_sync_obj->add_vnet(m_vnet);

    m_is_start = true;
}

void xmock_node_t::stop() {

    if (m_is_start) {
        
        m_sync_obj->remove_vnet(m_vnet);
        m_sync_obj->stop();

        m_sync_thread->close();
        for (auto it: m_sync_account_thread_pool)
            it->close();
        for (auto it: m_sync_handler_thread_pool)
            it->close();

        m_is_start = false;
    }
}

void xmock_node_t::update_chain_timer(common::xlogic_time_t chain_current_time) {
    //m_chain_timer->update_time(chain_current_time.xtime_round);
    //m_chain_timer->update_time(1);
}

}
}
