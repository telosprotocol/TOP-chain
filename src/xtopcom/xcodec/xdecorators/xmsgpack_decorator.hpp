// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xcodec/xcodec_error.h"

#include <msgpack.hpp>

NS_BEG3(top, codec, decorators)

template <typename T>
struct xtop_msgpack_decorator final
{
    xtop_msgpack_decorator()                                           = delete;
    xtop_msgpack_decorator(xtop_msgpack_decorator const &)             = delete;
    xtop_msgpack_decorator & operator=(xtop_msgpack_decorator const &) = delete;
    xtop_msgpack_decorator(xtop_msgpack_decorator &&)                  = delete;
    xtop_msgpack_decorator & operator=(xtop_msgpack_decorator &&)      = delete;
    ~xtop_msgpack_decorator()                                          = delete;

    using message_type = T;

    static
    xbyte_buffer_t
    encode(message_type const & message) {
        try {
            ::msgpack::sbuffer buffer;
            ::msgpack::pack(buffer, message);

            return { buffer.data(), buffer.data() + buffer.size() };
        } catch (...) {
            throw top::codec::xcodec_error_t{ top::codec::xcodec_errc_t::encode_error, __LINE__, __FILE__ };
        }
    }

    static
    message_type
    decode(xbyte_buffer_t const & in) {
        try {
            auto object_handle = msgpack::unpack(reinterpret_cast<char const *>(in.data()), in.size(), nullptr);
            auto object = object_handle.get();
            return object.as<T>();
        } catch (...) {
            throw top::codec::xcodec_error_t{ top::codec::xcodec_errc_t::decode_error, __LINE__, __FILE__ };
        }
    }
};

template <typename T>
using xmsgpack_decorator_t = xtop_msgpack_decorator<T>;

NS_END3
