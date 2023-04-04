// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <atomic>
#include <mutex>
#include <unordered_map>
#include "xdata/xdata_common.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xutl.h"

#include "xmbus/xmessage_bus.h"
#include "xvledger/xvblock.h"
#include "xsync/xsync_store.h"
#include "xsync/xrole_chains_mgr.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_sender.h"
#include "xmbus/xevent_behind.h"

NS_BEG2(top, sync)

class xsync_chain_info_t {
public:
    xsync_chain_info_t(uint64_t _start_height, uint64_t _end_height):
    start_height(_start_height),
    end_height(_end_height) {
    }

    xsync_chain_info_t(const xsync_chain_info_t &other) {
        start_height = other.start_height;
        end_height = other.end_height;
    }

    void update(uint64_t _start_height, uint64_t _end_height) {
        start_height = _start_height;
        end_height = _end_height;
    }

public:
    uint64_t start_height{0};
    uint64_t end_height{0};
};

class xsync_peerset_t {
    using xsync_peer_chains_t = std::map<std::string, xsync_chain_info_t>;
    using xsync_role_peers_t = std::map<vnetwork::xvnode_address_t, xsync_peer_chains_t>;

public:
    xsync_peerset_t(std::string vnode_id);
    void add_group(const vnetwork::xvnode_address_t &self_address);
    void remove_group(const vnetwork::xvnode_address_t &self_address);
    void add_peer(const vnetwork::xvnode_address_t &self_address, const vnetwork::xvnode_address_t &peer_address);
    void remove_peer(const vnetwork::xvnode_address_t &self_address, const vnetwork::xvnode_address_t &peer_address);
    void frozen_add_peer(xsync_role_peers_t &role_peers, const vnetwork::xvnode_address_t &peer_address);
    void update(const vnetwork::xvnode_address_t &self_address, const vnetwork::xvnode_address_t &peer_address, const std::vector<xchain_state_info_t> &info_list);
    bool get_newest_peer(const vnetwork::xvnode_address_t &self_address, const std::string &address, uint64_t &start_height, uint64_t &end_height, vnetwork::xvnode_address_t &peer_addr, bool random_check = false);
    bool get_group_size(const vnetwork::xvnode_address_t &self_address, uint32_t &count);
    bool get_peer_height_info_map(const vnetwork::xvnode_address_t &self_address, const std::string &address, 
                                  const uint64_t local_start_height, const uint64_t local_end_height, 
                                  std::multimap<uint64_t, mbus::chain_behind_event_address> &chain_behind_address_map);
    uint32_t get_frozen_limit();
    bool get_archive_group(vnetwork::xvnode_address_t &self_addr, std::vector<vnetwork::xvnode_address_t> &neighbors);
    bool get_group_nodes(const vnetwork::xvnode_address_t &self_addr, std::vector<vnetwork::xvnode_address_t> &neighbors);
    std::map<std::string, std::vector<std::string>> get_neighbors();

private:
    std::string m_vnode_id;
    std::mutex m_lock;
    std::map<vnetwork::xvnode_address_t, xsync_role_peers_t> m_multi_role_peers;
};

NS_END2
