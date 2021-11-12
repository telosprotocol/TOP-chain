// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xerror/xerror.h"

#include <string>
#include <system_error>

NS_BEG2(top, contract_common)

struct xtop_contract_execution_status {
    std::error_code ec{error::xerrc_t::ok};
    std::string extra_msg{};
};
using xcontract_execution_status_t = xtop_contract_execution_status;

NS_END2
