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

xobject_ptr_t<base::xvbstate_t> xstatectx_base_t::create_proposal_unit_bstate(base::xvbstate_t* prev_bstate, std::string const& last_block_hash) {
    // clone new state for proposal
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(last_block_hash, *prev_bstate);
    return proposal_bstate;    
}

data::xaccountstate_ptr_t xstatectx_base_t::create_proposal_account_state(base::xaccount_index_t const& account_index, data::xunitstate_ptr_t const& unitstate) {
    assert(!account_index.get_latest_unit_hash().empty());
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(account_index.get_latest_unit_hash(), *unitstate->get_bstate().get());
    if (nullptr == bstate) {
        xerror("xstatectx_base_t::create_proposal_account_state fail.addr=%s,index=%s", unitstate->get_bstate()->get_account().c_str(),account_index.dump().c_str());
        return nullptr;
    }
    assert(bstate->get_last_block_hash() == account_index.get_latest_unit_hash());
    data::xunitstate_ptr_t unitstate_proposal = std::make_shared<data::xunit_bstate_t>(bstate.get(), unitstate->get_bstate().get());  // modify-state        
    return std::make_shared<data::xaccount_state_t>(unitstate_proposal, account_index);
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
        xwarn("xstatectx_base_t::load_inner_table_commit_unit_state fail-load unit block.%s,index=%s", addr.to_string().c_str(), account_index.dump().c_str());
        return nullptr;
    }
    XMETRICS_GAUGE(metrics::xmetrics_tag_t::statectx_load_block_succ, 1);
    return unitstate;
}

base::xvblockstore_t*  xstatectx_base_t::get_blockstore() const {
    return base::xvchain_t::instance().get_xblockstore();
}

bool xstatectx_base_t::load_account_index(common::xaccount_address_t const& address, base::xaccount_index_t & account_index) const {
    return get_account_index(m_table_state, address.to_string(), account_index);                                            
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
