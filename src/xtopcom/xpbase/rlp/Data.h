// Copyright © 2017-2020 Trust Wallet.
//
// This file is part of Trust. The full Trust copyright notice, including
// terms governing use, modification, and redistribution, is contained in the
// file LICENSE at the root of the source code distribution tree.

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <array>
#include <algorithm> 

using byte = std::uint8_t;
using Data = std::vector<byte>;

inline void pad_left(Data& data, const uint32_t size) {
    data.insert(data.begin(), size - data.size(), 0);
}

inline Data data(const std::string& data) {
    return std::vector<byte>(data.begin(), data.end());
}

inline Data data(const byte* data, size_t size) {
    return std::vector<byte>(data, data + size);
}

inline void append(Data& data, const Data& suffix) {
    data.insert(data.end(), suffix.begin(), suffix.end());
}

inline void append(Data& data, const byte suffix) {
    data.push_back(suffix);
}

/// Return a part (subdata) of the requested size of the input data.
Data subData(const Data& data, size_t index, size_t length);

/// Determines if a byte array has a specific prefix.
template <typename T>
inline bool has_prefix(const Data& data, T& prefix) {
    return std::equal(prefix.begin(), prefix.end(), data.begin(), data.begin() + std::min(data.size(), prefix.size()));
}

inline uint64_t to_uint64(const std::string& input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint64_t)-input.size(), 0);
    return *(uint64_t*)output.data();
}
inline uint32_t to_uint32(const std::string& input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint32_t)-input.size(), 0);
    return *(uint32_t*)output.data();
}
inline uint16_t to_uint16(const std::string& input) {
    std::string output(input);
    std::reverse(output.begin(), output.end());
    output += std::string(sizeof(uint16_t)-input.size(), 0);
    return *(uint16_t*)output.data();
}
inline std::string data_to_string(const Data& data) {
    return std::string(data.begin(), data.end());
}