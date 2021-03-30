// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xbyte_buffer.h"

#include <array>
#include <limits>
#include <random>
#include <algorithm>

NS_BEG1(top)

xbyte_buffer_t
random_bytes(std::size_t const size) {
    std::uniform_int_distribution<xbyte_t> distribution
    {
        std::numeric_limits<xbyte_t>::min(),
        std::numeric_limits<xbyte_t>::max()
    };

    std::random_device rd{};

    xbyte_buffer_t ret(size);
    for (auto & byte : ret) {
        byte = static_cast<xbyte_t>(distribution(rd));
    }

    return ret;
}

static xbyte_buffer_t base58
{
    '1', '2', '3', '4', '5', '6', '7', '8', '9',

    'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i',
    'j', 'k', 'm', 'n', 'o', 'p', 'q', 'r', 's',
    't', 'u', 'v', 'w', 'x', 'y', 'z',

    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J',
    'K', 'L', 'M', 'N', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z'
};

xbyte_buffer_t
random_base58_bytes(std::size_t const size) {
    std::random_device rd;
    std::mt19937_64 g(rd());

    std::shuffle(std::begin(base58), std::end(base58), g);

    std::uniform_int_distribution<std::size_t> distribution
    {
        0,
        base58.size() - 1
    };

    xbyte_buffer_t ret(size);
    for (auto & byte : ret) {
        byte = base58[distribution(rd)];
    }

    return ret;
}


NS_END1
