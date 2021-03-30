// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xlog.h"
#include "xcodec/xdecorators/xprotobuf_decorator.hpp"

#include <cassert>

NS_BEG2(top, codec)

template <typename T>
struct xtop_protobuf_codec final
{
    xtop_protobuf_codec()                                        = delete;
    xtop_protobuf_codec(xtop_protobuf_codec const &)             = delete;
    xtop_protobuf_codec & operator=(xtop_protobuf_codec const &) = delete;
    xtop_protobuf_codec(xtop_protobuf_codec &&)                  = delete;
    xtop_protobuf_codec & operator=(xtop_protobuf_codec &&)      = delete;
    ~xtop_protobuf_codec()                                       = delete;

    template <typename ... ArgsT>
    static
    xbyte_buffer_t
    encode(T const & message, ArgsT && ... args) {
        return decorators::xprotobuf_decorator_t<T>::encode(message, std::forward<ArgsT>(args)...);
    }

    template <typename ... ArgsT>
    static
    auto
    decode(xbyte_buffer_t const & in, ArgsT && ... args) -> decltype(decorators::xprotobuf_decorator_t<T>::decode(in, std::forward<ArgsT>(args)...)) {
        return decorators::xprotobuf_decorator_t<T>::decode(in, std::forward<ArgsT>(args)...);
    }
};

template <typename T>
using xprotobuf_codec_t = xtop_protobuf_codec<T>;

template <typename T, typename ... ArgsT>
xbyte_buffer_t
protobuf_encode(T const & message, ArgsT && ... args) {
    try {
        return xprotobuf_codec_t<T>::encode(message, std::forward<ArgsT>(args)...);
    } catch (std::exception const & eh) {
        xwarn(u8"[protobuf encode] caught exception: %s", eh.what());
        assert(false);
        return {};
    } catch (...) {
        xwarn(u8"[protobuf encode] caught unknown exception");
        assert(false);
        return{};
    }
}

template <typename T, typename ... ArgsT>
T
protobuf_decode(xbyte_buffer_t const & bytes, ArgsT && ... args) {
    try {
        return xprotobuf_codec_t<T>::decode(bytes, std::forward<ArgsT>(args)...);
    } catch (std::exception const & eh) {
        xwarn(u8"[protobuf decode] caught exception: %s", eh.what());
        assert(false);
        return {};
    } catch (...) {
        xwarn(u8"[protobuf decode] caught unknown exception");
        assert(false);
        return {};
    }
}

NS_END2
