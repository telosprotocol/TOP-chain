// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <unordered_map>
#include "xsync/xsync_sender.h"
#include "xdata/xblock.h"
#include "xsync/xrole_xips_manager.h"

NS_BEG2(top, sync)

extern std::vector<uint32_t> calc_push_mapping(uint32_t src_count, uint32_t dst_count, uint32_t src_self_position, uint32_t random);
extern std::set<uint32_t> calc_push_select(uint32_t dst_count, uint32_t random);

class xsync_pusher_t {
public:
    xsync_pusher_t(std::string vnode_id, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender);
    void push_newblock_to_archive(const data::xblock_ptr_t &block);

private:
    std::string m_vnode_id;
    xrole_xips_manager_t *m_role_xips_mgr;
    xsync_sender_t *m_sync_sender;
};

NS_END2