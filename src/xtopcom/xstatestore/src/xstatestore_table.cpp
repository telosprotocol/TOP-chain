// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xchain_fork/xutility.h"
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
xstatestore_table_t::xstatestore_table_t(common::xtable_address_t const&  table_addr, std::shared_ptr<xstatestore_resources_t> para)
: m_table_addr(table_addr), m_table_executor(table_addr, this) {
    m_prune = std::make_shared<xstatestore_prune_t>(table_addr, para);
    m_table_executor.init();
    xdbg("xstatestore_table_t::xstatestore_table_t table=%s,this=%p", table_addr.to_string().c_str(), this);
}

xtablestate_ext_ptr_t xstatestore_table_t::get_latest_connectted_table_state() const {
    return m_table_executor.get_latest_executed_tablestate_ext();
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

bool xstatestore_table_t::on_table_block_committed_by_height(uint64_t height, const std::string & block_hash) const {
    return m_table_executor.on_table_block_committed_by_height(height, block_hash);
}

xtablestate_ext_ptr_t xstatestore_table_t::do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::map<std::string, base::xaccount_index_t> const& account_index_map, std::error_code & ec) const {
    return m_table_executor.do_commit_table_all_states(current_block, tablestate_store, account_index_map, ec);
}

xtablestate_ext_ptr_t xstatestore_table_t::get_tablestate_ext_from_block(base::xvblock_t* target_block, bool bstate_must) const {
    std::error_code ec;
    xtablestate_ext_ptr_t tablestate = m_table_executor.execute_and_get_tablestate_ext(target_block, bstate_must, ec);
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
        xwarn("xstatestore_table_t::get_unit_state_from_table_block fail-get accountindex.%s,block=%s", account_address.to_string().c_str(), table_block->dump().c_str());
        return nullptr;        
    }
    return get_unit_state_from_accountindex(account_address, account_index);
}

uint64_t xstatestore_table_t::get_latest_executed_block_height() const {
    return m_table_executor.get_latest_executed_block_height();
}
uint64_t xstatestore_table_t::get_need_sync_state_block_height() const {
    return m_table_executor.get_need_sync_state_block_height();
}

void xstatestore_table_t::raise_execute_height(const xstate_sync_info_t & sync_info) {
    return m_table_executor.raise_execute_height(sync_info);
}

// void xstatestore_table_t::state_prune() {
//     m_prune.prune(get_latest_executed_block_height());
// }

void xstatestore_table_t::on_executed(uint64_t height) {
    m_prune->on_table_block_executed(height);
}

NS_END2
