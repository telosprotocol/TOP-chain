// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <unordered_map>
#include <memory>

#include "xbasic/xmemory.hpp"
#include "xsync/xsync_store.h"
#include "xsync/xsync_sender.h"
#include "xsync/xsync_message.h"
// TODO(jimmy) #include "xbase/xvledger.h"

NS_BEG2(top, sync)

class xsync_block_announce_t {
public:
    xsync_block_announce_t(uint64_t _height, const std::string &_hash,
        const vnetwork::xvnode_address_t &_network_self, const vnetwork::xvnode_address_t &_from_address, int64_t _tm):
    height(_height),
    hash(_hash),
    network_self(_network_self),
    from_address(_from_address),
    tm(_tm) {
    }
public:
    uint64_t height;
    std::string hash;
    vnetwork::xvnode_address_t network_self;
    vnetwork::xvnode_address_t from_address;
    int64_t tm;
};

using xsync_block_announce_ptr_t = std::shared_ptr<xsync_block_announce_t>;

class xchain_block_fetcher_t {
public:
    xchain_block_fetcher_t(std::string vnode_id,
        const std::string &address,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xsync_sender_t *sync_sender);

public:
    void on_timer();
    void on_newblock(data::xblock_ptr_t &block, const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address);
    void on_newblockhash(uint64_t height, const std::string &hash, const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &from_address);
    void on_response_blocks(data::xblock_ptr_t & block, const vnetwork::xvnode_address_t & network_self, const vnetwork::xvnode_address_t & from_address);

private:

    void request_sync_blocks(const xsync_block_announce_ptr_t &announce);
    void insert_block(const data::xblock_ptr_t & block);
    void add_blocks();
    void import_block(data::xblock_ptr_t & block);
    void forget_hash(const std::string &hash);
    void forget_block(const std::string &hash);
    int64_t get_time();

private:
    std::string m_vnode_id;
    std::string m_address;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_store_face_t *m_sync_store;
    xsync_sender_t *m_sync_sender;

    // announced->fetching->completing
    std::unordered_map<vnetwork::xvnode_address_t, int> m_announces;
    std::map<std::string, std::vector<xsync_block_announce_ptr_t>> m_announced;
    std::unordered_map<std::string, xsync_block_announce_ptr_t> m_fetching;
    std::unordered_map<std::string, xsync_block_announce_ptr_t> m_completing;

    std::map<std::string, data::xblock_ptr_t> m_blocks;
};

using xchain_block_fetcher_ptr_t = std::shared_ptr<xchain_block_fetcher_t>;

NS_END2
