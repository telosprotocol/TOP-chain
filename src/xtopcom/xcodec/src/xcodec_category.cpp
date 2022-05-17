// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcodec/xcodec_errc.h"
#include "xcodec/xcodec_category.h"
#include <string>
#include <system_error>

NS_BEG2(top, codec)

static
char const *
xcodec_errc_map(int const errc) noexcept {
    auto const ec = static_cast<xcodec_errc_t>(errc);
    switch (ec) {
        case xcodec_errc_t::decode_error:
            return "codec decode error";

        case xcodec_errc_t::decode_prepend_size_error:
            return "decode prepend size error";

        case xcodec_errc_t::decode_input_buffer_not_enough:
            return "decode input buffer not enough";

        case xcodec_errc_t::decode_input_buffer_too_long:
            return "decode input buffer too long";

        case xcodec_errc_t::decode_wrong_checksum:
            return "decode wrong checksum";

        case xcodec_errc_t::decode_wrong_header:
            return "decode wrong header";

        case xcodec_errc_t::decode_missing_codec_type:
            return "decode missing codec type";

        case xcodec_errc_t::decode_wrong_codec_type:
            return "decode wrong codec type";

        case xcodec_errc_t::decode_input_empty:
            return "decode input empty";

        case xcodec_errc_t::encode_error:
            return "encode error";

        case xcodec_errc_t::encode_input_buffer_too_long:
            return "encode input buffer too long";

        default:
            return "codec unknown error";
    }
}

class xtop_codec_category final : public std::error_category
{
    char const *
    name() const noexcept override {
        return "codec";
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
