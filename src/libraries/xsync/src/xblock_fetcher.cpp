// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xblock_fetcher.h"
#include "xsync/xsync_log.h"
#include "xmbus/xevent_blockfetcher.h"
#include "xmbus/xevent_behind.h"
#include "xmbus/xevent_downloader.h"
#include "xsync/xsync_util.h"

NS_BEG2(top, sync)

using namespace metrics;
using namespace mbus;

const uint32_t QUEUE_SIZE = 16;
// 5s
const uint32_t FETCHER_TIMEOUT = 5000;

// 10min
const uint32_t CHAIN_CHECK_INTERVAL = 600000;

const char* pending_type_string(enum_pending_type type) {

    const char *p = nullptr;

    switch(type) {
    case enum_pending_type_consensus:
        p = "consensus";
        break;
    case enum_pending_type_gossip:
        p = "gossip";
        break;
    case enum_pending_type_newblock:
        p = "newblock";
        break;
    case enum_pending_type_newblockhash:
        p = "newblockhash";
        break;
    default:
        break;
    }

    return p;
}

xblockfetcher_event_monitor_t::xblockfetcher_event_monitor_t(observer_ptr<mbus::xmessage_bus_face_t> const &mbus, 
        observer_ptr<base::xiothread_t> const & iothread,
        xblock_fetcher_t* block_fetcher):
xbase_sync_event_monitor_t(mbus, 10000, iothread),
m_block_fetcher(block_fetcher) {

    mbus::xevent_queue_cb_t cb = std::bind(&xblockfetcher_event_monitor_t::push_event, this, std::placeholders::_1);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_timer, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_sync_executor, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_blockfetcher, cb);
    m_reg_holder.add_listener((int) mbus::xevent_major_type_downloader, cb);
}

bool xblockfetcher_event_monitor_t::filter_event(const mbus::xevent_ptr_t& e) {
    XMETRICS_COUNTER_INCREMENT("sync_blockfetcher_event_count", 1);
    return true;
}

void xblockfetcher_event_monitor_t::process_event(const mbus::xevent_ptr_t& e) {
    XMETRICS_COUNTER_INCREMENT("sync_blockfetcher_event_count", -1);
    m_block_fetcher->process_event(e);
}

xblock_fetcher_t::xblock_fetcher_t(std::string vnode_id, observer_ptr<base::xiothread_t> const & iothread, const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
        const observer_ptr<base::xvcertauth_t> &certauth,
        xsync_store_face_t *sync_store,
        xsync_broadcast_t *sync_broadcast,
        xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_mbus(mbus),
m_certauth(certauth),
m_sync_store(sync_store),
m_sync_broadcast(sync_broadcast),
m_sync_sender(sync_sender) {

    m_self_mbus = top::make_unique<xmessage_bus_t>();
    m_monitor = top::make_unique<xblockfetcher_event_monitor_t>(make_observer(m_self_mbus.get()), iothread, this);
}

void xblock_fetcher_t::handle_consensus_block(const xblock_ptr_t &block, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    const std::string &address = block->get_account();

    bool is_table_address = data::is_table_address(common::xaccount_address_t{address});
    if (is_table_address) {

        mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_blockfetcher_consensus_t>(block, network_self, from_address);
        m_monitor->push_event(ev);
    } else {
        xsync_warn("[xblock_fetcher_t] handle_consensus_block is not table %s", address.c_str());
    }
}

void xblock_fetcher_t::handle_gossip_behind(const std::string &address, uint64_t height, uint64_t view_id,
    const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_blockfetcher_gossip_t>(address, height, view_id, network_self, from_address);
    m_monitor->push_event(ev);
}

void xblock_fetcher_t::handle_newblock(xblock_ptr_t &block, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    if (block == nullptr)
        return;

    if (!is_beacon_table(block->get_account())) {
        if (!check_auth(m_certauth, block)) {
            xsync_info("[xblock_fetcher_t] handle_newblock(auth failed) : %s %s", block->dump().c_str(), from_address.to_string().c_str());
            return;
        }
    }

    // check src addr type and block type

    mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_blockfetcher_newblock_t>(block, network_self, from_address);
    m_monitor->push_event(ev);
}

void xblock_fetcher_t::handle_newblockhash(const std::string &address, uint64_t height, uint64_t view_id, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    base::xauto_ptr<base::xvblock_t> vblock = m_sync_store->get_current_block(address);
    if (vblock!=nullptr && (vblock->get_height()>height || vblock->get_viewid()>=view_id)) {
        xsync_warn("[xblock_fetcher_t] handle_newblockhash local is exist %s", vblock->dump().c_str());
        return;
    }

    mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_blockfetcher_newblockhash_t>(address, height, view_id, network_self, from_address);
    m_monitor->push_event(ev);
}

void xblock_fetcher_t::on_timer_event(const mbus::xevent_ptr_t& e) {
    m_monitor->push_event(e);
}

void xblock_fetcher_t::on_downloader_event(const mbus::xevent_ptr_t& e) {
    m_monitor->push_event(e);
}

bool xblock_fetcher_t::get_highest_info(const std::string &address, uint64_t &height, uint64_t &view_id) const {
    return m_block_keeper.get_info(address, height, view_id);
}

bool xblock_fetcher_t::filter_block(const std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    uint32_t size = blocks.size();

    if (size != 1)
        return false;

    xblock_ptr_t block = blocks[0];

    // if is syncing?
    {
        std::string key = get_key(block->get_block_owner(), block->get_height());
        // rw lock
        std::unique_lock<std::mutex> lock(m_lock);
        auto it = m_fetching_list.find(key);
        if (it == m_fetching_list.end()) {
            return false;
        }
    }

    mbus::xevent_ptr_t e = std::make_shared<mbus::xevent_sync_response_blocks_t>(blocks, network_self, from_address);
    m_monitor->push_event(e);

    return true;
}

void xblock_fetcher_t::process_event(const mbus::xevent_ptr_t& e) {

    if (e->major_type == mbus::xevent_major_type_timer) {
        XMETRICS_TIME_RECORD("sync_cost_blockfetcher_timer");
        on_timer(e);
    } else if (e->major_type == mbus::xevent_major_type_blockfetcher) {
        XMETRICS_TIME_RECORD("sync_cost_blockfetcher_self");
        on_blockfetcher_event(e);
    } else if (e->major_type == mbus::xevent_major_type_sync_executor && e->minor_type == mbus::xevent_sync_executor_t::blocks) {
        XMETRICS_TIME_RECORD("sync_cost_blockfetcher_response");
        on_blocks_event(e);
    } else if (e->major_type == mbus::xevent_major_type_downloader && e->minor_type==mbus::xevent_downloader_t::complete) {
        XMETRICS_TIME_RECORD("sync_cost_blockfetcher_connect");
        on_downloader_complete(e);
    }
}

void xblock_fetcher_t::on_timer(const mbus::xevent_ptr_t& e) {

    int64_t now = get_time();

    {
        std::unique_lock<std::mutex> lock(m_lock);

        std::unordered_map<std::string, std::shared_ptr<xfetch_context_t>>::iterator it = m_fetching_list.begin();
        for (;it!=m_fetching_list.end();) {
            if ((now - it->second->tm) > FETCHER_TIMEOUT) {
                xsync_info("[xblock_fetcher_t] remove timeout %s type(%d)", it->first.c_str(), it->second->type);
                m_fetching_list.erase(it++);
            } else {
                ++it;
            }
        }
    }

    {
        for (auto &it: m_chains) {
            if ((now - it.second->m_check_time) > CHAIN_CHECK_INTERVAL) {
                const std::string &address = it.first;
                //xsync_info("[xblock_fetcher_t] timer check %s", address.c_str());

                // remove not exist obj
                process_newest_block(address);
            }
        }
    }
}

void xblock_fetcher_t::on_blockfetcher_event(const mbus::xevent_ptr_t& e) {
    if (e->minor_type == mbus::xevent_blockfetcher_t::consensus || e->minor_type == mbus::xevent_blockfetcher_t::newblock) {
        auto bme = std::static_pointer_cast<mbus::xevent_blockfetcher_block_t>(e);

        const xblock_ptr_t &block = bme->block;
        const vnetwork::xvnode_address_t &network_self = bme->network_self;
        const vnetwork::xvnode_address_t &target_address = bme->target_address;
        const std::string &address = block->get_account();

        enum_pending_type pending_type = enum_pending_type_none;
        mbus::enum_behind_source source_type = mbus::enum_behind_source_none;

        if (e->minor_type == mbus::xevent_blockfetcher_t::consensus) {
            pending_type = enum_pending_type_consensus;
            source_type = mbus::enum_behind_source::enum_behind_source_consensus;

        } else if (e->minor_type == mbus::xevent_blockfetcher_t::newblock) {
            pending_type = enum_pending_type_newblock;
            source_type = mbus::enum_behind_source::enum_behind_source_newblock;

        } else {
            assert(0);
        }

        xsync_info("[xblock_fetcher_t] on_fetcher_event %s,height(%lu),viewid(%lu) type(%s)",
            block->get_account().c_str(), block->get_height(), block->get_viewid(), pending_type_string(pending_type));

        handle_fetch_block(pending_type, block, target_address, network_self);
        process_newest_block(address);
        notify_downloader(address, source_type, network_self, target_address);
        sync_newest_block(address);

    } else if (e->minor_type == mbus::xevent_blockfetcher_t::gossip || e->minor_type == mbus::xevent_blockfetcher_t::newblockhash) {
        auto bme = std::static_pointer_cast<mbus::xevent_blockfetcher_blockinfo_t>(e);
        std::string &address = bme->address;
        uint64_t height = bme->height;
        uint64_t view_id = bme->view_id;
        const vnetwork::xvnode_address_t &network_self = bme->network_self;
        const vnetwork::xvnode_address_t &target_address = bme->target_address;

        enum_pending_type pending_type = enum_pending_type_none;
        mbus::enum_behind_source source_type = mbus::enum_behind_source_none;

        if (e->minor_type == mbus::xevent_blockfetcher_t::gossip) {
            pending_type = enum_pending_type_gossip;
            source_type = mbus::enum_behind_source::enum_behind_source_gossip;

        } else if (e->minor_type == mbus::xevent_blockfetcher_t::newblockhash) {
            pending_type = enum_pending_type_newblockhash;
            source_type = mbus::enum_behind_source::enum_behind_source_newblockhash;

        } else {
            assert(0);
        }

        xsync_info("[xblock_fetcher_t] on_fetcher_event %s,height(%lu),viewid(%lu) type(%s)",
            address.c_str(), height, view_id, pending_type_string(pending_type));

        xblock_ptr_t cached_block = get_cached_block(address, height);
        if (cached_block == nullptr) {
            get_blocks(address, height, pending_type, network_self, target_address);
        } else {
            if (cached_block->get_viewid() < view_id) {
                get_blocks(address, height, pending_type, network_self, target_address);
            } else if (cached_block->get_viewid() == view_id) {
                notify_downloader(address, source_type, network_self, target_address);
            } else {
                // ignore
                xsync_info("[xblock_fetcher_t] cached block is newer %s cached_view:%lu peer_view:%lu", address.c_str(), cached_block->get_viewid(), view_id); 
            }
        }

    } else {
        assert(0);
    }
}

void xblock_fetcher_t::on_blocks_event(const mbus::xevent_ptr_t& e) {

    auto bme = std::static_pointer_cast<mbus::xevent_sync_response_blocks_t>(e);

    xblock_ptr_t &block = bme->blocks[0];
    const vnetwork::xvnode_address_t &network_self = bme->self_address;
    const vnetwork::xvnode_address_t &from_address = bme->from_address;

    XMETRICS_COUNTER_INCREMENT("sync_blockfetcher_response", 1);

    std::shared_ptr<xfetch_context_t> ctx = nullptr;
    const std::string &address = block->get_account();
    std::string key = get_key(address, block->get_height());
    {
        std::unique_lock<std::mutex> lock(m_lock);
        auto it = m_fetching_list.find(key);
        if (it == m_fetching_list.end()) {
            //assert(0);
            return;
        }

        ctx = it->second;
        m_fetching_list.erase(key);
    }

    enum_pending_type pending_type = ctx->type;

    if (!is_beacon_table(block->get_account())) {
        if (!check_auth(m_certauth, block)) {
            xsync_info("[xblock_fetcher_t] on_response_event(auth failed) : %s %s", block->dump().c_str(), from_address.to_string().c_str());
            return;
        }
    }

    xsync_info("[xblock_fetcher_t] on_response_event success : %s type(%s) %s",
        block->dump().c_str(), pending_type_string(pending_type), from_address.to_string().c_str());

    mbus::enum_behind_source source_type = mbus::enum_behind_source_none;
    if (pending_type == enum_pending_type_gossip) {
        source_type = mbus::enum_behind_source_gossip;
    } else if (pending_type == enum_pending_type_newblockhash) {
        source_type = mbus::enum_behind_source_newblockhash;
    } else {
        assert(0);
    }

    handle_fetch_block(pending_type, block, from_address, network_self);
    process_newest_block(address);
    notify_downloader(address, source_type, network_self, from_address);
    sync_newest_block(address);
}

void xblock_fetcher_t::process_newest_block(const std::string &address) {

    auto it = m_chains.find(address);
    if (it == m_chains.end())
        return;

    int64_t now = get_time();

    std::shared_ptr<xchain_fetcher_info_t> &chain = it->second;
    std::map<uint64_t, std::shared_ptr<xpending_block_t>> &blocks = chain->m_blocks;

    std::map<uint64_t, std::shared_ptr<xpending_block_t>>::iterator it2 = blocks.begin();
    for (; it2!=blocks.end();) {
        base::xauto_ptr<base::xvblock_t> current_block = m_sync_store->get_current_block(address);
        std::shared_ptr<xpending_block_t> pending_block = it2->second;

        if (pending_block->block->get_height() < current_block->get_height()) {
            xsync_dbg("[xblock_fetcher_t] ignore low height block %s current(%lu) pending(%lu)", 
                    address.c_str(), current_block->get_height(), pending_block->block->get_height());
            blocks.erase(it2++);
        } else if (pending_block->block->get_height() == current_block->get_height()) {
            if (pending_block->block->get_viewid() >= current_block->get_viewid()) {
                handle_block(pending_block->type, pending_block->block, pending_block->from_address, pending_block->network_self);
                xsync_info("[xblock_fetcher_t] after handle %s", m_sync_store->get_current_block(address)->dump().c_str());
            } else {
                xsync_dbg("[xblock_fetcher_t] ignore same height block %s height(%lu) current_view(%lu) pending_view(%lu)", 
                        address.c_str(), current_block->get_height(), current_block->get_viewid(), pending_block->block->get_viewid());
            }
            blocks.erase(it2++);

        } else if (pending_block->block->get_height() == (current_block->get_height()+1)) {

            if (current_block->get_block_hash() == pending_block->block->get_last_block_hash()) {
                handle_block(pending_block->type, pending_block->block, pending_block->from_address, pending_block->network_self);
                xsync_info("[xblock_fetcher_t] after handle %s", m_sync_store->get_current_block(address)->dump().c_str());
                blocks.erase(it2++);
            } else {
                break;
            }
        } else {
            break;
        }
    }

    it->second->m_check_time = now;
}

void xblock_fetcher_t::sync_newest_block(const std::string &address) {

    auto it = m_chains.find(address);
    if (it == m_chains.end())
        return;

    std::shared_ptr<xchain_fetcher_info_t> &chain = it->second;
    std::map<uint64_t, std::shared_ptr<xpending_block_t>> &blocks = chain->m_blocks;

    // consider from_address

    std::shared_ptr<xpending_block_t> successor = nullptr;

    std::map<uint64_t, std::shared_ptr<xpending_block_t>>::reverse_iterator rit = blocks.rbegin();
    for (; rit!=blocks.rend(); rit++) {
        if (successor != nullptr) {

            if (successor->block->get_height() <= chain->m_start_height)
                return;

            uint64_t cur_height = rit->first;

            if ((cur_height+1) != successor->block->get_height()) {
                get_blocks(address, successor->block->get_height()-1, enum_pending_type_gossip, successor->network_self, successor->from_address);
                return;
            }

            if (successor->block->get_last_block_hash() != rit->second->block->get_block_hash()) {
                get_blocks(address, successor->block->get_height()-1, enum_pending_type_gossip, successor->network_self, successor->from_address);
                return;
            }
        }

        successor = rit->second;
    }

    if (successor == nullptr)
        return;

    if (blocks.size() >= QUEUE_SIZE)
        return;

    if (successor->block->get_height() <= chain->m_start_height)
        return;

    base::xauto_ptr<base::xvblock_t> current_block = m_sync_store->get_current_block(address);
    uint64_t height = successor->block->get_height() - 1;
    if (height == 0)
        return;

    if (height > current_block->get_height()) {
        get_blocks(address, height, enum_pending_type_gossip, successor->network_self, successor->from_address);

    } else if (height == current_block->get_height() &&
        successor->block->get_last_block_hash() != current_block->get_block_hash()) {
        get_blocks(address, height, enum_pending_type_gossip, successor->network_self, successor->from_address);
    }
}

void xblock_fetcher_t::on_downloader_complete(const mbus::xevent_ptr_t& e) {

    auto bme = std::static_pointer_cast<mbus::xevent_downloader_complete_t>(e);
    const std::string &address = bme->address;
    uint64_t height = bme->height;

    xsync_info("[xblock_fetcher_t] on_downloader_complete %s,height=%lu,", address.c_str(), height);

    process_newest_block(address);
    sync_newest_block(address);
}

void xblock_fetcher_t::handle_fetch_block(enum_pending_type type, const xblock_ptr_t &block,
    const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_dbg("[xblock_fetcher_t] handle_fetch_block type(%s) %s,height=%lu,view_id=%lu,", 
        pending_type_string(type), block->get_account().c_str(), block->get_height(), block->get_viewid());

    const std::string &address = block->get_account();

    // the newest blocks write ahead, avoid fork caused by no enough data
    if (type==enum_pending_type_gossip || type==enum_pending_type_newblock || type==enum_pending_type_newblockhash) {
        std::string account_prefix;
        uint32_t table_id = 0;
        if (data::xdatautil::extract_parts(address, account_prefix, table_id)) {
            if (account_prefix == sys_contract_sharding_table_block_addr) {
                base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
                bool ret = m_sync_store->store_block(vblock);
                if (!ret) {
                    xsync_info("[xblock_fetcher_t] handle_fetch_block write ahead failed %s", block->dump().c_str());
                }
            }
        }
    }

    m_block_keeper.update(block);

    std::shared_ptr<xchain_fetcher_info_t> chain = nullptr;

    auto it = m_chains.find(address);
    if (it == m_chains.end()) {
        chain = std::make_shared<xchain_fetcher_info_t>();
        chain->m_check_time = get_time();
        m_chains.insert(std::make_pair(address, chain));
    } else {
        chain = it->second;
    }

    // filter and ensure notify downloader
    if (block->get_height() < chain->m_start_height) {
        xsync_warn("[xblock_fetcher_t] handle_fetch_block lower than start_height %s,height(%lu) start_height(%lu)",
            block->get_account().c_str(), block->get_height(), chain->m_start_height);
        return;
    }

    std::map<uint64_t, std::shared_ptr<xpending_block_t>> &blocks = chain->m_blocks;
    std::shared_ptr<xpending_block_t> pending_block = std::make_shared<xpending_block_t>(type, block, from_address, network_self);

    auto it2 = blocks.find(block->get_height());
    if (it2 == blocks.end()) {
        blocks.insert(std::make_pair(block->get_height(), pending_block));
    } else {
        // if higher view, replace
        if (block->get_viewid() > it2->second->block->get_viewid()) {
            blocks.insert(std::make_pair(block->get_height(), pending_block));
        } else {
            xsync_info("[xblock_fetcher_t] handle_fetch_block lower than exist block %s,height(%lu) new_viewid(%lu), exist_viewid(%lu)",
                block->get_account().c_str(), block->get_height(), block->get_viewid(), it2->second->block->get_viewid());
        }
    }

    uint32_t count = blocks.size();
    std::map<uint64_t, std::shared_ptr<xpending_block_t>>::iterator it3 = blocks.begin();
    for (; it3!=blocks.end() && count>QUEUE_SIZE;) {

        std::shared_ptr<xpending_block_t> &ptr = it3->second;

        xsync_info("[xblock_fetcher_t] insert cache is full and ignore type(%s) %s,height=%lu,view_id=%lu,", 
            pending_type_string(ptr->type), ptr->block->get_account().c_str(), ptr->block->get_height(), ptr->block->get_viewid());

        blocks.erase(it3++);
        count--;
    }
}

void xblock_fetcher_t::notify_downloader(const std::string &address, mbus::enum_behind_source source_type,
    const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &target_address) {

    auto it = m_chains.find(address);
    if (it == m_chains.end()) {
        xsync_warn("[xblock_fetcher_t] notify_downloader failed. not found %s", address.c_str());
        return;
    }

    std::shared_ptr<xchain_fetcher_info_t> &chain = it->second;
    std::map<uint64_t, std::shared_ptr<xpending_block_t>> &blocks = chain->m_blocks;

    if (blocks.size() == 0)
        return;

    std::map<uint64_t, std::shared_ptr<xpending_block_t>>::iterator it2 = blocks.begin();

    xblock_ptr_t root_block = it2->second->block;
    uint64_t root_height = root_block->get_height();

    if (root_height == 1)
        return;

    if (root_height < chain->m_start_height) {
        xsync_info("[xblock_fetcher_t] notify_downloader failed %s root_height:%lu start_height:%lu", 
                    address.c_str(), root_height, chain->m_start_height);
        return;
    }

    chain->m_start_height = root_height;

    xsync_info("[xblock_fetcher_t] notify_downloader succ %s successor:height(%lu) viewid(%lu)", 
                    address.c_str(), root_height, root_block->get_viewid());

    mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_behind_block_t>(root_block, source_type, "", network_self, target_address);
    m_mbus->push_event(ev);
}

xblock_ptr_t xblock_fetcher_t::get_cached_block(const std::string &address, uint64_t height) {
    auto it = m_chains.find(address);
    if (it == m_chains.end())
        return nullptr;

    std::shared_ptr<xchain_fetcher_info_t> &chain = it->second;
    std::map<uint64_t, std::shared_ptr<xpending_block_t>> &blocks = chain->m_blocks;
    auto it2 = blocks.find(height);
    if (it2 == blocks.end())
        return nullptr;

    return it2->second->block;
}

void xblock_fetcher_t::handle_block(enum_pending_type type, xblock_ptr_t &block, 
        const vnetwork::xvnode_address_t &from_address, const vnetwork::xvnode_address_t &network_self) {

    xsync_info("[xblock_fetcher_t] handle_block type(%s) %s,height=%lu,view_id=%lu", 
            pending_type_string(type), block->get_account().c_str(), block->get_height(), block->get_viewid());

    const std::string &address = block->get_account();

    // consensus result must not store again
    if (type != enum_pending_type_consensus) {
        if (is_beacon_table(block->get_account())) {

            if (!check_auth(m_certauth, block)) {
                xsync_warn("[xblock_fetcher_t] handle_block auth failed type(%s) %s", pending_type_string(type), block->dump().c_str());
                return;
            }

            base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
            bool ret = m_sync_store->store_block(vblock);

            if (!ret) {

                base::xauto_ptr<base::xvblock_t> blk = m_sync_store->get_current_block(address);
                if (blk != nullptr) {
                    if (blk->get_height() >= block->get_height()) {
                        xsync_info("[xblock_fetcher_t] store data succ(exist cert) type(%s) %s", pending_type_string(type), blk->dump().c_str());
                    } else {
                        assert(0);
                        return;
                    }
                }
            }
        } else {
            base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
            bool ret = m_sync_store->store_block(vblock);

            if (!ret) {

                base::xauto_ptr<base::xvblock_t> blk = m_sync_store->get_current_block(address);
                if (blk != nullptr) {
                    if (blk->get_height() >= block->get_height()) {
                        xsync_info("[xblock_fetcher_t] store data succ(exist cert) type(%s) %s", pending_type_string(type), blk->dump().c_str());
                    } else {
                        assert(0);
                        return;
                    }
                }
            }
        }
    }

    if (type == enum_pending_type_consensus) {
        if (m_sync_broadcast != nullptr)
            m_sync_broadcast->broadcast_newblock_to_archive(block);
    } else if (type == enum_pending_type_gossip) {
        // do nothing
    } else if (type == enum_pending_type_newblock) {
        if (m_sync_broadcast != nullptr)
            m_sync_broadcast->broadcast_newblockhash(block, network_self);
    } else if (type == enum_pending_type_newblockhash) {
        // do nothing
    } else {
        assert(0);
    }
}

void xblock_fetcher_t::get_blocks(const std::string &address, uint64_t height, enum_pending_type type,
    const vnetwork::xvnode_address_t &network_self, const vnetwork::xvnode_address_t &target_address) {

    if (height == 0)
        return;

    std::string key = get_key(address, height);
    {
        std::unique_lock<std::mutex> lock(m_lock);
        auto it = m_fetching_list.find(key);
        if (it != m_fetching_list.end()) {
            xsync_info("[xblock_fetcher_t] get_blocks is processing %s %lu old_type(%s) type(%s)", 
                    address.c_str(), height, pending_type_string(it->second->type), pending_type_string(type)); 
            return;
        }

        std::shared_ptr<xfetch_context_t> ctx = std::make_shared<xfetch_context_t>(type, get_time());
        m_fetching_list[key] = ctx;
    }

    uint64_t start_height = height;
    uint32_t count = 1;

    XMETRICS_COUNTER_INCREMENT("sync_blockfetcher_request", 1);

    xsync_info("[xblock_fetcher_t] send sync request(block). type(%s) %s range[%lu,%lu] %s", 
                pending_type_string(type), address.c_str(), start_height, start_height+count-1, target_address.to_string().c_str());

    m_sync_sender->send_get_blocks(address, start_height, count, network_self, target_address);
}

std::string xblock_fetcher_t::get_key(const std::string &address, uint64_t height) {
    std::string key = address + "_" + std::to_string(height);
    return key;
}

int64_t xblock_fetcher_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

NS_END2
