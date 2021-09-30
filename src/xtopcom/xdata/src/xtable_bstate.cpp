// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xtable_bstate.h"

NS_BEG2(top, data)

xtable_bstate_t::xtable_bstate_t(base::xvbstate_t* bstate) {
    bstate->add_ref();
    m_bstate.attach(bstate);

    cache_receiptid();
}

xtable_bstate_t::~xtable_bstate_t() {
    m_bstate->close();  // must do close firstly
    m_bstate = nullptr;
}

std::string xtable_bstate_t::make_snapshot() {
    std::string property_snapshot;
    auto canvas = get_bstate()->rebase_change_to_snapshot();
    canvas->encode(property_snapshot);
    xassert(!property_snapshot.empty());
    return property_snapshot;
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

bool xtable_bstate_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) const {
    if (false == get_bstate()->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        xwarn("xtable_bstate_t::get_account_index fail-find property.account=%s,height=%ld", get_account().c_str(), get_block_height());
        return false;
    }
    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);  // TODO(jimmy) use hash map
    auto value = propobj->query(account);
    if (value.empty()) {
        return false;
    }
    account_index.serialize_from(value);
    return true;
}

std::set<std::string> xtable_bstate_t::get_all_accounts() const {
    if (false == get_bstate()->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        xwarn("xtable_bstate_t::get_account_index fail-find property.account=%s,height=%ld", get_account().c_str(), get_block_height());
        return {};
    }
    std::set<std::string> all_accounts;
    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);
    std::map<std::string,std::string> values = propobj->query();  // TODO(jimmy)
    for (auto & v : values) {
        base::xaccount_index_t account_index;
        account_index.serialize_from(v.second);
        all_accounts.insert(v.first);
    }
    return all_accounts;
}

std::set<std::string> xtable_bstate_t::get_unconfirmed_accounts() const {
    if (false == get_bstate()->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        xwarn("xtable_bstate_t::get_account_index fail-find property.account=%s,height=%ld", get_account().c_str(), get_block_height());
        return {};
    }
    std::set<std::string> unconfirmed_accounts;
    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);
    std::map<std::string,std::string> values = propobj->query();  // TODO(jimmy)
    for (auto & v : values) {
        base::xaccount_index_t account_index;
        account_index.serialize_from(v.second);
        if (account_index.is_has_unconfirm_tx()) {
            unconfirmed_accounts.insert(v.first);
        }
    }
    xdbg("xtable_bstate_t::get_unconfirmed_accounts account=%s,height=%ld,all_size=%zu,unconfirm_size=%zu",
        get_account().c_str(), get_block_height(), values.size(), unconfirmed_accounts.size());
    return unconfirmed_accounts;
}

int32_t xtable_bstate_t::get_account_size() const {
    if (false == get_bstate()->find_property(XPROPERTY_TABLE_ACCOUNT_INDEX)) {
        return 0;
    }
    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_ACCOUNT_INDEX);
    std::map<std::string,std::string> values = propobj->query();
    return (int32_t)values.size();
}

bool xtable_bstate_t::find_receiptid_pair(base::xtable_shortid_t sid, base::xreceiptid_pair_t & pair) const {
    if (false == get_bstate()->find_property(XPROPERTY_TABLE_RECEIPTID)) {
        xdbg("xtable_bstate_t::find_receiptid_pair fail-find property.account=%s,height=%ld", get_account().c_str(), get_block_height());
        return false;
    }
    auto propobj = get_bstate()->load_string_map_var(XPROPERTY_TABLE_RECEIPTID);
    std::string field = base::xstring_utl::tostring(sid);
    auto value = propobj->query(field);
    if (value.empty()) {
        return false;
    }
    pair.serialize_from(value);
    return true;
}

void xtable_bstate_t::cache_receiptid() {
    m_cache_receiptid = make_receiptid_from_state(get_bstate().get());
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

NS_END2
