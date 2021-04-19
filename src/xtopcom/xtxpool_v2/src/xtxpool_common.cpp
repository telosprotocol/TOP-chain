// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xmodule_type.h"
#include "xtxpool_v2/xtxpool_log.h"

#include <string>

NS_BEG2(top, xtxpool_v2)

std::string get_error_str(int32_t code) {
    return chainbase::xmodule_error_to_str(code);
}

NS_END2
