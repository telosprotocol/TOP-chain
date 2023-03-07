// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>


NS_BEG2(top, evm_common)

inline void pad_left(xbytes_t & data, uint32_t const size) {
    data.insert(data.begin(), size - data.size(), 0);
}

inline xbytes_t data(std::string const & data) {
    return std::vector<xbyte_t>{data.begin(), data.end()};
}

inline xbytes_t data(xbyte_t const * data, size_t const size) {
    return std::vector<xbyte_t>{data, data + size};
}

inline void append(xbytes_t & data, xbytes_t const & suffix) {
    data.insert(data.end(), suffix.begin(), suffix.end());
}

inline void append(xbytes_t & data, xbyte_t const suffix) {
    data.push_back(suffix);
}

/// Return a part (subdata) of the requested size of the input data.
xbytes_t sub_data(xbytes_t const & data, size_t index, size_t length);
xbytes_t sub_data(xbytes_t const & data, size_t start_index);

/// Determines if a xbyte_t array has a specific prefix.
template <typename T>
inline bool has_prefix(xbytes_t const & data, T & prefix) {
    return std::equal(prefix.begin(), prefix.end(), data.begin(), data.begin() + std::min(data.size(), prefix.size()));
}

inline uint64_t to_uint64(std::string const & input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint64_t) - input.size(), 0);
    return *reinterpret_cast<uint64_t const *>(output.data());
}
inline uint32_t to_uint32(std::string const & input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint32_t) - input.size(), 0);
    return *reinterpret_cast<uint32_t const *>(output.data());
}

NS_END2
