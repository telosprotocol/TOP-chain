// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgasfee.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xgasfee/xerror/xerror.h"

#define CHECK_EC_RETURN(ec)                                                                                                                                                        \
    do {                                                                                                                                                                           \
        if (ec) {                                                                                                                                                                  \
            xwarn("[xtop_gasfee] error occured, category: %s, msg: %s!", ec.category().name(), ec.message().c_str());                                                        \
            return;                                                                                                                                                                \
        }                                                                                                                                                                          \
    } while (0)

namespace top {
namespace gasfee {

xtop_gasfee::xtop_gasfee(std::shared_ptr<data::xunit_bstate_t> const & state, xobject_ptr_t<data::xcons_transaction_t> const & tx, uint64_t time, uint64_t onchain_tgas_deposit)
  : xgasfee_interface(state, tx, time, onchain_tgas_deposit) {
}

void xtop_gasfee::check(std::error_code & ec) {
    // max gas fee
    const uint64_t max_gasfee = deposit();
    // top balance
    const uint64_t balance = account_balance();
    // top from eth balance
    const evm_common::u256 balance_from_eth = wei_to_utop(account_eth_balance());
    // top balance to use
    evm_common::u256 top_balance_to_use{0};
    // eth balance to use
    evm_common::u256 eth_balance_to_use{0};
    if (max_gasfee > 0) {
        // unreachable code
        xassert(false);
        xerror("[xtop_gasfee::check] unreachable code");
        // xdbg("[xtop_gasfee::check] not evm tx, gasfee limit(deposit): %lu", max_gasfee);
        // if (max_gasfee > balance) {
        //     ec = gasfee::error::xenum_errc::account_balance_not_enough;
        //     xwarn("[xtop_gasfee::check] account_balance_not_enough, max_gasfee: %lu, balance: %lu", max_gasfee, balance);
        //     return;
        // } else {
        //     top_balance_to_use = max_gasfee;
        // }
        // if (max_gasfee < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
        //     ec = gasfee::error::xenum_errc::tx_deposit_not_enough;
        //     xwarn("[xtop_gasfee::check] tx_deposit_not_enough: %lu", max_gasfee);
        //     return;
        // }
    } else {
        // evm tx or eth transfer
        const evm_common::u256 eth_max_gasfee = tx_eth_limited_gasfee();
        xdbg("[xtop_gasfee::check] evm tx, eth_max_gasfee: %s", eth_max_gasfee.str().c_str());
        if (eth_max_gasfee > balance) {
            // need use eth balance
            if (eth_max_gasfee > balance + balance_from_eth) {
                ec = gasfee::error::xenum_errc::account_balance_not_enough;
                xwarn("[xtop_gasfee::check] account_balance_not_enough, eth_max_gasfee: %s, balance: %lu, balance_from_eth: %s",
                      eth_max_gasfee.str().c_str(),
                      balance,
                      balance_from_eth.str().c_str());
                return;
            } else {
                // use extra eth balance
                top_balance_to_use = balance;
                eth_balance_to_use = eth_max_gasfee - balance;
                xinfo("[xtop_gasfee::check] use extra balance from eth, eth_max_gasfee: %s, balance: %lu, balance_from_eth: %s, eth_balance_to_use: %s",
                      eth_max_gasfee.str().c_str(),
                      balance,
                      balance_from_eth.str().c_str(),
                      eth_balance_to_use.str().c_str());
            }
        } else {
            top_balance_to_use = eth_max_gasfee;
            xinfo("[xtop_gasfee::check] use only top balance, eth_max_gasfee: %s, balance: %lu, top_balance_to_use: %s",
                  eth_max_gasfee.str().c_str(),
                  balance,
                  top_balance_to_use.str().c_str());
        }
        if (eth_max_gasfee < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
            ec = gasfee::error::xenum_errc::tx_deposit_not_enough;
            xwarn("[xtop_gasfee::check] tx_deposit_not_enough: %s", eth_max_gasfee.str().c_str());
            return;
        }
    }
}

void xtop_gasfee::init(std::error_code & ec) {
    // max gas fee
    const uint64_t max_gasfee = deposit();
    // top balance
    const uint64_t balance = account_balance();
    // top from eth balance
    const evm_common::u256 balance_from_eth = wei_to_utop(account_eth_balance());
    // top balance to use
    evm_common::u256 top_balance_to_use{0};
    // eth balance to use
    evm_common::u256 eth_balance_to_use{0};
    if (max_gasfee > 0) {
        // unreachable code
        xassert(false);
        xerror("[xtop_gasfee::init] unreachable code");
        // xdbg("[xtop_gasfee::init] not evm tx, gasfee limit(deposit): %lu", max_gasfee);
        // if (max_gasfee > balance) {
        //     ec = gasfee::error::xenum_errc::account_balance_not_enough;
        //     xwarn("[xtop_gasfee::init] account_balance_not_enough, max_gasfee: %lu, balance: %lu", max_gasfee, balance);
        //     return;
        // } else {
        //     top_balance_to_use = max_gasfee;
        // }
        // if (max_gasfee < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
        //     ec = gasfee::error::xenum_errc::tx_deposit_not_enough;
        //     xwarn("[xtop_gasfee::init] tx_deposit_not_enough: %lu", max_gasfee);
        //     return;
        // }
    } else {
        // evm tx or eth transfer
        const evm_common::u256 eth_max_gasfee = tx_eth_limited_gasfee();
        xdbg("[xtop_gasfee::init] evm tx, eth_max_gasfee: %s", eth_max_gasfee.str().c_str());
        if (eth_max_gasfee > balance) {
            // need use eth balance
            if (eth_max_gasfee > balance + balance_from_eth) {
                ec = gasfee::error::xenum_errc::account_balance_not_enough;
                xwarn("[xtop_gasfee::init] account_balance_not_enough, eth_max_gasfee: %s, balance: %lu, balance_from_eth: %s",
                      eth_max_gasfee.str().c_str(),
                      balance,
                      balance_from_eth.str().c_str());
                top_balance_to_use = balance;
                eth_balance_to_use = balance_from_eth;
            } else {
                // use extra eth balance
                top_balance_to_use = balance;
                eth_balance_to_use = eth_max_gasfee - balance;
                xinfo("[xtop_gasfee::init] use extra eth balance, eth_max_gasfee: %s, balance: %lu, balance_from_eth: %s, eth_balance_to_use: %s",
                      eth_max_gasfee.str().c_str(),
                      balance,
                      balance_from_eth.str().c_str(),
                      eth_balance_to_use.str().c_str());
            }
        } else {
            top_balance_to_use = eth_max_gasfee;
            xinfo("[xtop_gasfee::init] use only top balance, eth_max_gasfee: %s, balance: %lu, top_balance_to_use: %s",
                  eth_max_gasfee.str().c_str(),
                  balance,
                  top_balance_to_use.str().c_str());
        }
    }
    m_converted_tgas = balance_to_tgas(top_balance_to_use);
    m_eth_converted_tgas = balance_to_tgas(eth_balance_to_use);
    m_free_tgas = account_available_tgas(m_time, m_onchain_tgas_deposit);
    xinfo("[xtop_gasfee::init] final, m_free_tgas: %s, m_converted_tgas: %s, m_eth_converted_tgas: %s",
          m_free_tgas.str().c_str(),
          m_converted_tgas.str().c_str(),
          m_eth_converted_tgas.str().c_str());
}

void xtop_gasfee::add(const evm_common::u256 tgas, std::error_code & ec) {
    xassert(m_free_tgas >= m_free_tgas_usage);
    xassert(m_converted_tgas >= m_converted_tgas_usage);
    const evm_common::u256 left_available_tgas = m_free_tgas + m_converted_tgas + m_eth_converted_tgas - m_free_tgas_usage - m_converted_tgas_usage - m_eth_converted_tgas_usage;
    xdbg(
        "[xtop_gasfee::add] supplement: %s, m_free_tgas_usage: %s, m_converted_tgas_usage: %s, m_eth_converted_tgas_usage: %s, m_free_tgas: %s, m_converted_tgas: %s, "
        "m_eth_converted_tgas: %s, left_available_tgas: %s",
        tgas.str().c_str(),
        m_free_tgas_usage.str().c_str(),
        m_converted_tgas_usage.str().c_str(),
        m_eth_converted_tgas_usage.str().c_str(),
        m_free_tgas.str().c_str(),
        m_converted_tgas.str().c_str(),
        m_eth_converted_tgas.str().c_str(),
        left_available_tgas.str().c_str());
    if (tgas == 0) {
        return;
    }
    if (tgas > left_available_tgas) {
        m_converted_tgas_usage = m_converted_tgas;
        m_eth_converted_tgas_usage = m_eth_converted_tgas;
        m_free_tgas_usage = m_free_tgas;
        xwarn("[xtop_gasfee::add] transaction not enough deposit to tgas, m_converted_tgas_usage to: %s, m_eth_converted_tgas_usage to: %s, m_free_tgas_usage to: %s",
              m_converted_tgas_usage.str().c_str(),
              m_eth_converted_tgas_usage.str().c_str(),
              m_free_tgas_usage.str().c_str());
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
    } else if (tgas > (m_free_tgas - m_free_tgas_usage)) {
        if (tgas > (m_free_tgas - m_free_tgas_usage + m_converted_tgas - m_converted_tgas_usage)) {
            m_eth_converted_tgas_usage += tgas - (m_free_tgas - m_free_tgas_usage + m_converted_tgas - m_converted_tgas_usage);
            m_converted_tgas_usage = m_converted_tgas;
            m_free_tgas_usage = m_free_tgas;
            xdbg("[xtop_gasfee::add] m_converted_tgas_usage to: %s, m_free_tgas_usage to: %s, m_eth_converted_tgas_usage to: %s",
                 m_converted_tgas_usage.str().c_str(),
                 m_free_tgas_usage.str().c_str(),
                 m_eth_converted_tgas_usage.str().c_str());
        } else {
            m_converted_tgas_usage += tgas - (m_free_tgas - m_free_tgas_usage);
            m_free_tgas_usage = m_free_tgas;
            xdbg("[xtop_gasfee::add] m_converted_tgas_usage to: %s, m_free_tgas_usage to: %s", m_converted_tgas_usage.str().c_str(), m_free_tgas_usage.str().c_str());
        }
    } else {
        m_free_tgas_usage += tgas;
        xdbg("[xtop_gasfee::add] m_free_tgas_usage to: %s", m_free_tgas_usage.str().c_str());
    }
    return;
}

void xtop_gasfee::calculate(const evm_common::u256 supplement_gas, std::error_code & ec) {
    // 1. fixed tgas
    process_fixed_tgas(ec);
    CHECK_EC_RETURN(ec);
    // 2. bandwith tgas
    process_bandwith_tgas(ec);
    CHECK_EC_RETURN(ec);
    // 3. disk tgas
    process_disk_tgas(ec);
    CHECK_EC_RETURN(ec);
    // 4. calculation tgas
    process_calculation_tgas(supplement_gas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_fixed_tgas(std::error_code & ec) {
    evm_common::u256 fixed_tgas = tx_fixed_tgas();
    xdbg("[xtop_gasfee::process_fixed_tgas] fixed_tgas to burn: %s", fixed_tgas.str().c_str());
    add(fixed_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_bandwith_tgas(std::error_code & ec) {
    evm_common::u256 bandwith_tgas = tx_bandwith_tgas();
    xdbg("[xtop_gasfee::process_bandwith_tgas] bandwith_tgas: %s", bandwith_tgas.str().c_str());
    add(bandwith_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_disk_tgas(std::error_code & ec) {
    evm_common::u256 disk_tgas = tx_disk_tgas();
    xdbg("[xtop_gasfee::process_disk_tgas] disk_tgas_: %s", disk_tgas.str().c_str());
    add(disk_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_calculation_tgas(const evm_common::u256 calculation_gas, std::error_code & ec) {
    xdbg("[xtop_gasfee::process_calculation_tgas] calculation_gas: %s", calculation_gas.str().c_str());
    evm_common::u256 gas_limit = tx_eth_gas_limit();
    if (calculation_gas > gas_limit) {
        xwarn("[xtop_gasfee::process_calculation_tgas] calculation_gas %s over limit %s", calculation_gas.str().c_str(), gas_limit.str().c_str());
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
        m_converted_tgas_usage = m_converted_tgas;
        m_eth_converted_tgas_usage = m_eth_converted_tgas;
        m_free_tgas_usage = m_free_tgas;
        xwarn("[xtop_gasfee::process_calculation_tgas] transaction out of gas, m_converted_tgas_usage to: %s, m_eth_converted_tgas_usage to: %s, m_free_tgas_usage to: %s",
              m_converted_tgas_usage.str().c_str(),
              m_eth_converted_tgas_usage.str().c_str(),
              m_free_tgas_usage.str().c_str());
        return;
    }
    evm_common::u256 calculation_tgas = calculation_gas * XGET_ONCHAIN_GOVERNANCE_PARAMETER(eth_gas_to_tgas_exchange_ratio);
    add(calculation_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::store_in_one_stage() {
    evm_common::u256 state_used_tgas = m_free_tgas_usage + account_formular_used_tgas(m_time);
    if (state_used_tgas > UINT64_MAX) {
        xwarn("[xtop_gasfee::store_in_one_stage] state_used_tgas %s over linit", state_used_tgas.str().c_str());
        state_used_tgas = UINT64_MAX;
    }
    m_detail.m_state_used_tgas = static_cast<uint64_t>(state_used_tgas);
    m_detail.m_state_last_time = m_time;
    evm_common::u256 top_usage = tgas_to_balance(m_converted_tgas_usage);
    if (top_usage > UINT64_MAX) {
        xwarn("[xtop_gasfee::store_in_one_stage] top_usage %s over linit", top_usage.str().c_str());
        top_usage = UINT64_MAX;
    }
    m_detail.m_state_burn_balance = static_cast<uint64_t>(top_usage);
    evm_common::u256 tx_used_tgas = m_free_tgas_usage;
    if (tx_used_tgas > UINT64_MAX) {
        xwarn("[xtop_gasfee::store_in_one_stage] tx_used_tgas %s over linit", tx_used_tgas.str().c_str());
        tx_used_tgas = UINT64_MAX;
    }
    m_detail.m_tx_used_tgas = static_cast<uint64_t>(tx_used_tgas);
    evm_common::u256 eth_usage = utop_to_wei(tgas_to_balance(m_eth_converted_tgas_usage));
    m_detail.m_state_burn_eth_balance = eth_usage;
    evm_common::u256 tx_used_deposit = top_usage + tgas_to_balance(m_eth_converted_tgas_usage);
    if (tx_used_deposit > UINT64_MAX) {
        xwarn("[xtop_gasfee::store_in_one_stage] tx_used_deposit %s over linit", tx_used_deposit.str().c_str());
        tx_used_deposit = UINT64_MAX;
    }
    m_detail.m_tx_used_deposit = static_cast<uint64_t>(tx_used_deposit);
    xdbg("[xtop_gasfee::store_in_one_stage] m_free_tgas: %s, m_converted_tgas_usage: %s, top_usage: %s, m_eth_converted_tgas_usage: %s, eth_usage: %s",
         m_free_tgas.str().c_str(),
         m_converted_tgas_usage.str().c_str(),
         top_usage.str().c_str(),
         m_eth_converted_tgas_usage.str().c_str(),
         eth_usage.str().c_str());
    xdbg("[xtop_gasfee::store_in_one_stage] gasfee_detail: %s", m_detail.str().c_str());
}

// void xtop_gasfee::store_in_send_stage() {
//     m_detail.m_state_used_tgas = m_free_tgas_usage + account_formular_used_tgas(m_time);
//     m_detail.m_state_last_time = m_time;
//     uint64_t deposit_total = tgas_to_balance(m_converted_tgas);
//     m_detail.m_state_lock_balance = deposit_total;
//     const uint64_t deposit_usage = tgas_to_balance(m_converted_tgas_usage);
//     m_detail.m_tx_used_tgas = m_free_tgas_usage;
//     m_detail.m_tx_used_deposit = deposit_usage;
//     xdbg("[xtop_gasfee::store_in_send_stage] m_free_tgas_usage: %lu, m_converted_tgas_usage: %lu, deposit_usage: %lu, m_converted_tgas: %lu, deposit_total: %lu",
//          m_free_tgas_usage,
//          m_converted_tgas_usage,
//          deposit_usage,
//          m_converted_tgas,
//          deposit_total);
//     xdbg("[xtop_gasfee::store_in_send_stage] gasfee_detail: %s", m_detail.str().c_str());
// }

// void xtop_gasfee::store_in_recv_stage() {
//     m_detail.m_tx_used_deposit = tx_last_action_used_deposit();
// }

// void xtop_gasfee::store_in_confirm_stage() {
//     uint64_t deposit_total = tgas_to_balance(m_converted_tgas);
//     m_detail.m_state_unlock_balance = deposit_total;
//     uint64_t deposit_usage = tx_last_action_used_deposit();
//     m_detail.m_state_burn_balance = deposit_usage;
//     m_detail.m_tx_used_deposit = deposit_usage;
//     xdbg("[xtop_gasfee::store_in_confirm_stage] m_free_tgas_usage: %lu, deposit_usage: %lu, deposit_total: %lu", m_free_tgas_usage, deposit_usage, deposit_total);
//     xdbg("[xtop_gasfee::store_in_confirm_stage] gasfee_detail: %s", m_detail.str().c_str());
// }

// void xtop_gasfee::preprocess_one_stage(std::error_code & ec) {
//     // 0. init
//     init(ec);
//     CHECK_EC_RETURN(ec);
//     // 1. calculate common tgas
//     calculate(0, ec);
//     // 2. store if not eth tx(need postprocess)
//     store_in_one_stage();
// }

// void xtop_gasfee::preprocess_send_stage(std::error_code & ec) {
//     // 0. init
//     init(ec);
//     CHECK_EC_RETURN(ec);
//     // 1. calculate common tgas
//     calculate(0, ec);
//     // 2. store
//     if (tx_type() == data::xtransaction_type_transfer) {
//         store_in_one_stage();
//     } else {
//         store_in_send_stage();
//     }
// }

// void xtop_gasfee::preprocess_recv_stage(std::error_code & ec) {
//     store_in_recv_stage();
// }

// void xtop_gasfee::preprocess_confirm_stage(std::error_code & ec) {
//     // ignore transfer
//     if (tx_type() == data::xtransaction_type_transfer) {
//         return;
//     }
//     // 0. init
//     init(ec);
//     CHECK_EC_RETURN(ec);
//     store_in_confirm_stage();
// }

void xtop_gasfee::postprocess_one_stage(const evm_common::u256 supplement_gas, std::error_code & ec) {
    do {
        init(ec);
        if (ec) {
            break;
        }
        calculate(supplement_gas, ec);
        if (ec) {
            break;
        }
    } while(0);
    store_in_one_stage();
}

// void xtop_gasfee::postprocess_send_stage(const evm_common::u256 supplement_gas, std::error_code & ec) {
//     return;
// }

// void xtop_gasfee::postprocess_recv_stage(const evm_common::u256 supplement_gas, std::error_code & ec) {
//     return;
// }

// void xtop_gasfee::postprocess_confirm_stage(const evm_common::u256 supplement_gas, std::error_code & ec) {
//     return;
// }

void xtop_gasfee::preprocess(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        return;
    }
    if (is_one_stage_tx()) {
        // preprocess_one_stage(ec);
        check(ec);
        CHECK_EC_RETURN(ec);
    } else {
        // unreachable code
        xassert(false);
        xerror("[xtop_gasfee::preprocess] unreachable code");
        // auto tx_stage = tx_subtype();
        // if (tx_stage == base::enum_transaction_subtype_send) {
        //     preprocess_send_stage(ec);
        // } else if (tx_stage == base::enum_transaction_subtype_recv) {
        //     preprocess_recv_stage(ec);
        // } else if (tx_stage == base::enum_transaction_subtype_confirm) {
        //     preprocess_confirm_stage(ec);
        // } else {
        //     xerror("[xtop_gasfee::preprocess] error tx stage: %d", tx_stage);
        //     xassert(false);
        // }
    }
}

void xtop_gasfee::postprocess(const evm_common::u256 supplement_gas, std::error_code & ec) {
     // ignore gas calculation of system contracts
     if (data::is_sys_contract_address(sender())) {
         return;
     }
    if (is_one_stage_tx()) {
        postprocess_one_stage(supplement_gas, ec);
    } else {
        // unreachable code
        xassert(false);
        xerror("[xtop_gasfee::postprocess] unreachable code");
        // auto tx_stage = tx_subtype();
        // if (tx_stage == base::enum_transaction_subtype_send) {
        //     postprocess_send_stage(supplement_gas, ec);
        // } else if (tx_stage == base::enum_transaction_subtype_recv) {
        //     postprocess_recv_stage(supplement_gas, ec);
        // } else if (tx_stage == base::enum_transaction_subtype_confirm) {
        //     postprocess_confirm_stage(supplement_gas, ec);
        // } else {
        //     xerror("[xtop_gasfee::postprocess] error tx stage: %d", tx_stage);
        //     xassert(false);
        // }
    }
    CHECK_EC_RETURN(ec);
}

}  // namespace gasfee
}  // namespace top