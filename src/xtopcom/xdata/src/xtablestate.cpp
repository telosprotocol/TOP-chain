// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xdata/xtablestate.h"
#include "xdata/xtableblock.h"
#include "xdata/xfull_tableblock.h"

NS_BEG2(top, data)

xtablestate_t::xtablestate_t()
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    m_accountindex_state = make_object_ptr<base::xtable_mbt_new_state_t>();
    m_receiptid_state = make_object_ptr<base::xreceiptid_state_t>();
}

xtablestate_t::xtablestate_t(const std::string & full_data, uint64_t full_height, const std::string & binlog_data, uint64_t binlog_height)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    xassert(binlog_height >= full_height);
    m_accountindex_state = make_object_ptr<base::xtable_mbt_new_state_t>();
    m_receiptid_state = make_object_ptr<base::xreceiptid_state_t>();
    if (full_height > 0) {
        xassert(!full_data.empty());
        bool ret = serialize_from_full_offdata(full_data);
        xassert(ret);
    }
    if (!binlog_data.empty()) {
        bool ret = serialize_from_binlog(binlog_data);
        xassert(ret);
    }
    set_full_height(full_height);
    set_binlog_height(binlog_height);
}

xtablestate_t::xtablestate_t(const xobject_ptr_t<base::xvboffdata_t> & full_data, uint64_t full_height, const std::string & binlog_data, uint64_t binlog_height)
:base::xdataunit_t(base::xdataunit_t::enum_xdata_type_undefine) {
    xassert(binlog_height >= full_height);
    m_accountindex_state = make_object_ptr<base::xtable_mbt_new_state_t>();
    m_receiptid_state = make_object_ptr<base::xreceiptid_state_t>();
    if (full_height > 0) {
        xassert(full_data != nullptr);
        bool ret = set_block_full_data(full_data);
        xassert(ret);
    }
    if (!binlog_data.empty()) {
        bool ret = serialize_from_binlog(binlog_data);
        xassert(ret);
    }
    set_full_height(full_height);
    set_binlog_height(binlog_height);
}


int32_t xtablestate_t::do_write(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    xassert(m_receiptid_state != nullptr);
    m_receiptid_state->serialize_to(stream);
    xassert(m_accountindex_state != nullptr);
    m_accountindex_state->serialize_to(stream);
    return (stream.size() - begin_size);
}

int32_t xtablestate_t::do_read(base::xstream_t & stream) {
    const int32_t begin_size = stream.size();
    m_receiptid_state = make_object_ptr<base::xreceiptid_state_t>();
    m_receiptid_state->serialize_from(stream);
    m_accountindex_state = make_object_ptr<base::xtable_mbt_new_state_t>();
    m_accountindex_state->serialize_from(stream);
    return (begin_size - stream.size());
}

xobject_ptr_t<base::xvboffdata_t> xtablestate_t::get_block_full_data() const {
    xobject_ptr_t<base::xvboffdata_t> full_data = make_object_ptr<base::xvboffdata_t>();
    full_data->set_offdata(XTABLE_PROPERTY_ACCOUNTINDEX, m_accountindex_state->get_last_full_state());
    full_data->set_offdata(XTABLE_PROPERTY_RECEIPTID, m_receiptid_state->get_last_full_state());
    return full_data;
}
xobject_ptr_t<base::xvboffdata_t> xtablestate_t::clone_block_full_data() const {
    xobject_ptr_t<base::xvboffdata_t> full_data = get_block_full_data();
    std::string offdata_bin;
    full_data->serialize_to_string(offdata_bin);
    base::xauto_ptr<base::xvboffdata_t> _offdata = base::xvblock_t::create_offdata_object(offdata_bin);
    if (nullptr == _offdata) {
        xassert(false);
        return false;
    }
    return _offdata;
}
xobject_ptr_t<base::xvboffdata_t> xtablestate_t::get_block_binlog_data() const {
    xobject_ptr_t<base::xvboffdata_t> full_data = make_object_ptr<base::xvboffdata_t>();
    full_data->set_offdata(XTABLE_PROPERTY_ACCOUNTINDEX, m_accountindex_state->get_binlog());
    full_data->set_offdata(XTABLE_PROPERTY_RECEIPTID, m_receiptid_state->get_binlog());
    return full_data;
}

xobject_ptr_t<xtablestate_t> xtablestate_t::clone() {
    std::string bin_str;
    serialize_to_string(bin_str);
    xobject_ptr_t<xtablestate_t> newstate = make_object_ptr<xtablestate_t>();
    newstate->serialize_from_string(bin_str);
    newstate->m_full_height = m_full_height;
    newstate->m_binlog_height = m_binlog_height;
    return newstate;
}

bool xtablestate_t::set_block_full_data(const xobject_ptr_t<base::xvboffdata_t>  & full_data) {
    xobject_ptr_t<base::xtable_mbt_t> _accountindex = dynamic_xobject_ptr_cast<base::xtable_mbt_t>(full_data->query_offdata(XTABLE_PROPERTY_ACCOUNTINDEX));
    if (nullptr == _accountindex) {
        xassert(false);
        return false;
    }
    m_accountindex_state->set_last_full_state(_accountindex);

    xobject_ptr_t<base::xreceiptid_pairs_t> _receiptid = dynamic_xobject_ptr_cast<base::xreceiptid_pairs_t>(full_data->query_offdata(XTABLE_PROPERTY_RECEIPTID));
    if (nullptr == _receiptid) {
        xassert(false);
        return false;
    }
    m_receiptid_state->set_last_full_state(_receiptid);
    return true;
}
bool xtablestate_t::set_block_binlog_data(const xobject_ptr_t<base::xvboffdata_t>  & binlog_data) {
    xobject_ptr_t<base::xtable_mbt_binlog_t> _accountindex = dynamic_xobject_ptr_cast<base::xtable_mbt_binlog_t>(binlog_data->query_offdata(XTABLE_PROPERTY_ACCOUNTINDEX));
    if (nullptr == _accountindex) {
        xassert(false);
        return false;
    }
    m_accountindex_state->set_binlog(_accountindex);

    xobject_ptr_t<base::xreceiptid_pairs_t> _receiptid = dynamic_xobject_ptr_cast<base::xreceiptid_pairs_t>(binlog_data->query_offdata(XTABLE_PROPERTY_RECEIPTID));
    if (nullptr == _receiptid) {
        xassert(false);
        return false;
    }
    m_receiptid_state->set_binlog(_receiptid);
    return true;
}

bool xtablestate_t::serialize_from_full_offdata(const std::string & last_full_offdata) {
    base::xauto_ptr<base::xvboffdata_t> _offdata = base::xvblock_t::create_offdata_object(last_full_offdata);
    if (nullptr == _offdata) {
        xassert(false);
        return false;
    }
    return set_block_full_data(_offdata);
}

bool xtablestate_t::serialize_from_binlog(const std::string & binlog) {
    if (!binlog.empty()) {
        base::xauto_ptr<base::xvboffdata_t> _offdata = base::xvblock_t::create_offdata_object(binlog);
        if (nullptr == _offdata) {
            xassert(false);
            return false;
        }
        return set_block_binlog_data(_offdata);
    } else {
        m_accountindex_state->clear_binlog();
        m_receiptid_state->clear_binlog();
        return true;
    }
}

std::string xtablestate_t::serialize_to_full_data_string() const {
    xobject_ptr_t<base::xvboffdata_t> full_data = get_block_full_data();
    std::string bin_str;
    full_data->serialize_to_string(bin_str);
    return bin_str;
}

std::string xtablestate_t::serialize_to_binlog_data_string() const {
    xobject_ptr_t<base::xvboffdata_t> full_data = get_block_binlog_data();
    std::string bin_str;
    full_data->serialize_to_string(bin_str);
    return bin_str;
}

bool xtablestate_t::execute_block(base::xvblock_t* block) {
    xassert(m_binlog_height + 1 == block->get_height());
    if (block->get_block_class() == base::enum_xvblock_class_light) {
        return execute_lighttable(block);
    } else if (block->get_block_class() == base::enum_xvblock_class_full) {
        return execute_fulltable(block);
    } else if (block->get_block_class() == base::enum_xvblock_class_nil) {
        m_binlog_height = block->get_height();
        return true;
    }
    xassert(false);
    return false;
}

bool xtablestate_t::execute_lighttable(base::xvblock_t* block) {
    xtable_block_t* lighttable = dynamic_cast<xtable_block_t*>(block);
    xassert(lighttable != nullptr);

    const base::xtable_mbt_binlog_ptr_t & accountindex_binlog = m_accountindex_state->get_binlog();
    std::map<std::string, xaccount_index_t> changed_indexs = lighttable->get_units_index();
    accountindex_binlog->set_accounts_index(changed_indexs);

    base::xreceiptid_check_t receiptid_check;
    auto & units = lighttable->get_tableblock_units(true);
    xblock_t::batch_units_to_receiptids(units, receiptid_check);
    xassert(receiptid_check.check_contious(m_receiptid_state));
    receiptid_check.update_state(m_receiptid_state);
    m_binlog_height = block->get_height();
    xdbg("xtablestate_t::execute_lighttable block=%s", block->dump().c_str());
    return true;
}

bool xtablestate_t::execute_fulltable(base::xvblock_t* block) {
    xfull_tableblock_t* fulltable = dynamic_cast<xfull_tableblock_t*>(block);
    xassert(fulltable != nullptr);
    merge_new_full();
    // should clone a new fulldata, then set to block offdata
    xobject_ptr_t<base::xvboffdata_t> new_full_data = clone_block_full_data();
    std::string root_in_block = fulltable->get_offdata_hash();
    std::string root_in_state = new_full_data->build_root_hash(block->get_cert()->get_crypto_hash_type());
    xassert(root_in_block == root_in_state);
    if (root_in_block != root_in_state) {
        xerror("xtablestate_t::execute_fulltable root not match.block=%s", block->dump().c_str());
        return false;
    }
    block->reset_block_offdata(new_full_data.get());
    m_full_height = block->get_height();
    m_binlog_height = block->get_height();
    xdbg("xtablestate_t::execute_fulltable block=%s,offdata_root=%s",
        block->dump().c_str(), base::xstring_utl::to_hex(root_in_state).c_str());
    return true;
}

void xtablestate_t::merge_new_full() {
    m_accountindex_state->merge_new_full();
    m_receiptid_state->merge_new_full();
}

std::string xtablestate_t::build_root_hash() {
    xobject_ptr_t<base::xvboffdata_t> full_data = get_block_full_data();
    return full_data->build_root_hash(enum_xhash_type_sha2_256);
}

bool xtablestate_t::get_account_index(const std::string & account, base::xaccount_index_t & account_index) {
    return m_accountindex_state->get_account_index(account, account_index);
}

bool xtablestate_t::find_receiptid_pair(base::xtable_shortid_t sid, base::xreceiptid_pair_t & pair) {
    return m_receiptid_state->find_pair(sid, pair);
}

void xtablestate_t::set_account_index(const std::string & account, const base::xaccount_index_t & account_index) {
    m_accountindex_state->get_binlog()->set_account_index(account, account_index);
}

NS_END2
