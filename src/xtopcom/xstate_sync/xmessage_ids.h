// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xvnetwork/xmessage.h"

namespace top {
namespace state_sync {

XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_trie_request, 0x01);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_trie_response, 0x02);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_table_request, 0x03);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_table_response, 0x04);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_unit_request, 0x05);
XDEFINE_MSG_ID(xmessage_category_state_sync, xmessage_id_sync_unit_response, 0x06);

}  // namespace state_sync
}  // namespace top