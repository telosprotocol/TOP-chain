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
  : xgas_state_operator_t(state), xgas_tx_operator_t(tx), m_time(time), m_onchain_tgas_deposit(onchain_tgas_deposit) {
    xassert(state != nullptr);
    xassert(tx != nullptr);
}

void xtop_gasfee::init(std::error_code & ec) {
    const uint64_t balance = account_balance();
    uint64_t max_gasfee{0};
    max_gasfee = deposit();
    do {
        if (max_gasfee > 0) {
            xdbg("[xtop_gasfee::init] not evm tx, gasfee limit(deposit): %lu", max_gasfee);
        } else {
            // evm tx or eth transfer
            auto eth_max_gasfee = tx_eth_limited_gasfee();
            if (eth_max_gasfee > UINT64_MAX) {
                ec = gasfee::error::xenum_errc::tx_limited_gasfee_exceeded;
                xwarn("[xtop_gasfee::init] tx_limited_gasfee_exceeded: %s", eth_max_gasfee.str().c_str());
                max_gasfee = balance;
                break;
            }
            max_gasfee = static_cast<uint64_t>(eth_max_gasfee);
            xdbg("[xtop_gasfee::init] evm tx, gasfee limit(eth_limited_gasfee): %lu, eth_max_gasfee: %s", max_gasfee, eth_max_gasfee.str().c_str());
        }
        if (max_gasfee > balance) {
            ec = gasfee::error::xenum_errc::account_balance_not_enough;
            xwarn("[xtop_gasfee::init] account_balance_not_enough, max_gasfee: %lu, balance: %lu", max_gasfee, balance);
            max_gasfee = balance;
            break;
        }
        if (max_gasfee < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
            ec = gasfee::error::xenum_errc::tx_deposit_not_enough;
            xwarn("[xtop_gasfee::init] tx_deposit_not_enough: %lu", max_gasfee);
            break;
        }
    } while(0);
    m_converted_tgas = balance_to_tgas(max_gasfee);
    m_free_tgas = account_available_tgas(m_time, m_onchain_tgas_deposit);
    xdbg("[xtop_gasfee::init] final, m_free_tgas: %lu, m_converted_tgas: %lu", m_free_tgas, m_converted_tgas);
}

void xtop_gasfee::add(const uint64_t tgas, std::error_code & ec) {
    xassert(m_free_tgas >= m_free_tgas_usage);
    xassert(m_converted_tgas >= m_converted_tgas_usage);
    const uint64_t left_available_tgas = m_free_tgas + m_converted_tgas - m_free_tgas_usage - m_converted_tgas_usage;
    xdbg("[xtop_gasfee::add] supplement: %lu, m_free_tgas_usage: %lu, m_converted_tgas_usage: %lu, m_free_tgas: %lu, m_converted_tgas: %lu, left_available_tgas: %lu",
         tgas,
         m_free_tgas_usage,
         m_converted_tgas_usage,
         m_free_tgas,
         m_converted_tgas,
         left_available_tgas);
    if (tgas == 0) {
        return;
    }
    if (tgas > left_available_tgas) {
        m_converted_tgas_usage = m_converted_tgas;
        m_free_tgas_usage = m_free_tgas;
        xwarn("[xtop_gasfee::add] transaction not enough deposit to tgas, m_converted_tgas_usage to: %lu, m_free_tgas_usage to: %lu", m_converted_tgas_usage, m_free_tgas_usage);
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
    } else if (tgas > (m_free_tgas - m_free_tgas_usage)) {
        m_converted_tgas_usage += tgas + m_free_tgas_usage - m_free_tgas;
        m_free_tgas_usage = m_free_tgas;
        xdbg("[xtop_gasfee::add] m_converted_tgas_usage to: %lu, m_free_tgas_usage to: %lu", m_converted_tgas_usage, m_free_tgas_usage);
    } else {
        m_free_tgas_usage += tgas;
        xdbg("[xtop_gasfee::add] m_free_tgas_usage to: %lu", m_free_tgas_usage);
    }
    return;
}

void xtop_gasfee::calculate(std::error_code & ec) {
    // 1. fixed tgas
    process_fixed_tgas(ec);
    CHECK_EC_RETURN(ec);
    // 2. bandwith tgas
    process_bandwith_tgas(ec);
    CHECK_EC_RETURN(ec);
    // 3. disk tgas
    process_disk_tgas(ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_fixed_tgas(std::error_code & ec) {
    uint64_t fixed_tgas = tx_fixed_tgas();
    xdbg("[xtop_gasfee::process_fixed_tgas] fixed_tgas to burn: %lu", fixed_tgas);
    add(fixed_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_bandwith_tgas(std::error_code & ec) {
    uint64_t bandwith_tgas = tx_bandwith_tgas();
    xdbg("[xtop_gasfee::process_bandwith_tgas] bandwith_tgas: %lu", bandwith_tgas);
    add(bandwith_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_disk_tgas(std::error_code & ec) {
    uint64_t disk_tgas = tx_disk_tgas();
    xdbg("[xtop_gasfee::process_disk_tgas] disk_tgas_: %lu", disk_tgas);
    add(disk_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::process_calculation_tgas(const uint64_t calculation_gas, std::error_code & ec) {
    xdbg("[xtop_gasfee::process_calculation_tgas] calculation_gas: %lu", calculation_gas);
    evm_common::u256 gas_limit = tx_eth_gas_limit();
    if (calculation_gas > gas_limit) {
        xwarn("[xtop_gasfee::process_calculation_tgas] calculation_gas %lu over limit %s", calculation_gas, gas_limit.str().c_str());
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
        return;
    }
    evm_common::u256 calculation_tgas = calculation_gas / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tgas_to_eth_gas_exchange_ratio);
    if (calculation_tgas > UINT64_MAX) {
        xwarn("[xtop_gasfee::process_calculation_tgas] tx_calculation_tgas_exceeded: %s", calculation_tgas.str().c_str());
        ec = gasfee::error::xenum_errc::tx_out_of_gas;
        return;
    }
    add(static_cast<uint64_t>(calculation_tgas), ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::store_in_one_stage() {
    m_detail.m_state_used_tgas = m_free_tgas_usage + account_formular_used_tgas(m_time);
    m_detail.m_state_last_time = m_time;
    uint64_t deposit_usage = tgas_to_balance(m_converted_tgas_usage);
    m_detail.m_state_burn_balance = deposit_usage;
    m_detail.m_tx_used_tgas = m_free_tgas_usage;
    m_detail.m_tx_used_deposit = deposit_usage;
    xdbg("[xtop_gasfee::store_in_one_stage] m_free_tgas_usage: %lu, m_converted_tgas_usage: %lu, deposit_usage: %lu", m_free_tgas_usage, m_converted_tgas_usage, deposit_usage);
    xdbg("[xtop_gasfee::store_in_one_stage] gasfee_detail: %s", m_detail.str().c_str());
}

void xtop_gasfee::store_in_send_stage() {
    m_detail.m_state_used_tgas = m_free_tgas_usage + account_formular_used_tgas(m_time);
    m_detail.m_state_last_time = m_time;
    uint64_t deposit_total = tgas_to_balance(m_converted_tgas);
    m_detail.m_state_lock_balance = deposit_total;
    const uint64_t deposit_usage = tgas_to_balance(m_converted_tgas_usage);
    m_detail.m_tx_used_tgas = m_free_tgas_usage;
    m_detail.m_tx_used_deposit = deposit_usage;
    xdbg("[xtop_gasfee::store_in_send_stage] m_free_tgas_usage: %lu, m_converted_tgas_usage: %lu, deposit_usage: %lu, m_converted_tgas: %lu, deposit_total: %lu",
         m_free_tgas_usage,
         m_converted_tgas_usage,
         deposit_usage,
         m_converted_tgas,
         deposit_total);
    xdbg("[xtop_gasfee::store_in_send_stage] gasfee_detail: %s", m_detail.str().c_str());
}

void xtop_gasfee::store_in_recv_stage() {
    m_detail.m_tx_used_deposit = tx_last_action_used_deposit();
}

void xtop_gasfee::store_in_confirm_stage() {
    uint64_t deposit_total = tgas_to_balance(m_converted_tgas);
    m_detail.m_state_unlock_balance = deposit_total;
    uint64_t deposit_usage = tx_last_action_used_deposit();
    m_detail.m_state_burn_balance = deposit_usage;
    m_detail.m_tx_used_deposit = deposit_usage;
    xdbg("[xtop_gasfee::store_in_confirm_stage] m_free_tgas_usage: %lu, deposit_usage: %lu, deposit_total: %lu", m_free_tgas_usage, deposit_usage, deposit_total);
    xdbg("[xtop_gasfee::store_in_confirm_stage] gasfee_detail: %s", m_detail.str().c_str());
}

void xtop_gasfee::store_abnormal() {
    m_detail.m_state_used_tgas = m_free_tgas + account_formular_used_tgas(m_time);
    m_detail.m_state_last_time = m_time;
    uint64_t deposit_usage = tgas_to_balance(m_converted_tgas);
    m_detail.m_state_burn_balance = deposit_usage;
    m_detail.m_tx_used_deposit = deposit_usage;
    m_detail.m_tx_used_tgas = m_free_tgas;
    xdbg("[xtop_gasfee::store_abnormal] m_free_tgas: %lu, m_converted_tgas: %lu, deposit_usage: %lu", m_free_tgas, m_converted_tgas, deposit_usage);
    xdbg("[xtop_gasfee::store_abnormal] gasfee_detail: %s", m_detail.str().c_str());
}

void xtop_gasfee::preprocess_one_stage(std::error_code & ec) {
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    // 1. calculate common tgas
    calculate(ec);
    CHECK_EC_RETURN(ec);
    // 2. store if not eth tx(need postprocess)
    store_in_one_stage();
}

void xtop_gasfee::preprocess_send_stage(std::error_code & ec) {
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    // 1. calculate common tgas
    calculate(ec);
    CHECK_EC_RETURN(ec);
    // 2. store
    if (tx_type() == data::xtransaction_type_transfer) {
        store_in_one_stage();
    } else {
        store_in_send_stage();
    }
}

void xtop_gasfee::preprocess_recv_stage(std::error_code & ec) {
    store_in_recv_stage();
}

void xtop_gasfee::preprocess_confirm_stage(std::error_code & ec) {
    // ignore transfer
    if (tx_type() == data::xtransaction_type_transfer) {
        return;
    }
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    store_in_confirm_stage();
}

void xtop_gasfee::postprocess_one_stage(const uint64_t supplement_gas, std::error_code & ec) {
    process_calculation_tgas(supplement_gas, ec);
    CHECK_EC_RETURN(ec);
    store_in_one_stage();
}

void xtop_gasfee::postprocess_send_stage(const uint64_t supplement_gas, std::error_code & ec) {
    return;
}

void xtop_gasfee::postprocess_recv_stage(const uint64_t supplement_gas, std::error_code & ec) {
    return;
}

void xtop_gasfee::postprocess_confirm_stage(const uint64_t supplement_gas, std::error_code & ec) {
    return;
}

void xtop_gasfee::preprocess(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        return;
    }
    if (is_one_stage_tx()) {
        preprocess_one_stage(ec);
    } else {
        auto tx_stage = tx_subtype();
        if (tx_stage == base::enum_transaction_subtype_send) {
            preprocess_send_stage(ec);
        } else if (tx_stage == base::enum_transaction_subtype_recv) {
            preprocess_recv_stage(ec);
        } else if (tx_stage == base::enum_transaction_subtype_confirm) {
            preprocess_confirm_stage(ec);
        } else {
            xerror("[xtop_gasfee::preprocess] error tx stage: %d", tx_stage);
            xassert(false);
        }
    }
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::postprocess(const uint64_t supplement_gas, std::error_code & ec) {
     // ignore gas calculation of system contracts
     if (data::is_sys_contract_address(sender())) {
         return;
     }
    if (is_one_stage_tx()) {
        postprocess_one_stage(supplement_gas, ec);
    } else {
        auto tx_stage = tx_subtype();
        if (tx_stage == base::enum_transaction_subtype_send) {
            postprocess_send_stage(supplement_gas, ec);
        } else if (tx_stage == base::enum_transaction_subtype_recv) {
            postprocess_recv_stage(supplement_gas, ec);
        } else if (tx_stage == base::enum_transaction_subtype_confirm) {
            postprocess_confirm_stage(supplement_gas, ec);
        } else {
            xerror("[xtop_gasfee::postprocess] error tx stage: %d", tx_stage);
            xassert(false);
        }
    }
    CHECK_EC_RETURN(ec);
}

txexecutor::xvm_gasfee_detail_t xtop_gasfee::gasfee_detail() const {
    return m_detail;
}

}  // namespace gasfee
}  // namespace top