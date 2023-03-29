// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_mpt/xstate_object.h"

#include "xevm_common/trie/xtrie_db.h"
#include "xmetrics/xmetrics.h"
#include "xstatistic/xbasic_size.hpp"

namespace top {
namespace state_mpt {

xtop_state_object::xtop_state_object(common::xaccount_address_t const & _account, const base::xaccount_index_t & _index) : xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_mpt_state_object), account(_account), index(_index) {
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_mpt_state_object, 1);
}

xtop_state_object::~xtop_state_object() {
    statistic_del();
    XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_mpt_state_object, -1);
}

std::shared_ptr<xtop_state_object> xtop_state_object::new_object(common::xaccount_address_t const & account, const base::xaccount_index_t & new_index) {
    return std::make_shared<xtop_state_object>(account, new_index);
}

base::xaccount_index_t xtop_state_object::get_account_index() {
    return index;
}

void xtop_state_object::set_account_index(const base::xaccount_index_t & new_index) {
    index = new_index;
}

size_t xtop_state_object::get_object_size_real() const {
    // common::xaccount_address_t account;
    // base::xaccount_index_t index;
    // xbytes_t unit_bytes;
    // bool dirty_unit{false};
    size_t total_size = sizeof(*this);
    total_size += get_size(account.to_string());
    xdbg("------cache size------ xtop_state_object total_size:%zu account:%d", total_size, get_size(account.to_string()));
    return total_size;
}

}
}