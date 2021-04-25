// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_range_mgr.h"
#include <inttypes.h>
#include "xsync/xsync_log.h"
#include "xdata/xgenesis_data.h"
#include "xmbus/xevent_account.h"

NS_BEG2(top, sync)

using namespace data;
using namespace mbus;
using namespace base;

// 10s
#define TABLE_PREV_COUNT 1000

xsync_range_mgr_t::xsync_range_mgr_t(std::string vnode_id, const std::string &address):
m_vnode_id(vnode_id),
m_address(address) {
}

enum_role_changed_result xsync_range_mgr_t::on_role_changed(const xchain_info_t &chain_info) {

    return enum_role_changed_result_none;

#if 0
    if (chain_info == m_chain_info)
        return enum_role_changed_result_none;

    // for any account, active windows policy is equal under any multi-roles
    // so only need to check history policy
    if (chain_info.history_policy == m_chain_info.history_policy)
        return enum_role_changed_result_none;

    m_chain_info = chain_info;

    if (chain_info.history_policy.sync_full) {
        return enum_role_changed_result_add_history;
    }
#endif

#if 0
    bool is_table_address = data::is_table_address(common::xaccount_address_t{m_chain_info.address});

    if (is_table_address) {
        xauto_ptr<xvblock_t> block = m_data_mgr->get_latest_committed_block();
        if (block != nullptr) {
            uint64_t height = block->get_height();
            if (height > TABLE_PREV_COUNT) {
                height -= TABLE_PREV_COUNT;
            } else {
                height = 0;
            }
            m_range.start = height;
        } else {
            m_range.start = 0;
        }

    } else {
        // get latest full
        // TODO if no full-unit property?
        xauto_ptr<xvblock_t> block = m_data_mgr->get_latest_full_block();
        if (block != nullptr) {
            m_range.start = block->get_height();
        } else {
            m_range.start = 0;
        }
    }
    m_range.length = 1;
#endif
    return enum_role_changed_result_remove_history;
}

int xsync_range_mgr_t::set_behind_info(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy,
        const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    // running, ignore
    if (m_behind_height != 0)
        return 1;

    if (end_height == 0)
        return -1;

    int64_t now = get_time();

    if (end_height < start_height) {
        return -2;
    }

    m_behind_height = end_height;
    m_sync_policy = sync_policy;
    m_behind_self_addr = self_addr;
    m_behind_target_addr = target_addr;
    m_behind_time = now;
    return 0;
}

uint64_t xsync_range_mgr_t::get_behind_height() const {
    return m_behind_height;
}

int64_t xsync_range_mgr_t::get_behind_time() const {
    return m_behind_time;
}

uint64_t xsync_range_mgr_t::get_current_sync_start_height() const {
    return m_current_sync_start_height;
}

void xsync_range_mgr_t::clear_behind_info() {
    m_behind_height = 0;
    m_behind_time = 0;
    m_current_sync_start_height = 0;
}

int xsync_range_mgr_t::update_progress(const data::xblock_ptr_t &current_block) {

    uint64_t current_height = current_block->get_height();
    uint64_t current_viewid = current_block->get_viewid();

    // TODO highqc forked??
    if (current_height >= m_behind_height) {
        clear_behind_info();
    }

    return 0;
}

bool xsync_range_mgr_t::get_next_behind(uint64_t current_height, uint32_t count_limit, uint64_t &start_height, uint32_t &count,
    vnetwork::xvnode_address_t &self_addr, vnetwork::xvnode_address_t &target_addr) {

    if (m_behind_height == 0)
        return false;

    if (current_height >= m_behind_height)
        return false;

    start_height = current_height + 1;

    count = m_behind_height - start_height + 1;
    if (count > count_limit)
        count = count_limit;

    self_addr = m_behind_self_addr;
    target_addr = m_behind_target_addr;
    m_current_sync_start_height = start_height;
    return count > 0;
}

int64_t xsync_range_mgr_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

bool xsync_range_mgr_t::get_sync_policy(enum_chain_sync_policy &sync_policy) const {
    if (m_behind_height == 0)
        return false;

    sync_policy = m_sync_policy;

    return true;
}

NS_END2
