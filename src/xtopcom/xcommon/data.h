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

inline void pad_left(xbytes_t & data, const uint32_t size) {
    data.insert(data.begin(), size - data.size(), 0);
}

inline xbytes_t data(const std::string & data) {
    return std::vector<xbyte_t>{data.begin(), data.end()};
}

inline xbytes_t data(const xbyte_t * data, size_t const size) {
    return std::vector<xbyte_t>{data, data + size};
}

inline void append(xbytes_t & data, const xbytes_t & suffix) {
    data.insert(data.end(), suffix.begin(), suffix.end());
}

inline void append(xbytes_t & data, const xbyte_t suffix) {
    data.push_back(suffix);
}

/// Return a part (subdata) of the requested size of the input data.
xbytes_t subData(const xbytes_t & data, size_t index, size_t length);

xbytes_t subData(const xbytes_t & data, size_t startIndex);
    /// Determines if a xbyte_t array has a specific prefix.
template <typename T>
inline bool has_prefix(const xbytes_t & data, T & prefix) {
    return std::equal(prefix.begin(), prefix.end(), data.begin(), data.begin() + std::min(data.size(), prefix.size()));
}

inline uint64_t to_uint64(const std::string & input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint64_t) - input.size(), 0);
    return *(uint64_t *)output.data();
}
inline uint32_t to_uint32(const std::string & input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint32_t) - input.size(), 0);
    return *(uint32_t *)output.data();
}

NS_END2
