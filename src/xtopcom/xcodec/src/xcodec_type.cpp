// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xcodec_type.h"

#include <type_traits>

NS_BEG1(top)

template <>
xbyte_t to_byte<codec::xcodec_type_t>(codec::xcodec_type_t const & codec_type) {
    return static_cast<xbyte_t>(static_cast<std::underlying_type<codec::xcodec_type_t>::type>(codec_type));
}

template <>
codec::xcodec_type_t from_byte<codec::xcodec_type_t>(xbyte_t byte) {
    return static_cast<codec::xcodec_type_t>(byte);
}

NS_END1
