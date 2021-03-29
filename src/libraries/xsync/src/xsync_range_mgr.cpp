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
#define BEHIND_TIMEOUT 60000
#define BEHIND_TRY_COUNT 1
#define TABLE_PREV_COUNT 1000

xsync_range_mgr_t::xsync_range_mgr_t(std::string vnode_id, const std::string &address, const observer_ptr<mbus::xmessage_bus_face_t> &mbus):
m_vnode_id(vnode_id),
m_address(address),
m_mbus(mbus) {
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

int xsync_range_mgr_t::set_behind_info(const xblock_ptr_t &current_block, const xblock_ptr_t &successor_block,
        const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr) {

    if (successor_block->get_height()==0 || successor_block->get_height()==1)
        return -1;

    int64_t now = get_time();

    // 1.compare current and successor
    if ((current_block->get_height()+1) > successor_block->get_height() ||
        ((current_block->get_height()+1)==successor_block->get_height() && current_block->get_block_hash()==successor_block->get_last_block_hash())) {
        return -2;
    }

    // 2.check if successor should be update
    if (successor_block->get_height() > m_successor_height) {
        // update
    } else if (successor_block->get_height() == m_successor_height) {
        if (successor_block->get_viewid() > m_successor_view_id) {
            // update
        } else if (successor_block->get_viewid() == m_successor_view_id) {
            if ((now-m_behind_update_time) < BEHIND_TIMEOUT)
                return -3;
            // update
        } else {
            return -4;
        }
    }

    m_behind_height = successor_block->get_height() - 1;
    m_behind_hash = successor_block->get_last_block_hash();

    m_successor_height = successor_block->get_height();
    m_successor_view_id = successor_block->get_viewid();

    m_behind_try_count = 0;
    m_behind_self_addr = self_addr;
    m_behind_target_addr = target_addr;
    m_behind_update_time = now;
    return 0;
}

uint64_t xsync_range_mgr_t::get_behind_height() const {
    return m_behind_height;
}

void xsync_range_mgr_t::clear_behind_info() {
    m_behind_height = 0;
    m_behind_try_count = 0;
}

void xsync_range_mgr_t::get_try_sync_info(uint8_t &try_count, int64_t &try_time) {
    try_count = m_behind_try_count;
    try_time = m_behind_try_sync_time;
}

int xsync_range_mgr_t::update_progress(const data::xblock_ptr_t &current_block, bool head_forked) {

    uint64_t current_height = current_block->get_height();
    uint64_t current_viewid = current_block->get_viewid();

    if (head_forked) {
        // ensure retry
        m_behind_try_count = 0;
        return 0;
    }

    m_behind_try_count = 0;

    if (current_height == m_behind_height) {
        if (current_block->get_block_hash() == m_behind_hash) {
            m_behind_height = 0;
        } else {
            m_behind_height = 0;
            return -1;
        }
    } else if (current_height > m_behind_height) {
        m_behind_height = 0;
        return 0;
    }

    return 0;
}

// TODO consider create time and synced height
bool xsync_range_mgr_t::get_next_behind(const data::xblock_ptr_t &current_block, bool forked, uint32_t count_limit, uint64_t &start_height, uint32_t &count, 
        vnetwork::xvnode_address_t &self_addr, vnetwork::xvnode_address_t &target_addr) {

    if (m_behind_height == 0)
        return false;

    if (m_behind_try_count >= BEHIND_TRY_COUNT) {
        xsync_info("[account] behind reach max count %s %lu", m_address.c_str(), m_behind_height);
        m_behind_try_count = 0;
        m_behind_height = 0;
        return false;
    }

    if (current_block->get_height() > m_behind_height)
        return false;

    if (current_block->get_height() == m_behind_height) {
        if (current_block->get_block_hash() == m_behind_hash)
            return false;
    }

    if (forked) {
        if (current_block->get_height() > 1)
            start_height = current_block->get_height() - 1;
        else
            start_height = current_block->get_height();
    } else {
        start_height = current_block->get_height() + 1;
    }

    count = m_behind_height - start_height + 1;
    if (count > count_limit)
        count = count_limit;

    m_behind_try_count++;
    m_behind_try_sync_time = get_time();

    self_addr = m_behind_self_addr;
    target_addr = m_behind_target_addr;

    return count > 0;
}

int64_t xsync_range_mgr_t::get_time() {
    return base::xtime_utl::gmttime_ms();
}

NS_END2
