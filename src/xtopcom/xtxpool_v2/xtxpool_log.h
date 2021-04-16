// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlog.h"
#include "xbase/xns_macro.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

#undef __MODULE__
#define __MODULE__ "xtxpool_v2"

std::string get_error_str(int32_t code);

#define xtxpool_dbg xdbg
#define xtxpool_dbg_info xdbg_info
#define xtxpool_info xinfo
#define xtxpool_kinfo xkinfo
#define xtxpool_warn xwarn
#define xtxpool_error xerror

NS_END2
