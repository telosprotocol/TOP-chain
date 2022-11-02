// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xchain_fork/xutility.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xstatectx_base.h"
#include "xstatestore/xstatestore_face.h"

NS_BEG2(top, statectx)

xstatectx_base_t::xstatectx_base_t(base::xvblock_t* prev_block, const statestore::xtablestate_ext_ptr_t & prev_table_state, base::xvblock_t* commit_block, const statestore::xtablestate_ext_ptr_t & commit_table_state, uint64_t clock)
: m_table_state(prev_table_state), m_commit_table_state(commit_table_state), m_clock(clock) {
    prev_block->add_ref();
    m_pre_block.attach(prev_block);
    commit_block->add_ref();
    m_commit_block.attach(commit_block);
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::create_proposal_bstate(base::xvblock_t* prev_block, base::xvbstate_t* prev_bstate, uint64_t clock) {
    // create proposal header, clock use to set block version
    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block, clock);
    // always clone new state
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate);
    return proposal_bstate;    
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::create_proposal_unit_bstate(std::string const& account, uint64_t height, std::string const& last_block_hash, base::xvbstate_t* prev_bstate, uint64_t clock) {
    // create proposal header, clock use to set block version
    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(account, height, last_block_hash, clock);
    // always clone new state
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate);
    return proposal_bstate;    
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::change_to_proposal_block_state(base::xaccount_index_t const& account_index, base::xvbstate_t* prev_bstate) const {
    if (account_index.get_latest_unit_hash().empty()) {
        xassert(false);
        return nullptr;
    }
    return create_proposal_unit_bstate(prev_bstate->get_account(), account_index.get_latest_unit_height()+1, account_index.get_latest_unit_hash(), prev_bstate, m_clock);
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::change_to_proposal_block_state(base::xvblock_t* prev_block, base::xvbstate_t* prev_bstate) const {
    return create_proposal_bstate(prev_block, prev_bstate, m_clock);
}

void xstatectx_base_t::sync_unit_block(const base::xvaccount_t & _vaddr, uint64_t end_height) const {
    base::xaccount_index_t commit_accountindex;
    auto ret = get_account_index(m_commit_table_state, _vaddr.get_account(), commit_accountindex);
    if (!ret) {
        return;
    }
    uint64_t latest_connect_height = get_blockstore()->get_latest_connected_block_height(_vaddr);
    data::xblocktool_t::check_lacking_unit_and_try_sync(_vaddr, commit_accountindex, latest_connect_height, get_blockstore(), "statectx");
    xinfo("xstatectx_base_t::sync_unit_block account=%s,end_h=%ld,connect_h=%ld", _vaddr.get_account().c_str(), end_height, latest_connect_height);
}

xobject_ptr_t<base::xvblock_t> xstatectx_base_t::load_block_object(const base::xvaccount_t & addr, base::xaccount_index_t const& account_index) const {
    auto & unit_hash = account_index.get_latest_unit_hash();
    xobject_ptr_t<base::xvblock_t> unit_block = nullptr;
    // TODO(jimmy) fork
    if (!unit_hash.empty()) {
        unit_block = get_blockstore()->load_block_object(
            addr, account_index.get_latest_unit_height(), unit_hash, false);
    } else {
        unit_block = get_blockstore()->load_block_object(
            addr, account_index.get_latest_unit_height(), account_index.get_latest_unit_viewid(), false);
    }
    return unit_block;
}

data::xunitstate_ptr_t xstatectx_base_t::load_different_table_unit_state(const base::xvaccount_t & addr) const {
    // should use latest committed block for different table
    auto prev_block = get_blockstore()->get_latest_committed_block(addr);
    if (prev_block == nullptr) {
        xerror("xstatectx_base_t::load_different_table_unit_state fail-load unit block.%s", addr.get_address().c_str());
        return nullptr;
    }

    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(prev_block.get());
    if (unitstate == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 0);
        sync_unit_block(addr, prev_block->get_height());
        xwarn("xstatectx_base_t::load_different_table_unit_state fail-get target state. block=%s",
            prev_block->dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 1);
    // the unit state in different table should not be modified, so not need create proposal state
    return unitstate;
}

data::xunitstate_ptr_t xstatectx_base_t::load_inner_table_commit_unit_state(const common::xaccount_address_t & addr) const {
    base::xaccount_index_t account_index;
    auto ret = get_account_index(m_commit_table_state, addr.to_string(), account_index);
    if (!ret) {
        return nullptr;
    }
    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(addr, account_index);
    if (unitstate == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 0);
        sync_unit_block(addr.vaccount(), account_index.get_latest_unit_height());
        xwarn("xstatectx_base_t::load_inner_table_commit_unit_state fail-load unit block.%s,index=%s", addr.to_string().c_str(), account_index.dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 1);
    return unitstate;
}

base::xvblockstore_t*  xstatectx_base_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

base::xvblkstatestore_t* xstatectx_base_t::get_xblkstatestore() const {
    return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
}

bool xstatectx_base_t::load_account_index(const base::xvaccount_t & account, base::xaccount_index_t & account_index) const {
    return get_account_index(m_table_state, account.get_account(), account_index);                                            
}

bool xstatectx_base_t::get_account_index(const statestore::xtablestate_ext_ptr_t & table_state,
                                         const std::string & account,
                                         base::xaccount_index_t & account_index) const {
    std::error_code ec;
    table_state->get_accountindex(account, account_index, ec);
    if (ec) {
        xerror("xstatectx_base_t::get_account_index fail.account=%s,accountindex=%s,ec=%s",account.c_str(),account_index.dump().c_str(),ec.message().c_str());
        return false;
    }
    return true;
}

NS_END2
