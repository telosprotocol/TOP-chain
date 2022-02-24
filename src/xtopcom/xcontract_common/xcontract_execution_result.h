// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>

#include "xcontract_common/xcontract_execution_output.h"
#include "xcontract_common/xcontract_execution_status.h"

NS_BEG2(top, contract_common)

std::string const RECEITP_DATA_ASSET_OUT = "src_asset_out";

struct xtop_contract_execution_result {
    xcontract_execution_status_t status;
    xcontract_execution_output_t output;
};
using xcontract_execution_result_t = xtop_contract_execution_result;

NS_END2
