// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlog.h"
#include "xbasic/xns_macro.h"

#include <string>

NS_BEG2(top, xtxpool)

#undef __MODULE__
#define __MODULE__ "xtxpool"

#define XTXPOOL_TX_WARN(transaction, fmt, ...) xwarn("[global_trace][xtxpool]%s" fmt, transaction->get_digest_hex_str().c_str(), ##__VA_ARGS__)

std::string get_error_str(int32_t code);

NS_END2
