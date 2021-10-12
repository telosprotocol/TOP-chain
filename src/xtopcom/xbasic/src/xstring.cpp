// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xstring.h"

NS_BEG1(top)

template <>
std::string to_string<int>(int const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<long>(long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<long long>(long long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<unsigned int>(unsigned int const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<unsigned long>(unsigned long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<unsigned long long>(unsigned long long const & input) {
    return std::to_string(input);
}

template <>
std::string to_string<std::string>(std::string const & input) {
    return input;
}

template <>
std::string to_string<xbytes_t>(xbytes_t const & input) {
    return {input.begin(), input.end()};
}

template <>
int from_string<int>(std::string const & input) {
    return std::stoi(input);
}

template <>
long from_string<long>(std::string const & input) {
    return std::stol(input);
}

template <>
long long from_string<long long>(std::string const & input) {
    return std::stoll(input);
}

template <>
unsigned long from_string<unsigned long>(std::string const & input) {
    return std::stoul(input);
}

template <>
unsigned long long from_string<unsigned long long>(std::string const & input) {
    return std::stoull(input);
}

template <>
float from_string<float>(std::string const & input) {
    return std::stof(input);
}

template <>
double from_string<double>(std::string const & input) {
    return std::stod(input);
}

template <>
long double from_string<long double>(std::string const & input) {
    return std::stold(input);
}

template <>
xbytes_t from_string<xbytes_t>(std::string const & input) {
    return {input.begin(), input.end()};
}

NS_END1
