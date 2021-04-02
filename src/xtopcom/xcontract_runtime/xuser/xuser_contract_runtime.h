#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_runtime/xuser/xuser_contract_runtime_fwd.h"
#include "xcontract_runtime/xuser/xlua/xengine.h"
#include "xcontract_runtime/xvm/xtype.h"
#include "xcontract_runtime/xvm_session.h"
#include "xdata/xtransaction.h"
#include "xcontract_runtime/xvm/xbasic_runtime.h"

#include <unordered_map>

NS_BEG3(top, contract_runtime, user)

class xtop_user_contract_runtime : public vm::xbasic_runtime_t {
private:
    std::unordered_map<vm::xtype_t, std::shared_ptr<lua::xengine_t>> engines_;

public:
    xtransaction_execution_result_t execute_transaction(observer_ptr<contract_common::xcontract_execution_context_t> tx_ctx) override;
};

NS_END3
