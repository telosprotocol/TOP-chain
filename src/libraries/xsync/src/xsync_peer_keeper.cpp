// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_peer_keeper.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

const uint32_t COMMON_TIME_INTERVAl = 180;
const uint32_t frozen_broadcast_factor = 10;

xsync_peer_keeper_t::xsync_peer_keeper_t(std::string vnode_id, xsync_store_face_t *sync_store, xrole_chains_mgr_t *role_chains_mgr,
    xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender, xsync_peerset_t *peerset):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_role_chains_mgr(role_chains_mgr),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender),
m_peerset(peerset) {
}

void xsync_peer_keeper_t::on_timer() {
    XMETRICS_TIME_RECORD("sync_cost_peerkeeper_timer_event");

    std::unique_lock<std::mutex> lock(m_lock);
    if (m_time_rejecter.reject()){
        return;
    }

    if (m_count % COMMON_TIME_INTERVAl == 0) {
        process_timer();
        m_count = 1;
    } else {
        m_count++;
    }
}

std::vector<vnetwork::xvnode_address_t> xsync_peer_keeper_t::get_random_neighbors(const vnetwork::xvnode_address_t& addr) const {
    std::vector<vnetwork::xvnode_address_t> all_neighbors;
    // rec,zec,consensus,archive
    if (common::has<common::xnode_type_t::rec>(addr.type()) || common::has<common::xnode_type_t::zec>(addr.type()) ||
        common::has<common::xnode_type_t::consensus>(addr.type()) || common::has<common::xnode_type_t::storage>(addr.type())) {
        all_neighbors = m_role_xips_mgr->get_all_neighbors(addr);
    }

    std::vector<vnetwork::xvnode_address_t> random_neighbors;
    // sqrt(N)*N < N*N, but it is enough
    uint32_t select = sqrt(all_neighbors.size()) + 1;
    uint32_t size = all_neighbors.size();
    for (uint32_t i = 0; i < all_neighbors.size(); ++i) {
        uint32_t idx = RandomUint32() % (size - i);
        if (idx < select) {
            random_neighbors.push_back(all_neighbors[i]);
            select--;
        }
    }

    return random_neighbors;
}

void xsync_peer_keeper_t::add_role(const vnetwork::xvnode_address_t& addr) {

    XMETRICS_TIME_RECORD("sync_cost_peerkeeper_add_role_event");

    {
        std::unique_lock<std::mutex> lock(m_lock);
        for (auto &it: m_maps) {
            // remove old
            if (it.first.cluster_address() == addr.cluster_address()) {
                m_peerset->remove_group(it.first);
                m_maps.erase(it.first);
                break;
            }
        }
    }

    auto random_neighbors = get_random_neighbors(addr);
    std::vector<vnetwork::xvnode_address_t> add_list;
    std::unique_lock<std::mutex> lock(m_lock);
    if (m_maps.find(addr) == m_maps.end()) {
        std::set<vnetwork::xvnode_address_t> peer_group;
        for (auto &neighbor: random_neighbors) {
            peer_group.insert(neighbor);
            add_list.push_back(neighbor);
        }
        m_maps.insert(std::make_pair(addr, peer_group));
    } else {
        xsync_warn("xsync_peer_keeper add_role exist %s", addr.to_string().c_str());
        return;
    }

    m_peerset->add_group(addr);
    for (auto &it: add_list) {
        m_peerset->add_peer(addr, it);
    }

    std::vector<vnetwork::xvnode_address_t> peers_neighbors;
    m_peerset->get_group_nodes(addr, peers_neighbors);
    xsync_info("xsync_peer_keeper add_role neighbor count is %u %s", peers_neighbors.size(), addr.to_string().c_str());
    for (auto &it: peers_neighbors) {
        xsync_info("xsync_peer_keeper add_role %s", it.to_string().c_str());
    }

    std::shared_ptr<xrole_chains_t> role_chains = m_role_chains_mgr->get_role(addr);
    if (role_chains == nullptr)
        return;

    walk_role(addr, m_maps[addr], role_chains);
}

void xsync_peer_keeper_t::remove_role(const vnetwork::xvnode_address_t& addr) {

    xsync_info("xsync_peer_keeper remove_role %s", addr.to_string().c_str());

    std::unique_lock<std::mutex> lock(m_lock);
    m_maps.erase(addr);
    m_peerset->remove_group(addr);
}

void xsync_peer_keeper_t::handle_message(const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address,
        const std::vector<xchain_state_info_t> &info_list) {

    m_peerset->update(network_self, from_address, info_list);
}

uint32_t xsync_peer_keeper_t::get_frozen_broadcast_factor() {
    return frozen_broadcast_factor;
}

void xsync_peer_keeper_t::walk_role(const vnetwork::xvnode_address_t &self_addr, const std::set<vnetwork::xvnode_address_t> &target_list, const std::shared_ptr<xrole_chains_t> &role_chains) {

    std::vector<xchain_state_info_t> info_list;

    const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
    for (const auto &it: chains) {

        const std::string &address = it.first;
        const xchain_info_t &chain_info = it.second;
        xchain_state_info_t info;
        info.address = address;
        if (chain_info.sync_policy == enum_chain_sync_policy_fast) {
            base::xauto_ptr<base::xvblock_t> latest_start_block = m_sync_store->get_latest_start_block(address, chain_info.sync_policy);
            if (latest_start_block == nullptr || !latest_start_block->is_full_state_block()) {
                info.start_height = 0;
                info.end_height = 0;
            } else {
                info.start_height = latest_start_block->get_height();
                info.end_height = m_sync_store->get_latest_end_block_height(address, chain_info.sync_policy);
            }
        } else {
            info.start_height = m_sync_store->get_latest_start_block_height(address, chain_info.sync_policy);
            info.end_height = m_sync_store->get_latest_end_block_height(address, chain_info.sync_policy);
        }
        info_list.push_back(info);
    }

    if (common::has<common::xnode_type_t::frozen>(self_addr.type())) {
        send_frozen_chain_state(self_addr, info_list);
    } else {
        send_chain_state(self_addr, target_list, info_list);
    }
}

void xsync_peer_keeper_t::process_timer() {

    for (auto &it: m_maps) {
        const vnetwork::xvnode_address_t &self_addr = it.first;
        std::set<vnetwork::xvnode_address_t> &peers = it.second;
        std::shared_ptr<xrole_chains_t> role_chains = m_role_chains_mgr->get_role(self_addr);
        if (role_chains == nullptr)
            continue;
        walk_role(self_addr, peers, role_chains);
    }
}

void xsync_peer_keeper_t::send_chain_state(const xvnode_address_t &self_addr, const std::set<vnetwork::xvnode_address_t> &target_list, std::vector<xchain_state_info_t> &info_list) {

    if (info_list.empty())
        return;

    XMETRICS_COUNTER_INCREMENT("sync_broadcast_chain_state_send", 1);

    for (const auto &it : target_list) {
        m_sync_sender->send_broadcast_chain_state(info_list, self_addr, it);
    }
}

void xsync_peer_keeper_t::send_frozen_chain_state(const xvnode_address_t &self_addr, std::vector<xchain_state_info_t> &info_list) {

    if (info_list.empty())
        return;

    XMETRICS_COUNTER_INCREMENT("sync_frozen_broadcast_chain_state_send", 1);

    uint32_t factor = get_frozen_broadcast_factor();

    for (uint32_t i=0; i<factor; i++) {
        m_sync_sender->send_frozen_broadcast_chain_state(info_list, self_addr);
    }
}

NS_END2
