// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

#include <exception>

NS_BEG2(top, codec)

template <typename T>
struct xtop_bytes_codec final {
    xtop_bytes_codec() = delete;
    xtop_bytes_codec(xtop_bytes_codec const &) = delete;
    xtop_bytes_codec & operator=(xtop_bytes_codec const &) = delete;
    xtop_bytes_codec(xtop_bytes_codec &&) = delete;
    xtop_bytes_codec & operator=(xtop_bytes_codec &&) = delete;
    ~xtop_bytes_codec() = delete;

    template <typename... ArgsT>
    static xbytes_t encode(T const & input, ArgsT&&... args) {
        return top::to_bytes<T>(input, std::forward<ArgsT>(args)...);
    }

    template <typename... ArgsT>
    static T decode(xbytes_t const & bytes, std::error_code & ec, ArgsT && ... args) {
        return top::from_bytes<T>(bytes, ec, std::forward<ArgsT>(args)...);
    }
};

template <typename T>
using xbytes_codec_t = xtop_bytes_codec<T>;

NS_END2
