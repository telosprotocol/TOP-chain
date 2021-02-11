#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xutility.h"
#include "xcodec/xcodec_errc.h"
#include "xcodec/xcodec_error.h"

#include <google/protobuf/message.h>

#include <cstring>
#include <memory>
#include <type_traits>

NS_BEG3(top, codec, decorators)

template <typename T>
struct xtop_protobuf_decorator final
{
    XSTATIC_ASSERT((std::is_base_of<google::protobuf::Message, T>::value));

    xtop_protobuf_decorator(xtop_protobuf_decorator const &)             = delete;
    xtop_protobuf_decorator & operator=(xtop_protobuf_decorator const &) = delete;
    xtop_protobuf_decorator(xtop_protobuf_decorator &&)                  = delete;
    xtop_protobuf_decorator & operator=(xtop_protobuf_decorator &&)      = delete;
    ~xtop_protobuf_decorator()                                           = delete;

    static
    xbyte_buffer_t
    encode(T const & message) {
        auto const length = message.ByteSize();
        auto buffer = xbyte_buffer_t(length);

        auto const successful = message.SerializeToArray(buffer.data(), static_cast<int>(buffer.size()));
        if (!successful) {
            XTHROW(xcodec_error_t,
                   xcodec_errc_t::encode_error,
                   std::string{});
        }

        return buffer;
    }

    static
    T
    decode(xbyte_buffer_t const & in) {
        T message;
        if (!message.ParseFromArray(in.data(), static_cast<int>(in.size()))) {
            XTHROW(xcodec_error_t,
                   xcodec_errc_t::decode_error,
                   std::string{});
        }
        return T();
    }
};

template <typename T>
using xprotobuf_decorator_t = xtop_protobuf_decorator<T>;

NS_END3
