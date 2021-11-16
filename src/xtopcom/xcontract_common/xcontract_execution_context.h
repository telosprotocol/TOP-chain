// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_common/xcontract_execution_param.h"
#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xcontract_state.h"
#include "xdata/xreceipt_data_store.h"
#include "xdata/xconsensus_action.h"
#include "xdata/xtop_action.h"
#include "xdata/xtransaction.h"

#include <map>
#include <string>

NS_BEG2(top, contract_common)


class xtop_contract_execution_context {
private:
    observer_ptr<xcontract_state_t> m_contract_state{};
    std::unique_ptr<data::xbasic_top_action_t const> m_action;
    xreceipt_data_store_t m_receipt_data; // input receipt

    data::xconsensus_action_stage_t m_stage{data::xconsensus_action_stage_t::invalid};
    xcontract_execution_result_t m_execution_result; // execution result

    using xcontract_object_cb_t = std::function<observer_ptr<xbasic_contract_t>(common::xaccount_address_t const & address)>;
    xcontract_object_cb_t m_system_contract;

public:
    xtop_contract_execution_context() = default;
    xtop_contract_execution_context(xtop_contract_execution_context const &) = delete;
    xtop_contract_execution_context & operator=(xtop_contract_execution_context const &) = delete;
    xtop_contract_execution_context(xtop_contract_execution_context &&) = default;
    xtop_contract_execution_context & operator=(xtop_contract_execution_context &&) = default;
    ~xtop_contract_execution_context() = default;

    explicit xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action, observer_ptr<xcontract_state_t> s) noexcept;

    observer_ptr<xcontract_state_t> contract_state() const noexcept;
    void contract_state(observer_ptr<xcontract_state_t> new_state) noexcept;

    data::xconsensus_action_stage_t consensus_action_stage() const noexcept;
    void consensus_action_stage(data::xconsensus_action_stage_t const stage) noexcept;
    // observer_ptr<xbasic_contract_t> system_contract(common::xaccount_address_t const & address) const;
    // void system_contract(xcontract_object_cb_t cb) noexcept;
    xcontract_execution_result_t const & execution_result() const noexcept;
    void add_followup_transaction(data::xcons_transaction_ptr_t tx, xfollowup_transaction_schedule_type_t type);
    std::vector<xfollowup_transaction_datum_t> const & followup_transaction() const noexcept;

    void input_receipt_data(xreceipt_data_store_t const& receipt_data);
    xreceipt_data_store_t& output_receipt_data() noexcept;
    xbyte_buffer_t input_receipt_data(std::string const & key) const;
    void remove_input_receipt_data(std::string const & key);
    data::xaction_consensus_exec_status action_consensus_result() const noexcept;

    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;
    common::xaccount_address_t contract_address() const;

    data::enum_xtransaction_type transaction_type() const noexcept;

    std::string action_name() const;
    data::enum_xaction_type action_type() const;
    data::enum_xaction_type source_action_type() const;
    data::enum_xaction_type target_action_type() const;
    xbyte_buffer_t action_data() const;
    data::xconsensus_action_stage_t action_stage() const;

    uint64_t last_nonce() const;
    uint64_t nonce() const;
    uint256_t hash() const;
    uint64_t deposit() const;
    uint32_t used_tgas() const;
    uint32_t used_disk() const;
    uint32_t size() const;
    std::string digest_hex() const;
    uint32_t last_action_send_tx_lock_tgas() const;
    uint32_t last_action_used_deposit() const;
    uint32_t last_action_recv_tx_use_send_tx_tgas() const;
    data::enum_xunit_tx_exec_status last_action_exec_status() const;

    xcontract_execution_fee_t action_preprocess(std::error_code & ec);

private:
    xcontract_execution_fee_t execute_default_source_action(std::error_code & ec);
    xcontract_execution_fee_t execute_default_target_action(std::error_code & ec);
    xcontract_execution_fee_t execute_default_confirm_action(std::error_code & ec);
    void update_tgas_disk_sender(bool is_contract, xcontract_execution_fee_t & fee_change, std::error_code & ec);
    void calc_used_tgas(uint64_t deposit, uint64_t & cur_tgas_usage, uint64_t & deposit_usage, std::error_code & ec) const;
    void incr_used_tgas(uint64_t num, std::error_code & ec);
    uint64_t calc_available_tgas() const;
    uint64_t calc_total_tgas() const;
    uint64_t calc_free_tgas() const;
    uint64_t calc_decayed_tgas() const;
    uint64_t calc_token_price() const;
    uint64_t calc_cost_tgas(bool is_contract) const;
    uint64_t calc_cost_disk(bool is_contract) const;

    std::string source_action_name() const;
    std::string target_action_name() const;
    xbytes_t source_action_data() const;
    xbytes_t target_action_data() const;
    data::xproperty_asset asset() const;
};
using xcontract_execution_context_t = xtop_contract_execution_context;

NS_END2
