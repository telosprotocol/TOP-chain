// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgas_state_operator.h"

#include "xbasic/xmodule_type.h"
#include "xgasfee/xerror/xerror.h"

namespace top {
namespace gasfee {

xtop_gas_state_operator::xtop_gas_state_operator(std::shared_ptr<data::xunit_bstate_t> const & state)
  : m_state(state) {
}

uint64_t xtop_gas_state_operator::account_balance() const {
    return m_state->balance();
}

evm_common::u256 xtop_gas_state_operator::account_eth_balance() const {
    return m_state->tep_token_balance(common::xtoken_id_t::eth);
}

uint64_t xtop_gas_state_operator::account_available_tgas(uint64_t current_time, uint64_t onchain_total_gas_deposit) const {    
    return m_state->available_tgas(current_time, onchain_total_gas_deposit);
}

uint64_t xtop_gas_state_operator::account_formular_used_tgas(uint64_t current_time) const {
    return m_state->calc_decayed_tgas(current_time);
}

}  // namespace gasfee
}  // namespace top