#pragma once

// #include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_state.h"
#include "xevm_contract_runtime/xevm_context.h"

namespace top {
namespace evm_runtime {

using xevm_state_t = top::contract_common::xcontract_state_t;
using xevm_param_t = top::contract_common::xcontract_execution_param_t;
// using xevm_context_t = top::contract_common::xcontract_execution_context_t;
using xevm_context_t = top::evm_runtime::xevm_context_t;

}  // namespace evm_runtime
}  // namespace top