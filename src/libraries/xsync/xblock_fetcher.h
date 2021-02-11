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
#include "xsync/xsync_broadcast.h"
#include "xmbus/xbase_sync_event_monitor.hpp"
#include "xbase/xvledger.h"
#include "xmbus/xevent_behind.h"

NS_BEG2(top, sync)

enum enum_pending_type {
    enum_pending_type_none,
    enum_pending_type_consensus,
    enum_pending_type_gossip,
    enum_pending_type_newblock,
    enum_pending_type_newblockhash,
};

const char* pending_type_string(enum_pending_type type);

class xblock_fetcher_t;

class xblockfetcher_event_monitor_t : public mbus::xbase_sync_event_monitor_t {
public:
    xblockfetcher_event_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const &mbus, 
        observer_ptr<base::xiothread_t> const & iothread,
        xblock_fetcher_t* block_fetcher);
    bool filter_event(const mbus::xevent_ptr_t& e) override;
    void process_event(const mbus::xevent_ptr_t& e) override;

private:
    xblock_fetcher_t* m_block_fetcher{};
};

class xfetch_context_t {
public:
    xfetch_context_t(enum_pending_type _type, int64_t _tm):
    type(_type),
    tm(_tm) {
    }
    enum_pending_type type;
    int64_t tm;
};

class xpending_block_t {
public:
    xpending_block_t(enum_pending_type _type, const data::xblock_ptr_t &_block,
        const vnetwork::xvnode_address_t &_from_address,
        const vnetwork::xvnode_address_t &_network_self):
    type(_type),
    block(_block),
    from_address(_from_address),
    network_self(_network_self) {
        XMETRICS_COUNTER_INCREMENT("sync_aggregate_pending_block", 1);
    }

    ~xpending_block_t() {
        XMETRICS_COUNTER_INCREMENT("sync_aggregate_pending_block", -1);
    }

    enum_pending_type type;
    data::xblock_ptr_t block;
    vnetwork::xvnode_address_t from_address;
    vnetwork::xvnode_address_t network_self;
};

class xchain_fetcher_info_t {
public:
    std::map<uint64_t, std::shared_ptr<xpending_block_t>> m_blocks;
    // for split downloader and fetcher, the start height of fetcher
    uint64_t m_start_height{0};
    // if a chain is inactive, check it's pending blocks periodically.
    int64_t m_check_time{0};
};

class xblock_fetcher_t {
public:
    xblock_fetcher_t(std::string vnode_id, observer_ptr<base::xiothread_t> const & iothread, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xsync_broadcast_t *sync_broadcast,
        xsync_sender_t *sync_sender);

    void handle_consensus_block(const xblock_ptr_t &block, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void handle_gossip_behind(const std::string &address, uint64_t height, uint64_t view_id, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void handle_newblock(xblock_ptr_t &block, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void handle_newblockhash(const std::string &address, uint64_t height, uint64_t view_id, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void on_timer_event(const mbus::xevent_ptr_t& e);
    void on_downloader_event(const mbus::xevent_ptr_t& e);
    bool get_highest_info(const std::string &address, uint64_t &height, uint64_t &view_id) const;

public:
    bool filter_block(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);

public:
    void process_event(const mbus::xevent_ptr_t& e);

private:
    void on_timer(const mbus::xevent_ptr_t& e);
    void on_blockfetcher_event(const mbus::xevent_ptr_t& e);
    void on_blocks_event(const mbus::xevent_ptr_t& e);
    void on_downloader_complete(const mbus::xevent_ptr_t& e);

    void process_newest_block(const std::string &address);
    void sync_newest_block(const std::string &address);
    void handle_fetch_block(enum_pending_type type, const xblock_ptr_t &block,
        const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void notify_downloader(const std::string &address, mbus::enum_behind_source source_type,
            const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &target_address);
    xblock_ptr_t get_cached_block(const std::string &address, uint64_t height);
    void handle_block(enum_pending_type type, xblock_ptr_t &block, 
        const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);

    void add_pending_block(enum_pending_type type, const xblock_ptr_t &block, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self);
    void get_blocks(const std::string &address, uint64_t height, enum_pending_type type, const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &target_address);
    std::string get_key(const std::string &address, uint64_t height);
    int64_t get_time();

private:
    std::string m_vnode_id;
    observer_ptr<mbus::xmessage_bus_face_t> m_mbus;
    observer_ptr<base::xvcertauth_t> m_certauth;
    xsync_store_face_t *m_sync_store;
    xsync_broadcast_t *m_sync_broadcast;
    xsync_sender_t *m_sync_sender;

    std::unique_ptr<mbus::xmessage_bus_face_t> m_self_mbus{};
    std::unique_ptr<xblockfetcher_event_monitor_t> m_monitor{};

    xblock_keeper_t m_block_keeper;

    std::mutex m_lock;
    std::unordered_map<std::string, std::shared_ptr<xfetch_context_t>> m_fetching_list;

    // no lock
    std::unordered_map<std::string, std::shared_ptr<xchain_fetcher_info_t>> m_chains;
};

NS_END2