// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_reseter.h"

#include "xbasic/xmemory.hpp"
#include "xchain_fork/xutility.h"
#include "xdata/xnative_contract_address.h"
#include "xstate_reset/xstate_tablestate_reseter_sample.h"
#include "xstate_reset/xstate_tablestate_reseter_continuous_sample.h"
#include "xstatectx/xstatectx.h"

NS_BEG2(top, state_reset)

xstate_reseter::xstate_reseter(statectx::xstatectx_face_ptr_t statectx_ptr, std::shared_ptr<std::vector<data::xcons_transaction_ptr_t>> txs_ptr, uint64_t current_time_block_height)
  : m_statectx_ptr{statectx_ptr}, m_txs_ptr{txs_ptr}, m_table_account{m_statectx_ptr->get_table_address()}, m_current_time_block_height{current_time_block_height} {
    // xdbg("xstate_reseter: reseter of table %s", m_table_account.get_address().c_str());
    // xdbg("xstate_reseter: table id: %d", m_table_account.get_ledger_subaddr());

    switch (m_table_account.get_zone_index()) {
    case base::enum_chain_zone_consensus_index: {
        // shard 1-4
        m_corresponse_contract_address =
            base::xvaccount_t::make_account_address(std::string{sys_contract_sharding_fork_info_addr}, static_cast<uint16_t>(m_table_account.get_ledger_subaddr()));
        m_need_fork = true;
        break;
    }
    case base::enum_chain_zone_evm_index: {
        // evm shard 0
        // m_corresponse_contract_address =
        //     base::xvaccount_t::make_account_address(std::string{sys_contract_eth_fork_info_addr}, static_cast<uint16_t>(m_table_account.get_ledger_subaddr()));
        m_corresponse_contract_address = std::string{sys_contract_eth_fork_info_addr};  // this const char* already have table id `@0`
        assert(m_table_account.get_ledger_subaddr() == 0);
        m_need_fork = true;
        break;
    }
    default:
        xdbg("xstate_reseter: not consider this table %s", m_table_account.get_address().c_str());
        break;
    }
}

bool xstate_reseter::exec_reset() {
    // 0. if constructor well . might exit more early.
    if (!m_need_fork) {
        xdbg("xstate_reseter::exec_reset need fork = false");
        return false;
    }
    // 1. check contain contract self-tx. each table && each fork need this check.
    if (!check_tx_contains_contract()) {
        xinfo("xstate_reseter:check_tx_contains_contract false");
        return false;
    }

    // 2 && 3 fork time && contract properties should be linked.
    assert(!m_corresponse_contract_address.empty());
    auto fork_info_contract_unit_state = m_statectx_ptr->load_unit_state(common::xaccount_address_t{m_corresponse_contract_address});
    if (fork_info_contract_unit_state == nullptr) {
        return false;
    }
    auto const fork_properties = fork_info_contract_unit_state->string_get(std::string{data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY});

#define IS_FORK_POINT_FROM(from_properties, fork_point)                                                                                                                            \
    (chain_fork::xutility_t::is_forked(fork_points::fork_point, m_current_time_block_height) && fork_properties == (from_properties))

    /// @brief Sample fork code, one and for all.
    if (IS_FORK_POINT_FROM("", v10902_table_tickets_reset)) {
        xkinfo("xstate_reseter::exec_reset v10902_table_tickets_reset enabled");
        auto reseter_ptr = top::make_unique<xstate_tablestate_reseter_sample>(m_statectx_ptr, std::string{"v10902_table_tickets_reset"});
        return reseter_ptr->exec_reset_tablestate();
    }

    xkinfo("xstate_reseter::exec_reset v10902_table_tickets_reset not enabled");
    /// @brief Sample fork code, continues block 
    /// if (IS_FORK_POINT_FROM("", TEST_FORK)) {
    ///     xstate_tablestate_reseter_base_ptr reseter_ptr = top::make_unique<xstate_tablestate_reseter_continuous_sample>(m_statectx_ptr, "TEST_FORK");
    ///     auto fork_index_properties = fork_info_contract_unit_state->string_get(std::string{data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY});
    ///     return reseter_ptr->exec_reset_tablestate(static_cast<std::size_t>(atoi(fork_index_properties.c_str())));
    /// }

    /// @brief Sample fork code every fork point code should be keeped.
    /// else if (IS_FORK_POINT_FROM...) {
    ///     ...
    /// }

    return false;

#undef IS_FORK_POINT_FROM
}

bool xstate_reseter::check_tx_contains_contract() {
    if (m_corresponse_contract_address.empty() || m_txs_ptr->empty()) {
        return false;
    }
    xinfo("xstate_reseter: m_corresponse_contract_address:%s", m_corresponse_contract_address.c_str());
    return std::find_if(m_txs_ptr->begin(), m_txs_ptr->end(), [&](data::xcons_transaction_ptr_t & tx) {
               if (tx->is_self_tx() && tx->get_source_addr() == tx->get_target_addr() && tx->get_source_addr() == m_corresponse_contract_address) {
                   xinfo("xstate_reseter:check_tx_contains_contract, address: %s", tx->get_source_addr().c_str());
                   return true;
               }
               return false;
           }) != m_txs_ptr->end();
}

NS_END2
