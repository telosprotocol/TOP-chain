// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>
#include <limits>
#include <iosfwd>

#include "xcodec/xbuffer_codec.hpp"
#include "xcodec/xcodec_error.h"
#include "xcodec/xdecorators/xprepend_size_decorator.hpp"
#include "xnetwork/xcodec/xdecorators/xnetwork_bytes_message_decorator.h"
#include "xzlib/xdecorators/xcrc32_decorator.h"
#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xcodec/xcodec_errc.h"

NS_BEG4(top, network, codec, decorators)

static constexpr std::uint8_t header_length{ 32 };
static constexpr std::size_t  crc_length{ sizeof(std::uint32_t) };

xbyte_buffer_t
xtop_network_bytes_message_decorator::encode(xbyte_buffer_t const & in) {
    using namespace top::codec;
    using namespace top::codec::decorators;
    using namespace zlib::decorators;

    if (in.size() > std::numeric_limits<std::uint16_t>::max() - header_length - crc_length) {
        throw xcodec_error_t{ xcodec_errc_t::encode_input_buffer_too_long, __LINE__, __FILE__ };
    }

    auto const result = xbuffer_codec_t<xprepend_size_decorator_t<std::uint16_t>>::encode(in, crc_length);

    return xbuffer_codec_t<xcrc32_decorator_t>::encode(result);
}

xbyte_buffer_t
xtop_network_bytes_message_decorator::decode(xbyte_buffer_t const & in) {
    using namespace top::codec;
    using namespace top::codec::decorators;
    using namespace zlib::decorators;

    if (in.size() < header_length + crc_length) {
        throw xcodec_error_t{ xcodec_errc_t::decode_input_buffer_not_enough, __LINE__, __FILE__ };
    }

    auto result = xbuffer_codec_t<xcrc32_decorator_t>::decode(in);
    return xbuffer_codec_t<xprepend_size_decorator_t<std::uint16_t>>::decode(in, crc_length);
}

NS_END4
