// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xstatestore/xtablestate_ext.h"

NS_BEG2(top, statestore)

xtablestate_ext_t::xtablestate_ext_t(const data::xtablestate_ptr_t & table_state, std::shared_ptr<state_mpt::xstate_mpt_t> const& state_mpt)
: m_table_state(table_state), m_state_mpt(state_mpt) {

}

void xtablestate_ext_t::get_accountindex(std::string const & unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const {
    xassert(nullptr != m_state_mpt);
    if (nullptr != m_state_mpt && !m_state_mpt->get_original_root_hash().empty()) {
        // new version block state
        account_index = m_state_mpt->get_account_index(common::xaccount_address_t{unit_addr}, ec);
        return;
    }
    // old version block state
    xassert(nullptr != m_table_state);
    m_table_state->get_account_index(unit_addr, account_index);
}

xtablestate_store_t::xtablestate_store_t(const data::xtablestate_ptr_t & table_state, 
                            std::shared_ptr<state_mpt::xstate_mpt_t> const& state_mpt, 
                            evm_common::xh256_t const & state_root,
                            std::vector<std::pair<data::xunitstate_ptr_t, std::string>> const& unitstates)
: m_table_state(table_state), m_state_mpt(state_mpt), m_state_root(state_root), m_unitstates(unitstates) {

}

NS_END2
