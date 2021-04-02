#pragma once

#include "xbasic/xns_macro.h"

#include <string>
#include <system_error>

NS_BEG2(top, contract_common)

struct xtop_contract_execution_status {
    std::error_code ec;
    std::string extra_msg;
};
using xcontract_execution_status_t = xtop_contract_execution_status;

NS_END2
