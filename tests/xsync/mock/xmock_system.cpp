// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xmock_system.h"
#include "xcommon/xnode_type.h"
#include "xchain_timer/xchain_timer.h"
#include "xcrypto/xckey.h"
#include "xbase/xutl.h"
#include "xcertauth/xcertauth_face.h"
#include "tests/mock/xvchain_creator.hpp"

namespace top { namespace mock {

using namespace store;
using namespace common;
using namespace data;
using namespace time;
using namespace base;

xmock_system_t::xmock_system_t(xmock_network_t &network) {

    m_thread = xiothread_t::create_thread(xcontext_t::instance(), 0, -1);
    assert(m_thread != nullptr);
    m_timer = new simple_timer(top::base::xcontext_t::instance(), m_thread->get_thread_id(), this);

    m_forward_thread = top::base::xiothread_t::create_thread(top::base::xcontext_t::instance(), 0, -1);
    m_forward_ptr = std::make_shared<xmock_transport_t>(m_forward_thread, std::bind(&xmock_system_t::get_vhost, this, std::placeholders::_1, std::placeholders::_2));

    std::vector<std::shared_ptr<xmock_node_info_t>> all_nodes = network.get_all_nodes();
    create_mock_node(all_nodes);
}

xmock_system_t::~xmock_system_t() {

    if (m_is_start) {
        stop();
    }

    m_timer->release_ref();
    m_thread->release_ref();
    m_forward_thread->release_ref();
}

void xmock_system_t::start() {
    if (!m_is_start) {

        for (auto it: m_addr_nodes) {
            std::shared_ptr<xmock_node_t> &node_ptr = it.second;
            node_ptr->start();
        }
        m_forward_ptr->start();
        m_timer->start(0, 10000);
        m_is_start = true;
    }
}

void xmock_system_t::stop() {

    if (m_is_start) {

        m_timer->close();
        while (m_timer->is_active()) {
            base::xtime_utl::sleep_ms(10);
        }
        base::xtime_utl::sleep_ms(100);
        m_thread->close();

        m_forward_ptr->stop();
        for (auto it: m_addr_nodes) {
            std::shared_ptr<xmock_node_t> &node_ptr = it.second;
            node_ptr->stop();
        }
        m_forward_thread->close();
        m_is_start = false;
    }
}

void xmock_system_t::on_timer() {
    if (m_is_start) {

        static common::xlogic_time_t chain_current_time;

        // chain_current_time.beacon_round++;
        // chain_current_time.beacon_height++;
        // chain_current_time.xtimestamp = base::xtime_utl::gmttime_ms();
        chain_current_time++;
        // chain_current_time.local_update_time = base::xtime_utl::gmttime_ms();

        for (auto it: m_addr_nodes) {
            std::shared_ptr<xmock_node_t> &node = it.second;
            node->update_chain_timer(chain_current_time);
        }
    }
}

std::shared_ptr<xmock_node_t> xmock_system_t::get_node(const std::string &node_name) {
    auto it = m_name_nodes.find(node_name);
    if (it == m_name_nodes.end())
        return nullptr;

    return it->second;
}

std::vector<std::shared_ptr<xmock_node_t>> xmock_system_t::get_group_node(const std::string &group_name) {
    std::vector<std::shared_ptr<xmock_node_t>> vector_nodes;
    auto it = m_name_group.find(group_name);
    if (it != m_name_group.end()) 
        vector_nodes = it->second;

    return vector_nodes;
}

void xmock_system_t::set_delay(uint32_t delay_ms) {
    m_forward_ptr->set_delay(delay_ms);
}

void xmock_system_t::set_packet_loss_rate(uint32_t rate) {
    m_forward_ptr->set_packet_loss_rate(rate);
}

void xmock_system_t::create_mock_node(std::vector<std::shared_ptr<xmock_node_info_t>> &all_nodes) {
    for (auto &it: all_nodes) {
        std::shared_ptr<xmock_node_info_t> &node_info = it;

        std::vector<xmock_node_info_t*> vector_neighbor_info = node_info->m_neighbor_nodes;
        std::vector<xmock_node_info_t*> vector_parent_info = node_info->m_parent_nodes;
        std::vector<xmock_node_info_t*> vector_child_info = node_info->m_child_nodes;

        std::vector<vnetwork::xvnode_address_t> all_addr;
        for (auto &it: node_info->m_all_nodes) {
            all_addr.push_back(it->m_addr);
        }

        std::vector<top::vnetwork::xvnode_address_t> all_archvie_addr;
        for (auto &it: node_info->m_archive_nodes) {
            all_archvie_addr.push_back(it->m_addr);
        }

        std::shared_ptr<xmock_node_t> node_ptr = std::make_shared<xmock_node_t>();

        node_ptr->m_vnode_id = node_info->m_vnode_id;
        node_ptr->m_addr = node_info->m_addr;
        node_ptr->m_xip = node_info->m_xip;
        node_ptr->m_public_key = node_info->m_public_key;
        node_ptr->m_private_key = node_info->m_private_key;
        node_ptr->m_nodesvr = node_info->m_nodesvr;
        node_ptr->m_certauth = node_info->m_certauth;

        std::shared_ptr<xmock_vhost_t> vhost_ptr = std::make_shared<xmock_vhost_t>(node_ptr->m_vnode_id, node_ptr->m_addr,
            std::bind(&xmock_transport_t::unicast, m_forward_ptr.get(), std::placeholders::_1, std::placeholders::_2, std::placeholders::_3),
            std::bind(&xmock_transport_t::broadcast, m_forward_ptr.get(), std::placeholders::_1, std::placeholders::_2));

        std::shared_ptr<xmock_vnet_t> vnet_ptr = std::make_shared<xmock_vnet_t>(node_ptr->m_addr,
                vector_neighbor_info, vector_parent_info, vector_child_info, all_addr, all_archvie_addr, vhost_ptr);

        vhost_ptr->attach(std::bind(&xmock_vnet_t::on_message, vnet_ptr.get(), std::placeholders::_1, std::placeholders::_2));

        node_ptr->m_vhost = vhost_ptr;
        node_ptr->m_vnet = vnet_ptr;
        node_ptr->m_mbus = top::make_unique<mbus::xmessage_bus_t>(true, 1000);

        mock::xvchain_creator creator;
        creator.create_blockstore_with_xstore();
        xobject_ptr_t<store::xstore_face_t> store;
        store.attach(creator.get_xstore());
        xobject_ptr_t<base::xvblockstore_t> blockstore;
        blockstore.attach(creator.get_blockstore());
  
        node_ptr->m_store = store;
        node_ptr->m_blockstore = blockstore;

        node_ptr->create_sync();

        vnetwork::xvnode_address_t addr = node_info->m_addr;

        m_addr_nodes[addr] = node_ptr;
        m_name_nodes[node_info->m_node_name] = node_ptr;

        {
            std::string group_name = node_info->m_group_name;
            auto it = m_name_group.find(group_name);
            if (it == m_name_group.end()) {
                std::vector<std::shared_ptr<xmock_node_t>> vector_nodes;
                vector_nodes.push_back(node_ptr);
                m_name_group[group_name] = vector_nodes;

            } else {
                it->second.push_back(node_ptr);
            }
        }

        {
            vnetwork::xvnode_address_t addr1 = top::vnetwork::address_cast<top::common::xnode_type_t::group>(addr);
            vnetwork::xcluster_address_t group_addr = addr1.cluster_address();
            auto it = m_addr_group.find(group_addr);
            if (it == m_addr_group.end()) {
                std::vector<std::shared_ptr<xmock_node_t>> vector_nodes;
                vector_nodes.push_back(node_ptr);
                m_addr_group[group_addr] = vector_nodes;
            } else {
                it->second.push_back(node_ptr);
            }
        }
    }
}

int32_t xmock_system_t::get_vhost(const top::common::xnode_address_t &addr, std::vector<std::shared_ptr<xmock_vhost_t>> &vector_vhost) {
    common::xnode_type_t type = addr.type();

    if (!addr.account_address().empty()) {

        auto it = m_addr_nodes.find(addr);
        if (it != m_addr_nodes.end()) {
            vector_vhost.push_back(it->second->m_vhost);
            return 0;
        }

    } else {
        auto group_addr = addr.cluster_address();
        auto it = m_addr_group.find(group_addr);
        if (it != m_addr_group.end()) {
            std::vector<std::shared_ptr<xmock_node_t>> &nodes = it->second;
            for (auto &it2: nodes) {
                vector_vhost.push_back(it2->m_vhost);
            }
            return 0;
        }
    }

    return -1;
}

}
}
