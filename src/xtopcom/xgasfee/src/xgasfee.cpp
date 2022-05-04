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
    const uint64_t deposit_ = deposit();
    const uint64_t deposit_to_tgas = deposit_ / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    if (deposit_ < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
        ec = gasfee::error::xenum_errc::tx_deposit_not_enough;
        return;
    }
    m_free_tgas = account_available_tgas(m_time, m_onchain_tgas_deposit);
    m_total_available_tgas = m_free_tgas + deposit_to_tgas;
    xdbg("[xtop_gasfee::init] deposit: %lu, deposit_to_tgas: %lu, m_free_tgas: %lu, m_total_available_tgas: %lu",
         deposit_,
         deposit_to_tgas,
         m_free_tgas,
         m_total_available_tgas);
}

void xtop_gasfee::add(const uint64_t tgas, std::error_code & ec) {
    uint64_t new_tgas_usage = m_free_tgas_usage + tgas;
    xdbg("[xtop_gasfee::add_tgas_usage] m_free_tgas_usage: %lu, supplement: %lu, m_free_tgas: %lu, m_total_available_tgas: %lu",
         m_free_tgas_usage,
         tgas,
         m_free_tgas,
         m_total_available_tgas);
    if (new_tgas_usage > m_total_available_tgas) {
        m_deposit_usage = deposit();
        m_free_tgas_usage = m_free_tgas;
        xwarn("[xtop_gasfee::process_send_stage] transaction not enough deposit to tgas, m_deposit_usage to: %lu, m_free_tgas_usage to: %lu",
              m_deposit_usage,
              m_free_tgas_usage);
        ec = gasfee::error::xenum_errc::tx_deposit_to_tgas_not_enough;
    } else if (new_tgas_usage > m_free_tgas) {
        m_deposit_usage = (new_tgas_usage - m_free_tgas) * XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
        m_free_tgas_usage = m_free_tgas;
        xdbg("[xtop_gasfee::add_tgas_usage] m_deposit_usage to: %lu, m_free_tgas_usage: %lu", m_deposit_usage, m_free_tgas_usage);
    } else {
        m_free_tgas_usage = new_tgas_usage;
        xdbg("[xtop_gasfee::add_tgas_usage] m_deposit_usage: %lu, m_free_tgas_usage to: %lu", m_deposit_usage, m_free_tgas_usage);
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
    if (fixed_tgas > 0) {
        burn(static_cast<base::vtoken_t>(fixed_tgas), ec);
        CHECK_EC_RETURN(ec);
    }
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
    evm_common::u256 gas_limit = tx_gas_limit();
    if (calculation_gas > gas_limit) {
        xwarn("[xtop_gasfee::process_calculation_tgas] calculation_gas %lu over limit %s", calculation_gas, gas_limit.str().c_str());
        ec = gasfee::error::xenum_errc::tx_calculation_gas_over_limit;
        return;
    }
    evm_common::u256 calculation_tgas = calculation_gas * tx_top_fee_per_gas();
    if (calculation_tgas > UINT64_MAX) {
        xwarn("[xtop_gasfee::process_calculation_tgas] tx_calculation_tgas_exceeded: %s", calculation_tgas.str().c_str());
        ec = gasfee::error::xenum_errc::tx_calculation_tgas_exceeded;
        return;
    }
    add(static_cast<uint64_t>(calculation_tgas), ec);
    CHECK_EC_RETURN(ec);
    evm_common::u256 tx_limited_tgas_ = tx_limited_tgas();
    if (m_free_tgas_usage > tx_limited_tgas_) {
        xwarn("[xtop_gasfee::process_calculation_tgas] m_free_tgas_usage: %s over limit tgas: %s", m_free_tgas_usage, tx_limited_tgas_.str().c_str());
        ec = gasfee::error::xenum_errc::tx_used_tgas_over_limit;
        return;
    }
}

void xtop_gasfee::store_in_one_stage(std::error_code & ec) {
    tx_set_used_tgas(m_free_tgas_usage);
    tx_set_used_deposit(m_deposit_usage);
    state_set_used_tgas(m_free_tgas_usage + account_formular_used_tgas(m_time), ec);
    CHECK_EC_RETURN(ec);
    state_set_last_time(m_time, ec);
    CHECK_EC_RETURN(ec);
    if (m_deposit_usage > 0) {
        burn(static_cast<base::vtoken_t>(m_deposit_usage), ec);
        CHECK_EC_RETURN(ec);
    }
    xdbg("[xtop_gasfee::store] m_free_tgas_usage: %lu, m_deposit_usage: %lu", m_free_tgas_usage, m_deposit_usage);
}

void xtop_gasfee::store_in_send_stage(std::error_code & ec) {
    tx_set_used_tgas(m_free_tgas_usage);
    tx_set_used_deposit(m_deposit_usage);
    state_set_used_tgas(m_free_tgas_usage + account_formular_used_tgas(m_time), ec);
    CHECK_EC_RETURN(ec);
    state_set_last_time(m_time, ec);
    CHECK_EC_RETURN(ec);
    uint64_t deposit_ = deposit();
    xassert(deposit_ > 0);
    lock(static_cast<base::vtoken_t>(deposit_), ec);
    CHECK_EC_RETURN(ec);
    xdbg("[xtop_gasfee::store] m_free_tgas_usage: %lu, m_deposit_usage: %lu, deposit: %lu", m_free_tgas_usage, m_deposit_usage, deposit_);
}

void xtop_gasfee::store_in_recv_stage(std::error_code & ec) {
    tx_set_used_deposit(tx_last_action_used_deposit());
    tx_set_current_recv_tx_use_send_tx_tgas(0);
}

void xtop_gasfee::store_in_confirm_stage(std::error_code & ec) {
    uint64_t deposit_ = deposit();
    xassert(deposit_ > 0);
    unlock(static_cast<base::vtoken_t>(deposit_), ec);
    uint64_t recv_tgas_usage = tx_last_action_recv_tx_use_send_tx_tgas();
    m_deposit_usage = tx_last_action_used_deposit();
    if (recv_tgas_usage > 0) {
        add(recv_tgas_usage, ec);
        CHECK_EC_RETURN(ec);
        state_set_used_tgas(m_free_tgas_usage + account_formular_used_tgas(m_time), ec);
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
    xdbg("[xtop_gasfee::store] m_free_tgas_usage: %lu, m_deposit_usage: %lu, deposit: %lu", m_free_tgas_usage, m_deposit_usage, deposit_);
}

void xtop_gasfee::preprocess_one_stage(std::error_code & ec) {
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    // 1. calculate common tgas
    calculate(ec);
    CHECK_EC_RETURN(ec);
    // 2. store if not eth tx(need postprocess)
    if (tx_version() != data::xtransaction_version_3) {
        store_in_one_stage(ec);
        CHECK_EC_RETURN(ec);
    }
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
        store_in_one_stage(ec);
        CHECK_EC_RETURN(ec);
    } else {
        store_in_send_stage(ec);
        CHECK_EC_RETURN(ec);
    }
}

void xtop_gasfee::preprocess_recv_stage(std::error_code & ec) {
    store_in_recv_stage(ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::preprocess_confirm_stage(std::error_code & ec) {
    // ignore transfer
    if (tx_type() == data::xtransaction_type_transfer) {
        return;
    }
    // 0. init
    init(ec);
    CHECK_EC_RETURN(ec);
    store_in_confirm_stage(ec);
    CHECK_EC_RETURN(ec);
}

void xtop_gasfee::postprocess_one_stage(const uint64_t supplement_gas, std::error_code & ec) {
    if (tx_version() != data::xtransaction_version_3) {
        return;
    }
    process_calculation_tgas(supplement_gas, ec);
    CHECK_EC_RETURN(ec);
    store_in_one_stage(ec);
    CHECK_EC_RETURN(ec);
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

bool xtop_gasfee::is_one_stage_tx() {
    bool is_one_stage_tx{false};
    if (tx_subtype() == base::enum_transaction_subtype_self || tx_version() == data::xtransaction_version_3) {
        is_one_stage_tx = true;
    }
    return is_one_stage_tx;
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

}  // namespace gasfee
}  // namespace top