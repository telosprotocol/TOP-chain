// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xhex.h"

#include "xbasic/xerror/xerror.h"

#include <algorithm>
#include <cassert>
#include <cstring>

NS_BEG1(top)

bool has_hex_prefix(xstring_view_t const input) noexcept {
    return input.size() >= 2 && (input.compare(0, 2, "0x") == 0 || input.compare(0, 2, "0X") == 0);
}

xbyte_t hex_char_to_binary(char const ch, std::error_code & ec) noexcept {
    assert(!ec);

    if (ch >= '0' && ch <= '9') {
        return static_cast<xbyte_t>(ch - '0');
    }

    if (ch >= 'a' && ch <= 'f') {
        return static_cast<xbyte_t>(ch - 'a' + 10);
    }

    if (ch >= 'A' && ch <= 'F') {
        return static_cast<xbyte_t>(ch - 'A' + 10);
    }

    ec = error::xbasic_errc_t::invalid_char_data;
    assert(false);
    return ch;
}

xbytes_t from_hex(xstring_view_t const input, std::error_code & ec) {
    assert(!ec);

    std::size_t prefix_size = has_hex_prefix(input) ? 2 : 0;
    std::vector<uint8_t> ret;
    ret.reserve((input.size() - prefix_size + 1) / 2);

    if (input.size() % 2) {
        auto const h = hex_char_to_binary(input[prefix_size++], ec);
        if (!ec) {
            ret.push_back(h);
        } else {
            return xbytes_t{};
        }
    }
    for (std::size_t i = prefix_size; i < input.size(); i += 2) {
        auto const h = hex_char_to_binary(input[i], ec);
        if (ec) {
            return xbytes_t{};
        }

        int const l = hex_char_to_binary(input[i + 1], ec);
        if (ec) {
            return xbytes_t{};
        }

        ret.push_back(static_cast<xbyte_t>(h << 4 | l));
    }
    return ret;
}

xbytes_t from_hex(xstring_view_t const input) {
    std::error_code ec;
    auto ret = from_hex(input, ec);
    top::error::throw_error(ec);
    return ret;
}

bool is_hex_string(std::string const & input) noexcept {
    return is_hex_string_with_prefix(input) || is_hex_string_without_prefix(input);
}

bool is_hex_string_with_prefix(xstring_view_t const input) noexcept {
    return has_hex_prefix(input) && is_hex_string_without_prefix(input.substr(0, 2));
}

bool is_hex_string_without_prefix(xstring_view_t const input) noexcept {
    return std::all_of(input.begin(), input.end(), [](unsigned char const c) { return std::isxdigit(c); });
}

NS_END1
