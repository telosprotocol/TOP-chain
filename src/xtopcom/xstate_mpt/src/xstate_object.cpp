// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_object.h"

#include "xevm_common/trie/xtrie_db.h"

namespace top {
namespace state_mpt {

xtop_state_object::xtop_state_object(common::xaccount_address_t const & _account, const base::xaccount_index_t & _index) : account(_account), index(_index) {}

std::shared_ptr<xtop_state_object> xtop_state_object::new_object(common::xaccount_address_t const & account, const base::xaccount_index_t & new_index) {
    return std::make_shared<xtop_state_object>(account, new_index);
}

base::xaccount_index_t xtop_state_object::get_account_index() {
    return index;
}

xbytes_t xtop_state_object::get_unit(evm_common::trie::xkv_db_face_ptr_t db) {
    if (!unit_bytes.empty()) {
        return unit_bytes;
    }
    auto hash = index.get_latest_state_hash();
    auto unit = ReadUnitWithPrefix(db, xhash256_t({hash.begin(), hash.end()}));
    unit_bytes = unit;
    return unit_bytes;
}

xstate_index_change_t xtop_state_object::set_account_index(const base::xaccount_index_t & new_index) {
    xstate_index_change_t j{account, index};
    index = new_index;
    return j;
}

xstate_index_change_t xtop_state_object::set_account_index_with_unit(const base::xaccount_index_t & new_index, const xbytes_t & unit) {
    xstate_index_change_t j{account, index};
    index = new_index;
    unit_bytes = unit;
    dirty_unit = true;
    return j;
}

}
}