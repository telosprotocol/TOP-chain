// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xdata/xblockextract.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xbase/xlru_cache.h"
#include "xstatestore/xstatestore_table.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, statestore)
xstatestore_table_t::xstatestore_table_t(common::xaccount_address_t const&  table_addr)
: m_table_addr(table_addr), m_table_executor(table_addr) {
    init_cache();
}

void xstatestore_table_t::init_cache() {
    xobject_ptr_t<base::xvblock_t> _block = m_store_base.get_blockstore()->get_latest_connected_block(m_table_addr.vaccount());
    if (nullptr != _block) {
        std::error_code ec;
        xtablestate_ext_ptr_t ext_tablestate = nullptr;
        m_table_executor.execute_and_get_tablestate_ext(_block.get(), ext_tablestate, ec);
        if (nullptr != ext_tablestate) {
            m_table_cache.init_latest_connectted_tablestate(ext_tablestate->get_table_state());
            return;
        }
    }
    xobject_ptr_t<base::xvblock_t> _block2 = m_store_base.get_blockstore()->get_genesis_block(m_table_addr.vaccount());
    if (nullptr != _block) {
        std::error_code ec;
        xtablestate_ext_ptr_t ext_tablestate = nullptr;
        m_table_executor.execute_and_get_tablestate_ext(_block.get(), ext_tablestate, ec);
        if (nullptr != ext_tablestate) {
            m_table_cache.init_latest_connectted_tablestate(ext_tablestate->get_table_state());
            xwarn("xstatestore_table_t::init_cache with genesis block=%s", _block->dump().c_str());
            return;
        }
    }    
}

data::xtablestate_ptr_t xstatestore_table_t::get_latest_connectted_table_state() const {
    data::xtablestate_ptr_t cache_tablestate = m_table_cache.get_latest_connectted_tablestate();
    auto latest_commit_height = m_store_base.get_blockstore()->get_latest_connected_block_height(m_table_addr.vaccount());
    if (cache_tablestate->height() >= latest_commit_height) {
        return cache_tablestate;
    }

    xobject_ptr_t<base::xvblock_t> _block = m_store_base.get_blockstore()->get_latest_connected_block(m_table_addr.vaccount());
    if (nullptr == _block) {
        xassert(false);
        return nullptr;
    }

    xtablestate_ext_ptr_t ext_tablestate = get_tablestate_ext_from_block(_block.get());
    if (nullptr != ext_tablestate) {
        m_table_cache.set_latest_connectted_tablestate(ext_tablestate->get_table_state());
        return ext_tablestate->get_table_state();
    }
    xwarn("xstatestore_table_t::get_latest_connectted_table_state return old cache tablestate.%s",cache_tablestate->get_bstate()->dump().c_str());
    return cache_tablestate;
}

data::xunitstate_ptr_t xstatestore_table_t::get_unit_state_from_block(common::xaccount_address_t const & account_address, base::xvblock_t * target_block) const {
    std::error_code ec;
    data::xunitstate_ptr_t unitstate = nullptr;
    m_table_executor.build_unitstate_by_unit(account_address, target_block, unitstate, ec);
    return unitstate;
}

data::xunitstate_ptr_t xstatestore_table_t::get_unit_state_from_accountindex(common::xaccount_address_t const & account_address, base::xaccount_index_t const& index) const {
    std::error_code ec;
    data::xunitstate_ptr_t unitstate = nullptr;
    m_table_executor.build_unitstate_by_accountindex(account_address, index, unitstate, ec);
    return unitstate;
}

void xstatestore_table_t::on_table_block_committed(base::xvblock_t* block) const {
    m_table_executor.on_table_block_committed(block);
}

xtablestate_ext_ptr_t xstatestore_table_t::do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const {
    return m_table_executor.write_table_all_states(current_block, tablestate_store, ec);
}

xtablestate_ext_ptr_t xstatestore_table_t::get_tablestate_ext_from_block(base::xvblock_t* target_block) const {
    std::error_code ec;
    xtablestate_ext_ptr_t tablestate = nullptr;
    m_table_executor.execute_and_get_tablestate_ext(target_block, tablestate, ec);
    return tablestate;
}

bool xstatestore_table_t::get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const {
    std::error_code ec;
    m_table_executor.execute_and_get_accountindex(table_block, account_address, account_index, ec);
    if (ec) {
        return false;
    }
    return true;
}

data::xunitstate_ptr_t xstatestore_table_t::get_unit_state_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block) const {
    base::xaccount_index_t account_index;
    if (false == get_accountindex_from_table_block(account_address, table_block, account_index)) {
        xwarn("xstatestore_table_t::get_unit_state_from_table_block fail-get accountindex.%s,block=%s",account_address.value().c_str(), table_block->dump().c_str());
        return nullptr;        
    }
    return get_unit_state_from_accountindex(account_address, account_index);
}

uint64_t xstatestore_table_t::get_latest_executed_block_height() const {
    return m_table_executor.get_latest_executed_block_height();
}

void xstatestore_table_t::raise_execute_height(const xstate_sync_info_t & sync_info) {
    return m_table_executor.raise_execute_height(sync_info);
}

NS_END2