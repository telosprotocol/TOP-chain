// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xchain_downloader.h"

#include "xmbus/xevent_executor.h"
#include "xmetrics/xmetrics.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xgenesis_data.h"
#include "xpbase/base/top_utils.h"
#include "xsync/xsync_message.h"
#include "xdata/xfull_tableblock.h"

NS_BEG2(top, sync)

using namespace mbus;
using namespace data;

#define BATCH_SIZE 20
#define GET_TOKEN_RETRY_INTERVAL 6000

xchain_downloader_t::xchain_downloader_t(std::string vnode_id,
                                 xsync_store_face_t * sync_store,
                                 const observer_ptr<mbus::xmessage_bus_face_t> &mbus,
                                 const observer_ptr<base::xvcertauth_t> &certauth,
                                 xsync_sender_t *sync_sender,
                                 xsync_ratelimit_face_t *ratelimit,
                                 const std::string & address):
  m_vnode_id(vnode_id),
  m_sync_store(sync_store),
  m_mbus(mbus),
  m_certauth(certauth),
  m_sync_sender(sync_sender),
  m_ratelimit(ratelimit),
  m_address(address),
  m_sync_range_mgr(vnode_id, address) {
    //m_sync_range_mgr.on_role_changed(address);

    xsync_info("chain_downloader create_chain %s", m_address.c_str());

    XMETRICS_COUNTER_INCREMENT("sync_downloader_chain_count", 1);
}

xchain_downloader_t::~xchain_downloader_t() {

    xsync_info("chain_downloader destroy %s", m_address.c_str());
    XMETRICS_COUNTER_INCREMENT("sync_downloader_chain_count", -1);
}

const std::string& xchain_downloader_t::get_address() const {
    return m_address;
}

void xchain_downloader_t::on_role_changed(const xchain_info_t & chain_info) {
    enum_role_changed_result ret = m_sync_range_mgr.on_role_changed(chain_info);
    if (ret == enum_role_changed_result_none) {
        return;
    }

    xsync_dbg("chain_downloader on_role_changed %s", m_address.c_str());
}

int64_t xchain_downloader_t::get_next_timeout() {

    return get_next_send_time();
}

void xchain_downloader_t::destroy() {
    clear();
}

void xchain_downloader_t::on_timer(int64_t now) {

    int64_t next_send_time = get_next_send_time();
    if (next_send_time!=0 && now>=next_send_time) {
        send_request(now);
    }
}

void xchain_downloader_t::on_response(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) {

    uint32_t count = blocks.size();

    if (m_request == nullptr)
        return;

    if (count == 0)
        return;

    int64_t now = get_time();
    int64_t total_cost = now - m_request->send_time;

    xsync_info("chain_downloader on_response(overview) %s count(%u) cost(%ldms) %s",
        m_address.c_str(), count, total_cost, from_addr.to_string().c_str());

    m_ratelimit->feedback(total_cost, now);
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
            xsync_info("chain_downloader on_response(auth_failed) %s,height=%lu,", m_address.c_str(), block->get_height());
            m_sync_range_mgr.clear_behind_info();
            return;
        }
    }

    enum_chain_sync_policy sync_policy;
    if (!m_sync_range_mgr.get_sync_policy(sync_policy)) {
        xsync_info("chain_downloader on_response(not behind) %s,", m_address.c_str());
        return;
    }

    bool forked = false;

    // compare before and after
    for (uint32_t i = 0; i < count; i++) {
          xblock_ptr_t &block = blocks[i];

        uint64_t height = block->get_height();
        uint64_t viewid = block->get_viewid();

        enum_result_code ret = handle_block(block, is_elect_chain, sync_policy);

        if (ret == enum_result_code::success) {
            xsync_info("chain_downloader on_response(succ) %s,height=%lu,viewid=%lu,prev_hash:%s,",
                m_address.c_str(), height, viewid, to_hex_str(block->get_last_block_hash()).c_str());
        } else if (ret == enum_result_code::failed) {
            xsync_warn("chain_downloader on_response(failed) %s", block->dump().c_str());
        } else if (ret == enum_result_code::auth_failed) {
            xsync_info("chain_downloader on_response(auth_failed) %s,height=%lu,viewid=%lu,", m_address.c_str(), height, viewid);
            m_sync_range_mgr.clear_behind_info();
            break;
        } else {
            assert(0);
            break;
        }
    }

    if (m_sync_range_mgr.get_current_sync_start_height() != blocks.begin()->get()->get_height()) {
        xsync_info("chain_downloader on_response expect height is %llu, but real height is %llu", m_sync_range_mgr.get_current_sync_start_height(), blocks.begin()->get()->get_height());
        return;
    }

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->get_latest_end_block(m_address, sync_policy);
    xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);

    int ret = m_sync_range_mgr.update_progress(current_block);
    if (ret < 0) {
        xsync_warn("chain_downloader on_response(update_progress failed ret=%d) %s", ret, current_vblock->dump().c_str());
    }

    xsync_info("chain_downloader on_response(total) %s,current(height=%lu,viewid=%lu,hash=%s) behind(height=%lu)",
        m_address.c_str(), current_vblock->get_height(), current_vblock->get_viewid(), to_hex_str(current_vblock->get_block_hash()).c_str(), m_sync_range_mgr.get_behind_height());

    m_request = nullptr;

    if (sync_policy != enum_chain_sync_pocliy_fast) {
        handle_next(current_block->get_height());
        return;
    }

    xauto_ptr<xvblock_t> table_block = m_sync_store->get_latest_start_block(m_address, sync_policy);
    xblock_ptr_t block = autoptr_to_blockptr(table_block);
    xsync_info("chain_downloader on_response %s, height=%lu",m_address.c_str(), table_block->get_height());
    if (!block->is_full_state_block()) {
        xsync_info("chain_downloader on_response(chain_snapshot) %s,current(height=%lu,viewid=%lu,hash=%s) behind(height=%lu)",
        m_address.c_str(), table_block->get_height(), table_block->get_viewid(), to_hex_str(table_block->get_block_hash()).c_str(), m_sync_range_mgr.get_behind_height());
        handle_fulltable(block->get_height(), self_addr, from_addr);
        return;
    }

    block = autoptr_to_blockptr(current_vblock);
    if (block->get_height() >= m_sync_range_mgr.get_behind_height()){
        clear();
        return;
    }

    handle_next(current_block->get_height());
}

void xchain_downloader_t::on_chain_snapshot_response(const std::string &tbl_account_addr,
        const xobject_ptr_t<base::xvboffdata_t> chain_snapshot, uint64_t height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) {
    if (m_request == nullptr)
        return;

    int64_t now = get_time();
    int64_t total_cost = now - m_request->send_time;

    xsync_info("chain_downloader on_response(overview) %s  cost(%ldms) %s",
        m_address.c_str(), total_cost, from_addr.to_string().c_str());

    m_ratelimit->feedback(total_cost, now);
    XMETRICS_COUNTER_INCREMENT("sync_downloader_response", 1);
    XMETRICS_COUNTER_INCREMENT("sync_cost_peer_response", total_cost);

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->get_latest_start_block(m_address, enum_chain_sync_pocliy_fast);
    data::xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
    if (!current_block->is_full_state_block() && current_block->get_height() == height) {
        current_block->reset_block_offdata(chain_snapshot.get());
        m_sync_store->store_block(current_block.get());
    }

    int ret = m_sync_range_mgr.update_progress(autoptr_to_blockptr(current_vblock));
    if (ret < 0) {
        xsync_warn("chain_downloader on_response(update_progress failed ret=%d) %s", ret, current_vblock->dump().c_str());
    }

    xsync_info("chain_downloader on_response(total) %s,current(height=%lu,viewid=%lu,hash=%s) behind(height=%lu)",
        m_address.c_str(), current_vblock->get_height(), current_vblock->get_viewid(), to_hex_str(current_vblock->get_block_hash()).c_str(), m_sync_range_mgr.get_behind_height());

    m_request = nullptr;
    base::xauto_ptr<base::xvblock_t> vblock = m_sync_store->get_latest_end_block(m_address,enum_chain_sync_pocliy_fast);
    current_block = autoptr_to_blockptr(vblock);
    if (current_block->get_height() >= m_sync_range_mgr.get_behind_height()){
        clear();
        return;
    }
    handle_next(current_block->get_height());
}

void xchain_downloader_t::on_behind(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string &reason) {

    if (end_height == 0) {
        xsync_warn("chain_downloader on_behind(param error) %s,height=%lu,", m_address.c_str(), end_height);
        return;
    }

    std::string account_prefix;
    uint32_t table_id = 0;

    if (!data::xdatautil::extract_parts(m_address, account_prefix, table_id)){
        return;
    }
    {
        if (account_prefix == sys_contract_beacon_table_block_addr) {
            // ignore
        } else if (account_prefix == sys_contract_zec_table_block_addr) {

            if (!check_behind(end_height, sys_contract_rec_elect_rec_addr)) {
                xsync_info("chain_downloader on_behind(depend chain is syncing) %s,height=%lu,", m_address.c_str(), end_height);
                return;
            }

        } else if (account_prefix == sys_contract_sharding_table_block_addr) {

            if (!check_behind(end_height, sys_contract_zec_elect_consensus_addr)) {
                xsync_info("chain_downloader on_behind(depend chain is syncing) %s,height=%lu,", m_address.c_str(), end_height);
                return;
            }

        } else {
            return;
        }
    }

    xauto_ptr<xvblock_t> start_vblock = m_sync_store->get_latest_start_block(m_address, sync_policy);
    xauto_ptr<xvblock_t> end_vblock = m_sync_store->get_latest_end_block(m_address, sync_policy);
    xblock_ptr_t end_block = autoptr_to_blockptr(end_vblock);

    auto now = get_time();
    if (m_request != nullptr) {
        const int request_create_timeout = 5 * 1000;
        if ((now - m_request->create_time) > request_create_timeout) {
            xsync_info("chain_downloader on_behind(request life time out) %s, now %lld, last create time %lld reason=%s",
                     m_address.c_str(), now, m_request->create_time, reason.c_str());
            clear();
        } else {
            xsync_info("chain_downloader on_behind(in process) %s,local(height=%lu,) peer(height=%lu) reason=%s",
                m_address.c_str(), end_block->get_height(), end_height, reason.c_str());
            return;
        }
    } else {
        int64_t behind_time = m_sync_range_mgr.get_behind_time();
        const int behind_info_create_timeout = 30 * 1000;
        if (behind_time > 0 && (now-behind_time) > behind_info_create_timeout) {
            xsync_info("chain_downloader on_behind(behind info life time out) %s, now %lld, last create time %lld peer(height=%lu) reason=%s",
                     m_address.c_str(), now, behind_time, end_height, reason.c_str());
            clear();
        }
    }

    int ret = 0;
    if (sync_policy == enum_chain_sync_pocliy_fast) {
        ret = m_sync_range_mgr.set_behind_info(start_height, end_height, enum_chain_sync_pocliy_fast,self_addr, target_addr);
    } else {
        ret = m_sync_range_mgr.set_behind_info(start_vblock->get_height(), end_height, enum_chain_sync_pocliy_full,self_addr, target_addr);
    }

    if (ret > 0) {
        // in process
        xsync_info("chain_downloader on_behind(ret=%d) %s,local(height=%lu,) peer(height=%lu,) reason=%s",
            ret, m_address.c_str(), start_height, end_height, reason.c_str());
        return;
    } else if (ret < 0) {
        xsync_info("chain_downloader on_behind(ret=%d) %s,local(height=%lu,) peer(height=%lu,) reason=%s",
            ret, m_address.c_str(), start_height, end_height, reason.c_str());
        return;
    }

    xsync_info("chain_downloader on_behind %s,local(height=%lu,) peer(height=%lu,) reason=%s %s -> %s",
            m_address.c_str(),
            start_vblock->get_height(),
            end_height,
            reason.c_str(),
            self_addr.to_string().c_str(),
            target_addr.to_string().c_str());

    XMETRICS_COUNTER_INCREMENT("sync_downloader_block_behind", 1);
    // fast sync for table level block chain, write the full table bock first
    // then write the index snapshot
    if (sync_policy != enum_chain_sync_pocliy_fast) {
        handle_next(sync::derministic_height(end_vblock->get_height(), std::make_pair(start_height,end_height)));
        return;
    }

    data::xblock_ptr_t current_block = autoptr_to_blockptr(start_vblock);
    if (!current_block->is_full_state_block() && current_block->get_height() == start_height) {
        handle_fulltable(start_height, self_addr, target_addr);
        return;
    }

    current_block = autoptr_to_blockptr(end_vblock);
    if ((current_block->get_height() <= end_height) && (current_block->get_height() >= start_height)) {
        handle_next(sync::derministic_height(current_block->get_height(), std::make_pair(start_height,end_height)));
    } else {
        handle_next(start_height - 1);
    }
}

enum_result_code xchain_downloader_t::handle_block(xblock_ptr_t &block, bool is_elect_chain, enum_chain_sync_policy sync_policy) {

    if (is_elect_chain) {
        if (!check_auth(m_certauth, block)) {
            return enum_result_code::auth_failed;
        }
    }

    block->set_block_flag(enum_xvblock_flag_authenticated);
    base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
    bool ret = m_sync_store->store_block(vblock);
    if (ret) {
        return enum_result_code::success;
    }

    return enum_result_code::failed;
}

int64_t xchain_downloader_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

bool xchain_downloader_t::check_behind(uint64_t height, const char *elect_address) {
    base::xauto_ptr<base::xvblock_t> blk_commit = m_sync_store->get_latest_committed_block(elect_address);

    if (blk_commit == nullptr) {
        xsync_info("chain_downloader on_behind(wait_auth_chain) %s,height=%lu,%s(0,0)",
                m_address.c_str(), height, elect_address);
        return false;
    }

    // TODO consider if latest_fullunit is 0?
    base::xauto_ptr<base::xvblock_t> blk_connect = m_sync_store->get_latest_connected_block(elect_address);

    if (blk_connect == nullptr) {
        xsync_info("chain_downloader on_behind(wait_auth_chain) %s,height=%lu,%s(0,%lu)",
                m_address.c_str(), height, elect_address, blk_commit->get_height());
        return false;
    }

    if (blk_connect->get_height() < blk_commit->get_height()) {
        xsync_info("chain_downloader on_behind(wait_auth_chain) %s,height=%lu,%s(%lu,%lu)",
                m_address.c_str(), height, elect_address, blk_connect->get_height(), blk_commit->get_height());
        return false;
    }

    return true;
}

int64_t xchain_downloader_t::get_next_send_time() {
    if (m_request == nullptr)
        return 0;

    if (m_request->send_time == 0) {
        int64_t next_time = m_request->try_time + GET_TOKEN_RETRY_INTERVAL;
        return next_time;
    }

    return 0;
}

void xchain_downloader_t::send_request(int64_t now) {

    if (m_request == nullptr)
        return;

    if (m_request->send_time != 0)
        return;

    if (!m_ratelimit->get_token(now)) {
        xsync_dbg("chain_downloader get token failed. %s", m_address.c_str());
        m_request->try_time = now;
        return;
    }

    XMETRICS_COUNTER_INCREMENT("sync_downloader_request", 1);

    m_request->send_time = now;

    int64_t queue_cost = m_request->send_time - m_request->create_time;

    xsync_info("chain_downloader send sync request(block). %s,range[%lu,%lu] get_token_cost(%ldms) %s",
                m_request->owner.c_str(), m_request->start_height, m_request->start_height+m_request->count-1,
                queue_cost, m_request->target_addr.to_string().c_str());

    m_sync_sender->send_get_blocks(m_request->owner, m_request->start_height, m_request->count, m_request->self_addr, m_request->target_addr);
}

void xchain_downloader_t::send_request(int64_t now, const xsync_message_chain_snapshot_meta_t &chain_snapshot_meta) {
    if (m_request == nullptr)
        return;
    if (m_request->send_time != 0)
        return;
    if (!m_ratelimit->get_token(now)) {
        xsync_dbg("chain_downloader get token failed. %s", m_address.c_str());
        m_request->try_time = now;
        return;
    }
    XMETRICS_COUNTER_INCREMENT("sync_downloader_request", 1);
    m_request->send_time = now;
    int64_t queue_cost = m_request->send_time - m_request->create_time;
    xsync_info("chain_downloader send sync request(block). %s,range[%lu,%lu] get_token_cost(%ldms) %s",
                m_request->owner.c_str(), m_request->start_height, m_request->start_height+m_request->count-1,
                queue_cost, m_request->target_addr.to_string().c_str());
    m_sync_sender->send_chain_snapshot_meta(chain_snapshot_meta, xmessage_id_sync_chain_snapshot_request, m_request->self_addr, m_request->target_addr);
}
void xchain_downloader_t::handle_next(uint64_t current_height) {
    vnetwork::xvnode_address_t self_addr;
    vnetwork::xvnode_address_t target_addr;

    int count_limit = BATCH_SIZE;
    uint64_t start_height = 0;
    uint32_t count = 0;

    m_sync_range_mgr.get_next_behind(current_height, count_limit, start_height, count, self_addr, target_addr);
    //xsync_info("chain_downloader next_behind %s range[%lu,%lu]", m_address.c_str(), start_height, start_height+count-1);

    if (count > 0) {
        // xsync_dbg("get_next_range=%s size=%d", m_address.c_str(), next_blocks.size());

        int64_t now = get_time();

        xentire_block_request_ptr_t req = create_request(start_height, count);
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

void xchain_downloader_t::handle_fulltable(uint64_t fulltable_height_of_tablechain, const vnetwork::xvnode_address_t self_addr, const vnetwork::xvnode_address_t target_addr) {
    int64_t now = get_time();

    xsync_message_chain_snapshot_meta_t chain_snapshot_meta{m_address, fulltable_height_of_tablechain};
    xentire_block_request_ptr_t req = create_request(fulltable_height_of_tablechain, 1);
    if (req != nullptr) {
        req->self_addr = self_addr;
        req->target_addr = target_addr;
        req->create_time = now;
        req->try_time = 0;
        req->send_time = 0;
        m_request = req;
        send_request(now, chain_snapshot_meta);
    }
}

void xchain_downloader_t::clear() {
    m_request = nullptr;
    m_sync_range_mgr.clear_behind_info();
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

dependency_info_t xchain_downloader_t::get_depend_elect_info(const data::xblock_ptr_t & block) {
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

xentire_block_request_ptr_t xchain_downloader_t::create_request(uint64_t start_height, uint32_t count) {

    std::string account_prefix;
    uint32_t table_id = 0;
    if (!data::xdatautil::extract_parts(m_address, account_prefix, table_id))
        return nullptr;

    xentire_block_request_ptr_t ptr = std::make_shared<xentire_block_request_t>(
                                m_address,
                                start_height,
                                count);

    xsync_info("chain_downloader create_sync_request %s range[%lu,%lu]",
            m_address.c_str(), start_height, start_height+count-1);

    return ptr;
}

NS_END2
