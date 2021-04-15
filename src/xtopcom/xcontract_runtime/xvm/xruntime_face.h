#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_state_fwd.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xvm/xruntime_face_fwd.h"
#include "xcontract_runtime/xvm_session.h"
#include "xdata/xtransaction.h"

NS_BEG3(top, contract_runtime, vm)

class xtop_runtime_face {
public:
    xtop_runtime_face() = default;
    xtop_runtime_face(xtop_runtime_face const &) = delete;
    xtop_runtime_face & operator=(xtop_runtime_face const &) = delete;
    xtop_runtime_face(xtop_runtime_face &&) = default;
    xtop_runtime_face & operator=(xtop_runtime_face &&) = default;
    virtual ~xtop_runtime_face() = default;

    virtual xtransaction_execution_result_t execute_transaction(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) = 0;
    virtual std::unique_ptr<xsession_t> new_session(observer_ptr<contract_common::xcontract_state_t> contract_state) = 0;
};

NS_END3
