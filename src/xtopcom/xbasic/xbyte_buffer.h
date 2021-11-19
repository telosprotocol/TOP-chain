// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xint.h"
#include "xbasic/xbyte.h"
#include "xbasic/xerror/xerror.h"

#include <string>
#include <system_error>
#include <vector>

NS_BEG1(top)

using xbyte_buffer_t = std::vector<xbyte_t>;
using xbytes_t = xbyte_buffer_t;

xbyte_buffer_t
random_bytes(std::size_t const size);

xbyte_buffer_t
random_base58_bytes(std::size_t const size);

template <typename T>
xbytes_t to_bytes(T const & input) {
    return input.to_bytes();
}

template <typename T>
T from_bytes(xbytes_t const & input, std::error_code & ec) {
    T ret;
    ret.from_bytes(input, ec);
    return ret;
}

template <typename T>
T from_bytes(xbytes_t const & input) {
    std::error_code ec;
    auto ret = top::from_bytes<T>(input, ec);
    top::error::throw_error(ec);
    return ret;
}

template <>
xbytes_t to_bytes<char>(char const & input);

template <>
xbytes_t to_bytes<int>(int const & input);

template <>
xbytes_t to_bytes<long>(long const & input);

template <>
xbytes_t to_bytes<long long>(long long const & input);

template <>
xbytes_t to_bytes<unsigned char>(unsigned char const & input);

template <>
xbytes_t to_bytes<unsigned int>(unsigned int const & input);

template <>
xbytes_t to_bytes<unsigned long>(unsigned long const & input);

template <>
xbytes_t to_bytes<unsigned long long>(unsigned long long const & input);

template <>
xbytes_t to_bytes<std::string>(std::string const & input);

template <>
xbytes_t to_bytes<xbytes_t>(xbytes_t const & input);

template <>
xbytes_t to_bytes<uint256_t>(uint256_t const & input);

template <>
xbytes_t from_bytes<xbytes_t>(xbytes_t const & input, std::error_code & ec);

template <>
std::string from_bytes<std::string>(xbytes_t const & input, std::error_code & ec);

template <>
uint256_t from_bytes<uint256_t>(xbytes_t const & input, std::error_code & ec);

template <>
char from_bytes<char>(xbytes_t const & input, std::error_code & ec);

template <>
int from_bytes<int>(xbytes_t const & input, std::error_code & ec);

template <>
long from_bytes<long>(xbytes_t const & input, std::error_code & ec);

template <>
long long from_bytes<long long>(xbytes_t const & input, std::error_code & ec);

template <>
unsigned char from_bytes<unsigned char>(xbytes_t const & input, std::error_code & ec);

template <>
unsigned int from_bytes<unsigned int>(xbytes_t const & input, std::error_code & ec);

template <>
unsigned long from_bytes<unsigned long>(xbytes_t const & input, std::error_code & ec);

template <>
unsigned long long from_bytes<unsigned long long>(xbytes_t const & input, std::error_code & ec);

NS_END1
