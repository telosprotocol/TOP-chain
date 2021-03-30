// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbasic/xns_macro.h"

NS_BEG2(top, config)

template <typename T>
struct xtop_to_string_helper final {
    static std::string to_string(T const & v) { return std::to_string(v); }
};

template <>
struct xtop_to_string_helper<char const *> final {
    static std::string to_string(char const * v) { return std::string{v}; }
};

template <typename T>
using xto_string_helper_t = xtop_to_string_helper<T>;

NS_END2
