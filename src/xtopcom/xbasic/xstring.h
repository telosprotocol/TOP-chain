// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xerror/xerror.h"
#include "xbasic/xbyte_buffer.h"

#include <string>
#include <system_error>

NS_BEG1(top)

template <typename T>
std::string to_string(T const & input) {
    return input.to_string();
}

template <typename T>
T from_string(std::string const & input, std::error_code & ec) {
    T ret;
    ret.from_string(input, ec);
    return ret;
}

template <typename T>
T from_string(std::string const & input) {
    std::error_code ec;
    auto ret = top::from_string<T>(input, ec);
    top::error::throw_error(ec);
    return ret;
}

template <>
std::string to_string(short const & input);

template <>
std::string to_string<int>(int const & input);

template <>
std::string to_string<long>(long const & input);

template <>
std::string to_string<long long>(long long const & input);

template <>
std::string to_string<unsigned int>(unsigned int const & input);

template <>
std::string to_string<unsigned long>(unsigned long const & input);

template <>
std::string to_string<unsigned long long>(unsigned long long const & input);

template <>
std::string to_string<std::string>(std::string const & input);

template <>
std::string to_string<xbytes_t>(xbytes_t const & input);

template <>
short int from_string<short int>(std::string const & input, std::error_code & ec);

template <>
int from_string<int>(std::string const & input, std::error_code & ec);

template <>
long from_string<long>(std::string const & input, std::error_code & ec);

template <>
long long from_string<long long>(std::string const & input, std::error_code & ec);

template <>
unsigned short int from_string<unsigned short int>(std::string const & input, std::error_code & ec);

template <>
unsigned int from_string<unsigned int>(std::string const & input, std::error_code & ec);

template <>
unsigned long from_string<unsigned long>(std::string const & input, std::error_code & ec);

template <>
unsigned long long from_string<unsigned long long>(std::string const & input, std::error_code & ec);

template<>
xbytes_t from_string<xbytes_t>(std::string const & input, std::error_code & ec);

NS_END1
