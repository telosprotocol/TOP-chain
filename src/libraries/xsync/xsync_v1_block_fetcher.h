// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <unordered_map>
#include <memory>
#include "xbasic/xns_macro.h"
#include "xbasic/xmemory.hpp"
#include "xsync/xsync_store.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_message.h"
// TODO(jimmy) #include "xbase/xvledger.h"
#include "xsync/xrole_chains_mgr.h"

NS_BEG2(top, sync)

class xfetch_context_t {
public:
    xfetch_context_t(const std::string &_address, uint64_t _height, const vnetwork::xvnode_address_t &_network_self, const vnetwork::xvnode_address_t &_from_address, int64_t _tm):
    address(_address),
    height(_height),
    network_self(_network_self),
    from_address(_from_address),
    tm(_tm) {
    }
    std::string address;
    uint64_t height;
    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t from_address;
    int64_t tm;
};

class xsync_v1_block_fetcher_t {
public:
    xsync_v1_block_fetcher_t(std::string vnode_id,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xsync_sender_t *sync_sender);

public:
    void handle_v1_newblockhash(const std::string &address, uint64_t height, uint64_t view_id, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    bool filter_block(data::xblock_ptr_t &block);
    void on_timer_check_v1_newblockhash();

private:
    std::string get_key(const std::string &address, uint64_t height);
    int64_t get_time();

private:
    std::string m_vnode_id;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_store_face_t *m_sync_store;
    xsync_sender_t *m_sync_sender;

    std::mutex m_lock;
    std::unordered_map<std::string, std::shared_ptr<xfetch_context_t>> m_announce_list;
    std::unordered_map<std::string, std::shared_ptr<xfetch_context_t>> m_fetching_list;
};

NS_END2
