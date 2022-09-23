// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xevm_common/trie/xtrie_kv_db_face.h"
#include "xstate_mpt/xstate_mpt_journal.h"
#include "xvledger/xaccountindex.h"

namespace top {
namespace state_mpt {

struct xtop_state_object {
    xtop_state_object(const std::string & account, const base::xaccount_index_t & new_index);
    ~xtop_state_object() = default;

    static std::shared_ptr<xtop_state_object> new_object(const std::string & account, const base::xaccount_index_t & new_index);

    base::xaccount_index_t get_account_index();
    xbytes_t get_unit(evm_common::trie::xkv_db_face_ptr_t db);
    xstate_index_change_t set_account_index(const base::xaccount_index_t & new_index);
    xstate_index_change_t set_account_index_with_unit(const base::xaccount_index_t & new_index, const xbytes_t & unit);

    std::string account;
    base::xaccount_index_t index;
    xbytes_t unit_bytes;
    bool dirty_unit{false};
};
using xstate_object_t = xtop_state_object;

}  // namespace state_mpt
}  // namespace top