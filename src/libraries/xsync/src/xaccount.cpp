// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xaccount.h"

#include "xmbus/xevent_executor.h"
#include "xmbus/xevent_lack.h"
#include "xmbus/xevent_store.h"
#include "xmetrics/xmetrics.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
#include "xpbase/base/top_utils.h"
#include "xmbus/xevent_downloader.h"
#include "xsync/xsync_message.h"

NS_BEG2(top, sync)

using namespace mbus;
using namespace data;

#define BATCH_SIZE 20
#define GET_TOKEN_RETRY_INTERVAL 6000

xaccount_base_t::xaccount_base_t(std::string vnode_id,
                                 xsync_store_face_t * sync_store,
                                 const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
                                 const observer_ptr<base::xvcertauth_t> &certauth,
                                 xsync_sender_t *sync_sender,
                                 xsync_ratelimit_face_t *ratelimit,
                                 const xchain_info_t & chain_info):
  m_vnode_id(vnode_id),
  m_sync_store(sync_store),
  m_mbus(mbus),
  m_certauth(certauth),
  m_sync_sender(sync_sender),
  m_ratelimit(ratelimit),
  m_address(chain_info.address),
  m_sync_range_mgr(vnode_id, chain_info.address, mbus),
  m_sync_mgr(vnode_id, chain_info.address) {
    m_sync_range_mgr.on_role_changed(chain_info);

    xsync_info("[account][event] create %s (%s)", m_address.c_str(), m_sync_store->get_current_block(m_address)->dump().c_str());

    XMETRICS_COUNTER_INCREMENT("sync_downloader_chain_count", 1);
}
xaccount_base_t::~xaccount_base_t() {

    xsync_info("[account][event] destroy %s (%s)", m_address.c_str(), m_sync_store->get_current_block(m_address)->dump().c_str());
    XMETRICS_COUNTER_INCREMENT("sync_downloader_chain_count", -1);
}

std::string xaccount_base_t::get_address() {
    return m_address;
}

int64_t xaccount_base_t::get_next_timeout() {

    return get_next_send_time();
}

void xaccount_base_t::on_timer_event(int64_t now) {

    int64_t next_send_time = get_next_send_time();
    if (next_send_time!=0 && now>=next_send_time) {
        send_request(now);
    }
}

void xaccount_base_t::on_response_event(const mbus::xevent_ptr_t & e) {
    auto bme = std::static_pointer_cast<mbus::xevent_sync_response_blocks_t>(e);
    std::vector<data::xblock_ptr_t> &blocks = bme->blocks;
    uint32_t count = blocks.size();

    if (m_request == nullptr)
        return;

    if (count == 0)
        return;

    int64_t now = get_time();
    int64_t total_cost = now - m_request->send_time;

    xsync_info("[account][event] on_response_event(overview) %s count(%u) cost(%ldms) %s", 
        m_address.c_str(), count, total_cost, bme->from_address.to_string().c_str());

    m_ratelimit->on_response(total_cost, now);
    XMETRICS_COUNTER_INCREMENT("sync_downloader_response", 1);
    XMETRICS_COUNTER_INCREMENT("sync_cost_peer_response", total_cost);

    // 1.verify shard-table block multi-sign
    bool is_elect_chain = false;

    std::string account_prefix;
    uint32_t table_id = 0;
    data::xdatautil::extract_parts(m_address, account_prefix, table_id);

    if (account_prefix==sys_contract_beacon_table_block_addr || account_prefix==sys_contract_zec_table_block_addr) {
        is_elect_chain = true;
    }

    if (!is_elect_chain) {
        xblock_ptr_t &block = blocks[count-1];
        if (!check_auth(m_certauth, block)) {
            xsync_info("[account][event] on_response_event(pending_auth) %s event::%lu", m_address.c_str(), block->get_height());
            m_sync_range_mgr.clear_behind_info();
            return;
        }
    }

    // 2.modify shard-table block flag
    if (!is_elect_chain) {

        for (uint32_t i = 0; i < count; i++) {
            xblock_ptr_t &block = blocks[i];
            int flag = enum_xvblock_flag_authenticated;

            block->set_block_flag(enum_xvblock_flag(flag));

            if ((count - i) > 1) {
                flag |= enum_xvblock_flag::enum_xvblock_flag_locked;
                block->set_block_flag(enum_xvblock_flag(enum_xvblock_flag::enum_xvblock_flag_locked));
            }

            if ((count - i) > 2) {
                flag |= enum_xvblock_flag::enum_xvblock_flag_committed;
                block->set_block_flag(enum_xvblock_flag(enum_xvblock_flag::enum_xvblock_flag_committed));
            }

            xassert(block->get_block_flags() == flag);
        }
    }

    bool head_forked = false;

    // compare before and after
    for (uint32_t i = 0; i < count; i++) {

        base::xauto_ptr<base::xvblock_t> before_vblock = m_sync_store->get_current_block(m_address);

        xblock_ptr_t &block = blocks[i];

        uint64_t height = block->get_height();
        uint64_t viewid = block->get_viewid();

        enum_result_code ret = handle_block(before_vblock, block, is_elect_chain);

        if (ret == enum_result_code::success) {
            xsync_info("[account][event] on_response_event(succ) %s height:%lu viewid:%lu", m_address.c_str(), height, viewid);
            continue;
        } else if (ret == enum_result_code::exist) {
            xsync_info("[account][event] on_response_event(exist) %s height:%lu viewid:%lu", m_address.c_str(), height, viewid);
            continue;

        } else if (ret == enum_result_code::pending_auth) {
            xsync_info("[account] on_response_event(pending_auth) %s height:%lu viewid:%lu", m_address.c_str(), height, viewid);
            m_sync_range_mgr.clear_behind_info();
            break;

        } else if (ret == enum_result_code::failed) {
            xsync_warn("[account] on_response_event(failed) %s", block->dump().c_str());
            m_sync_range_mgr.clear_behind_info();
            break;
        } else if (ret == enum_result_code::forked) {
            xsync_info("[account][event] on_response_event(forked) %s", block->dump().c_str());
            head_forked = true;
            break;
        } else {
            assert(0);
            break;
        }
    }

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->get_current_block(m_address);
    xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);

    // tail fork, the successor_block of peer node means it has correct forerunner data
    if (m_sync_range_mgr.update_progress(current_block, head_forked) < 0) {
        xsync_warn("[account][event] on_response_event(tail_forked) %s", current_vblock->dump().c_str());
    }

    uint8_t try_count = 0;
    int64_t try_time = 0;
    m_sync_range_mgr.get_try_sync_info(try_count, try_time);

    xsync_info("[account][event] on_response_event(total) %s current(height:%lu viewid:%lu) behind(try_count:%u height:%lu)", 
        m_address.c_str(), current_vblock->get_height(), current_vblock->get_viewid(), try_count, m_sync_range_mgr.get_behind_height());

    // if try_count is set, it means it's lower than cur_height, may be a old request has been override or discard.
    // when the response of the latest request comes back, it can clear this flag.
    if (try_count > 0) {
        xsync_warn("[account][event] on_response_event out of try count %s %u", m_address.c_str(), try_count);
        return;
    }

    m_request = nullptr;

    handle_next(current_block);

    if (m_request == nullptr) {
        mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_downloader_complete_t>(m_address, current_block->get_height());
        m_mbus->push_event(ev);
    }
}

void xaccount_base_t::on_behind_event(const mbus::xevent_ptr_t & e) {
    auto bme = std::static_pointer_cast<mbus::xevent_behind_block_t>(e);
    xblock_ptr_t successor_block = bme->successor_block;
    if (successor_block->get_height() == 0) {
        assert(0);
        return;
    }

    if (successor_block->get_height() == 1) {
        mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_downloader_complete_t>(m_address, 0);
        m_mbus->push_event(ev);
        return;
    }
    uint64_t behind_height = successor_block->get_height() - 1;

    if (m_address == sys_contract_beacon_timer_addr) {
        return;
    } else {

        std::string account_prefix;
        uint32_t table_id = 0;

        if (!data::xdatautil::extract_parts(m_address, account_prefix, table_id))
            return;

        if (account_prefix == sys_contract_beacon_table_block_addr) {
            // ignore
        } else if (account_prefix == sys_contract_zec_table_block_addr) {

            if (!check_behind(behind_height, sys_contract_rec_elect_rec_addr)) {
                xsync_info("[account][event] on_behind_event(depend chain is syncing) successor(%s)", successor_block->dump().c_str());
                return;
            }

        } else if (account_prefix == sys_contract_sharding_table_block_addr) {

            if (!check_behind(behind_height, sys_contract_zec_elect_consensus_addr)) {
                xsync_info("[account][event] on_behind_event(depend chain is syncing) successor(%s)", successor_block->dump().c_str());
                return;
            }

        } else {
            return;
        }
    }

    vnetwork::xvnode_address_t & self_addr = bme->self_addr;
    vnetwork::xvnode_address_t & target_addr = bme->from_addr;

    uint8_t try_count = 0;
    int64_t try_time = 0;
    m_sync_range_mgr.get_try_sync_info(try_count, try_time);

    xauto_ptr<xvblock_t> current_vblock = m_sync_store->get_current_block(m_address);
    xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);

    if ((current_block->get_height()+1) > successor_block->get_height() ||
        ((current_block->get_height()+1)==successor_block->get_height() && current_block->get_block_hash()==successor_block->get_last_block_hash())) {
        mbus::xevent_ptr_t ev = std::make_shared<mbus::xevent_downloader_complete_t>(m_address, behind_height);
        m_mbus->push_event(ev);
        return;
    }

    int ret = m_sync_range_mgr.set_behind_info(current_block, successor_block, self_addr, target_addr);

    if (ret == 0) {

        if ((current_block->get_height()+1)==successor_block->get_height() && current_block->get_block_hash()==successor_block->get_last_block_hash()) {
            xsync_info("[account][event] on_behind_event(hash-chain disconnect) current(%s) successor(%s)", current_block->dump().c_str(), successor_block->dump().c_str());
        } else {
            xsync_info("[account][event] on_behind_event %s current(height=%lu,viewid=%lu) successor(height=%lu,viewid=%lu) reason(%d) %s -> %s",
                   m_address.c_str(),
                   current_block->get_height(),
                   current_block->get_viewid(),
                   successor_block->get_height(),
                   successor_block->get_viewid(),
                   bme->source_type,
                   self_addr.to_string().c_str(),
                   target_addr.to_string().c_str());
        }

        if (bme->source_type == enum_behind_source::enum_behind_source_consensus) {
            XMETRICS_COUNTER_INCREMENT("sync_downloader_behind_from_consensus", 1);
        } else if (bme->source_type == enum_behind_source::enum_behind_source_gossip) {
            XMETRICS_COUNTER_INCREMENT("sync_downloader_behind_from_gossip", 1);
        } else if (bme->source_type == enum_behind_source::enum_behind_source_newblock) {
            XMETRICS_COUNTER_INCREMENT("sync_downloader_behind_from_newblock", 1);
        } else if (bme->source_type == enum_behind_source::enum_behind_source_newblockhash) {
            XMETRICS_COUNTER_INCREMENT("sync_downloader_behind_from_newblockhash", 1);
        }

        XMETRICS_COUNTER_INCREMENT("sync_downloader_block_behind", 1);

        if (try_count == 0) {
            clear();
            handle_next(current_block);
        } else {
            // if sync cost > 2000ms, discard current request
            if ((get_time() - try_time) > 2000) {
                xsync_info("[account][event] on_behind_event(discard current request) %s", m_address.c_str());
                clear();
                handle_next(current_block);
            }
        }
    } else {
        xsync_info("[account][event] on_behind_event(update failed ret=%d) current(%s) successor(%s)", ret, current_block->dump().c_str(), successor_block->dump().c_str());
    }
}

bool xaccount_base_t::is_idle() {
    return false;
}

enum_result_code xaccount_base_t::handle_block(const base::xauto_ptr<base::xvblock_t> &current_block, xblock_ptr_t &block, bool is_elect_chain) {

    if (is_elect_chain) {
        if (!check_auth(m_certauth, block)) {
            return enum_result_code::pending_auth;
        }
    }

    if (current_block->get_height() > block->get_height() || 
        (current_block->get_height() == block->get_height() && current_block->get_block_hash() == block->get_block_hash())) {
        return enum_result_code::exist;
    }

    base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
    bool ret = m_sync_store->store_block(vblock);
    if (ret) {

        base::xauto_ptr<base::xvblock_t> new_current_block = m_sync_store->get_current_block(m_address);
        if (new_current_block->get_height() < current_block->get_height()) {
            return enum_result_code::forked;
        }

        // same height with multi view is allowed
        return enum_result_code::success;
    }

    return enum_result_code::failed;
}

int64_t xaccount_base_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

bool xaccount_base_t::check_behind(uint64_t height, const char *elect_address) {
    base::xauto_ptr<base::xvblock_t> blk_commit = m_sync_store->get_latest_committed_block(elect_address);

    if (blk_commit == nullptr) {
        xsync_info("[account][event] on_behind_event(wait_auth_chain) %s event:%lu %s(0,0)",
                m_address.c_str(), height, elect_address);
        return false;
    }

    // TODO consider if latest_fullunit is 0?
    base::xauto_ptr<base::xvblock_t> blk_connect = m_sync_store->get_latest_connected_block(elect_address);

    if (blk_connect == nullptr) {
        xsync_info("[account][event] on_behind_event(wait_auth_chain) %s event:%lu %s(0,%lu)",
                m_address.c_str(), height, elect_address, blk_commit->get_height());
        return false;
    }

    if (blk_connect->get_height() < blk_commit->get_height()) {
        xsync_info("[account][event] on_behind_event(wait_auth_chain) %s event:%lu %s(%lu,%lu)",
                m_address.c_str(), height, elect_address, blk_connect->get_height(), blk_commit->get_height());
        return false;
    }

    return true;
}

int64_t xaccount_base_t::get_next_send_time() {
    if (m_request == nullptr)
        return 0;

    if (m_request->send_time == 0) {
        int64_t next_time = m_request->try_time + GET_TOKEN_RETRY_INTERVAL;
        return next_time;
    }

    return 0;
}

void xaccount_base_t::send_request(int64_t now) {

    if (m_request == nullptr)
        return;

    if (m_request->send_time != 0)
        return;

    if (!m_ratelimit->consume(now)) {
        xsync_dbg("[account] get token failed. %s", m_address.c_str());
        m_request->try_time = now;
        return;
    }

    XMETRICS_COUNTER_INCREMENT("sync_downloader_request", 1);

    m_request->send_time = now;

    int64_t queue_cost = m_request->send_time - m_request->create_time;

    xsync_info("[account] send sync request(block). %s range[%lu,%lu] get_token_cost(%ldms) %s",
                m_request->owner.c_str(), m_request->start_height, m_request->start_height+m_request->count-1, 
                queue_cost, m_request->target_addr.to_string().c_str());

    m_sync_sender->send_get_blocks(m_request->owner, m_request->start_height, m_request->count, m_request->self_addr, m_request->target_addr);
}

void xaccount_base_t::handle_next(const xblock_ptr_t &current_block) {
    vnetwork::xvnode_address_t self_addr;
    vnetwork::xvnode_address_t target_addr;

    int count_limit = BATCH_SIZE;
    uint64_t start_height = 0;
    uint32_t count = 0;

    m_sync_range_mgr.get_next_behind(current_block, count_limit, start_height, count, self_addr, target_addr);
    //xsync_info("[account] next_behind %s range[%lu,%lu]", m_address.c_str(), start_height, start_height+count-1);

    if (count > 0) {
        // xsync_dbg("get_next_range=%s size=%d", m_address.c_str(), next_blocks.size());

        int64_t now = get_time();

        xentire_block_request_ptr_t req = m_sync_mgr.create_request(start_height, count);
        if (req != nullptr) {
            req->self_addr = self_addr;
            req->target_addr = target_addr;
            req->create_time = now;
            req->try_time = 0;
            req->send_time = 0;
            m_request = req;
            send_request(now);
        }
    }
}

void xaccount_base_t::clear() {
    m_request = nullptr;
}

static void get_elect_info(const xvip2_t & target_group, std::string & elect_address, uint64_t & elect_height) {
    // TODO use macro

    uint8_t zone_id = get_zone_id_from_xip2(target_group);
    uint8_t cluster_id = get_cluster_id_from_xip2(target_group);
    uint8_t group_id = get_group_id_from_xip2(target_group);

    if (zone_id == 0) {
        if (cluster_id == 1) {
            if (group_id >= 1 && group_id < 127)
                elect_address = sys_contract_zec_elect_consensus_addr;
        }
    } else if (zone_id == 1) {
        if (cluster_id == 0 && group_id == 0)
            elect_address = sys_contract_rec_elect_rec_addr;
    } else if (zone_id == 2) {
        if (cluster_id == 0 && group_id == 0)
            elect_address = sys_contract_rec_elect_zec_addr;
    }

    elect_height = get_network_height_from_xip2(target_group);
}

dependency_info_t xaccount_base_t::get_depend_elect_info(const data::xblock_ptr_t & block) {
    dependency_info_t info;

    const xvip2_t & validator_xip = block->get_cert()->get_validator();
    std::string validator_elect_addr;
    uint64_t validator_elect_height = 0;
    get_elect_info(validator_xip, validator_elect_addr, validator_elect_height);
    info.m_list.push_back(elect_item_t{validator_elect_addr, validator_elect_height});

    const xvip2_t & auditor_xip = block->get_cert()->get_auditor();
    std::string auditor_elect_addr;
    uint64_t auditor_elect_height = 0;
    get_elect_info(auditor_xip, auditor_elect_addr, auditor_elect_height);
    info.m_list.push_back(elect_item_t{auditor_elect_addr, auditor_elect_height});

    return info;
}

///////////////
xaccount_general_t::xaccount_general_t(std::string vnode_id,
                                       xsync_store_face_t * sync_store,
                                       const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
                                       const observer_ptr<base::xvcertauth_t> &certauth,
                                       xsync_sender_t *sync_sender,
                                       xsync_ratelimit_face_t *ratelimit,
                                       const xchain_info_t & chain_info)
  : xaccount_base_t(vnode_id, sync_store, mbus, certauth, sync_sender, ratelimit, chain_info) {}

xaccount_general_t::~xaccount_general_t() {}

void xaccount_general_t::on_role_changed(const xchain_info_t & chain_info) {
    enum_role_changed_result ret = m_sync_range_mgr.on_role_changed(chain_info);
    if (ret == enum_role_changed_result_none) {
        return;
    }

    xsync_dbg("[account][event] on_role_changed %s", m_address.c_str());

    if (ret == enum_role_changed_result_remove_history || ret == enum_role_changed_result_add_history) {
        clear();

        if (m_sync_range_mgr.get_behind_height() > 0) {
            xauto_ptr<xvblock_t> current_vblock = m_sync_store->get_current_block(m_address);
            xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
            handle_next(current_block);
        }
    }
}

void xaccount_general_t::on_find_block(uint64_t height, uint64_t block_time) {
    // event from source
    // cache data
#if 0
    height_info_t info1 = get_height_info();
    update_max_height(height, block_time);
    height_info_t info2 = get_height_info();

    xsync_dbg("[account][event] on_find_block %s event:%lu (%s) => (%s)",
                    m_address.c_str(),
                    height,
                    info1.to_string().c_str(),
                    info2.to_string().c_str());

    handle_next(requests);
    // TODO trigger gossip
#endif
}

void xaccount_general_t::on_lack_event(const std::set<uint64_t> & set_heights) {
// TODO
#if 0
    //TODO less than start height
    height_info_t info1 = get_height_info();
    m_sync_range_mgr.set_lack_info(set_heights);
    height_info_t info2 = get_height_info();

    xsync_dbg("[account][event] on_lack_event %s (%s) => (%s)", m_address.c_str(),
            info1.to_string().c_str(), info2.to_string().c_str());
    handle_next(requests);
#endif
}

////////////
xaccount_sequence_t::xaccount_sequence_t(std::string vnode_id,
                                         xsync_store_face_t * sync_store,
                                         const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
                                         const observer_ptr<base::xvcertauth_t> &certauth,
                                         xsync_sender_t *sync_sender,
                                         xsync_ratelimit_face_t *ratelimit,
                                         const xchain_info_t & chain_info):
xaccount_base_t(vnode_id, sync_store, mbus, certauth, sync_sender, ratelimit, chain_info) {}

xaccount_sequence_t::~xaccount_sequence_t() {}

// do nothing
void xaccount_sequence_t::on_role_changed(const xchain_info_t & chain_info) {}

void xaccount_sequence_t::on_find_block(uint64_t height, uint64_t block_time) {

    // update max_height
}

void xaccount_sequence_t::on_lack_event(const std::set<uint64_t> & set_heights) {
#if 0
    height_info_t info1 = get_height_info();
    m_sync_range_mgr.set_lack_info(set_heights);
    height_info_t info2 = get_height_info();

    xsync_info("[account][event] on_lack_event %s (%s) => (%s)",
            m_address.c_str(),
            info1.to_string().c_str(),
            info2.to_string().c_str());

    handle_next(requests);
#endif
}

NS_END2
