// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <vector>
#include <algorithm>
#include <cstring>
#include <string>
#include "math.h"
#include "xbase/xint.h"
#include "xdata/xdatautil.h"
#include "xerror/xrpc_error.h"

NS_BEG2(top, xrpc)
using std::vector;

// transform hex str to raw uint8 string
inline std::string hex_to_uint8_str(std::string const& str) {
    auto ret_vec = data::hex_to_uint(str);
    std::stringstream ss;
    for(size_t i = 0; i < ret_vec.size(); i++){
        ss << ret_vec[i];
    }
    return ss.str();
}

inline bool is_valid_hex(std::string const& str) {
    if (str.size() < 2 || str.size() % 2 || str[0] != '0' || str[1] != 'x') {
        return false;
    }
    for (size_t i = 2; i < str.size(); ++i) {
        if (-1 == data::hex_to_dec(str[i])) {
            return false;
        }
    }
    return true;
}


NS_END2
