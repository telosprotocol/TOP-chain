// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xmemory.hpp"
#include "xbase/xobject_ptr.h"
#include "xcommon/xaddress.h"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xcontract_execution_result.h"
#include "xdata/xtransaction.h"
#include "xdata/xconsensus_action_stage.h"

#include <map>
#include <string>

NS_BEG2(top, contract_common)

template <typename ActionT>
class xtop_action_execution_context {
private:
    observer_ptr<xcontract_state_t> m_contract_state{};
    ActionT m_action;
    std::map<std::string, xbyte_buffer_t> m_receipt_data; // input receipt
    xcontract_execution_result_t m_execution_result; // execution result

public:
    xtop_action_execution_context() = default;
    xtop_action_execution_context(xtop_action_execution_context const &) = delete;
    xtop_action_execution_context & operator=(xtop_action_execution_context const &) = delete;
    xtop_action_execution_context(xtop_action_execution_context &&) = default;
    xtop_action_execution_context & operator=(xtop_action_execution_context &&) = default;
    ~xtop_action_execution_context() = default;

    xtop_action_execution_context(ActionT action, observer_ptr<xcontract_state_t> s) noexcept;

    observer_ptr<xcontract_state_t> contract_state() const noexcept;

    data::xconsensus_action_stage_t consensus_action_stage() const noexcept;

    xcontract_execution_result_t execution_result() const noexcept;
    void add_followup_transaction(data::xtransaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type);

    void receipt_data(std::map<std::string, xbyte_buffer_t> receipt_data);
    std::map<std::string, xbyte_buffer_t> const & receipt_data() const noexcept;
    xbyte_buffer_t const & receipt_data(std::string const & key, std::error_code & ec) const noexcept;

    common::xaccount_address_t sender() const;
    common::xaccount_address_t recver() const;
    common::xaccount_address_t contract_address() const;

    data::enum_xtransaction_type transaction_type() const noexcept;

    std::string action_name() const;
    data::enum_xaction_type action_type() const;
    xbyte_buffer_t action_data() const;
};
using xcontract_execution_context_t = xtop_action_execution_context;

NS_END2

#include "xbasic/xutility.h"
#include "xcontract_common/xerror/xerror.h"

NS_BEG2(top, contract_common)

template <typename ActionT>
xtop_action_execution_context<ActionT>::xtop_action_execution_context(ActionT action, observer_ptr<xcontract_state_t> s) noexcept : m_action{ std::move(action) }, m_contract_state{ s } {
}

template <typename ActionT>
observer_ptr<xcontract_state_t> xtop_action_execution_context<ActionT>::contract_state() const noexcept {
    return m_contract_state;
}

template <typename ActionT>
data::xconsensus_action_stage_t xtop_action_execution_context<ActionT>::consensus_action_stage() const noexcept {
    return m_action.stage();
}

template <typename ActionT>
xcontract_execution_result_t xtop_action_execution_context<ActionT>::execution_result() const noexcept {
    return m_execution_result;
}

template <typename ActionT>
void xtop_action_execution_context<ActionT>::add_followup_transaction(data::xtransaction_ptr_t && tx, xfollowup_transaction_schedule_type_t type) {
    m_execution_result.output.followup_transaction_data.emplace_back(std::move(tx), type);
}

template <typename ActionT>
void xtop_action_execution_context<ActionT>::receipt_data(std::map<std::string, xbyte_buffer_t> receipt_data) {
    m_receipt_data = std::move(receipt_data);
}

template <typename ActionT>
std::map<std::string, xbyte_buffer_t> const & xtop_action_execution_context<ActionT>::receipt_data() const noexcept {
    return m_execution_result.output.receipt_data;;
}

template <typename ActionT>
xbyte_buffer_t const & xtop_action_execution_context<ActionT>::receipt_data(std::string const & key, std::error_code & ec) const noexcept {
    static xbyte_buffer_t const empty;
    auto const it = m_receipt_data.find(key);
    if (it != std::end(m_receipt_data)) {
        return top::get<xbyte_buffer_t>(*it);
    }

    ec = error::xerrc_t::receipt_data_not_found;
    return empty;
}

template <typename ActionT>
common::xaccount_address_t xtop_action_execution_context<ActionT>::sender() const {
    return m_action.send_address();
}

template <typename ActionT>
common::xaccount_address_t xtop_action_execution_context<ActionT>::recver() const {
    return m_action.to_address();
}

template <typename ActionT>
common::xaccount_address_t xtop_action_execution_context<ActionT>::contract_address() const {
    return m_action.contract_address();
}

template <typename ActionT>
std::string xtop_action_execution_context<ActionT>::action_name() const {
    return m_action.action_name();
}

template <typename ActionT>
data::enum_xaction_type xtop_action_execution_context<ActionT>::action_type() const {
    return m_action.type();
}

template <typename ActionT>
xbyte_buffer_t xtop_action_execution_context<ActionT>::action_data() const {
    return m_action.action_data();
}

NS_END2
