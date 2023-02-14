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
}

bool xsync_range_mgr_t::set_behind_info(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy,
        const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    int64_t now = get_time();

    if (end_height < start_height) {
        return false;
    }

    m_behind_height = end_height;
    m_sync_policy = sync_policy;
    m_behind_self_addr = self_addr;
    m_behind_target_addr = target_addr;
    m_behind_time = now;
    m_current_sync_start_height = start_height;
    return true;
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

void xsync_range_mgr_t::set_current_sync_start_height(uint64_t height) {
    m_current_sync_start_height = height;
}

void xsync_range_mgr_t::clear_behind_info() {
    m_behind_height = 0;
    m_behind_time = 0;
    m_current_sync_start_height = 0;
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
