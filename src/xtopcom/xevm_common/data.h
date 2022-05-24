// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <algorithm>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace top {
namespace evm_common {

using byte = std::uint8_t;
using bytes = std::vector<byte>;

inline void pad_left(bytes & data, const uint32_t size) {
    data.insert(data.begin(), size - data.size(), 0);
}

inline bytes data(const std::string & data) {
    return std::vector<byte>(data.begin(), data.end());
}

inline bytes data(const byte * data, size_t size) {
    return std::vector<byte>(data, data + size);
}

inline void append(bytes & data, const bytes & suffix) {
    data.insert(data.end(), suffix.begin(), suffix.end());
}

inline void append(bytes & data, const byte suffix) {
    data.push_back(suffix);
}

/// Return a part (subdata) of the requested size of the input data.
bytes subData(const bytes & data, size_t index, size_t length);

bytes subData(const bytes & data, size_t startIndex);
    /// Determines if a byte array has a specific prefix.
template <typename T>
inline bool has_prefix(const bytes & data, T & prefix) {
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

}  // namespace evm_common
}  // namespace top