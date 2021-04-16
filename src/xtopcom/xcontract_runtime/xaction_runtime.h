#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xaction_runtime_fwd.h"
#include "xcontract_runtime/xaction_session_fwd.h"
#include "xdata/xtransaction.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
class xtop_action_runtime {
public:
    xtop_action_runtime() = default;
    xtop_ruxtop_action_runtimentime_face(xtop_action_runtime const &) = delete;
    xtop_action_runtime & operator=(xtop_action_runtime const &) = delete;
    xtop_action_runtime(xtop_action_runtime &&) = default;
    xtop_action_runtime & operator=(xtop_action_runtime &&) = default;
    ~xtop_action_runtime() = default;

    std::unique_ptr<xaction_session_t<ActionT>> new_session(observer_ptr<contract_common::xcontract_state_t> contract_state);

    xtransaction_execution_result_t execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx);
};

NS_END2

#include "xcontract_runtime/xaction_session.h"

NS_BEG2(top, contract_runtime)

template <typename ActionT>
std::unique_ptr<xaction_sessionT<ActionT>> xtop_action_runtime<ActionT>::new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) {
    return top::make_unique<xaction_session_t<ActionT>>(top::make_observer(this), contract_state);
}

template <typename ActionT>
xtransaction_execution_result_t xtop_action_runtime<ActionT>::execute(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) {

}

NS_END2
