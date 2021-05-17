// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <system_error>

#include "xcodec/xcodec_errc.h"
#include "xcodec/xcodec_category.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, codec)

static
char const *
xcodec_errc_map(int const errc) noexcept {
    auto const ec = static_cast<xcodec_errc_t>(errc);
    switch (ec) {
        case xcodec_errc_t::decode_error:
            return u8"codec decode error";

        case xcodec_errc_t::decode_prepend_size_error:
            return u8"decode prepend size error";

        case xcodec_errc_t::decode_input_buffer_not_enough:
            return u8"decode input buffer not enough";

        case xcodec_errc_t::decode_input_buffer_too_long:
            return u8"decode input buffer too long";

        case xcodec_errc_t::decode_wrong_checksum:
            return u8"decode wrong checksum";

        case xcodec_errc_t::decode_wrong_header:
            return u8"decode wrong header";

        case xcodec_errc_t::encode_error:
            return u8"encode error";

        case xcodec_errc_t::encode_input_buffer_too_long:
            return u8"encode input buffer too long";

        default:
            return u8"codec unknown error";
    }
}

class xtop_codec_category final : public std::error_category
{
    char const *
    name() const noexcept override {
        return u8"top::codec";
    }

    std::string
    message(int errc) const override {
        return xcodec_errc_map(errc);
    }
};

using xcodec_category_t = xtop_codec_category;

std::error_category const &
codec_cagegory() {
    static xcodec_category_t category{};
    return category;
}

NS_END2
