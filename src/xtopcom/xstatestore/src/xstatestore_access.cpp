// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatestore/xstatestore_access.h"
#include "xstatestore/xerror.h"

NS_BEG2(top, statestore)

xstatestore_cache_t::xstatestore_cache_t() : m_tablestate_cache(enum_max_table_state_lru_cache_max), m_unitstate_cache(enum_max_unit_state_lru_cache_max) {

}

xtablestate_ext_ptr_t xstatestore_cache_t::get_tablestate(std::string const& block_hash) const {
    xtablestate_ext_ptr_t state = nullptr;
    m_tablestate_cache.get(block_hash, state);
    XMETRICS_GAUGE(metrics::statestore_get_table_state_from_cache, state != nullptr ? 1 : 0);
    return state;
}

xtablestate_ext_ptr_t xstatestore_cache_t::get_latest_connectted_tablestate() const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_latest_connectted_tablestate;
}

void xstatestore_cache_t::set_latest_connectted_tablestate(xtablestate_ext_ptr_t const& tablestate) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_latest_connectted_tablestate = tablestate;
    xdbg("xstatestore_cache_t::set_latest_connectted_tablestate %s", tablestate->get_table_state()->get_bstate()->dump().c_str());
}

void xstatestore_cache_t::set_tablestate(std::string const& block_hash, xtablestate_ext_ptr_t const& state) const {
    m_tablestate_cache.put(block_hash, state);
    xdbg("xstatestore_cache_t::set_tablestate hash=%s,state=%s", base::xstring_utl::to_hex(block_hash).c_str(), state->get_table_state()->get_bstate()->dump().c_str());
}

data::xunitstate_ptr_t xstatestore_cache_t::get_unitstate(std::string const& block_hash) const {
    data::xunitstate_ptr_t state = nullptr;
    m_unitstate_cache.get(block_hash, state);
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, state != nullptr ? 1 : 0);
    return state;
}

void xstatestore_cache_t::set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& state) const {
    m_unitstate_cache.put(block_hash, state);
    xdbg("xstatestore_cache_t::set_unitstate hash=%s,state=%s", base::xstring_utl::to_hex(block_hash).c_str(), state->get_bstate()->dump().c_str());
}


//============================xstatestore_dbaccess_t============================
void xstatestore_dbaccess_t::write_table_bstate(common::xaccount_address_t const& address, data::xtablestate_ptr_t const& tablestate, const std::string & block_hash, std::error_code & ec) const {
    if (tablestate->height() > 0)
        xassert(tablestate->get_block_viewid() != 0);
    XMETRICS_GAUGE(metrics::store_state_table_write, 1);
    // 1.write table bstate to db
    std::string state_db_key = base::xvdbkey_t::create_prunable_state_key(address.vaccount(), tablestate->height(), block_hash);
    std::string state_db_bin;
    int32_t ret = tablestate->get_bstate()->serialize_to_string(state_db_bin);    
    if(ret < 0 || false == m_statestore_base.get_dbstore()->set_value(state_db_key, state_db_bin)) {
        ec = error::xerrc_t::statestore_db_write_err;
        xerror("xstatestore_dbaccess_t::write_table_bstate fail-write table bstate.state=%s", tablestate->get_bstate()->dump().c_str());
        return;
    }

    xinfo("xstatestore_dbaccess_t::write_table_bstate succ.state=%s",tablestate->get_bstate()->dump().c_str());
}

data::xtablestate_ptr_t xstatestore_dbaccess_t::read_table_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    const std::string state_db_key = base::xvdbkey_t::create_prunable_state_key(address.vaccount(), height, block_hash);
    const std::string state_db_bin = m_statestore_base.get_dbstore()->get_value(state_db_key);
    if(state_db_bin.empty()) {
        XMETRICS_GAUGE(metrics::statestore_get_table_state_from_db, 0);
        xwarn("xstatestore_dbaccess_t::read_table_bstate,fail to read from db for account=%s,height=%ld,hash=%s",
              address.to_string().c_str(),
              height,
              base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> state_ptr = base::xvblock_t::create_state_object(state_db_bin);
    if(nullptr == state_ptr) {//remove the error data for invalid data
        m_statestore_base.get_dbstore()->delete_value(state_db_key);
        xerror("xstatestore_dbaccess_t::read_table_bstate,fail invalid data at db for account=%s,height=%ld,hash=%s",
               address.to_string().c_str(),
               height,
               base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }
    if (state_ptr->get_address() != address.to_string()) {
        xerror("xstatestore_dbaccess_t::read_table_bstate,fail bad state(%s) vs ask(account:%s) ", state_ptr->dump().c_str(), address.to_string().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::statestore_get_table_state_from_db, 1);
    xdbg("xstatestore_dbaccess_t::read_table_bstate succ.account=%s,height=%ld,hash=%s", address.to_string().c_str(), height, base::xstring_utl::to_hex(block_hash).c_str());
    data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(state_ptr.get());
    return tablestate;
}

void xstatestore_dbaccess_t::write_unit_bstate(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) const {
    XMETRICS_GAUGE(metrics::store_state_unit_write, 1);
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(unitstate->account_address().vaccount(), unitstate->height(), block_hash);
    std::string state_db_bin;
    int32_t ret = unitstate->get_bstate()->serialize_to_string(state_db_bin);
    if(ret > 0) {
        if (m_statestore_base.get_dbstore()->set_value(state_db_key, state_db_bin)) {
            xinfo("xstatestore_dbaccess_t::write_unit_bstate succ.state=%s,hash=%s",unitstate->get_bstate()->dump().c_str(),base::xstring_utl::to_hex(block_hash).c_str());
            return;
        }
    }
    ec = error::xerrc_t::statestore_db_write_err;
    xerror("xstatestore_dbaccess_t::write_unit_bstate fail.state=%s",unitstate->get_bstate()->dump().c_str());
}

data::xunitstate_ptr_t xstatestore_dbaccess_t::read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(address.vaccount(), height, block_hash);
    const std::string state_db_bin = m_statestore_base.get_dbstore()->get_value(state_db_key);
    if(state_db_bin.empty()) {
        XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_db, 0);
        xwarn("xstatestore_dbaccess_t::read_unit_bstate,fail to read from db for account=%s,hash=%s", address.to_string().c_str(), base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> state_ptr = base::xvblock_t::create_state_object(state_db_bin);
    if(nullptr == state_ptr) {//remove the error data for invalid data
        m_statestore_base.get_dbstore()->delete_value(state_db_key);
        xerror(
            "xstatestore_dbaccess_t::read_unit_bstate,fail invalid data at db for account=%s,hash=%s", address.to_string().c_str(), base::xstring_utl::to_hex(block_hash).c_str());
        return nullptr;
    }
    if (state_ptr->get_address() != address.to_string()) {
        xerror("xstatestore_dbaccess_t::read_unit_bstate,fail bad state(%s) vs ask(account:%s) ", state_ptr->dump().c_str(), address.to_string().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_db, 1);
    xdbg("xstatestore_dbaccess_t::read_unit_bstate succ.account=%s,hash=%s", address.to_string().c_str(), base::xstring_utl::to_hex(block_hash).c_str());
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(state_ptr.get());
    return unitstate;
}


//============================xstatestore_accessor_t============================
xstatestore_accessor_t::xstatestore_accessor_t(common::xaccount_address_t const& address)
: m_table_addr(address) {

}

xtablestate_ext_ptr_t xstatestore_accessor_t::get_latest_connectted_table_state() const {
    return m_state_cache.get_latest_connectted_tablestate();
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_cache(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    xtablestate_ext_ptr_t tablestate = m_state_cache.get_tablestate(block_hash);
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate_from_db(common::xaccount_address_t const& address, base::xvblock_t* block) const {
    auto const & stateroot = m_store_base.get_state_root_from_block(block);
    std::error_code ec;
    std::shared_ptr<state_mpt::xstate_mpt_t> mpt = state_mpt::xstate_mpt_t::create(address, stateroot, m_store_base.get_dbstore(), ec);
    if (ec) {
        xwarn("xstatestore_accessor_t::read_table_bstate_from_db fail-create mpt.block=%s", block->dump().c_str());
        return nullptr;
    }

    data::xtablestate_ptr_t table_bstate = m_dbaccess.read_table_bstate(address, block->get_height(), block->get_block_hash());
    if (nullptr == table_bstate) {
        xwarn("xstatestore_accessor_t::read_table_bstate_from_db fail-read table bstate.block=%s", block->dump().c_str());
        return nullptr;
    }

    xtablestate_ext_ptr_t tablestate = std::make_shared<xtablestate_ext_t>(table_bstate, mpt);
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_accessor_t::read_table_bstate(common::xaccount_address_t const& address, base::xvblock_t* block) const {
    auto tablestate = m_state_cache.get_tablestate(block->get_block_hash());
    if (nullptr != tablestate) {
        return tablestate;
    }
    return read_table_bstate_from_db(address, block);
}

data::xunitstate_ptr_t xstatestore_accessor_t::read_unit_bstate(common::xaccount_address_t const& address, uint64_t height, const std::string & block_hash) const {
    data::xunitstate_ptr_t unitstate = m_state_cache.get_unitstate(block_hash);
    if (nullptr != unitstate) {
        return unitstate;
    }
    return m_dbaccess.read_unit_bstate(address, height, block_hash);
}

void xstatestore_accessor_t::set_latest_connectted_tablestate(xtablestate_ext_ptr_t const& tablestate) const {
    m_state_cache.set_latest_connectted_tablestate(tablestate);
}

void xstatestore_accessor_t::write_table_bstate_to_db(common::xaccount_address_t const& address, std::string const& block_hash, data::xtablestate_ptr_t const& state, std::error_code & ec) const {    
    m_dbaccess.write_table_bstate(address, state, block_hash, ec);
    if (ec) {
        return;
    }
}

void xstatestore_accessor_t::write_table_bstate_to_cache(common::xaccount_address_t const& address, std::string const& block_hash, xtablestate_ext_ptr_t const& state) const {    
    m_state_cache.set_tablestate(block_hash, state);
}

void xstatestore_accessor_t::write_unitstate_to_db(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash, std::error_code & ec) const {
    m_dbaccess.write_unit_bstate(unitstate, block_hash, ec);
}

void xstatestore_accessor_t::write_unitstate_to_cache(data::xunitstate_ptr_t const& unitstate, const std::string & block_hash) const {
    m_state_cache.set_unitstate(block_hash, unitstate);
}


NS_END2
