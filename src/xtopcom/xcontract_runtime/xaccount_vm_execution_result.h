#pragma once

#include "xcontract_runtime/xtransaction_execution_result.h"

#include <string>
#include <system_error>
#include <vector>

NS_BEG2(top, contract_runtime)

struct xtop_account_vm_execution_status {
    std::error_code ec;
    std::string extra_msg;
};
using xaccount_vm_execution_status_t = xtop_account_vm_execution_status;

struct xtop_account_vm_execution_result {
    xaccount_vm_execution_status_t status;
    std::vector<xtransaction_execution_result_t> transaction_results;
};
using xaccount_vm_execution_result_t = xtop_account_vm_execution_result;

NS_END2
