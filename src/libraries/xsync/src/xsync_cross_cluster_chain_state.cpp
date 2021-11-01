// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_cross_cluster_chain_state.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

xsync_cross_cluster_chain_state_t::xsync_cross_cluster_chain_state_t(std::string vnode_id, xsync_store_face_t* sync_store,
            xrole_chains_mgr_t *role_chains_mgr, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender, xdownloader_face_t *downloader):
m_vnode_id(vnode_id),
m_sync_store(sync_store),
m_role_chains_mgr(role_chains_mgr),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender),
m_downloader(downloader) {

}

void xsync_cross_cluster_chain_state_t::on_timer() {

    if (m_time_rejecter.reject()){
        return;
    }

    // 10 min
    m_counter++;
    if (m_counter < 600)
        return;
    m_counter = 0;

    XMETRICS_TIME_RECORD("sync_cost_cross_cluster_chain_state_timer_event");

    xsync_roles_t roles = m_role_chains_mgr->get_roles();
    for (const auto &role_it: roles) {

        const vnetwork::xvnode_address_t &self_addr = role_it.first;
        const std::shared_ptr<xrole_chains_t> &role_chains = role_it.second;

        common::xnode_type_t node_type = self_addr.type();
        std::map<enum_chain_sync_policy, std::vector<vnetwork::xvnode_address_t>> archives;

        enum_chain_sync_policy sync_policy = enum_chain_sync_policy_fast;
        std::vector<vnetwork::xvnode_address_t> archive_list;

        if (common::has<common::xnode_type_t::rec>(node_type) || common::has<common::xnode_type_t::zec>(node_type) ||
            common::has<common::xnode_type_t::consensus>(node_type)) {
            std::vector<vnetwork::xvnode_address_t> nodes;
            auto inserted = m_role_xips_mgr->get_rand_archives(1);
            nodes.insert(nodes.end(), inserted.begin(), inserted.end());
            inserted = m_role_xips_mgr->get_edge_archive_list();
            nodes.insert(nodes.end(), inserted.begin(), inserted.end());
            archives[enum_chain_sync_policy_fast] = nodes;
        } else if (common::has<common::xnode_type_t::storage_archive>(node_type)) {
            archives[enum_chain_sync_policy_full] = m_role_xips_mgr->get_edge_archive_list();
        } else {
            continue;
        }

        for (auto archive : archives) {
            std::vector<xchain_state_info_t> info_list;
            const map_chain_info_t &chains = role_chains->get_chains_wrapper().get_chains();
            for (const auto &it: chains) {

                const std::string &address = it.first;
                const xchain_info_t &chain_info = it.second;

                xchain_state_info_t info;
                info.address = address;
                info.start_height = m_sync_store->get_latest_start_block_height(address, archive.first);
                info.end_height = m_sync_store->get_latest_end_block_height(address, archive.first);
                info_list.push_back(info);
            }

            xsync_info("xsync_cross_cluster_chain_state_t on_timer send info exchange %s count(%d)", self_addr.to_string().c_str(), info_list.size());
            if (info_list.empty()) {
                continue;
            }

            for (auto &it: archive.second)
                m_sync_sender->send_cross_cluster_chain_state(info_list, self_addr, it);
        }
    }
}

void xsync_cross_cluster_chain_state_t::handle_message(const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address,
        const std::vector<xchain_state_info_t> &info_list) {

    std::string reason = "cross_cluster";

    for (const auto &it: info_list) {
        const xchain_state_info_t &info = it;

        const std::string &address = info.address;
        uint64_t peer_start_height = info.start_height;
        uint64_t peer_end_height = info.end_height;

        if (!m_role_chains_mgr->exists(address)) {
            xsync_warn("xsync_cross_cluster_chain_state_t local not exist %s peer(start_height=%lu,end_height=%lu)",
                            address.c_str(), peer_start_height, peer_end_height);
            continue;
        }

        enum_chain_sync_policy sync_policy = enum_chain_sync_policy_fast;
        if (common::has<common::xnode_type_t::storage_full_node>(network_self.type()) && common::has<common::xnode_type_t::storage_archive>(from_address.type())) {
            sync_policy = enum_chain_sync_policy_full;
        }

        uint64_t latest_end_block_height = m_sync_store->get_latest_end_block_height(address, sync_policy);
        if (peer_end_height > latest_end_block_height) {
            xsync_info("cross_cluster_chain_state notify %s,local(start_height=%lu,end_height=%lu) peer(start_height=%lu,end_height=%lu)", 
                        address.c_str(), m_sync_store->get_latest_start_block_height(address, sync_policy), latest_end_block_height, peer_start_height, peer_end_height);

            mbus::xevent_ptr_t ev = make_object_ptr<mbus::xevent_behind_download_t>(address, 
                    peer_start_height, peer_end_height, sync_policy, network_self, from_address, reason);
            m_downloader->push_event(ev);
        }
    }
}

NS_END2
