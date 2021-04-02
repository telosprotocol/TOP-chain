#pragma once

#include "xcontract_common/xcontract_execution_output.h"
#include "xcontract_common/xcontract_execution_status.h"

NS_BEG2(top, contract_common)

struct xtop_contract_execution_result {
    xcontract_execution_status_t status;
    xcontract_execution_output_t output;
};
using xcontract_execution_result_t = xtop_contract_execution_result;

NS_END2
