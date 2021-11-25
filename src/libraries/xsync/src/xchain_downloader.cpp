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
#include "xdata/xtable_bstate.h"
#include "xsync/xsync_store_shadow.h"

NS_BEG2(top, sync)

using namespace mbus;
using namespace data;

#define BATCH_SIZE 20

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


void xchain_downloader_t::destroy() {
    clear();
}

bool xchain_downloader_t::downloading(int64_t now) {
    if (!m_task.finished()){
        if (!m_task.expired()) {
            return true;
        }
        m_continuous_times++;
    }

    return false;
}

bool xchain_downloader_t::on_timer(int64_t now) {
    std::pair<uint64_t, uint64_t> interval;
    vnetwork::xvnode_address_t self_addr;
    vnetwork::xvnode_address_t target_addr;

    for (uint32_t index = (m_current_object_index + 1) % enum_chain_sync_policy_max, count = 0; count < enum_chain_sync_policy_max;
            count++, index = (index + 1) % enum_chain_sync_policy_max){
        if (!m_chain_objects[index].pick(interval, self_addr, target_addr)){
            continue;
        }

        if (index == enum_chain_sync_policy_full) {
            auto shadow = m_sync_store->get_shadow();
            uint64_t genesis_height = shadow->genesis_connect_height(m_address);
            if (genesis_height > m_chain_objects[index].height()) {
                interval.first = genesis_height + 1;
            } else {
                interval.first = m_chain_objects[index].height();
            }
        }

        m_current_object_index = index;
        m_task.start();
        xsync_download_command_t command(interval, (enum_chain_sync_policy)index, self_addr, target_addr);
        xsync_command_execute_result result = m_task.execute(command);
        if (finish == result) {
            m_chain_objects[index].set_height(m_chain_objects[index].picked_height() + 1);
            m_continuous_times = 0;
        } else if (abort == result) {
            m_continuous_times++;
        }

        if (m_continuous_times >= 5){
            for (uint32_t i = 0; i < enum_chain_sync_policy_max; i++) {
                m_chain_objects[i].clear();
            }

            m_continuous_times = 0;
        }

        if (abort_overflow == result){
            return false;
        }
        break;
    }

    return true;
}

void xchain_downloader_t::on_response(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) {
    xsync_on_blocks_response_command_t command(blocks, self_addr, from_addr);
    xsync_info("chain_downloader on_response %s count(%u) %s, %d",
        m_address.c_str(), blocks.size(), from_addr.to_string().c_str(), m_task.finished());
    if ((!m_task.finished()) && (!m_task.expired())) {
        xsync_command_execute_result result = m_task.execute(command);
        if (finish == result) {
            m_chain_objects[m_current_object_index].set_height(m_chain_objects[m_current_object_index].picked_height() + 1);
            m_continuous_times = 0;
        } else if (abort == result) {
            m_continuous_times++;
        } else {
             m_continuous_times = 0;
        }
    }
}

void xchain_downloader_t::on_chain_snapshot_response(
        const std::string & chain_snapshot, uint64_t height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) {
    xsync_on_snapshot_response_command_t command(chain_snapshot, height, self_addr, from_addr);
    if ((!m_task.finished()) && (!m_task.expired())) {
        xsync_command_execute_result result = m_task.execute(command);
        if (finish == result) {
            m_chain_objects[m_current_object_index].set_height(m_chain_objects[m_current_object_index].picked_height() + 1);
            m_continuous_times = 0;
        } else if (abort == result) {
            m_continuous_times++;
        } else {
            m_continuous_times = 0;
        }
    }
}

void xchain_downloader_t::on_behind(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string &reason) {
    if ((start_height > end_height) || end_height == 0) {
        return;
    }

    uint64_t height = 0;
    uint64_t picked_height = 0;
    height = m_chain_objects[sync_policy].height();
    picked_height = m_chain_objects[sync_policy].picked_height();
    m_chain_objects[sync_policy] = xchain_object_t{start_height, end_height, self_addr, target_addr};
    auto shadow = m_sync_store->get_shadow();
    if (sync_policy == enum_chain_sync_policy_full) {
        uint64_t now = base::xtime_utl::gmttime_ms();
        uint64_t refresh_time = shadow->genesis_height_refresh_time_ms(m_address);
        if (refresh_time > m_refresh_time) {
            m_refresh_time = refresh_time;
        }

        if (now < m_refresh_time) {
            m_refresh_time = now;
        }

        if (now - m_refresh_time > 120000) {
            m_refresh_time = now;
            uint64_t genesis_height = shadow->genesis_connect_height(m_address);
            height = genesis_height + 1;
            xsync_dbg("chain_downloader genesis height sticky too long, account is %s, genesis height %llu", m_address.c_str(), height);
        }
    }

    if (height >= start_height) {
        m_chain_objects[sync_policy].set_height(height);
    } else {
        m_chain_objects[sync_policy].set_height(start_height);
    }
    m_chain_objects[sync_policy].set_picked_height(picked_height);

    xsync_info("chain_downloader on_behind expect start_height=%lu, end_height=%llu, target address %s, sync policy %d, chain is %s",
                start_height, end_height, target_addr.to_string().c_str(), sync_policy, m_address.c_str());
}

void xchain_downloader_t::on_block_committed_event(uint64_t height) {
    xsync_on_commit_event_command_t command(height);
    if ((!m_task.finished()) && (!m_task.expired())) {
        xsync_command_execute_result result = m_task.execute(command);
        if (finish == result) {
            m_chain_objects[m_current_object_index].set_height(m_chain_objects[m_current_object_index].picked_height() + 1);
            m_continuous_times = 0;
        } else if (abort == result) {
            m_continuous_times++;
        } else {
            m_continuous_times = 0;
        }
    }
}


enum_result_code xchain_downloader_t::pre_handle_block(
    std::vector<data::xblock_ptr_t> &blocks,
    bool is_elect_chain,
    uint64_t quota_height,
    std::vector<base::xvblock_t*> &processed_blocks) {

    for (auto block : blocks){
        if (is_elect_chain) {
            if (!check_auth(m_certauth, block)) {
                xsync_warn("chain_downloader check auth fail, block is: %s", block->dump().c_str());
                return enum_result_code::auth_failed;
            }
        }

        //temperary code
        auto vbindex = m_sync_store->load_block_object(block->get_block_owner(), block->get_height(), false, block->get_viewid());
        if (vbindex == nullptr) {
        //XTODO,need doublecheck whether allow set flag of authenticated without verify signature
            block->set_block_flag(enum_xvblock_flag_authenticated);
            base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
            wait_committed_event_group(block->get_height(), quota_height);
        } else {
            if (vbindex->check_block_flag(enum_xvblock_flag_committed)) {
                m_sync_store->get_shadow()->on_chain_event(block->get_block_owner(), block->get_height());
            } else {
                wait_committed_event_group(block->get_height(), quota_height);
            }
        }

        processed_blocks.push_back(dynamic_cast<base::xvblock_t*>(block.get()));
    }

    return enum_result_code::success;
}

enum_result_code xchain_downloader_t::handle_block(xblock_ptr_t &block, bool is_elect_chain, uint64_t quota_height) {
    if (is_elect_chain) {
        if (!check_auth(m_certauth, block)) {
            return enum_result_code::auth_failed;
        }
    }

    //temperary code
    auto vbindex = m_sync_store->load_block_object(block->get_block_owner(), block->get_height(), false, block->get_viewid());
    if (vbindex == nullptr) {
    //XTODO,need doublecheck whether allow set flag of authenticated without verify signature
        block->set_block_flag(enum_xvblock_flag_authenticated);
        
        base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
        wait_committed_event_group(block->get_height(), quota_height);
        bool ret = m_sync_store->store_block(vblock);
        if (!ret) {
            return enum_result_code::failed;
        }
    } else {
        if (vbindex->check_block_flag(enum_xvblock_flag_committed)) {
            m_sync_store->get_shadow()->on_chain_event(block->get_block_owner(), block->get_height());
        } else {
            wait_committed_event_group(block->get_height(), quota_height);
        }
    }

    return enum_result_code::success;
}

int64_t xchain_downloader_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

bool xchain_downloader_t::check_behind(uint64_t height, const char *elect_address) {
    //to be deleted
    uint64_t blk_commit_height = m_sync_store->get_latest_committed_block_height(elect_address);
    uint64_t blk_connect_height = m_sync_store->get_latest_connected_block_height(elect_address);
    if (blk_connect_height < blk_commit_height) {
        xsync_info("chain_downloader on_behind(wait_auth_chain) %s,height=%lu,%s(%lu,%lu)",
                m_address.c_str(), height, elect_address, blk_connect_height, blk_commit_height);
        return false;
    }

    return true;
}

bool xchain_downloader_t::send_request(int64_t now) {
    if (!m_ratelimit->get_token(now)) {
        xsync_dbg("chain_downloader get token failed. %s", m_address.c_str());
        XMETRICS_COUNTER_INCREMENT("xsync_downloader_overflow", 1);
        return false;
    }

    XMETRICS_COUNTER_INCREMENT("sync_downloader_request", 1);
    m_request->send_time = now;
    int64_t queue_cost = m_request->send_time - m_request->create_time;
    xsync_info("chain_downloader send sync request(block). %s,range[%lu,%lu] get_token_cost(%ldms) %s",
                m_request->owner.c_str(), m_request->start_height, m_request->start_height+m_request->count-1,
                queue_cost, m_request->target_addr.to_string().c_str());
    return m_sync_sender->send_get_blocks(m_request->owner, m_request->start_height, m_request->count, m_request->self_addr, m_request->target_addr);
}

bool xchain_downloader_t::send_request(int64_t now, const xsync_message_chain_snapshot_meta_t &chain_snapshot_meta) {
    if (!m_ratelimit->get_token(now)) {
        xsync_dbg("chain_downloader get token failed. %s", m_address.c_str());
        return false;
    }

    XMETRICS_COUNTER_INCREMENT("sync_downloader_request", 1);
    m_request->send_time = now;
    int64_t queue_cost = m_request->send_time - m_request->create_time;
    xsync_info("chain_downloader send sync request(block). %s,range[%lu,%lu] get_token_cost(%ldms) %s",
                m_request->owner.c_str(), m_request->start_height, m_request->start_height+m_request->count-1,
                queue_cost, m_request->target_addr.to_string().c_str());
    return m_sync_sender->send_chain_snapshot_meta(chain_snapshot_meta, xmessage_id_sync_chain_snapshot_request, m_request->self_addr, m_request->target_addr);
}

xsync_command_execute_result xchain_downloader_t::handle_next(uint64_t current_height) {
    vnetwork::xvnode_address_t self_addr;
    vnetwork::xvnode_address_t target_addr;

    int count_limit = BATCH_SIZE;
    uint64_t start_height = 0;
    uint32_t count = 0;
    uint64_t height = current_height;

    do {
        m_sync_range_mgr.get_next_behind(height, count_limit, start_height, count, self_addr, target_addr);
        if (count == 0) {
            break;
        }
        auto interval = m_sync_store->get_shadow()->get_continuous_unused_interval(m_address, std::make_pair(start_height, start_height + count - 1));
        if (interval.second == 0) {
            height = start_height + count - 1;          
            start_height = 0;
            count = 0;
        } else {
            start_height = interval.first;
            count = interval.second - interval.first + 1;
            m_sync_range_mgr.set_current_sync_start_height(start_height);
            break;
        }
    } while (true);
    
    if (count > 0) {
        int64_t now = get_time();

        xentire_block_request_ptr_t req = create_request(start_height, count);
        if (req != nullptr) {
            req->self_addr = self_addr;
            req->target_addr = target_addr;
            req->create_time = now;
            req->try_time = 0;
            req->send_time = 0;
            m_request = req;
            if (send_request(now)){
                return wait_response;
            } else {
                return abort_overflow;
            }
        }

        return abort;
    }

    return finish;
}

bool xchain_downloader_t::handle_fulltable(uint64_t fulltable_height_of_tablechain, const vnetwork::xvnode_address_t self_addr, const vnetwork::xvnode_address_t target_addr) {
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
        return send_request(now, chain_snapshot_meta);
    }
    return false;
}

void xchain_downloader_t::clear() {
    m_request = nullptr;
    m_sync_range_mgr.clear_behind_info();
    init_committed_event_group();
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
    xentire_block_request_ptr_t ptr = std::make_shared<xentire_block_request_t>(
                                m_address,
                                start_height,
                                count);

    xsync_info("chain_downloader create_sync_request %s range[%lu,%lu]",
            m_address.c_str(), start_height, start_height+count-1);

    return ptr;
}

xsync_command_execute_result xchain_downloader_t::execute_download(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr, const std::string &reason) {
    std::string account_prefix;
    uint32_t table_id = 0;

    if (m_address == sys_drand_addr) {
    } else {
        if (!data::xdatautil::extract_parts(m_address, account_prefix, table_id)) {
            xsync_dbg("xchain_downloader_t::execute_download check fail : %s", m_address.c_str());
            return abort;
        }
        if (account_prefix == sys_contract_beacon_table_block_addr) {
            // ignore
        } else if (account_prefix == sys_contract_zec_table_block_addr) {
            if (!check_behind(end_height, sys_contract_rec_elect_rec_addr)) {
                xsync_info("chain_downloader on_behind(depend chain is syncing) %s,height=%lu,", m_address.c_str(), end_height);
                return abort;
            }
        } else if (account_prefix == sys_contract_sharding_table_block_addr) {
            if (!check_behind(end_height, sys_contract_zec_elect_consensus_addr)) {
                xsync_info("chain_downloader on_behind(depend chain is syncing) %s,height=%lu,", m_address.c_str(), end_height);
                return abort;
            }
        } else {
            xsync_dbg("xchain_downloader_t::execute_download check fail: %s", m_address.c_str());
            return abort;
        }
    }

    int ret = 0;    
    if (sync_policy == enum_chain_sync_policy_fast) {
        ret = m_sync_range_mgr.set_behind_info(start_height, end_height, enum_chain_sync_policy_fast, self_addr, target_addr);
    } else {
        ret = m_sync_range_mgr.set_behind_info(start_height, end_height, enum_chain_sync_policy_full,self_addr, target_addr);
    }

    if (!ret) {
        return finish;
    }

    xsync_info("chain_downloader on_behind %s,local(height=%lu,) peer(height=%lu,) reason=%s %s -> %s, sync policy %d",
            m_address.c_str(),
            start_height,
            end_height,
            reason.c_str(),
            self_addr.to_string().c_str(),
            target_addr.to_string().c_str(),
            sync_policy);

    XMETRICS_COUNTER_INCREMENT("sync_downloader_block_behind", 1);

    uint64_t height = start_height;

    if (sync_policy == enum_chain_sync_policy_fast) {
        xauto_ptr<xvblock_t> start_vblock = m_sync_store->get_latest_start_block(m_address, sync_policy);
        data::xblock_ptr_t current_block = autoptr_to_blockptr(start_vblock);
        if (!current_block->is_full_state_block() && current_block->get_height() == start_height) {
            if (!handle_fulltable(start_height, self_addr, target_addr)){
                return abort_overflow;
            }
            return wait_response;
        }

        uint64_t latest_end_height = m_sync_store->get_latest_end_block_height(m_address, sync_policy);
        height = sync::derministic_height(latest_end_height, std::make_pair(start_height,end_height));
    }

    if (height > 0){
        return handle_next(height - 1);
    } else {
        return handle_next(height);
    }
}

xsync_command_execute_result xchain_downloader_t::execute_next_download(std::vector<data::xblock_ptr_t> &blocks, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) {
    uint32_t count = blocks.size();
    if (count == 0) {
        return ignore;
    }

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

    if (!is_elect_chain){
        xblock_ptr_t &block = blocks[count-1];
        if (!check_auth(m_certauth, block)) {
            xsync_info("chain_downloader on_response(auth_failed) %s,height=%lu,", m_address.c_str(), block->get_height());
            return ignore;
        }
    }

    enum_chain_sync_policy sync_policy;
    if (!m_sync_range_mgr.get_sync_policy(sync_policy)) {
        xsync_info("chain_downloader on_response(not behind) %s,", m_address.c_str());
        return abort;
    }

    if (m_sync_range_mgr.get_current_sync_start_height() != blocks.begin()->get()->get_height()) {
        xsync_info("chain_downloader on_response expect height is %llu, but real height is %llu", m_sync_range_mgr.get_current_sync_start_height(), blocks.begin()->get()->get_height());
        return ignore;
    }

    auto next_block = blocks[blocks.size() - 1];
    init_committed_event_group();
    #if 0
    std::vector<top::base::xvblock_t *> processed_blocks;
    enum_result_code ret = pre_handle_block(blocks, is_elect_chain, next_block->get_height(), processed_blocks);
    if (ret != enum_result_code::success) {
        init_committed_event_group();
        return abort;
    }

    if (!m_sync_store->store_blocks(processed_blocks)) {
        xsync_warn("chain_downloader on_response(failed) fail to store blocks");
    }
    #endif

    // compare before and after
    for (uint32_t i = 0; i < count; i++) {
        xblock_ptr_t &block = blocks[i];
        enum_result_code ret = handle_block(block, is_elect_chain, next_block->get_height());

        if (ret == enum_result_code::success) {
            xsync_dbg("chain_downloader on_response(succ) %s,height=%lu,viewid=%lu,prev_hash:%s,",
                m_address.c_str(), block->get_height(), block->get_viewid(), to_hex_str(block->get_last_block_hash()).c_str());
        } else if (ret == enum_result_code::failed) {
            xsync_warn("chain_downloader on_response(failed) reason %d, block is: %s", ret, block->dump().c_str());
        }
    }

    if (sync_policy == enum_chain_sync_policy_fast) {
        xauto_ptr<xvblock_t> table_block = m_sync_store->get_latest_start_block(m_address, sync_policy);
        xblock_ptr_t block = autoptr_to_blockptr(table_block);
        xsync_info("chain_downloader on_response %s, height=%lu",m_address.c_str(), table_block->get_height());
        if (!block->is_full_state_block()) {
            init_committed_event_group();
            m_sync_range_mgr.set_current_sync_start_height(next_block->get_height());
            xsync_info("chain_downloader on_response(chain_snapshot) %s,current(height=%lu,viewid=%lu,hash=%s) behind(height=%lu)",
            m_address.c_str(), table_block->get_height(), table_block->get_viewid(), to_hex_str(table_block->get_block_hash()).c_str(), m_sync_range_mgr.get_behind_height());
            if (!handle_fulltable(block->get_height(), self_addr, from_addr)) {
                return abort_overflow;
            }
            return wait_response;
        }
    }

    xsync_info("chain_downloader on_response(total) %s,current(height=%lu,viewid=%lu,hash=%s) behind(height=%lu)",
        m_address.c_str(), next_block->get_height(), next_block->get_viewid(), to_hex_str(next_block->get_block_hash()).c_str(), m_sync_range_mgr.get_behind_height());

    if (notified_committed_event_group()) {
        return handle_next(next_block->get_height());
    }

    return wait_response;
}

xsync_command_execute_result xchain_downloader_t::execute_next_download(const std::string & chain_snapshot,
    uint64_t height, const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &from_addr) {

    int64_t now = get_time();
    int64_t total_cost = now - m_request->send_time;

    xsync_info("chain_downloader on_snapshot_response(overview) %s  cost(%ldms) %s",
        m_address.c_str(), total_cost, from_addr.to_string().c_str());

    m_ratelimit->feedback(total_cost, now);
    XMETRICS_COUNTER_INCREMENT("sync_downloader_response", 1);
    XMETRICS_COUNTER_INCREMENT("sync_cost_peer_response", total_cost);

    base::xauto_ptr<base::xvblock_t> current_vblock = m_sync_store->get_latest_start_block(m_address, enum_chain_sync_policy_fast);
    data::xblock_ptr_t current_block = autoptr_to_blockptr(current_vblock);
    if (current_block->is_fullblock() && !current_block->is_full_state_block() && current_block->get_height() == height) {
        if (false == xtable_bstate_t::set_block_offsnapshot(current_vblock.get(), chain_snapshot)) {
            xsync_error("chain_downloader on_snapshot_response invalid snapshot. block=%s", current_vblock->dump().c_str());
            return abort;
        }
        xsync_dbg("chain_downloader on_snapshot_response valid snapshot. block=%s", current_vblock->dump().c_str());
        m_sync_store->store_block(current_block.get());
    }

    xsync_info("chain_downloader on_snapshot_response(total) %s,current(height=%lu,viewid=%lu,hash=%s) behind(height=%lu)",
        m_address.c_str(), current_vblock->get_height(), current_vblock->get_viewid(), to_hex_str(current_vblock->get_block_hash()).c_str(), m_sync_range_mgr.get_behind_height());

    return handle_next(m_sync_range_mgr.get_current_sync_start_height());
}

xsync_command_execute_result xchain_downloader_t::execute_next_download(uint64_t height) {
    if (!notify_committed_event_group(height)) {
        return ignore;
    }

    if (notified_committed_event_group()) {
        return handle_next(height + 2);
    }

    return wait_response;
}

void xchain_downloader_t::init_committed_event_group(){
    m_wait_committed_event_group.clear();
}

void xchain_downloader_t::wait_committed_event_group(uint64_t height, uint64_t quota_height) {
    if (height <= (quota_height - xsync_store_t::m_undeterministic_heights)) {
        m_wait_committed_event_group.insert(height);
    }
}

bool xchain_downloader_t::notify_committed_event_group(uint64_t height) {
    if (m_wait_committed_event_group.empty()) {
        return false;
    }

    if (height != *(m_wait_committed_event_group.begin())) {
        return false;
    }

    m_wait_committed_event_group.erase(height);

    return true;
}

bool xchain_downloader_t::notified_committed_event_group() {
    if (m_wait_committed_event_group.empty()){
        return true;
    }

    return false;
}

bool xchain_object_t::pick(std::pair<uint64_t, uint64_t> &interval, vnetwork::xvnode_address_t &self_addr, vnetwork::xvnode_address_t &target_addr){
    if (m_end_height == 0){
        return false;
    }

    if (m_current_height > m_end_height) {
        return false;
    } else {
        interval.second = (m_current_height + 130) > m_end_height ? m_end_height : m_current_height + 130;
    }

    interval.first = m_start_height;

    self_addr = m_self_addr;
    target_addr = m_target_addr;
    m_picked_height = interval.second;
    return true;
}

uint64_t xchain_object_t::height() {
    return m_current_height;
}

void xchain_object_t::set_height(uint64_t height) {
    m_current_height = height;
}

void xchain_object_t::set_picked_height(uint64_t height) {
    m_picked_height = height;
}

uint64_t xchain_object_t::picked_height() {
    return m_picked_height;
}

void xchain_object_t::clear() {
    m_start_height = 0;
    m_end_height = 0;
}

NS_END2
