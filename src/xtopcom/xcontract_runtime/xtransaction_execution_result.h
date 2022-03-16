// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xcontract_execution_output.h"
#include "xcontract_common/xcontract_execution_status.h"
#include "xcontract_common/xcontract_execution_result.h"

NS_BEG2(top, contract_runtime)

using xtransaction_execution_status_t = contract_common::xcontract_execution_status_t;
using xtransaction_execution_output_t = contract_common::xcontract_execution_output_t;
using xtransaction_execution_result_t = contract_common::xcontract_execution_result_t;

NS_END2
