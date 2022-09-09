// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xstate_mpt/xstate_mpt.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xaccountindex.h"
#include "xvledger/xvblockbuild.h"
#include "xvledger/xvledger.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xstatectx_base.h"

NS_BEG2(top, statectx)

xstatectx_base_t::xstatectx_base_t(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, base::xvblock_t* commit_block, const data::xtablestate_ptr_t & commit_table_state, uint64_t clock)
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

void xstatectx_base_t::sync_unit_block(const base::xvaccount_t & _vaddr, uint64_t end_height) const {
    base::xaccount_index_t commit_accountindex;
    auto ret = get_account_index(m_commit_block, m_commit_table_state, _vaddr.get_account(), commit_accountindex);
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
    auto prev_block = load_inner_table_unit_block(addr);
    if (prev_block == nullptr) {
        return nullptr;
    }

    auto state_ptr = load_proposal_block_state(addr, prev_block.get());
    return state_ptr;
}

data::xblock_ptr_t xstatectx_base_t::load_inner_table_unit_block(const base::xvaccount_t & addr) const {
    base::xaccount_index_t account_index;
    auto ret = get_account_index(m_pre_block, m_table_state, addr.get_account(), account_index);
    if (!ret) {
        return nullptr;
    }
    xobject_ptr_t<base::xvblock_t> prev_block = load_block_object(addr, account_index);
    if (prev_block == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 0);
        sync_unit_block(addr, account_index.get_latest_unit_height());
        xwarn("xstatectx_base_t::load_inner_table_unit_block fail-load unit block.%s,index=%s",
                addr.get_address().c_str(), account_index.dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 1);
    return  data::xblock_t::raw_vblock_to_object_ptr(prev_block.get());
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

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::load_inner_table_commit_unit_state(const base::xvaccount_t & addr) const {
    base::xaccount_index_t account_index;
    auto ret = get_account_index(m_commit_block, m_commit_table_state, addr.get_account(), account_index);
    if (!ret) {
        return nullptr;
    }
    xobject_ptr_t<base::xvblock_t> prev_block = load_block_object(addr, account_index);
    if (prev_block == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 0);
        sync_unit_block(addr, account_index.get_latest_unit_height());
        xwarn("xstatectx_base_t::load_inner_table_commit_unit_state fail-load unit block.%s,index=%s",
                addr.get_address().c_str(), account_index.dump().c_str());
        return nullptr;
    }
    xobject_ptr_t<base::xvbstate_t> prev_bstate = get_xblkstatestore()->get_block_state(prev_block.get());
    if (prev_bstate == nullptr) {
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_state_succ, 0);
        sync_unit_block(addr, prev_block->get_height());
        xwarn("xstatectx_base_t::load_inner_table_commit_unit_state fail-get target state. block=%s",
            prev_block->dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 1);
    return prev_bstate;
}

base::xvblockstore_t*  xstatectx_base_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

base::xvblkstatestore_t* xstatectx_base_t::get_xblkstatestore() const {
    return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
}

bool xstatectx_base_t::load_account_index(const base::xvaccount_t & account, base::xaccount_index_t & account_index) const {
    return get_account_index(m_pre_block, m_table_state, account, account_index);                                            
}

bool xstatectx_base_t::get_account_index(const data::xvblock_ptr_t & block,
                                         const data::xtablestate_ptr_t & table_state,
                                         const base::xvaccount_t & account,
                                         base::xaccount_index_t & account_index) const {
    evm_common::xh256_t state_root;
    auto ret = data::xblockextract_t::get_state_root(block.get(), state_root);
    if (!ret) {
        xwarn("xstatectx_base_t::get_account_index get_state_root fail.block:%s", block->dump().c_str());
        return false;
    }

    if (state_root != evm_common::xh256_t()) {
        std::error_code ec;
        xhash256_t root_hash(state_root.to_bytes()); 
        auto mpt = state_mpt::xtop_state_mpt::create(root_hash, base::xvchain_t::instance().get_xdbstore(), table_state->account_address().to_string(), ec);
        if (ec) {
            xwarn("xstatectx_base_t::get_account_index create mpt fail.root hash:%s.state_root:%s.block:%s", root_hash.as_hex_str().c_str(), state_root.hex().c_str(), block->dump().c_str());
            return false;
        }

        account_index = mpt->get_account_index(account.get_account(), ec);
        if (ec) {
            xwarn("xstatectx_base_t::get_account_index get_account_index from mpt fail.root hash:%s.block:%s", root_hash.as_hex_str().c_str(), block->dump().c_str());
            return false;
        }
        xdbg("xstatectx_base_t::get_account_index succ root hash:%s.account:%s index:%s", root_hash.as_hex_str().c_str(), account.get_account().c_str(), account_index.dump().c_str());
        return true;
    }

    table_state->get_account_index(account.get_account(), account_index);
    return true;
}

NS_END2
