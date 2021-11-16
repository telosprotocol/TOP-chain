// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcontract_common/xcontract_execution_fee.h"
#include "xdata/xreceipt_data_store.h"
#include "xcontract_common/xfollowup_transaction_datum.h"

#include <map>
#include <string>
#include <vector>

NS_BEG2(top, contract_common)

struct xtop_contract_execution_output {
    xcontract_execution_fee_t fee_change;
    data::xreceipt_data_store_t receipt_data;
    std::vector<xfollowup_transaction_datum_t> followup_transaction_data;
    std::string binlog;
    std::string contract_state_snapshot;
};
using xcontract_execution_output_t = xtop_contract_execution_output;

NS_END2
