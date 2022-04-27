// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte.h"

#include <cstdint>

NS_BEG2(top, codec)

enum class xtop_codec_type : uint8_t {
    invalid = 0x00,
    msgpack = 0x01,
    xstream = 0x02,
    protobuf = 0x03,
    string = 0x04,
    bytes = 0x05,
};
using xcodec_type_t = xtop_codec_type;

NS_END2

NS_BEG1(top)

template <>
xbyte_t to_byte<codec::xcodec_type_t>(codec::xcodec_type_t const & codec_type);

template <>
codec::xcodec_type_t from_byte<codec::xcodec_type_t>(xbyte_t byte);

NS_END1
