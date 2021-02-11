// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <string>

NS_BEG1(top)

template <typename T>
struct xtop_enable_to_string
{
    xtop_enable_to_string()                                          = default;
    xtop_enable_to_string(xtop_enable_to_string const &)             = default;
    xtop_enable_to_string & operator=(xtop_enable_to_string const &) = default;
    xtop_enable_to_string(xtop_enable_to_string &&)                  = default;
    xtop_enable_to_string & operator=(xtop_enable_to_string &&)      = default;
    virtual ~xtop_enable_to_string()                                 = default;

    virtual
    std::string
    to_string() const = 0;
};
template <typename T>
using xenable_to_string_t = xtop_enable_to_string<T>;

template <typename T>
std::string
to_string(xenable_to_string_t<T> const & v) {
    return v.to_string();
}

NS_END1
