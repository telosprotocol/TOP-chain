// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xsync_broadcast.h"
#include "xsync/xsync_message.h"
#include "xsync/xsync_log.h"

NS_BEG2(top, sync)

using namespace data;

xsync_broadcast_t::xsync_broadcast_t(std::string vnode_id, xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender):
m_vnode_id(vnode_id),
m_role_xips_mgr(role_xips_mgr),
m_sync_sender(sync_sender) {

}

void xsync_broadcast_t::broadcast_newblock_to_archive(const xblock_ptr_t &block) {

    if (block == nullptr)
        return;

    const std::string account = block->get_block_owner();
    bool is_table_address = data::is_table_address(common::xaccount_address_t{account});
    if (!is_table_address)
        return;

    if (!m_block_keeper.update(block))
        return;

    base::xvblock_t* vblock = dynamic_cast<base::xvblock_t*>(block.get());
    vnetwork::xvnode_address_t self_xip;
    uint32_t self_position = 0;
    uint32_t deliver_node_count = 0;

    bool ret = m_role_xips_mgr->is_consensus_role_exist(vblock);
    if (!ret)
        return;

    ret = m_role_xips_mgr->vrf_send_newblock(vblock, self_xip, self_position, deliver_node_count);

    if (ret) {

        xsync_info("[broadcast] broadcast block %s", vblock->dump().c_str());
        m_sync_sender->broadcast_newblock(block, self_xip, self_position, deliver_node_count);
    }
}

void xsync_broadcast_t::broadcast_newblockhash(const xblock_ptr_t &block, const vnetwork::xvnode_address_t &network_self) {

    xsync_info("[broadcast] broadcast blockhash %s", block->dump().c_str());
    m_sync_sender->broadcast_newblockhash(block, network_self);
}

NS_END2