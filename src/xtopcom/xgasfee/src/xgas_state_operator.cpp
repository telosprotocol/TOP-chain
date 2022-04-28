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

void xtop_gas_state_operator::burn(base::vtoken_t token, std::error_code & ec) {
    return move_token(data::XPROPERTY_BALANCE_AVAILABLE, data::XPROPERTY_BALANCE_BURN, token, ec);
}

void xtop_gas_state_operator::lock(base::vtoken_t token, std::error_code & ec) {
    return move_token(data::XPROPERTY_BALANCE_AVAILABLE, data::XPROPERTY_BALANCE_LOCK, token, ec);
}

void xtop_gas_state_operator::unlock(base::vtoken_t token, std::error_code & ec) {
    return move_token(data::XPROPERTY_BALANCE_LOCK, data::XPROPERTY_BALANCE_AVAILABLE, token, ec);
}

void xtop_gas_state_operator::move_token(std::string const & from, std::string const & to, base::vtoken_t token, std::error_code & ec) {
    auto ret = m_state->token_withdraw(from, token);
    if (xsuccess != ret) {
        xwarn("[xtop_gas_state_operator::move_token] fail-withdraw, [ec: %d], from property: %s, to property: %s, amount: %ld", ret, from.c_str(), to.c_str(), token);
        ec = error::xenum_errc::account_withdraw_failed;
        return;
    }
    ret = m_state->token_deposit(to, token);
    if (xsuccess != ret) {
        xwarn("[xtop_gas_state_operator::move_token] fail-deposit, [ec: %d], from property: %s, to property: %s, amount: %ld", ret, from.c_str(), to.c_str(), token);
        ec = error::xenum_errc::account_deposit_failed;
        return;
    }
    xdbg("[xtop_gas_state_operator::move_token] success, from property: %s, to property: %s, amount: %ld", from.c_str(), to.c_str(), token);
    return;
}

void xtop_gas_state_operator::state_set_used_tgas(uint64_t tgas_usage, std::error_code & ec) {
    std::string key{data::XPROPERTY_USED_TGAS_KEY};
    std::string value{std::to_string(tgas_usage)};
    auto ret = m_state->string_set(key, value);
    if (xsuccess != ret) {
        xwarn("[xtop_gas_state_operator::set_used_tgas] fail-string_set, [ec: %d], property: %s, value: %s", ret, key.c_str(), value.c_str());
        ec = error::xenum_errc::account_property_set_failed;
        return;
    }
    xdbg("[xtop_gas_state_operator::set_used_tgas] success, property: %s, value: %s", key.c_str(), value.c_str());
    return;
}

void xtop_gas_state_operator::state_set_last_time(uint64_t current_time, std::error_code & ec) {
    std::string key{data::XPROPERTY_LAST_TX_HOUR_KEY};
    std::string value{std::to_string(current_time)};
    auto ret = m_state->string_set(key, value);
    if (xsuccess != ret) {
        xwarn("[xtop_gas_state_operator::set_last_time] fail-string_set, [ec: %d], property: %s, value: %s", ret, key.c_str(), value.c_str());
        ec = error::xenum_errc::account_property_set_failed;
        return;
    }
    xdbg("[xtop_gas_state_operator::set_last_time] success, property: %s, value: %s", key.c_str(), value.c_str());
    return;
}

uint64_t xtop_gas_state_operator::account_available_tgas(uint64_t current_time, uint64_t onchain_total_gas_deposit) const {    
    return m_state->available_tgas(current_time, onchain_total_gas_deposit);
}

uint64_t xtop_gas_state_operator::account_formular_used_tgas(uint64_t current_time) const {
    return m_state->calc_decayed_tgas(current_time);
}

}  // namespace gasfee
}  // namespace top