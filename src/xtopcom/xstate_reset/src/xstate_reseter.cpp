// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_reseter.h"

#include "xbasic/xmemory.hpp"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xdata/xnative_contract_address.h"
#include "xstate_reset/xstate_tablestate_reseter_sample.h"
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
        return false;
    }
    // 1. check contain contract self-tx. each table && each fork need this check.
    if (!check_tx_contains_contract()) {
        xinfo("xstate_reseter:check_tx_contains_contract false");
        return false;
    }

    /// TODO : 2 && 3 fork time && contract properties should be linked,
    /// as each fork-time corresponse a extra properties sring value changed from A to B
    /// so this code shoud be used multi-time and each fork chooses a fork point one after another.
    /// sample code:
    // // 2. check fork time
    // auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
    // auto if_forked = chain_fork::xchain_fork_config_center_t::is_forked(fork_config.__TODO__SOME_FORK_POINT, m_current_time_block_height);
    // if (!if_forked) {
    //     return false;
    // }
    // // 3. check contract properties is unset.
    // assert(m_statectx_ptr != nullptr);
    // auto fork_info_contract_unit_state = m_statectx_ptr->load_unit_state(base::xvaccount_t{m_corresponse_contract_address});
    // if (fork_info_contract_unit_state == nullptr) {
    //     return false;
    // }
    // auto fork_properties = fork_info_contract_unit_state->string_get(std::string{data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY});
    // if (fork_properties != "") {
    //     xinfo("xstate_reseter:check_contract_properties false");
    //     return false;
    // }

    /// used like this:
    assert(!m_corresponse_contract_address.empty());
    auto fork_info_contract_unit_state = m_statectx_ptr->load_unit_state(base::xvaccount_t{m_corresponse_contract_address});
    if (fork_info_contract_unit_state == nullptr) {
        return false;
    }
    auto fork_properties = fork_info_contract_unit_state->string_get(std::string{data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY});
    auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();

#define IS_FORK_POINT_FROM(from_properties, fork_point)                                                                                                                            \
    (chain_fork::xchain_fork_config_center_t::is_forked(fork_config.fork_point, m_current_time_block_height) && fork_properties == from_properties)

    if (IS_FORK_POINT_FROM("", v1_7_0_sync_point)) {  // use last fork point as sample code
        // test_fork_reset
        xstate_tablestate_reseter_base_ptr reseter_ptr = top::make_unique<xstate_tablestate_reseter_sample>(m_statectx_ptr, "TEST_FORK");
        reseter_ptr->exec_reset_tablestate();
    }
    // else if(IS_FORK_POINT_FROM...) { // every fork point code should be keeped.
    //   ...
    // }

    return false;

#undef IS_FORK_POINT_FROM
}

bool xstate_reseter::check_tx_contains_contract() {
    if (m_corresponse_contract_address.empty() || m_txs_ptr->empty()) {
        return false;
    };
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