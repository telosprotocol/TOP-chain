// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmodule_type.h"
#include "xdata/xtable_bstate.h"

NS_BEG2(top, data)

xtable_bstate_t::xtable_bstate_t(base::xvbstate_t* bstate, bool readonly)
: xbstate_ctx_t(bstate, readonly) {
    cache_receiptid(bstate); // TODO(jimmy) delete future
}

xtable_bstate_t::~xtable_bstate_t() {
}

bool xtable_bstate_t::set_block_offsnapshot(base::xvblock_t* block, const std::string & snapshot) {
    if (block->get_block_level() != base::enum_xvblock_level_table || block->get_block_class() != base::enum_xvblock_class_full) {
        xerror("xtable_bstate_t::set_block_offsnapshot fail-not fulltable block");
        return false;
    }
    if (snapshot.empty()) {
        xerror("xtable_bstate_t::set_block_offsnapshot fail-snapshot empty");
        return false;
    }

    if (block->is_full_state_block()) {
        xwarn("xtable_bstate_t::set_block_offsnapshot already has full state. block=%s", block->dump().c_str());
        return true;
    }

    std::string binlog_hash = base::xcontext_t::instance().hash(snapshot, block->get_cert()->get_crypto_hash_type());
    if (binlog_hash != block->get_fullstate_hash()) {
        xerror("xtable_bstate_t::set_block_offsnapshot fail-snapshot hash unmatch.block=%s", block->dump().c_str());
        return false;
    }
    if (false == block->set_offblock_snapshot(snapshot)) {
        xerror("xtable_bstate_t::set_block_offsnapshot set offblock snapshot state. block=%s", block->dump().c_str());
        return false;
    }
    return true;
}

bool xtable_bstate_t::set_account_index(const std::string & account, const base::xaccount_index_t & account_index, base::xvcanvas_t* canvas) {
    std::string value;
    account_index.serialize_to(value);

    if (false == get_bstate()->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        auto propobj = get_bstate()->new_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX, canvas);
        xassert(propobj != nullptr);
    }
    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);  // TODO(jimmy) use hash map
    auto old_value = propobj->query(account);
    xassert(old_value != value);
    propobj->insert(account, value, canvas);
    return true;
}

bool xtable_bstate_t::set_account_index(const std::string & account, const base::xaccount_index_t & account_index) {
    std::string value;
    account_index.serialize_to(value);

    if (false == get_bstate()->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        get_bstate()->new_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX, get_canvas().get());
    }

    map_set(XPROPERTY_TABLE_ACCOUNT_INDEX, account, value);
    return true;
}

bool xtable_bstate_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) const {
    std::string value = map_get(XPROPERTY_TABLE_ACCOUNT_INDEX, account);
    if (value.empty()) {
        return false;
    }
    account_index.serialize_from(value);
    return true;
}

std::set<std::string> xtable_bstate_t::get_all_accounts() const {
    std::set<std::string> all_accounts;
    std::map<std::string,std::string> values = map_get(XPROPERTY_TABLE_ACCOUNT_INDEX);
    for (auto & v : values) {
        base::xaccount_index_t account_index;
        account_index.serialize_from(v.second);
        all_accounts.insert(v.first);
    }
    return all_accounts;
}

int32_t xtable_bstate_t::get_account_size() const {
    int32_t size = 0;
    map_size(XPROPERTY_TABLE_ACCOUNT_INDEX, size);
    return size;
}

bool xtable_bstate_t::find_receiptid_pair(base::xtable_shortid_t sid, base::xreceiptid_pair_t & pair) const {
    std::string field = base::xstring_utl::tostring(sid);
    std::string value = map_get(XPROPERTY_TABLE_RECEIPTID, field);
    if (value.empty()) {
        return false;
    }
    pair.serialize_from(value);
    return true;
}

void xtable_bstate_t::cache_receiptid(base::xvbstate_t* bstate) {
    m_cache_receiptid = make_receiptid_from_state(bstate);
}

base::xreceiptid_state_ptr_t xtable_bstate_t::make_receiptid_from_state(base::xvbstate_t* bstate) {
    base::xreceiptid_state_ptr_t receiptid = std::make_shared<base::xreceiptid_state_t>(bstate->get_short_table_id(), bstate->get_block_height());
    if (false == bstate->find_property(XPROPERTY_TABLE_RECEIPTID)) {
        return receiptid;
    }
    auto propobj = bstate->load_string_map_var(XPROPERTY_TABLE_RECEIPTID);
    auto all_values = propobj->query();
    for (auto & v : all_values) {
        base::xtable_shortid_t sid = (base::xtable_shortid_t)base::xstring_utl::touint32(v.first);
        base::xreceiptid_pair_t pair;
        pair.serialize_from(v.second);
        receiptid->add_pair(sid, pair);
    }
    receiptid->update_unconfirm_tx_num();  // calc and cache unconfirm tx for get performance
    return receiptid;
}

std::string xtable_bstate_t::get_receiptid_property_bin(base::xvbstate_t* bstate) {
    std::string property_receiptid_bin;
    if (bstate->find_property(XPROPERTY_TABLE_RECEIPTID)) {
        auto propobj = bstate->load_property(XPROPERTY_TABLE_RECEIPTID);
        if (propobj != nullptr) {
            propobj->serialize_to_string(property_receiptid_bin);
        }
    }
    return property_receiptid_bin;
}

bool xtable_bstate_t::set_receiptid_pair(base::xtable_shortid_t sid, const base::xreceiptid_pair_t & pair, base::xvcanvas_t* canvas) {
    if (false == get_bstate()->find_property(XPROPERTY_TABLE_RECEIPTID)) {
        auto propobj = get_bstate()->new_string_map_var(XPROPERTY_TABLE_RECEIPTID, canvas);
        xassert(propobj != nullptr);
    }

    std::string field = base::xstring_utl::tostring(sid);
    std::string value;
    pair.serialize_to(value);

    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_RECEIPTID);
    auto old_value = propobj->query(field);
    xassert(old_value != value);
    propobj->insert(field, value, canvas);
    return true;
}

bool xtable_bstate_t::set_receiptid_pair(base::xtable_shortid_t sid, const base::xreceiptid_pair_t & pair) {
    std::string field = base::xstring_utl::tostring(sid);
    std::string value;
    pair.serialize_to(value);

    if (false == get_bstate()->find_property(XPROPERTY_TABLE_RECEIPTID)) {
        get_bstate()->new_string_map_var(XPROPERTY_TABLE_RECEIPTID, get_canvas().get());
    }

    int32_t ret = map_set(XPROPERTY_TABLE_RECEIPTID, field, value);
    return ret == xsuccess;
}

NS_END2
