// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xhex.h"

#include "xbasic/xerror/xerror.h"

#include <algorithm>

NS_BEG1(top)

namespace {
int from_hex_char(char _i) noexcept {
    if (_i >= '0' && _i <= '9')
        return _i - '0';
    if (_i >= 'a' && _i <= 'f')
        return _i - 'a' + 10;
    if (_i >= 'A' && _i <= 'F')
        return _i - 'A' + 10;
    return -1;
}
}  // namespace

xbytes_t from_hex(std::string const & input, std::error_code & ec) {
    std::size_t prefix_size = (input.size() >= 2 && input[0] == '0' && input[1] == 'x') ? 2 : 0;
    std::vector<uint8_t> ret;
    ret.reserve((input.size() - prefix_size + 1) / 2);

    if (input.size() % 2) {
        int h = from_hex_char(input[prefix_size++]);
        if (h != -1) {
            ret.push_back(h);
        } else {
            ec = error::xbasic_errc_t::invalid_char_data;
            return xbytes_t{};
        }
    }
    for (std::size_t i = prefix_size; i < input.size(); i += 2) {
        int h = from_hex_char(input[i]);
        int l = from_hex_char(input[i + 1]);
        if (h != -1 && l != -1) {
            ret.push_back((xbyte_t)(h * 16 + l));
        } else {
            ec = error::xbasic_errc_t::invalid_char_data;
            return xbytes_t{};
        }
    }
    return ret;
}

xbytes_t from_hex(std::string const & input) {
    std::error_code ec;
    auto ret = from_hex(input, ec);
    top::error::throw_error(ec);
    return ret;
}

bool is_hex_string(std::string const & input) noexcept {
    auto it = input.begin();
    if (input.compare(0, 2, "0x") == 0)
        it += 2;
    return std::all_of(it, input.end(), [](char c) { return from_hex_char(c) != -1; });
}
NS_END1