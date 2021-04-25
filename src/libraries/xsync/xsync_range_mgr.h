// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xsync/xchain_info.h"
#include "xstore/xstore_face.h"
#include "xmbus/xevent_behind.h"
#include "xdata/xblock.h"

NS_BEG2(top, sync)

enum enum_role_changed_result {
    enum_role_changed_result_none,
    enum_role_changed_result_add_history,
    enum_role_changed_result_remove_history
};

class xsync_range_mgr_t {
public:
    xsync_range_mgr_t(std::string vnode_id, const std::string &address);
    virtual ~xsync_range_mgr_t() {
    }

    enum_role_changed_result on_role_changed(const xchain_info_t &chain_info);

    int update_progress(const data::xblock_ptr_t &current_block);
    bool get_next_behind(uint64_t current_height, uint32_t count_limit, uint64_t &start_height, uint32_t &count, vnetwork::xvnode_address_t &self_addr, vnetwork::xvnode_address_t &target_addr);

    int set_behind_info(uint64_t start_height, uint64_t end_height, enum_chain_sync_policy sync_policy,
                const vnetwork::xvnode_address_t &self_addr, const vnetwork::xvnode_address_t &target_addr);

    void clear_behind_info();

    uint64_t get_behind_height() const;
    uint64_t get_current_sync_start_height() const;
    int64_t get_behind_time() const;
    bool get_sync_policy(enum_chain_sync_policy &sync_policy) const;

private:
    int64_t get_time();

private:
    std::string m_vnode_id;
    std::string m_address;
    xchain_info_t m_chain_info;

    uint64_t m_behind_height{0};
    enum_chain_sync_policy m_sync_policy;
    vnetwork::xvnode_address_t m_behind_self_addr;
    vnetwork::xvnode_address_t m_behind_target_addr;
    int64_t m_behind_time{0};
    uint64_t m_current_sync_start_height;
};

NS_END2
