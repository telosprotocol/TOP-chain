// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xuser/xuser_action_runtime.h"

#include "xcontract_common/xcontract_state.h"
#include "xcontract_runtime/xaction_session.h"
#include "xcontract_runtime/xerror/xerror.h"

NS_BEG2(top, contract_runtime)

using xuser_consensus_action_t = data::xconsensus_action_t<data::xtop_action_type_t::user>;

std::unique_ptr<xaction_session_t<xuser_consensus_action_t>> xtop_action_runtime<xuser_consensus_action_t>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<data::xconsensus_action_t<data::xtop_action_type_t::user>>>(top::make_observer(this), contract_state);
}

xtransaction_execution_result_t xtop_action_runtime<xuser_consensus_action_t>::execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) {
    xtransaction_execution_result_t result;

    try {
    } catch (error::xcontract_runtime_error_t const & eh) {
        result.status.ec = eh.code();
    } catch (std::exception const & eh) {
        result.status.ec = error::xerrc_t::unknown_error;
        result.status.extra_msg = eh.what();
    } catch (...) {
        result.status.ec = error::xerrc_t::unknown_error;
    }

    return result;
}

NS_END2
