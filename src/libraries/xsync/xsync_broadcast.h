#pragma once

#include <mutex>
#include <unordered_map>
#include "xbasic/xns_macro.h"
#include "xsync/xsync_sender.h"
#include "xsync/xblock_keeper.h"
#include "xdata/xblock.h"

NS_BEG2(top, sync)

class xsync_broadcast_t {
public:
    xsync_broadcast_t(std::string vnode_id, 
        xrole_xips_manager_t *role_xips_mgr, xsync_sender_t *sync_sender);

    void broadcast_newblock_to_archive(const data::xblock_ptr_t &block);
    void broadcast_newblockhash(const data::xblock_ptr_t &block, const vnetwork::xvnode_address_t &network_self);

private:
    std::string m_vnode_id;
    xrole_xips_manager_t *m_role_xips_mgr;
    xsync_sender_t *m_sync_sender;
    // for broadcast to archive
    xblock_keeper_t m_block_keeper;
};

NS_END2