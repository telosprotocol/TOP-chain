// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xbinary.h"

#include <algorithm>

NS_BEG1(top)

bool has_binary_prefix(xstring_view_t const input) noexcept {
    return input.size() >= 2 && (input.compare(0, 2, binary_prefix_cstr) == 0 || input.compare(0, 2, binary_prefix_uppercase_cstr) == 0);
}

bool is_binary_string(xstring_view_t const str) noexcept {
    return is_binary_string_with_prefix(str) || is_binary_string_without_prefix(str);
}

bool is_binary_string_with_prefix(xstring_view_t str) noexcept {
    if (!has_binary_prefix(str)) {
        return false;
    }

    return is_binary_string_without_prefix(str.substr(2));
}

bool is_binary_string_without_prefix(xstring_view_t str) noexcept {
    return std::all_of(str.begin(), str.end(), [](char const c) { return c == '0' || c == '1'; });
}

NS_END1
