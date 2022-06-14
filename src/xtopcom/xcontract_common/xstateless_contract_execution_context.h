// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xfollowup_transaction_datum.h"
#include "xdata/xconsensus_action_stage.h"
#include "xdata/xunit_bstate.h"

NS_BEG2(top, contract_common)

class xtop_stateless_contract_execution_context {
public:
    xtop_stateless_contract_execution_context() = delete;
    xtop_stateless_contract_execution_context(xtop_stateless_contract_execution_context const &) = delete;
    xtop_stateless_contract_execution_context & operator=(xtop_stateless_contract_execution_context const &) = delete;
    xtop_stateless_contract_execution_context(xtop_stateless_contract_execution_context &&) = default;
    xtop_stateless_contract_execution_context & operator=(xtop_stateless_contract_execution_context &&) = default;
    ~xtop_stateless_contract_execution_context() = default;

    explicit xtop_stateless_contract_execution_context(observer_ptr<data::xunit_bstate_t> unitstate_owned) : m_unitstate_owned{unitstate_owned} {
        xassert(unitstate_owned != nullptr);
    }

    xtop_stateless_contract_execution_context(observer_ptr<data::xunit_bstate_t> unitstate_owned, observer_ptr<data::xunit_bstate_t> unitstate_other)
      : m_unitstate_owned{unitstate_owned}, m_unitstate_other{unitstate_other} {
        xassert(unitstate_owned != nullptr);
        xassert(unitstate_other != nullptr);
    }

    observer_ptr<data::xunit_bstate_t> unitstate_owned() const {
        return m_unitstate_owned;
    }

    observer_ptr<data::xunit_bstate_t> unitstate_other() const {
        return m_unitstate_other;
    }

    void set_unitstate_other(observer_ptr<data::xunit_bstate_t> unitstate_other) {
        xassert(unitstate_other != nullptr);
        m_unitstate_other = unitstate_other;
    }

    data::xconsensus_action_stage_t action_stage() const {
        return m_stage;
    }

    void set_action_stage(const base::enum_transaction_subtype tx_stage) {
        switch (tx_stage) {  // NOLINT(clang-diagnostic-switch-enum)
        case base::enum_transaction_subtype_send:
            m_stage = data::xconsensus_action_stage_t::send;
            break;
        case base::enum_transaction_subtype_recv:
            m_stage = data::xconsensus_action_stage_t::recv;
            break;
        case base::enum_transaction_subtype_confirm:
            m_stage = data::xconsensus_action_stage_t::confirm;
            break;
        case base::enum_transaction_subtype_self:
            m_stage = data::xconsensus_action_stage_t::self;
            break;
        default:
            assert(false);
            m_stage = data::xconsensus_action_stage_t::invalid;
            break;
        }
    }

    std::string action_name() const {
        return m_action_name;
    }

    void set_action_name(std::string name) {
        m_action_name = name;
    }

    xbytes_t action_data() const {
        return m_action_data;
    }

    void set_action_data(xbytes_t data) {
        m_action_data = data;
    }

    std::vector<xfollowup_transaction_datum_t> followup_transaction() const {
        return m_execution_result.output.followup_transaction_data;
    };

    void add_followup_transaction(data::xcons_transaction_ptr_t tx, xfollowup_transaction_schedule_type_t type) {
        m_execution_result.output.followup_transaction_data.emplace_back(std::move(tx), type);
    }

private:
    observer_ptr<data::xunit_bstate_t> m_unitstate_owned{nullptr};
    observer_ptr<data::xunit_bstate_t> m_unitstate_other{nullptr};

    std::string m_action_name;
    xbytes_t m_action_data;
    data::xconsensus_action_stage_t m_stage{data::xconsensus_action_stage_t::invalid};
    xcontract_execution_result_t m_execution_result;
};
using xstateless_contract_execution_context_t = xtop_stateless_contract_execution_context;

NS_END2
