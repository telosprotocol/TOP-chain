// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <system_error>

NS_BEG2(top, codec)

enum class xenum_codec_errc
{
    decode_error                    = 0x00010000,
    decode_prepend_size_error       = 0x00010001,
    decode_input_buffer_not_enough  = 0x00010002,
    decode_input_buffer_too_long    = 0x00010003,
    decode_wrong_checksum           = 0x00010004,
    decode_wrong_header             = 0x00010005,
    encode_error                    = 0x00020000,
    encode_input_buffer_too_long    = 0x00020001,
};

using xcodec_errc_t = xenum_codec_errc;

std::error_code
make_error_code(xcodec_errc_t const errc);

std::error_condition
make_error_condition(xcodec_errc_t const errc);


NS_END2

NS_BEG1(std)

template <>
struct is_error_code_enum<top::codec::xcodec_errc_t> : std::true_type
{
};

NS_END1
