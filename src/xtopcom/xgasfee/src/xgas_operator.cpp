// Copyright (c) 2017-2022 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xgasfee/xgas_operator.h"

#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xgenesis_data.h"
#include "xgasfee/xerror/xerror.h"

#define CHECK_EC_RETURN(ec)                                                                                                                                                        \
    do {                                                                                                                                                                           \
        if (ec) {                                                                                                                                                                  \
            xwarn("[xtop_gas_operator] error occured, category: %s, msg: %s!", ec.category().name(), ec.message().c_str());                                                        \
            return;                                                                                                                                                                \
        }                                                                                                                                                                          \
    } while (0)

namespace top {
namespace gasfee {

xtop_gas_operator::xtop_gas_operator(std::shared_ptr<data::xunit_bstate_t> const & state,
                                     xobject_ptr_t<data::xcons_transaction_t> const & tx,
                                     uint64_t time,
                                     uint64_t total_gas_deposit)
  : xgas_state_operator_t(state), xgas_tx_operator_t(tx), m_time(time), m_total_gas_deposit(total_gas_deposit) {
}

void xtop_gas_operator::init(std::error_code & ec) {
    const uint64_t deposit_ = deposit();
    const uint64_t deposit_to_tgas = deposit_ / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    if (deposit_ < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
        xwarn("[xtop_gas_operator::init] deposit: %lu is not enough", deposit_);
        ec = gasfee::error::xenum_errc::tx_deposit_not_enough;
        return;
    }
    m_available_tgas = account_available_tgas(m_time, m_total_gas_deposit);
    m_max_tgas = m_available_tgas + deposit_to_tgas;
    xdbg("[xtop_gas_operator::init] deposit: %lu, deposit_to_tgas: %lu, m_available_tgas: %lu, m_max_tgas: %lu", deposit_, deposit_to_tgas, m_available_tgas, m_max_tgas);
}

void xtop_gas_operator::add(const uint64_t tgas, std::error_code & ec) {
    uint64_t new_tgas_usage = m_tgas_usage + tgas;
    xdbg("[xtop_gas_operator::add_tgas_usage] m_tgas_usage: %lu, supplement: %lu, m_available_tgas: %lu, m_max_tgas: %lu", m_tgas_usage, tgas, m_available_tgas, m_max_tgas);
    if (new_tgas_usage > m_max_tgas) {
        m_deposit_usage = deposit();
        m_tgas_usage = m_available_tgas;
        xwarn("[xtop_gas_operator::process_send_stage] transaction not enough deposit to tgas, m_deposit_usage to: %lu, m_tgas_usage to: %lu", m_deposit_usage, m_tgas_usage);
        ec = gasfee::error::xenum_errc::tx_deposit_to_tgas_not_enough;
    } else if (new_tgas_usage > m_available_tgas) {
        m_deposit_usage = (new_tgas_usage - m_available_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        m_tgas_usage = m_available_tgas;
        xdbg("[xtop_gas_operator::add_tgas_usage] m_deposit_usage to: %lu, m_tgas_usage: %lu", m_deposit_usage, m_tgas_usage);
    } else {
        m_tgas_usage = new_tgas_usage;
        xdbg("[xtop_gas_operator::add_tgas_usage] m_deposit_usage: %lu, m_tgas_usage to: %lu", m_deposit_usage, m_tgas_usage);
    }
    return;
}

void xtop_gas_operator::calculate(std::error_code & ec) {
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

void xtop_gas_operator::process_fixed_tgas(std::error_code & ec) {
    uint64_t fixed_tgas = tx_fixed_tgas();
    xdbg("[xtop_gas_operator::process_fixed_tgas] fixed_tgas to burn: %lu", fixed_tgas);
    if (fixed_tgas > 0) {
        burn(static_cast<base::vtoken_t>(fixed_tgas), ec);
        CHECK_EC_RETURN(ec);
    }
}

void xtop_gas_operator::process_bandwith_tgas(std::error_code & ec) {
    uint64_t bandwith_tgas = tx_bandwith_tgas();
    xdbg("[xtop_gas_operator::process_bandwith_tgas] bandwith_tgas: %lu", bandwith_tgas);
    add(bandwith_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gas_operator::process_disk_tgas(std::error_code & ec) {
    uint64_t disk_tgas = tx_disk_tgas();
    xdbg("[xtop_gas_operator::process_disk_tgas] disk_tgas_: %lu", disk_tgas);
    add(disk_tgas, ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gas_operator::process_calculation_tgas(const uint64_t calculation_gas, std::error_code & ec) {
    xdbg("[xtop_gas_operator::process_calculation_tgas] calculation_gas: %lu", calculation_gas);
    evm_common::u256 gas_limit = tx_gas_limit();
    if (calculation_gas > gas_limit) {
        xwarn("[xtop_gas_operator::process_calculation_tgas] calculation_gas %lu over limit %s", calculation_gas, gas_limit.str().c_str());
        ec = gasfee::error::xenum_errc::tx_calculation_gas_over_limit;
        return;
    }
    evm_common::u256 calculation_tgas = calculation_gas * tx_top_fee_per_gas();
    if (calculation_tgas > UINT64_MAX) {
        xwarn("[xtop_gas_operator::process_calculation_tgas] tx_calculation_tgas_exceeded: %s", calculation_tgas.str().c_str());
        ec = gasfee::error::xenum_errc::tx_calculation_tgas_exceeded;
        return;
    }
    add(static_cast<uint64_t>(calculation_tgas), ec);
    CHECK_EC_RETURN(ec);
    evm_common::u256 tx_limited_tgas_ = tx_limited_tgas();
    if (m_tgas_usage > tx_limited_tgas_) {
        xwarn("[xtop_gas_operator::process_calculation_tgas] m_tgas_usage: %s over limit tgas: %s", m_tgas_usage, tx_limited_tgas_.str().c_str());
        ec = gasfee::error::xenum_errc::tx_used_tgas_over_limit;
        return;
    }
}

bool xtop_gas_operator::need_postprocess() const {
    return (tx_version() == data::xtransaction_version_3);
}

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_self>(std::error_code & ec) {
    tx_set_used_tgas(m_tgas_usage);
    tx_set_used_deposit(m_deposit_usage);
    state_set_used_tgas(m_tgas_usage + account_formular_used_tgas(m_time), ec);
    CHECK_EC_RETURN(ec);
    state_set_last_time(m_time, ec);
    CHECK_EC_RETURN(ec);
    if (m_deposit_usage > 0) {
        burn(static_cast<base::vtoken_t>(m_deposit_usage), ec);
        CHECK_EC_RETURN(ec);
    }
    xdbg("[xtop_gas_operator::store] m_tgas_usage: %lu, m_deposit_usage: %lu", m_tgas_usage, m_deposit_usage);
}

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_send>(std::error_code & ec) {
    tx_set_used_tgas(m_tgas_usage);
    tx_set_used_deposit(m_deposit_usage);
    state_set_used_tgas(m_tgas_usage + account_formular_used_tgas(m_time), ec);
    CHECK_EC_RETURN(ec);
    state_set_last_time(m_time, ec);
    CHECK_EC_RETURN(ec);
    uint64_t deposit_ = deposit();
    xassert(deposit_ > 0);
    lock(static_cast<base::vtoken_t>(deposit_), ec);
    CHECK_EC_RETURN(ec);
    xdbg("[xtop_gas_operator::store] m_tgas_usage: %lu, m_deposit_usage: %lu, deposit: %lu", m_tgas_usage, m_deposit_usage, deposit_);
}

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_recv>(std::error_code & ec) {
    tx_set_used_deposit(last_action_used_deposit());
    if (tx_type() == data::xtransaction_type_run_contract) {
        tx_set_current_recv_tx_use_send_tx_tgas(0);
    }
}

template <>
void xtop_gas_operator::store<base::enum_transaction_subtype_confirm>(std::error_code & ec) {
    uint64_t deposit_ = deposit();
    xassert(deposit_ > 0);
    unlock(static_cast<base::vtoken_t>(deposit_), ec);
    uint64_t recv_tgas_usage = last_action_recv_tx_use_send_tx_tgas();
    m_deposit_usage = last_action_used_deposit();
    if (recv_tgas_usage > 0) {
        add(recv_tgas_usage, ec);
        CHECK_EC_RETURN(ec);
        state_set_used_tgas(m_tgas_usage + account_formular_used_tgas(m_time), ec);
        CHECK_EC_RETURN(ec);
        state_set_last_time(m_time, ec);
        CHECK_EC_RETURN(ec);
    }
    tx_set_used_tgas(recv_tgas_usage);
    tx_set_used_deposit(m_deposit_usage);
    if (m_deposit_usage > 0) {
        burn(static_cast<base::vtoken_t>(m_deposit_usage), ec);
        CHECK_EC_RETURN(ec);
    }
    xdbg("[xtop_gas_operator::store] m_tgas_usage: %lu, m_deposit_usage: %lu, deposit: %lu", m_tgas_usage, m_deposit_usage, deposit_);
}

void xtop_gas_operator::preprocess_one_stage_tx(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        return;
    }
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    // 1. calculate common tgas
    calculate(ec);
    CHECK_EC_RETURN(ec);
    // 2. store if not eth tx(need postprocess)
    if (!need_postprocess()) {
        store<base::enum_transaction_subtype_self>(ec);
        CHECK_EC_RETURN(ec);
    }
}

void xtop_gas_operator::postprocess_one_stage_tx(const uint64_t supplement_gas, std::error_code & ec) {
    if (!need_postprocess()) {
        return;
    }
    process_calculation_tgas(supplement_gas, ec);
    CHECK_EC_RETURN(ec);
    store<base::enum_transaction_subtype_self>(ec);
    CHECK_EC_RETURN(ec);
}

template <>
void xtop_gas_operator::process_three_stage_tx<base::enum_transaction_subtype_send>(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        return;
    }
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    // 1. calculate common tgas
    calculate(ec);
    CHECK_EC_RETURN(ec);
    // 2. store
    if (tx_type() == data::xtransaction_type_transfer) {
        store<base::enum_transaction_subtype_self>(ec);
        CHECK_EC_RETURN(ec);
    } else {
        store<base::enum_transaction_subtype_send>(ec);
        CHECK_EC_RETURN(ec);
    }
}

template <>
void xtop_gas_operator::process_three_stage_tx<base::enum_transaction_subtype_recv>(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        return;
    }
    store<base::enum_transaction_subtype_recv>(ec);
    CHECK_EC_RETURN(ec);
}

template <>
void xtop_gas_operator::process_three_stage_tx<base::enum_transaction_subtype_confirm>(std::error_code & ec) {
    // ignore gas calculation of system contracts
    if (data::is_sys_contract_address(sender())) {
        return;
    }
    // ignore transfer
    if (tx_type() == data::xtransaction_type_transfer) {
        return;
    }
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    store<base::enum_transaction_subtype_confirm>(ec);
    CHECK_EC_RETURN(ec);
}

}  // namespace gasfee
}  // namespace top