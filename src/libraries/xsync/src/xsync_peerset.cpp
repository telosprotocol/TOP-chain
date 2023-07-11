// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_peerset.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

const uint32_t frozen_role_limit = 100;

xsync_peerset_t::xsync_peerset_t(std::string vnode_id):
m_vnode_id(vnode_id) {

}

void xsync_peerset_t::add_group(const vnetwork::xvnode_address_t &self_address) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_multi_role_peers.find(self_address);
    if (it == m_multi_role_peers.end()) {
        xsync_role_peers_t peers;
        m_multi_role_peers[self_address] = peers;
    }
}

void xsync_peerset_t::remove_group(const vnetwork::xvnode_address_t &self_address) {
    std::unique_lock<std::mutex> lock(m_lock);
    m_multi_role_peers.erase(self_address);
}

void xsync_peerset_t::add_peer(const vnetwork::xvnode_address_t &self_address, const vnetwork::xvnode_address_t &peer_address) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_multi_role_peers.find(self_address);
    xassert(it != m_multi_role_peers.end());
    xsync_peer_chains_t peer_info;
    it->second[peer_address] = peer_info;
}

void xsync_peerset_t::remove_peer(const vnetwork::xvnode_address_t &self_address, const vnetwork::xvnode_address_t &peer_address) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_multi_role_peers.find(self_address);
    if (it != m_multi_role_peers.end()) {
        it->second.erase(peer_address);
    }
}

void xsync_peerset_t::frozen_add_peer(xsync_role_peers_t &peers, const vnetwork::xvnode_address_t &peer_address) {
    if (peers.find(peer_address) == peers.end()) {
        while (peers.size() >= frozen_role_limit) {
            // random remove one
            uint32_t rand = RandomUint32()%peers.size();
            uint32_t i = 0;
            for (auto ip = peers.begin(); ip!=peers.end(); ip++, i++) {
                if (i == rand) {
                    peers.erase(ip);
                    break;
                }
            }
        }

        xsync_peer_chains_t peer_info;
        peers[peer_address] = peer_info;
    }
}

// TODO consider gossip
void xsync_peerset_t::update(const vnetwork::xvnode_address_t &self_address, const vnetwork::xvnode_address_t &peer_address, const std::vector<xchain_state_info_t> &info_list) {

    std::unique_lock<std::mutex> lock(m_lock);

    auto it = m_multi_role_peers.find(self_address);
    if (it == m_multi_role_peers.end())
        return;

    // frozen role won't call add_peer(func).
    if (common::has<common::xnode_type_t::frozen>(self_address.type())) {
        frozen_add_peer(it->second, peer_address);
    }

    // here, peer must exist
    xsync_role_peers_t &peers = it->second;
    if (peers.find(peer_address) != peers.end()) {
        xsync_peer_chains_t &peer_chains = peers[peer_address];
        for (auto &info: info_list) {
            auto it3 = peer_chains.find(info.address);
            if (it3 == peer_chains.end()) {
                xsync_chain_info_t chaininfo(info.start_height, info.end_height);
                xsync_dbg("peerset update %s,start_height=%lu,end_height=%lu,%s",
                    info.address.c_str(), info.start_height, info.end_height, peer_address.to_string().c_str());
                peer_chains.insert(std::make_pair(info.address, chaininfo));
            } else {
                // TODO compare height and viewid
                if (info.end_height != it3->second.end_height) {
                    xsync_dbg("peerset update %s,start_height=%lu,end_height=%lu,%s",
                        info.address.c_str(), info.start_height, info.end_height, peer_address.to_string().c_str());
                    it3->second.update(info.start_height, info.end_height);
                }
            }
        }
    } else {
        xsync_dbg("peerset update %s not exist.", peer_address.to_string().c_str());
    }
}

bool xsync_peerset_t::get_newest_peer(const vnetwork::xvnode_address_t &self_address, const std::string &address, uint64_t &start_height, 
                                      uint64_t &end_height, vnetwork::xvnode_address_t &peer_addr, bool random_check) {

    std::unique_lock<std::mutex> lock(m_lock);

    auto it = m_multi_role_peers.find(self_address);
    if (it == m_multi_role_peers.end())
        return false;

    uint64_t local_start_height = start_height; 
    uint64_t local_end_height = end_height;
 
    xsync_role_peers_t& peers = it->second;
    auto it2 = peers.begin();

    if (random_check && peers.size() > 1) {
        uint32_t random_idx = top::base::xtime_utl::get_fast_randomu() % peers.size();
        std::advance(it2, random_idx);
    }

    for (uint32_t i = 0; i < peers.size(); i++, it2++) {
        if (it2 == peers.end())
            it2 = peers.begin();

        xsync_peer_chains_t& chains = it2->second;

        auto it3 = chains.find(address);
        if (it3 == chains.end())
            continue;

        xsync_chain_info_t &info = it3->second;
        if (info.end_height > end_height) {
            start_height = info.start_height;
            end_height = info.end_height;
            peer_addr = it2->first;
            continue;
        }
    }
    if(local_start_height != start_height || local_end_height!= end_height) {
        return true;
    } 
    return false;
}

bool xsync_peerset_t::get_group_size(const vnetwork::xvnode_address_t &self_address, uint32_t &count) {
    std::unique_lock<std::mutex> lock(m_lock);

    auto it = m_multi_role_peers.find(self_address);
    if (it == m_multi_role_peers.end())
        return false;

    count = it->second.size();
    return true;
}

uint32_t xsync_peerset_t::get_frozen_limit() {
    return frozen_role_limit;
}

bool xsync_peerset_t::get_archive_group(vnetwork::xvnode_address_t &self_addr, std::vector<vnetwork::xvnode_address_t> &neighbors) {

    std::unique_lock<std::mutex> lock(m_lock);
    for (auto &it: m_multi_role_peers) {
        const vnetwork::xvnode_address_t &addr = it.first;
        if (common::has<common::xnode_type_t::storage>(addr.type())) {

            self_addr = addr;
            for (auto &it2: it.second)
                neighbors.push_back(it2.first);

            return true;
        }
    }

    return false;
}

bool xsync_peerset_t::get_group_nodes(const vnetwork::xvnode_address_t &self_addr, std::vector<vnetwork::xvnode_address_t> &neighbors) {

    std::unique_lock<std::mutex> lock(m_lock);

    if (m_multi_role_peers.find(self_addr) != m_multi_role_peers.end()) {
        auto &role_peers = m_multi_role_peers[self_addr];
        for (auto &peer: role_peers)
            neighbors.push_back(peer.first);
        return true;
    }

    return false;
}

std::map<std::string, std::vector<std::string>> xsync_peerset_t::get_neighbors() {
    std::map<std::string, std::vector<std::string>> all_neighbors;
    for(auto role_peers : m_multi_role_peers){
        std::vector<std::string> neighbors;
        auto peers = role_peers.second;
        for(auto peer : peers){
            neighbors.push_back(peer.first.to_string());
        }
        all_neighbors[role_peers.first.to_string()] = neighbors;
    }

    return all_neighbors;
}

bool xsync_peerset_t::get_peer_height_info_map(const vnetwork::xvnode_address_t& self_address, const std::string& address,
    const uint64_t local_start_height, const uint64_t local_end_height,
    std::multimap<uint64_t, mbus::chain_behind_event_address>& chain_behind_address_map) {
    std::unique_lock<std::mutex> lock(m_lock);

    auto it = m_multi_role_peers.find(self_address);
    if (it == m_multi_role_peers.end())
        return false;

    xsync_role_peers_t& peers = it->second;

    bool is_frozenType = common::has<common::xnode_type_t::frozen>(self_address.type());
    std::map<std::string, uint64_t> genesis_accounts = data::xrootblock_t::get_all_genesis_accounts();

    for (auto& it2 : peers) {
        xsync_peer_chains_t& chains = it2.second;

        auto it3 = chains.find(address);
        if (it3 == chains.end())
            continue;

        xsync_dbg("get_peer_height_info_map index  self_address %s address %s is_frozenType %d  peers sie %d",
            self_address.to_string().c_str(), address.c_str(), is_frozenType, peers.size());

        if(is_frozenType) {
            auto from_addr = it2.first; 
            auto it = genesis_accounts.find(from_addr.account_address().to_string());
            if (it == genesis_accounts.end()) {
                continue;
            }
        }

        xsync_chain_info_t& info = it3->second;

        if (info.end_height > local_end_height && info.start_height < info.end_height) {
            mbus::chain_behind_event_address chain_behind_address_info;
            chain_behind_address_info.self_addr = self_address;
            chain_behind_address_info.from_addr = it2.first;
            //fast sync need start
            if (info.start_height > local_start_height) {
                chain_behind_address_info.start_height = info.start_height;
            } else {
                chain_behind_address_info.start_height = local_start_height;
            }
            chain_behind_address_map.insert(std::make_pair(info.end_height, chain_behind_address_info));
        }
    }

    if (!chain_behind_address_map.empty()) {
#ifdef DEBUG
        int32_t index = 0;
        for(auto chain_behind_info: chain_behind_address_map) {
            xsync_dbg("index %d address %s chain_behind_address_map end_height %llu start_height %llu self_address %s peer_address %s", 
            index, address.c_str(), chain_behind_info.first, chain_behind_info.second.start_height, 
            chain_behind_info.second.self_addr.to_string().c_str(), chain_behind_info.second.from_addr.to_string().c_str());
            index++;
        }
#endif
        return true;
    }
    return false;
}

NS_END2
