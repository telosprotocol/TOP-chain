// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xstatestore/xstateplugin.h"
#include "xstatestore/xstatehub.h"

NS_BEG2(top, statestore)

xvstatehub_t::xvstatehub_t(base::xvtable_t * target_table) {
    target_table->add_ref();
    m_target_table = target_table;    
    m_idle_timeout_ms = base::enum_state_plugin_idle_timeout_ms;
    xdbg_info("xvstatehub_t::xvstatehub_t,this=%p",this);
}
xvstatehub_t::~xvstatehub_t() {
    if (m_target_table != nullptr) {
        m_target_table->release_ref();
    }
    xdbg_info("xvstatehub_t::~xvstatehub_t this=%p", this);
}

base::xauto_ptr<xvstateplugin_t> get_stateplugin(base::xvtable_t* vtable, std::string const& account, uint64_t timeout_ms) {
    base::xauto_ptr<base::xvaccountobj_t> auto_account_ptr(vtable->get_account(account));
    assert(auto_account_ptr != nullptr);

    base::xauto_ptr<xvstateplugin_t> _state_plugin = dynamic_xobject_ptr_cast<xvstateplugin_t>(auto_account_ptr->get_plugin(base::enum_xvaccount_plugin_statemgr));
    if (nullptr != _state_plugin) {
        return _state_plugin;
    }

    base::xauto_ptr<xvstateplugin_t> new_plugin = new xvstateplugin_t(*auto_account_ptr, timeout_ms);
    base::xauto_ptr<base::xvactplugin_t> final_ptr(auto_account_ptr->get_set_plugin(new_plugin.get(), true));// not need monitor account state plugin seperately
    return dynamic_xobject_ptr_cast<xvstateplugin_t>(final_ptr);
}

void xvstatehub_t::set_unitstate(std::string const& block_hash, xobject_ptr_t<base::xvbstate_t> const& unitstate) {
    base::xauto_ptr<xvstateplugin_t> stateplugin = get_stateplugin(m_target_table, unitstate->get_account(), m_idle_timeout_ms);
    std::lock_guard<std::mutex> lock(m_hub_mutex);
    stateplugin->set_unitstate(block_hash, unitstate);
}

xobject_ptr_t<base::xvbstate_t> xvstatehub_t::get_unitstate(std::string const& account, uint64_t height, std::string const& block_hash) const {
    base::xauto_ptr<xvstateplugin_t> stateplugin = get_stateplugin(m_target_table, account, m_idle_timeout_ms);
    std::lock_guard<std::mutex> lock(m_hub_mutex);
    return stateplugin->get_unitstate(height, block_hash);
}

NS_END2