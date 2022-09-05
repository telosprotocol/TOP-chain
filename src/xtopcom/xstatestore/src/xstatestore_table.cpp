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

xstatestore_table_cache_t::xstatestore_table_cache_t() : m_tablestate_cache(enum_max_table_state_lru_cache_max), m_unitstate_cache(enum_max_unit_state_lru_cache_max) {

}

data::xtablestate_ptr_t xstatestore_table_cache_t::get_tablestate(std::string const& block_hash) const {
    data::xtablestate_ptr_t state = nullptr;
    m_tablestate_cache.get(block_hash, state);
    XMETRICS_GAUGE(metrics::statestore_get_table_state_from_cache, state != nullptr ? 1 : 0);
    return state;
}

data::xtablestate_ptr_t xstatestore_table_cache_t::get_latest_connectted_tablestate() const {
    std::lock_guard<std::mutex> lck(m_mutex);
    return m_latest_connectted_tablestate;
}

void xstatestore_table_cache_t::init_latest_connectted_tablestate(data::xtablestate_ptr_t const& tablestate) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_latest_connectted_tablestate = tablestate;
    xdbg("xstatestore_table_cache_t::init_latest_connectted_tablestate %s", tablestate->get_bstate()->dump().c_str());
}

void xstatestore_table_cache_t::set_latest_connectted_tablestate(data::xtablestate_ptr_t const& tablestate) const {
    std::lock_guard<std::mutex> lck(m_mutex);
    m_latest_connectted_tablestate = tablestate;
    xdbg("xstatestore_table_cache_t::set_latest_connectted_tablestate %s", tablestate->get_bstate()->dump().c_str());
}

void xstatestore_table_cache_t::set_tablestate(std::string const& block_hash, data::xtablestate_ptr_t const& state) const {
    m_tablestate_cache.put(block_hash, state);
    xdbg("xstatestore_table_cache_t::set_tablestate hash=%s,state=%s", base::xstring_utl::to_hex(block_hash).c_str(), state->get_bstate()->dump().c_str());
}

data::xunitstate_ptr_t xstatestore_table_cache_t::get_unitstate(std::string const& block_hash) const {
    data::xunitstate_ptr_t state = nullptr;
    m_unitstate_cache.get(block_hash, state);
    XMETRICS_GAUGE(metrics::statestore_get_table_state_from_cache, state != nullptr ? 1 : 0);
    return state;
}

void xstatestore_table_cache_t::set_unitstate(std::string const& block_hash, data::xunitstate_ptr_t const& state) const {
    m_unitstate_cache.put(block_hash, state);
    xdbg("xstatestore_table_cache_t::set_unitstate hash=%s,state=%s", base::xstring_utl::to_hex(block_hash).c_str(), state->get_bstate()->dump().c_str());
}



//--------------xstatestore_table_t----------------
xstatestore_table_t::xstatestore_table_t(common::xaccount_address_t const&  table_addr)
: m_table_addr(table_addr) {
    init_cache();
}

void xstatestore_table_t::init_cache() {
    data::xtablestate_ptr_t tablestate = m_store_base.get_latest_connectted_table_state(m_table_addr);
    if (nullptr == tablestate) {
        tablestate = m_store_base.get_genesis_table_state(m_table_addr);
    }
    m_table_cache.init_latest_connectted_tablestate(tablestate);
}

data::xtablestate_ptr_t xstatestore_table_t::get_latest_connectted_table_state() const {
    data::xtablestate_ptr_t cache_tablestate = m_table_cache.get_latest_connectted_tablestate();
    auto latest_commit_height = m_store_base.get_blockstore()->get_latest_connected_block_height(m_table_addr.vaccount());
    if (cache_tablestate->get_block_height() >= latest_commit_height) {
        return cache_tablestate;
    }

    data::xtablestate_ptr_t tablestate = m_store_base.get_latest_connectted_table_state(m_table_addr);
    if (nullptr != tablestate) {
        m_table_cache.set_latest_connectted_tablestate(tablestate);
        return tablestate;
    }
    return cache_tablestate;
}

data::xunitstate_ptr_t xstatestore_table_t::get_unit_state_from_accountindex(common::xaccount_address_t const & account_address, base::xaccount_index_t const& index) const {
    data::xunitstate_ptr_t unitstate = nullptr;
    if (!index.get_latest_unit_hash().empty()) {
        unitstate = m_table_cache.get_unitstate(index.get_latest_unit_hash());
        if (nullptr != unitstate) {
            xdbg("xstatestore_table_t::get_unit_state_from_accountindex succ-get from cache.addr=%s,index=%s",account_address.value().c_str(), index.dump().c_str());
            return unitstate;
        }
    }

    unitstate = m_store_base.get_unit_state_by_accountindex(account_address, index);
    if (nullptr != unitstate) {
        xdbg("xstatestore_table_t::get_unit_state_from_accountindex succ-get from store.addr=%s,index=%s",account_address.value().c_str(), index.dump().c_str());
        if (!index.get_latest_unit_hash().empty()) {
            m_table_cache.set_unitstate(index.get_latest_unit_hash(), unitstate);
        }
        return unitstate;
    }

    xwarn("xstatestore_table_t::get_unit_state_from_accountindex fail-get state.addr=%s,index=%s",account_address.value().c_str(), index.dump().c_str());
    return nullptr;
}

bool xstatestore_table_t::get_accountindex_from_table_block(common::xaccount_address_t const & account_address, base::xvblock_t * table_block, base::xaccount_index_t & account_index) const {
    evm_common::xh256_t state_root;
    auto ret = data::xblockextract_t::get_state_root(table_block, state_root);
    if (!ret) {
        xwarn("xstatestore_table_t::get_accountindex_from_table_block get_state_root fail.block:%s", table_block->dump().c_str());
        return false;
    }

    if (state_root != evm_common::xh256_t()) {
        std::error_code ec;
        xhash256_t root_hash(state_root.to_bytes()); 
        auto mpt = state_mpt::xtop_state_mpt::create(root_hash, base::xvchain_t::instance().get_xdbstore(), m_table_addr.to_string(), ec);
        if (ec) {
            xwarn("xstatestore_table_t::get_accountindex_from_table_block create mpt fail.root hash:%s.state_root:%s.block:%s", root_hash.as_hex_str().c_str(), state_root.hex().c_str(), table_block->dump().c_str());
            return false;
        }

        account_index = mpt->get_account_index(account_address.value(), ec);
        if (ec) {
            xwarn("xstatestore_table_t::get_accountindex_from_table_block get_account_index from mpt fail.root hash:%s.block:%s", root_hash.as_hex_str().c_str(), table_block->dump().c_str());
            return false;
        }
        xdbg("xstatestore_table_t::get_accountindex_from_table_block succ root hash:%s.account:%s index:%s", root_hash.as_hex_str().c_str(), account_address.value().c_str(), account_index.dump().c_str());
        return true;
    }

    data::xtablestate_ptr_t tablestate = get_table_state_from_block(table_block);
    if (nullptr == tablestate) {
        xwarn("xstatestore_table_t::get_accountindex_from_table_block fail-get table state.%s,block=%s",account_address.value().c_str(), table_block->dump().c_str());
        return false;
    }

    tablestate->get_account_index(account_address.value(), account_index);
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

data::xunitstate_ptr_t xstatestore_table_t::get_unit_state_from_block(base::xvblock_t * target_block) const {
    data::xunitstate_ptr_t unitstate = m_table_cache.get_unitstate(target_block->get_block_hash());
    if (nullptr != unitstate) {
        xdbg("xstatestore_table_t::get_unit_state_from_block succ-get from cache.block=%s",target_block->dump().c_str());
        return unitstate;
    }
    unitstate = m_store_base.get_unit_state_by_block(target_block);
    if (nullptr != unitstate) {
        xdbg("xstatestore_table_t::get_unit_state_from_block succ-get from store.block=%s",target_block->dump().c_str());
        m_table_cache.set_unitstate(target_block->get_block_hash(), unitstate);
        return unitstate;
    }

    xwarn("xstatestore_table_t::get_unit_state_from_block fail-get state.%s", target_block->dump().c_str());
    return nullptr;
}

data::xtablestate_ptr_t xstatestore_table_t::get_table_state_from_block(base::xvblock_t * target_block) const {
    data::xtablestate_ptr_t tablestate = m_table_cache.get_tablestate(target_block->get_block_hash());
    if (nullptr != tablestate) {
        xdbg("xstatestore_table_t::get_table_state_from_block succ-get from cache.block=%s",target_block->dump().c_str());
        return tablestate;
    }
    tablestate = m_store_base.get_table_state_by_block(target_block);
    if (nullptr != tablestate) {
        xdbg("xstatestore_table_t::get_table_state_from_block succ-get from store.block=%s",target_block->dump().c_str());
        m_table_cache.set_tablestate(target_block->get_block_hash(), tablestate);
        return tablestate;
    }

    xwarn("xstatestore_table_t::get_table_state_from_block fail-get state.%s", target_block->dump().c_str());
    return nullptr;
}

NS_END2
