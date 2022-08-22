// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xvledger/xvstate.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvblockstore.h"
#include "xvledger/xvstatestore.h"
#include "xvledger/xvledger.h"
#include "xdata/xtable_bstate.h"
#include "xdata/xunit_bstate.h"
#include "xdata/xblocktool.h"
#include "xstatectx/xtablestate_ctx.h"
#include "xstatectx/xunitstate_ctx.h"
#include "xstatectx/xstatectx_face.h"
#include "xstatectx/xstatectx.h"

NS_BEG2(top, statectx)

xstatectx_t::xstatectx_t(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, const data::xtablestate_ptr_t & commit_table_state, const xstatectx_para_t & para)
: m_statectx_base(prev_table_state, commit_table_state, para.m_clock), m_statectx_para(para) {
    // create proposal table state for context
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = xstatectx_base_t::create_proposal_bstate(prev_block, prev_table_state->get_bstate().get(), para.m_clock);
    data::xtablestate_ptr_t proposal_table_state = std::make_shared<data::xtable_bstate_t>(proposal_bstate.get(), false);  // change to modified state
    m_table_ctx = std::make_shared<xtablestate_ctx_t>(proposal_table_state);
}

bool xstatectx_t::is_same_table(const base::xvaccount_t & addr) const {
    return get_tableid() == addr.get_short_table_id();
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

xunitstate_ctx_ptr_t xstatectx_t::load_unit_ctx(const base::xvaccount_t & addr) {
    xobject_ptr_t<base::xvbstate_t> bstate = nullptr;
    data::xblock_ptr_t unitblock = nullptr;
    bool is_same = is_same_table(addr);
    xunitstate_ctx_ptr_t unit_ctx = find_unit_ctx(addr.get_address(), is_same);
    if (nullptr != unit_ctx) {
        return unit_ctx;
    }
    if (is_same) {
        unitblock = m_statectx_base.load_inner_table_unit_block(addr);
        if (nullptr != unitblock) {
            bstate = m_statectx_base.load_proposal_block_state(addr, unitblock.get());
            if (nullptr != bstate) {
                data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get(), false);  // modify-state
                unit_ctx = std::make_shared<xunitstate_ctx_t>(unitstate, unitblock);
            }
        }
    } else { // different table unit state is readonly
        bstate = m_statectx_base.load_different_table_unit_state(addr);
        if (nullptr != bstate) {
            data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(bstate.get(), true);  // readonly-state
            unit_ctx = std::make_shared<xunitstate_ctx_t>(unitstate);
        }
    }

    if (nullptr != unit_ctx) {
        add_unit_ctx(addr.get_address(), is_same, unit_ctx);
    } else {
        xwarn("xstatectx_t::load_unit_ctx fail-load state.addr=%s,is_same=%d", addr.get_address().c_str(), is_same);
    }
    return unit_ctx;
}

data::xunitstate_ptr_t xstatectx_t::load_unit_state(const base::xvaccount_t & addr) {
    xunitstate_ctx_ptr_t unit_ctx = load_unit_ctx(addr);
    if (nullptr != unit_ctx) {
        return unit_ctx->get_unitstate();
    }
    return nullptr;
}

const data::xtablestate_ptr_t &  xstatectx_t::get_table_state() const {
    return m_table_ctx->get_table_state();
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
        ret = unitctx.second->get_unitstate()->do_rollback();
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
        _size = unitctx.second->get_unitstate()->do_snapshot();
        snapshot_size += _size;
    }
    return snapshot_size;
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

std::vector<xunitstate_ctx_ptr_t> xstatectx_t::get_modified_unit_ctx() const {
    std::vector<xunitstate_ctx_ptr_t> unitctxs;
    for (auto & unitctx : m_unit_ctxs) {
        if (unitctx.second->get_unitstate()->is_state_changed()) {
            unitctxs.push_back(unitctx.second);
        }
    }
    return unitctxs;
}

xstatectx_ptr_t xstatectx_factory_t::create_latest_cert_statectx(base::xvblock_t* prev_block, const data::xtablestate_ptr_t & prev_table_state, const data::xtablestate_ptr_t & commit_table_state, const xstatectx_para_t & para) {
    statectx::xstatectx_ptr_t statectx_ptr = std::make_shared<statectx::xstatectx_t>(prev_block, prev_table_state, commit_table_state, para);
    return statectx_ptr;
}

xstatectx_ptr_t xstatectx_factory_t::create_statectx(const base::xvaccount_t & table_addr, base::xvblock_t* _block) {
    if (nullptr == _block) {
        xassert(nullptr != _block);
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> cert_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_block);
    if (nullptr == cert_bstate) {
        xwarn("xstatectx_factory_t::create_statectx fail-get target state.block=%s",_block->dump().c_str());
        return nullptr;
    }
    data::xtablestate_ptr_t cert_tablestate = std::make_shared<data::xtable_bstate_t>(cert_bstate.get());

    // TODO(jimmy) commit state is used for unit sync
    xobject_ptr_t<base::xvblock_t> _commit_block = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block(table_addr);
    if (nullptr == _commit_block) {
        xerror("xstatectx_factory_t::create_statectx fail-get commit block.");
        return nullptr;
    }

    base::xauto_ptr<base::xvbstate_t> commit_bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(_commit_block.get());
    if (nullptr == commit_bstate) {
        xwarn("xstatectx_factory_t::create_statectx fail-get commit state.block=%s",_commit_block->dump().c_str());
        return nullptr;
    }
    data::xtablestate_ptr_t commit_tablestate = std::make_shared<data::xtable_bstate_t>(commit_bstate.get());

    xstatectx_para_t statectx_para(_block->get_clock()+1);
    statectx::xstatectx_ptr_t statectx_ptr = std::make_shared<statectx::xstatectx_t>(_block, cert_tablestate, commit_tablestate, statectx_para);
    return statectx_ptr;
}

NS_END2
