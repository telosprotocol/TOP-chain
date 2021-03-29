// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <unordered_map>
#include "xbasic/xns_macro.h"
#include "xsync/xsync_sender.h"
#include "xdata/xblock.h"
#include "xsync/xrole_xips_manager.h"
#include "xsync/xsync_peerset.h"

NS_BEG2(top, sync)

class xsync_broadcast_t {
public:
    xsync_broadcast_t(std::string vnode_id, xsync_peerset_t *peerset, xsync_sender_t *sync_sender);
    void broadcast_newblockhash_to_archive_neighbors(const data::xblock_ptr_t &block);

private:
    std::string m_vnode_id;
    xsync_peerset_t *m_peerset;
    xsync_sender_t *m_sync_sender;
};

NS_END2