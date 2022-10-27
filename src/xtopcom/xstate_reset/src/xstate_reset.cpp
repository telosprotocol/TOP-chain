// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_reset/xstate_reset.h"

// #include "xchain_fork/xchain_upgrade_center.h"
#include "xdata/xnative_contract_address.h"
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
        break;
    }
    case base::enum_chain_zone_evm_index: {
        // evm shard 0
        // m_corresponse_contract_address =
        //     base::xvaccount_t::make_account_address(std::string{sys_contract_eth_fork_info_addr}, static_cast<uint16_t>(m_table_account.get_ledger_subaddr()));
        m_corresponse_contract_address = std::string{sys_contract_eth_fork_info_addr};  // this const char* already have table id `@0`
        assert(m_table_account.get_ledger_subaddr() == 0);
        break;
    }
    default:
        xdbg("xstate_reseter: not consider this table %s", m_table_account.get_address().c_str());
        break;
    }
}

bool xstate_reseter::exec_reset() {
    // 1. check fork time
    // auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
    // auto if_forked = chain_fork::xchain_fork_config_center_t::is_forked(fork_config.__TODO__SOME_FORK_POINT, m_current_time_block_height);
    // if (!if_forked) {
    //     return false;
    // }

    // 2. check contain contract self-tx
    if (!check_tx_contains_contract()) {
        xinfo("xstate_reseter:check_tx_contains_contract false");
        return false;
    }

    // 3. check contract properties is unset.
    assert(m_statectx_ptr != nullptr);
    xdbg("xstate_reseter: [DEBUG] %s", m_statectx_ptr->get_table_address().c_str());  // if this exist. constructor can be easier.
    auto fork_info_contract_unit_state = m_statectx_ptr->load_unit_state(base::xvaccount_t{m_corresponse_contract_address});
    if (fork_info_contract_unit_state == nullptr) {
        return false;
    }
    auto fork_properties = fork_info_contract_unit_state->string_get(std::string{data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY});
    // todo this should be some kind of fork version, use some greater-equal than \ less than operator.
    if (fork_properties != "") {
        xinfo("xstate_reseter:check_contract_properties false");
        return false;
    }

    return true;
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