// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xconsensus_action.h"
#include "xdata/xtop_action_fwd.h"
#include "xdata/xtop_action_type.h"

NS_BEG2(top, evm_runtime)

enum xtop_evm_action_type {
    invalid = 0,
    deploy_contract = 1,
    call_contract = 2,
};

NS_END2

NS_BEG2(top, data)

using evm_runtime::xtop_evm_action_type;

template <>
class xtop_consensus_action<xtop_action_type_t::evm> : public xtop_action_t<xtop_action_type_t::evm> {
public:
private:
    xtop_evm_action_type m_action_type{xtop_evm_action_type::invalid};

public:
    xtop_consensus_action(xtop_consensus_action const &) = default;
    xtop_consensus_action & operator=(xtop_consensus_action const &) = default;
    xtop_consensus_action(xtop_consensus_action &&) = default;
    xtop_consensus_action & operator=(xtop_consensus_action &&) = default;
    ~xtop_consensus_action() override = default;

    explicit xtop_consensus_action(xobject_ptr_t<data::xcons_transaction_t> const & tx) noexcept;

    xtop_evm_action_type evm_action() const;

    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;

    xbyte_buffer_t data() const;

    // In fact. this should be U256.
    // Since deploy and call usually be zero.
    // todo later
    uint64_t value() const;

    uint64_t gas_limit() const;

    uint64_t gas_price() const;

    // reference from src/xtopcom/xdata/xconsensus_action.h
    // common::xaccount_address_t sender() const;
    // common::xaccount_address_t recver() const;
    // common::xaccount_address_t contract_address() const;
    // common::xaccount_address_t execution_address() const;
    // uint64_t max_gas_amount() const;
    // uint64_t last_nonce() const noexcept;
    // uint64_t nonce() const noexcept;
    // uint256_t hash() const noexcept;
    // std::string source_action_name() const;
    // std::string target_action_name() const;
    // xbyte_buffer_t source_action_data() const;
    // xbyte_buffer_t target_action_data() const;
    // // xreceipt_data_store_t receipt_data() const;
    // xaction_consensus_exec_status action_consensus_result() const;
    // data::enum_xtransaction_type transaction_type() const;
    // data::enum_xaction_type source_action_type() const;
    // data::enum_xaction_type target_action_type() const;
    // uint32_t size() const;
    // uint64_t deposit() const;
    // std::string digest_hex() const;
    // uint32_t used_tgas() const;
    // uint32_t used_disk() const;
    // uint32_t last_action_send_tx_lock_tgas() const;
    // uint32_t last_action_used_deposit() const;
    // uint32_t last_action_recv_tx_use_send_tx_tgas() const;
    // enum_xunit_tx_exec_status last_action_exec_status() const;
};

NS_END2