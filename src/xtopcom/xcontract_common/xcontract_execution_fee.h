// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>
#include <map>

NS_BEG2(top, contract_common)

enum xtop_contract_execution_fee_option { send_tx_lock_tgas, used_tgas, used_deposit, used_disk, recv_tx_use_send_tx_tgas };
using xcontract_execution_fee_option_t = xtop_contract_execution_fee_option;

using xcontract_execution_fee_t = std::map<xcontract_execution_fee_option_t, uint64_t>;

NS_END2
