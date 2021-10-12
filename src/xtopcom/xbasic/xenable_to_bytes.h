// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

#include <system_error>

NS_BEG1(top)

template <typename T>
class xtop_enable_to_bytes {
public:
    xtop_enable_to_bytes() = default;
    xtop_enable_to_bytes(xtop_enable_to_bytes const &) = default;
    xtop_enable_to_bytes & operator=(xtop_enable_to_bytes const &) = default;
    xtop_enable_to_bytes(xtop_enable_to_bytes &&) = default;
    xtop_enable_to_bytes & operator=(xtop_enable_to_bytes &&) = default;
    virtual ~xtop_enable_to_bytes() = default;

    virtual xbytes_t to_bytes() const = 0;
    virtual void from_bytes(xbytes_t const & bytes, std::error_code & ec) = 0;
};
template <typename T>
using xenable_to_bytes_t = xtop_enable_to_bytes<T>;

NS_END1
