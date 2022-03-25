// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address.h"
#include "xcontract_runtime/xaction_session.h"
#include "xdata/xconsensus_action_fwd.h"
#include "xevm_statestore_helper/xstatestore_helper.h"
#include "xstate_accessor/xstate_accessor.h"

NS_BEG2(top, contract_runtime)

template <>
class xtop_action_session<data::xevm_consensus_action_t> {
private:
    observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> m_associated_runtime;
    observer_ptr<evm_statestore::xevm_statestore_helper_t> m_statestore_helper;
    // observer_ptr<contract_common::xcontract_state_t> m_contract_state;
    std::map<common::xaccount_address_t, state_accessor::xstate_accessor_t> m_state_accessor_cache;

    // todo add observer state store ptr

public:
    xtop_action_session(xtop_action_session const &) = delete;
    xtop_action_session & operator=(xtop_action_session const &) = delete;
    xtop_action_session(xtop_action_session &&) = default;
    xtop_action_session & operator=(xtop_action_session &&) = default;
    ~xtop_action_session() = default;

    xtop_action_session(observer_ptr<xaction_runtime_t<data::xevm_consensus_action_t>> associated_runtime,
                        observer_ptr<evm_statestore::xevm_statestore_helper_t> const & statestore_helper) noexcept;

    xtransaction_execution_result_t execute_action(std::unique_ptr<data::xbasic_top_action_t const> action);
};

NS_END2