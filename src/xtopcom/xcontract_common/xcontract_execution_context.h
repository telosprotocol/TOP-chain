// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xcommon/xaddress.h"
#include "xcontract_common/xaction_execution_param.h"
#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xcontract_execution_stage.h"
#include "xcontract_common/xcontract_state.h"
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
    std::map<std::string, xbyte_buffer_t> m_receipt_data; // input receipt

    xcontract_execution_stage_t m_execution_stage{xcontract_execution_stage_t::invalid};
    data::xconsensus_action_stage_t m_stage{data::xconsensus_action_stage_t::invalid};
    xcontract_execution_result_t m_execution_result; // execution result

public:
    xtop_contract_execution_context() = default;
    xtop_contract_execution_context(xtop_contract_execution_context const &) = delete;
    xtop_contract_execution_context & operator=(xtop_contract_execution_context const &) = delete;
    xtop_contract_execution_context(xtop_contract_execution_context &&) = default;
    xtop_contract_execution_context & operator=(xtop_contract_execution_context &&) = default;
    ~xtop_contract_execution_context() = default;

    explicit xtop_contract_execution_context(std::unique_ptr<data::xbasic_top_action_t const> action, observer_ptr<xcontract_state_t> s) noexcept;

    observer_ptr<xcontract_state_t> contract_state() const noexcept;
    xcontract_execution_stage_t execution_stage() const noexcept;
    void execution_stage(xcontract_execution_stage_t const stage) noexcept;
    data::xconsensus_action_stage_t consensus_action_stage() const noexcept;
    void consensus_action_stage(data::xconsensus_action_stage_t const stage) noexcept;
    xcontract_execution_result_t const & execution_result() const noexcept;
    void add_followup_transaction(data::xcons_transaction_ptr_t tx, xfollowup_transaction_schedule_type_t type);
    std::vector<xfollowup_transaction_datum_t> const & followup_transaction() const noexcept;

    void input_receipt_data(std::map<std::string, xbyte_buffer_t> receipt_data);
    std::map<std::string, xbyte_buffer_t> & input_receipt_data() noexcept;
    std::map<std::string, xbyte_buffer_t> & output_receipt_data() noexcept;
    xbyte_buffer_t const & input_receipt_data(std::string const & key, std::error_code & ec) const noexcept;

    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;
    common::xaccount_address_t contract_address() const;

    data::enum_xtransaction_type transaction_type() const noexcept;

    std::string action_name() const;
    data::enum_xaction_type action_type() const;
    data::enum_xaction_type source_action_type() const;
    data::enum_xaction_type target_action_type() const;
    xbyte_buffer_t action_data() const;
    std::string source_action_data() const;
    std::string target_action_data() const;
    data::xconsensus_action_stage_t action_stage() const;

    uint32_t size() const;
    uint32_t deposit() const;
    uint32_t used_tgas() const;
    uint32_t used_disk() const;
    std::string digest_hex() const;
    uint32_t last_action_send_tx_lock_tgas() const;
    uint32_t last_action_used_deposit() const;
    uint32_t last_action_recv_tx_use_send_tx_tgas() const;
    data::enum_xunit_tx_exec_status last_action_exec_status() const;

    bool verify_action(std::error_code & ec);
    void execute_default_source_action(contract_common::xcontract_execution_fee_t & fee_change);
    void execute_default_target_action(contract_common::xcontract_execution_fee_t & fee_change);
    void execute_default_confirm_action(contract_common::xcontract_execution_fee_t & fee_change);

private:
    void update_tgas_disk_sender(bool is_contract, contract_common::xcontract_execution_fee_t & fee_change, std::error_code & ec);
    void calc_resource(uint32_t & tgas, uint32_t deposit, uint32_t & used_deposit, std::error_code & ec);
    void calc_used_tgas(uint32_t deposit, uint32_t & cur_tgas_usage, uint64_t & deposit_usage, std::error_code & ec) const;
    void incr_used_tgas(uint64_t num, std::error_code & ec);
    uint64_t calc_available_tgas() const;
    uint64_t calc_total_tgas() const;
    uint64_t calc_free_tgas() const;
    uint64_t calc_decayed_tgas() const;
    uint64_t calc_token_price() const;
    uint32_t calc_cost_tgas(bool is_contract) const;
    uint32_t calc_cost_disk(bool is_contract) const;
};
using xcontract_execution_context_t = xtop_contract_execution_context;

NS_END2
