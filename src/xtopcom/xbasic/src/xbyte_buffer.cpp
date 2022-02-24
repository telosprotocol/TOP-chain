// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xbyte_buffer.h"

#include "xbasic/xerror/xerror.h"
#include "xbasic/xstring.h"

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

template <>
xbytes_t to_bytes<char>(char const & input) {
    return xbytes_t(1, static_cast<xbyte_t>(input));
}

template <>
xbytes_t to_bytes<int>(int const & input) {
    return to_bytes(top::to_string(input));
}

template <>
xbytes_t to_bytes<long>(long const & input) {
    return to_bytes(top::to_string(input));
}

template <>
xbytes_t to_bytes<long long>(long long const & input) {
    return to_bytes(top::to_string(input));
}

template <>
xbytes_t to_bytes<unsigned char>(unsigned char const & input) {
    return xbytes_t(1, input);
}

template <>
xbytes_t to_bytes<unsigned int>(unsigned int const & input) {
    return to_bytes(top::to_string(input));
}

template <>
xbytes_t to_bytes<unsigned long>(unsigned long const & input) {
    return to_bytes(top::to_string(input));
}

template <>
xbytes_t to_bytes<unsigned long long>(unsigned long long const & input) {
    return to_bytes(top::to_string(input));
}

template <>
xbytes_t to_bytes<std::string>(std::string const & input) {
    return {input.begin(), input.end()};
}

template <>
xbytes_t to_bytes<xbytes_t>(xbytes_t const & input) {
    return input;
}

template <>
xbytes_t to_bytes<uint256_t>(uint256_t const & input) {
    return xbytes_t{const_cast<xbyte_t *>(input.data()), const_cast<xbyte_t *>(input.data() + input.size())};
}

template <>
xbytes_t from_bytes<xbytes_t>(xbytes_t const & input, std::error_code & /*ec*/) {
    return input;
}

template <>
std::string from_bytes<std::string>(xbytes_t const & input, std::error_code & /*ec*/) {
    return {input.begin(), input.end()};
}

template <>
uint256_t from_bytes<uint256_t>(xbytes_t const & input, std::error_code & ec) {
    if (input.size() < uint256_t::enum_xint_size_bytes) {
        ec = error::xbasic_errc_t::deserialization_error;
        return uint256_t{};
    }

    return uint256_t{const_cast<xbyte_t *>(input.data())};
}

template <>
char from_bytes<char>(xbytes_t const & input, std::error_code & ec) {
    if (input.empty()) {
        ec = error::xbasic_errc_t::deserialization_error;
        return {};
    }
    return static_cast<char>(input.front());
}

template <>
int from_bytes<int>(xbytes_t const & input, std::error_code & ec) {
    auto const & string = from_bytes<std::string>(input, ec);
    if (ec) {
        return {};
    }

    return top::from_string<int>(string, ec);
}

template <>
long from_bytes<long>(xbytes_t const & input, std::error_code & ec) {
    auto const & string = from_bytes<std::string>(input, ec);
    if (ec) {
        return {};
    }
    return top::from_string<long>(string, ec);
}

template <>
long long from_bytes<long long>(xbytes_t const & input, std::error_code & ec) {
    auto const & string = from_bytes<std::string>(input, ec);
    if (ec) {
        return {};
    }

    return top::from_string<long long>(string, ec);
}

template <>
unsigned char from_bytes<unsigned char>(xbytes_t const & input, std::error_code & ec) {
    if (input.empty()) {
        ec = error::xbasic_errc_t::deserialization_error;
        return {};
    }
    return input.front();
}

template <>
unsigned int from_bytes<unsigned int>(xbytes_t const & input, std::error_code & ec) {
    auto const & string = from_bytes<std::string>(input, ec);
    if (ec) {
        return {};
    }
    return top::from_string<unsigned int>(string, ec);
}

template <>
unsigned long from_bytes<unsigned long>(xbytes_t const & input, std::error_code & ec) {
    auto const & string = from_bytes<std::string>(input, ec);
    if (ec) {
        return {};
    }
    return top::from_string<unsigned long>(string, ec);
}

template <>
unsigned long long from_bytes<unsigned long long>(xbytes_t const & input, std::error_code & ec) {
    auto const & string = from_bytes<std::string>(input, ec);
    if (ec) {
        return {};
    }

    return top::from_string<unsigned long long>(string, ec);
}

NS_END1
