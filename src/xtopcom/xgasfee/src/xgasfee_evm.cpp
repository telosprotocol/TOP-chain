// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgasfee_evm.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xgasfee/xerror/xerror.h"
#include "xgasfee/xgas_estimate.h"

#define CHECK_EC_RETURN(ec)                                                                                                                                                        \
    do {                                                                                                                                                                           \
        if (ec) {                                                                                                                                                                  \
            xwarn("[xtop_gasfee_evm] error occured, category: %s, msg: %s!", ec.category().name(), ec.message().c_str());                                                          \
            return;                                                                                                                                                                \
        }                                                                                                                                                                          \
    } while (0)

namespace top {
namespace gasfee {

xtop_gasfee_evm::xtop_gasfee_evm(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time)
  : xgas_state_operator_t(state), xgas_tx_operator_t(tx), m_time(time) {
    xassert(state != nullptr);
    xassert(tx != nullptr);
}

void xtop_gasfee_evm::init(std::error_code & ec) {

    if (tx_eth_fee_per_gas() < xgas_estimate::base_price()) {
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
        xwarn("[xtop_gasfee_evm::init] tx_eth_fee_per_gas_error, tx_eth_fee_per_gas: %s, base_price: %s",
              tx_eth_fee_per_gas().str().c_str(),
              xgas_estimate::base_price().str().c_str());
        return;
    }

    if (tx_eth_priority_fee_per_gas() > tx_eth_fee_per_gas()) {
        ec = gasfee::error::xenum_errc::tx_priority_fee_error;
        xwarn("[xtop_gasfee_evm::init] tx_priority_fee_error, eth_priority_fee_per_gas: %s, eth_fee_per_gas: %s",
              tx_eth_priority_fee_per_gas().str().c_str(),
              tx_eth_fee_per_gas().str().c_str());
        return;
    }

    //eth account_eth_balance
    if (tx_type() == data::enum_xtransaction_type::xtransaction_type_transfer) {
        auto eth_balance = account_eth_balance();
        auto eth_amount = get_eth_amount();
        if (eth_amount > eth_balance) {
            // eth balance not enough
            ec = gasfee::error::xenum_errc::account_balance_not_enough;
            xwarn("[xtop_gasfee_evm::init] account_balance_not_enough, eth_balance: %s, eth_amount: %s", eth_balance.str().c_str(), eth_amount.str().c_str());
            return;
        }
    }

    // top balance
    const uint64_t balance = account_balance();
    // gas fee
    const evm_common::u256 eth_max_gasfee_utop = tx_eth_limited_gasfee_to_utop(true);
    xdbg("[xtop_gasfee_evm::init] evm tx, eth_max_gasfee_utop: %s", eth_max_gasfee_utop.str().c_str());
    if (eth_max_gasfee_utop > balance) {
        // top balance not enough
        ec = gasfee::error::xenum_errc::account_balance_not_enough;
        xwarn("[xtop_gasfee_evm::init] account_balance_not_enough, eth_max_gasfee_utop: %s, balance: %lu", eth_max_gasfee_utop.str().c_str(), balance);
        return;
    }
    m_max_converted_utop = eth_max_gasfee_utop;
    xdbg("[xtop_gasfee_evm::init] final, balance: %lu,  m_max_converted_utop: %s", balance, m_max_converted_utop.str().c_str());
    return;
}

void xtop_gasfee_evm::preprocess(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        xerror("[xtop_gasfee_evm::preprocess] unreachable is_sys_contract_address");
        ec = gasfee::error::xenum_errc::invalid_param;
        assert(false);
        return;
    }

    if(!is_t6_address(sender()) || !is_t6_address(recver()) ){
        xerror("[xtop_gasfee_evm::postprocess] sender %s or  recver %s must be T6.", sender().to_string().c_str(), recver().to_string().c_str());
        ec = gasfee::error::xenum_errc::invalid_param;
        assert(false);
        return;
    }

    if (is_one_stage_tx()) {
        init(ec);
        CHECK_EC_RETURN(ec);
    } else {
        // unreachable code
        xassert(false);
        xerror("[xtop_gasfee_evm::preprocess] unreachable code");
        ec = gasfee::error::xenum_errc::invalid_param;
    }
}

void xtop_gasfee_evm::postprocess(const evm_common::u256 &gas_used, std::error_code & ec) {

    // calculate min priority fee price
    calculate_min_priority();
    calculate_gas_fee(gas_used, ec);
    store_in_one_stage();
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee_evm::calculate_min_priority() {
    evm_common::u256 max_gas_price = tx_eth_fee_per_gas();
    evm_common::u256 max_priority_fee_price = tx_eth_priority_fee_per_gas();
    evm_common::u256 config_base_price = xgas_estimate::base_price();
    m_priority_fee_price = std::min(max_priority_fee_price, (max_gas_price - config_base_price));
    xdbg("[xtop_gasfee_evm::calculate_min_priority] max_gas_price: %s,  max_priority_fee_price: %s, base_price %s, priority_fee_price %s.",
         max_gas_price.str().c_str(),
         max_priority_fee_price.str().c_str(),
         config_base_price.str().c_str(),
         m_priority_fee_price.str().c_str());
}

void xtop_gasfee_evm::calculate_gas_fee(const evm_common::u256& gas_used, std::error_code & ec) {
    evm_common::u256 eth_fee_wei{0};
    evm_common::u256 eth_fee_utop{0};
    evm_common::u256 eth_gas_price = xgas_estimate::base_price() + m_priority_fee_price;
    evm_common::u256 gas_limit = tx_eth_gas_limit();

    if (gas_used > gas_limit) {
        eth_fee_wei = eth_gas_price * gas_limit;
        eth_fee_utop = wei_to_utop(eth_fee_wei, true);
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
        m_max_converted_utop = eth_fee_utop;
        xwarn("[xtop_gasfee::calculate_gas_fee] gas_used %s over gas_limit %s, to: %s, eth_fee_wei: %s, eth_fee_utop: %s",
               gas_used.str().c_str(), gas_limit.str().c_str(), eth_fee_wei.str().c_str(), eth_fee_utop.str().c_str());
        return;
    }

    eth_fee_wei = eth_gas_price * gas_used;
    eth_fee_utop = wei_to_utop(eth_fee_wei, true);
    if (eth_fee_utop > m_max_converted_utop) {
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
        xwarn("[xtop_gasfee_evm::calculate_gas_fee] transaction out of gas, eth_fee_utop %s more than m_max_converted_utop %s",
              eth_fee_utop.str().c_str(),
              m_max_converted_utop.str().c_str());
        assert(false);
        return;
    }

    m_max_converted_utop = eth_fee_utop;
}

void xtop_gasfee_evm::store_in_one_stage() {
    m_detail.m_state_last_time = m_time;
    //m_max_converted_utop is gas_limit or gas_used*eth_gas_price,and gas_limit >= gas_used*eth_gas_price
    m_detail.m_state_burn_balance = static_cast<uint64_t>(m_max_converted_utop);
    m_detail.m_tx_used_deposit = static_cast<uint64_t>(m_max_converted_utop);
    m_detail.m_tx_priority_fee_price = m_priority_fee_price;
    xdbg("[xtop_gasfee_evm::store_in_one_stage] gasfee_detail: %s", m_detail.str().c_str());
}

txexecutor::xvm_gasfee_detail_t xtop_gasfee_evm::gasfee_detail() const {
    return m_detail;
}

}  // namespace gasfee
}  // namespace top