// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <string>
#include <system_error>

NS_BEG1(top)

template <typename T>
class xtop_enable_to_string {
public:
    xtop_enable_to_string()                                          = default;
    xtop_enable_to_string(xtop_enable_to_string const &)             = default;
    xtop_enable_to_string & operator=(xtop_enable_to_string const &) = default;
    xtop_enable_to_string(xtop_enable_to_string &&)                  = default;
    xtop_enable_to_string & operator=(xtop_enable_to_string &&)      = default;
    virtual ~xtop_enable_to_string()                                 = default;

    virtual std::string to_string() const = 0;
    virtual void from_string(std::string const & s, std::error_code & ec) = 0;
};
template <typename T>
using xenable_to_string_t = xtop_enable_to_string<T>;

NS_END1
