// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xstatectx_base.h"

NS_BEG2(top, statectx)

xstatectx_base_t::xstatectx_base_t(const data::xtablestate_ptr_t & prev_table_state, const data::xtablestate_ptr_t & commit_table_state, uint64_t clock)
: m_table_state(prev_table_state), m_commit_table_state(commit_table_state), m_clock(clock) {
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::create_proposal_bstate(base::xvblock_t* prev_block, base::xvbstate_t* prev_bstate, uint64_t clock) {
    // create proposal header, clock use to set block version
    base::xauto_ptr<base::xvheader_t> _temp_header = base::xvblockbuild_t::build_proposal_header(prev_block, clock);
    // always clone new state
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_header.get(), *prev_bstate);
    return proposal_bstate;    
}

void xstatectx_base_t::sync_unit_block(const base::xvaccount_t & _vaddr, uint64_t end_height) const {
    base::xaccount_index_t commit_accountindex;
    m_commit_table_state->get_account_index(_vaddr.get_account(), commit_accountindex);
    uint64_t latest_connect_height = get_blockstore()->get_latest_connected_block_height(_vaddr);
    data::xblocktool_t::check_lacking_unit_and_try_sync(_vaddr, commit_accountindex, latest_connect_height, get_blockstore(), "statectx");
    xinfo("xstatectx_base_t::sync_unit_block account=%s,end_h=%ld,connect_h=%ld", _vaddr.get_account().c_str(), end_height, latest_connect_height);
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::load_proposal_block_state(const base::xvaccount_t & addr, base::xvblock_t* prev_block) const {
    base::xauto_ptr<base::xvbstate_t> prev_bstate = get_xblkstatestore()->get_block_state(prev_block);
    if (prev_bstate == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 0);
        sync_unit_block(addr, prev_block->get_height());
        xwarn("xstatectx_base_t::load_proposal_block_state fail-get target state. block=%s",
            prev_block->dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 1);

    // always clone new state
    return create_proposal_bstate(prev_block, prev_bstate.get(), m_clock);
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::load_inner_table_unit_state(const base::xvaccount_t & addr) const {
    base::xaccount_index_t account_index;
    m_table_state->get_account_index(addr.get_address(), account_index);

    auto prev_block = get_blockstore()->load_block_object(
        addr, account_index.get_latest_unit_height(), account_index.get_latest_unit_viewid(), false);
    if (prev_block == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 0);
        sync_unit_block(addr, account_index.get_latest_unit_height());
        xwarn("xstatectx_base_t::load_inner_table_unit_state fail-load unit block.%s,index=%s",
                addr.get_address().c_str(), account_index.dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 1);

    auto state_ptr = load_proposal_block_state(addr, prev_block.get());
    return state_ptr;
}

data::xblock_ptr_t xstatectx_base_t::load_inner_table_unit_block(const base::xvaccount_t & addr) const {
    base::xaccount_index_t account_index;
    m_table_state->get_account_index(addr.get_address(), account_index);

    auto prev_block = get_blockstore()->load_block_object(
        addr, account_index.get_latest_unit_height(), account_index.get_latest_unit_viewid(), false);
    if (prev_block == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 0);
        sync_unit_block(addr, account_index.get_latest_unit_height());
        xwarn("xstatectx_base_t::load_inner_table_unit_block fail-load unit block.%s,index=%s",
                addr.get_address().c_str(), account_index.dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 1);
    return data::xblock_t::raw_vblock_to_object_ptr(prev_block.get());
}

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::load_different_table_unit_state(const base::xvaccount_t & addr) const {
    // should use latest committed block for different table
    auto prev_block = get_blockstore()->get_latest_committed_block(addr);
    if (prev_block == nullptr) {
        sync_unit_block(addr, 0);
        xerror("xstatectx_base_t::load_different_table_unit_state fail-load unit block.%s", addr.get_address().c_str());
        return nullptr;
    }

    xobject_ptr_t<base::xvbstate_t> prev_bstate = get_xblkstatestore()->get_block_state(prev_block.get());
    if (prev_bstate == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 0);
        sync_unit_block(addr, prev_block->get_height());
        xwarn("xstatectx_base_t::load_proposal_block_state fail-get target state. block=%s",
            prev_block->dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 1);
    // the unit state in different table should not be modified, so not need create proposal state
    return prev_bstate;
}

base::xvblockstore_t*  xstatectx_base_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

base::xvblkstatestore_t* xstatectx_base_t::get_xblkstatestore() const {
    return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
}



NS_END2
