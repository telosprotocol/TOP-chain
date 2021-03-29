// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_broadcast.h"
#include "xsync/xsync_message.h"
#include "xsync/xsync_log.h"
#include "xsync/xsync_util.h"
#include "xcommon/xnode_type.h"

NS_BEG2(top, sync)

using namespace data;

xsync_broadcast_t::xsync_broadcast_t(std::string vnode_id, xsync_peerset_t *peerset, xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_peerset(peerset),
m_sync_sender(sync_sender) {

}

void xsync_broadcast_t::broadcast_newblockhash_to_archive_neighbors(const data::xblock_ptr_t &block) {

    vnetwork::xvnode_address_t self_addr;
    std::vector<vnetwork::xvnode_address_t> neighbors;

    if (!m_peerset->get_archive_group(self_addr, neighbors)) {
        xsync_warn("get archive group failed %s", block->dump().c_str());
        return;
    }

    for (auto &it: neighbors) {
        const vnetwork::xvnode_address_t &target_addr = it;
        m_sync_sender->broadcast_newblockhash(block, self_addr, target_addr);
    }
}

NS_END2
