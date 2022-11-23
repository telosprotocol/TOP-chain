// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xstring_utility.h"

NS_BEG1(top)

std::vector<xstring_view_t> split(xstring_view_t const input, char const delimiter) {
    std::vector<xstring_view_t> res;

    size_t pos = 0;
    while (pos != xstring_view_t::npos) {
        auto const pos1 = input.find_first_of(delimiter, pos);
        if (pos1 != xstring_view_t::npos) {
            res.push_back(input.substr(pos, pos1 - pos));
            pos = pos1 + 1;
        } else {
            res.push_back(input.substr(pos));
            pos = pos1;
        }
    }

    return res;
}

std::vector<xstring_view_t> split(std::string const & input, char const delimiter) {
    xstring_view_t const input_sv{input.data(), input.size()};
    return split(input_sv, delimiter);
}


NS_END1
