// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatestore/xstateplugin.h"
#include "xstatestore/xerror.h"
#include "xmetrics/xmetrics.h"

NS_BEG2(top, statestore)

xvstateplugin_t::xvstateplugin_t(base::xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms)
:xvactplugin_t(parent_obj,idle_timeout_ms,base::enum_xvaccount_plugin_statemgr) {
    if (parent_obj.is_contract_address()) {
        m_max_count = 4;
    } else {
        m_max_count = 1;
    }
    xdbg_info("xvstateplugin_t::xvstateplugin_t %s,idle_timeout_ms=%ld,this=%p",parent_obj.get_account().c_str(), idle_timeout_ms, this);
}
xvstateplugin_t::~xvstateplugin_t() {
    xdbg_info("xvstateplugin_t::~xvstateplugin_t %s,this=%p", get_account().c_str(), this);
}

void xvstateplugin_t::set_unitstate(std::string const& block_hash, xobject_ptr_t<base::xvbstate_t> const& unitstate) {
    // m_bstates[unitstate->get_block_height()] = std::make_pair<std::string, xobject_ptr_t<base::xvbstate_t>>(block_hash, unitstate);
    auto iter = m_bstates.find(unitstate->get_block_height());
    if (iter != m_bstates.end()) {
        xdbg_info("xvstateplugin_t::set_unitstate repeat height. %s,height=%ld,hash=%s",unitstate->get_account().c_str(), unitstate->get_block_height(), base::xstring_utl::to_hex(block_hash).c_str());
        if (iter->second.first == block_hash) {
            return;
        } else {
            m_bstates.erase(iter);
        }            
    }

    m_bstates[unitstate->get_block_height()] = std::make_pair(block_hash, unitstate);
    if (m_bstates.size() > m_max_count) {
        m_bstates.erase(m_bstates.begin());  // erase small height state
        assert(m_bstates.size() == m_max_count);
    }
    xdbg_info("xvstateplugin_t::set_unitstate %s,height=%ld,hash=%s,size=%zu",unitstate->get_account().c_str(), unitstate->get_block_height(), base::xstring_utl::to_hex(block_hash).c_str(),m_bstates.size());
}

xobject_ptr_t<base::xvbstate_t> xvstateplugin_t::get_unitstate(uint64_t height, std::string const& block_hash) const {
    auto iter = m_bstates.find(height);
    if (iter != m_bstates.end()) {
        if (iter->second.first == block_hash) {
            xdbg_info("xvstateplugin_t::get_unitstate succ.%s,height=%ld,hash=%s",get_account().c_str(),height,base::xstring_utl::to_hex(block_hash).c_str());
            XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, 1);
            return iter->second.second;
        }
    }
    XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, 0);
    xwarn("xvstateplugin_t::get_unitstate fail.%s,height=%ld",get_account().c_str(),height);
    return nullptr;
}

NS_END2