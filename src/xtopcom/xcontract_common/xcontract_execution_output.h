#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcontract_common/xfollowup_transaction_datum.h"

#include <map>
#include <string>
#include <vector>

NS_BEG2(top, contract_common)

struct xtop_contract_execution_output {
    std::map<std::string, xbyte_buffer_t> receipt_data;
    std::vector<xfollowup_transaction_datum_t> followup_transaction_data;
};
using xcontract_execution_output_t = xtop_contract_execution_output;

NS_END2
