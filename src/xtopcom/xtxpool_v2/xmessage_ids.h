// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xvnetwork/xmessage.h"

namespace top {
namespace xtxpool_v2 {

XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_send_receipt, 0x00000001);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_recv_receipt, 0x00000002);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_recv_receipt, 0x00000003);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_confirm_receipt, 0x00000004);  // keep it for compatibility
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_push_receipt, 0x00000005);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_pull_confirm_receipt_v2, 0x00000006);
XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_receipt_id_state, 0x00000007);
// XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_neighbor_sync_req, 0x00000008);
// XDEFINE_MSG_ID(xmessage_category_txpool, xtxpool_msg_neighbor_sync_rsp, 0x00000009);

}  // namespace xtxpool_v2
}  // namespace top
