// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xcodec_category.h"
#include "xcodec/xcodec_errc.h"

NS_BEG2(top, codec)

std::error_code
make_error_code(xcodec_errc_t const errc) {
    return std::error_code{ static_cast<int>(errc), codec_cagegory() };
}

std::error_condition
make_error_condition(xcodec_errc_t const errc) {
    return std::error_condition{ static_cast<int>(errc), codec_cagegory() };
}

NS_END2
