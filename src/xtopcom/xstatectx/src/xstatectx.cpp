// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvledger.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xtablestate_ctx.h"
#include "xstatectx/xunitstate_ctx.h"
#include "xstatectx/xstatectx_face.h"
#include "xstatectx/xstatectx.h"
#include "xstatestore/xstatestore_face.h"

NS_BEG2(top, statectx)

xstatectx_t::xstatectx_t(base::xvblock_t* prev_block, const statestore::xtablestate_ext_ptr_t & prev_table_state, base::xvblock_t* commit_block, const statestore::xtablestate_ext_ptr_t & commit_table_state, const xstatectx_para_t & para)
: m_table_address(common::xtable_address_t::build_from(prev_block->get_account())), m_statectx_base(prev_block, prev_table_state, commit_block, commit_table_state, para.m_clock), m_statectx_para(para) {
    // create proposal table state for context
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(prev_block->get_block_hash(), *prev_table_state->get_table_state()->get_bstate().get());
    data::xtablestate_ptr_t proposal_table_state = std::make_shared<data::xtable_bstate_t>(proposal_bstate.get(), prev_table_state->get_table_state()->get_bstate().get());  // change to modified state
    m_table_ctx = std::make_shared<xtablestate_ctx_t>(proposal_table_state, prev_table_state->get_state_mpt());

    std::error_code ec;
    std::shared_ptr<state_mpt::xstate_mpt_t> current_prev_mpt = state_mpt::xstate_mpt_t::create(m_table_address,
                                                                                                prev_table_state->get_state_mpt()->original_root_hash(),
                                                                                                base::xvchain_t::instance().get_xdbstore(),
                                                                                                ec);
    xassert(nullptr != current_prev_mpt);
    auto const tablestate = std::make_shared<statestore::xtablestate_ext_t>(proposal_table_state, current_prev_mpt);
    m_prev_tablestate_ext = tablestate;
}

bool xstatectx_t::is_same_table(common::xaccount_address_t const& address) const {
    if (address.zone_id() == common::zone_id(m_table_address) && address.table_id() == common::table_id(m_table_address)) {
        return true;
    }
    return false;
}

xunitstate_ctx_ptr_t xstatectx_t::find_unit_ctx(const std::string & addr, bool is_same_table) {
    if (is_same_table) {
        auto iter = m_unit_ctxs.find(addr);
        if (iter != m_unit_ctxs.end()) {
            return iter->second;
        }
    } else {
        auto iter = m_other_table_unit_ctxs.find(addr);
        if (iter != m_other_table_unit_ctxs.end()) {
            return iter->second;
        }
    }
    return nullptr;
}

void xstatectx_t::add_unit_ctx(const std::string & addr, bool is_same_table, const xunitstate_ctx_ptr_t & unit_ctx) {
    if (is_same_table) {
        m_unit_ctxs[addr] = unit_ctx;
    } else {
        m_other_table_unit_ctxs[addr] = unit_ctx;
    }
}



xunitstate_ctx_ptr_t xstatectx_t::load_unit_ctx(common::xaccount_address_t const& address) {
    xobject_ptr_t<base::xvbstate_t> bstate = nullptr;
    data::xblock_ptr_t unitblock = nullptr;
    bool is_same = is_same_table(address);
    xunitstate_ctx_ptr_t unit_ctx = find_unit_ctx(address.to_string(), is_same);
    if (nullptr != unit_ctx) {
        return unit_ctx;
    }
    if (is_same) {
        base::xaccount_index_t account_index;
        if (false == m_statectx_base.load_account_index(address, account_index)) {
            xerror("xstatectx_t::load_unit_ctx fail-load accountindex.addr=%s", address.to_string().c_str());
            return nullptr;
        }

        data::xunitstate_ptr_t unitstate = nullptr;
        if (account_index.get_latest_unit_hash().empty()) {
            xassert(account_index.get_latest_unit_height() == 0);
            xobject_ptr_t<base::xvblock_t> _unit = m_statectx_base.get_blockstore()->load_unit(address.vaccount(), account_index.get_latest_unit_height());
            if (nullptr == _unit) {
                std::error_code ec;
                _unit = base::xvchain_t::instance().get_xblockstore()->create_genesis_block(address.vaccount(), ec);
                if (nullptr == _unit) {
                    xerror("xstatectx_t::load_unit_ctx fail-create genesis unit.addr=%s", address.to_string().c_str());
                    return nullptr;
                }
            }
            account_index.reset_unit_hash(_unit->get_block_hash());
            unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(_unit.get());
        } else {
            unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_accountindex(address, account_index);
        }
        if (nullptr == unitstate) {
            xerror("xstatectx_t::load_unit_ctx fail-load unitstate.addr=%s,index=%s", address.to_string().c_str(),account_index.dump().c_str());
            return nullptr;
        }
        data::xaccountstate_ptr_t accountstate = xstatectx_base_t::create_proposal_account_state(account_index, unitstate);
        if (nullptr == accountstate) {
            xerror("xstatectx_t::load_unit_ctx fail-create proposal state.addr=%s,index=%s", address.to_string().c_str(),account_index.dump().c_str());
            return nullptr;
        }
        unit_ctx = std::make_shared<xunitstate_ctx_t>(accountstate);
        xdbg("xstatectx_t::load_unit_ctx succ-return unit unitstate.table=%s,account=%s,index=%s", m_prev_tablestate_ext->get_table_state()->get_bstate()->dump().c_str(), address.to_string().c_str(), account_index.dump().c_str());
    } else { // different table unit state is readonly        
        // data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(common::xaccount_address_t(addr.get_account()));
        data::xaccountstate_ptr_t accountstate = statestore::xstatestore_hub_t::instance()->get_accountstate(LatestConnectBlock, address);
        if (nullptr == accountstate) {
            xerror("xstatectx_t::load_unit_ctx fail-get other table state.addr=%s", address.to_string().c_str());
            return nullptr;            
        }
        unit_ctx = std::make_shared<xunitstate_ctx_t>(accountstate);
    }

    add_unit_ctx(address.to_string(), is_same, unit_ctx);
    return unit_ctx;
}

data::xaccountstate_ptr_t xstatectx_t::load_account_state(common::xaccount_address_t const& address) {
    xunitstate_ctx_ptr_t unit_ctx = load_unit_ctx(address);
    if (nullptr != unit_ctx) {
        return unit_ctx->get_accoutstate();
    }
    return nullptr;
}

data::xunitstate_ptr_t xstatectx_t::load_unit_state(common::xaccount_address_t const& address) {
    xunitstate_ctx_ptr_t unit_ctx = load_unit_ctx(address);
    if (nullptr != unit_ctx) {
        return unit_ctx->get_unitstate();
    }
    return nullptr;
}

data::xunitstate_ptr_t xstatectx_t::load_commit_unit_state(common::xaccount_address_t const& address) {
    bool is_same = is_same_table(address);
    data::xunitstate_ptr_t unitstate = nullptr;
    // TODO(jimmy) load from statestore and invoke sync in statestore same-table should use commit-table
    if (is_same) {
        unitstate = m_statectx_base.load_inner_table_commit_unit_state(address); // TODO(jimmy) change to common::xaccount_address_
    } else {
        unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_state(address);
    }
    return unitstate;
}

data::xunitstate_ptr_t xstatectx_t::load_commit_unit_state(common::xaccount_address_t const& address, uint64_t height) {
    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_committed_state(address, height);
    return unitstate;
}

uint64_t xstatectx_t::load_account_height(common::xaccount_address_t const& address) {
    assert(address.vaccount().is_unit_address());
    base::xaccount_index_t account_index;
    bool is_same = is_same_table(address);
    if (is_same) {
        std::error_code ec;
        m_prev_tablestate_ext->get_accountindex(address.to_string(), account_index, ec);
    } else {
        statestore::xstatestore_hub_t::instance()->get_accountindex(LatestConnectBlock, address, account_index);
    }
    return account_index.get_latest_unit_height();
}

const data::xtablestate_ptr_t &  xstatectx_t::get_table_state() const {
    return m_prev_tablestate_ext->get_table_state();
}

bool xstatectx_t::do_rollback() {
    bool ret = false;
    ret = get_table_state()->do_rollback();
    if (!ret) {
        xerror("xstatectx_t::do_rollback fail-table do_rollback");
        return ret;
    }
    // no need rollback table-state
    for (auto & unitctx : m_unit_ctxs) {
        ret = unitctx.second->get_accoutstate()->do_rollback();
        if (!ret) {
            xerror("xstatectx_t::do_rollback fail-do_rollback");
            return ret;
        }
    }
    xdbg("xstatectx_t::do_rollback rollback table-addr %s", get_table_address().c_str());
    return true;
}

size_t xstatectx_t::do_snapshot() {
    size_t snapshot_size = 0;
    size_t _size = get_table_state()->do_snapshot();
    snapshot_size += _size;
    // no need snapshot table-state
    for (auto & unitctx : m_unit_ctxs) {
        _size = unitctx.second->get_accoutstate()->do_snapshot();
        snapshot_size += _size;
    }
    return snapshot_size;
}

void xstatectx_t::finish_execution() {
    assert(!m_finish_execution);
    for (auto iter = m_unit_ctxs.begin(); iter != m_unit_ctxs.end(); ) {
        if (false == iter->second->get_unitstate()->is_state_changed()) {
            iter = m_unit_ctxs.erase(iter);
        } else {
            iter++;
        }
    }
    m_finish_execution = true;
}

void xstatectx_t::do_commit(base::xvblock_t* current_block) {
    auto const & state_root_hash = data::xblockextract_t::get_state_root_from_block(current_block);
    
    auto const& unitctxs = get_modified_unit_ctx();
    std::vector<data::xunitstate_store_para_t> _unitstate_paras(unitctxs.size());
    std::map<std::string, base::xaccount_index_t> account_index_map;
    int i = 0;
    for (auto & v : unitctxs) {
        auto & unitctx = v.second;
        data::xunitstate_store_para_t _para;
        _para.m_unitstate = unitctx->get_unitstate();
        _para.m_unit_hash = unitctx->get_unit_hash();
        _para.m_unitstate_bin = unitctx->get_unitstate_bin();
        _unitstate_paras[i++] = std::move(_para);

        account_index_map[unitctx->get_unitstate()->get_bstate()->get_account()] = unitctx->get_accoutstate()->get_accountindex();
    }

    // XTODO create store table bstate by final block
    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*current_block, *get_table_state()->get_bstate());
    data::xtablestate_ptr_t table_bstate = std::make_shared<data::xtable_bstate_t>(current_state.get());

    std::error_code ec;
    statestore::xtablestate_store_ptr_t tablestate_store = std::make_shared<statestore::xtablestate_store_t>(table_bstate, m_prev_tablestate_ext->get_state_mpt(), state_root_hash, std::move(_unitstate_paras));
    statestore::xstatestore_hub_t::instance()->do_commit_table_all_states(current_block, tablestate_store, account_index_map, ec);
    if (ec) {
        xerror("xstatectx_t::do_commit fail. block:%s", current_block->dump().c_str());
        return;
    }
    xdbg_info("xstatectx_t::do_commit succ. block:%s", current_block->dump().c_str());
}

bool xstatectx_t::is_state_dirty() const {
    if (get_table_state()->is_state_dirty()) {
        xdbg("xstatectx_t::is_state_dirty table dirty. %s", get_table_state()->dump().c_str());
        return true;
    }
    for (auto & unitctx : m_unit_ctxs) {
        if (unitctx.second->get_unitstate()->is_state_dirty()) {
            xdbg("xstatectx_t::is_state_dirty unit dirty. %s", unitctx.second->get_unitstate()->dump().c_str());
            return true;
        }
    }
    return false;
}

std::map<std::string, xunitstate_ctx_ptr_t> const& xstatectx_t::get_modified_unit_ctx() const {
    assert(m_finish_execution);
    return m_unit_ctxs;
}

xstatectx_ptr_t xstatectx_factory_t::create_latest_cert_statectx(base::xvblock_t* prev_block, base::xvblock_t* commit_block, const xstatectx_para_t & para) {
    statestore::xtablestate_ext_ptr_t cert_tablestate = statestore::xstatestore_hub_t::instance()->get_tablestate_ext_from_block(prev_block);
    if (nullptr == cert_tablestate) {
        xwarn("xstatectx_factory_t::create_latest_cert_statectx fail-get cert-tablestate.block=%s", prev_block->dump().c_str());
        return nullptr;
    }

    statestore::xtablestate_ext_ptr_t commit_tablestate = statestore::xstatestore_hub_t::instance()->get_tablestate_ext_from_block(commit_block);
    if (nullptr == cert_tablestate) {
        xwarn("xstatectx_factory_t::create_latest_cert_statectx fail-get commit-tablestate.block=%s", commit_block->dump().c_str());
        return nullptr;
    }

    statectx::xstatectx_ptr_t statectx_ptr = std::make_shared<statectx::xstatectx_t>(prev_block, cert_tablestate, commit_block, commit_tablestate, para);
    return statectx_ptr;
}

xstatectx_ptr_t xstatectx_factory_t::create_statectx(const base::xvaccount_t & table_addr, base::xvblock_t* _block) {
    if (nullptr == _block) {
        xassert(nullptr != _block);
        return nullptr;
    }

    // TODO(jimmy) commit state is used for unit sync
    xobject_ptr_t<base::xvblock_t> _commit_block = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block(table_addr);
    if (nullptr == _commit_block) {
        xerror("xstatectx_factory_t::create_statectx fail-get commit block.");
        return nullptr;
    }

    xstatectx_para_t statectx_para(_block->get_clock()+1);
    return create_latest_cert_statectx(_block, _commit_block.get(), statectx_para);
}

NS_END2
