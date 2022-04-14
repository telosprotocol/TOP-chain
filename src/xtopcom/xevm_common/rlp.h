// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xevm_common/data.h"
#include "xevm_common/common.h"
#include "xevm_common/common_data.h"

#include <cstdint>
#include <string>
#include <vector>

namespace top {
namespace evm_common {
namespace rlp {
/// Implementation of Ethereum's RLP encoding.
///
/// - SeeAlso: https://github.com/ethereum/wiki/wiki/RLP
struct RLP {
    /// Encodes a string;
    static bytes encode(const std::string & string) noexcept {
        return encode(bytes(string.begin(), string.end()));
    }

    static bytes encode(uint8_t number) noexcept {
        return encode(uint64_t(number));
    }

    static bytes encode(uint16_t number) noexcept {
        return encode(uint64_t(number));
    }

    static bytes encode(int32_t number) noexcept {
        if (number < 0) {
            return {};  // RLP cannot encode negative numbers
        }
        return encode(static_cast<uint32_t>(number));
    }

    static bytes encode(uint32_t number) noexcept {
        return encode(uint64_t(number));
    }

    static bytes encode(int64_t number) noexcept {
        if (number < 0) {
            return {};  // RLP cannot encode negative numbers
        }
        return encode(static_cast<uint64_t>(number));
    }
    static bytes encode(uint64_t number) noexcept {
        bytes data = top::evm_common::toCompactBigEndian(number);
        u256 udata = top::evm_common::fromBigEndian<u256>(data);
        return encode(udata);
    }

    static bytes encode(const u256 & number) noexcept;

    /// Wraps encoded data as a list.
    static bytes encodeList(const bytes & encoded) noexcept;

    /// Encodes a block of data.
    static bytes encode(const bytes & data) noexcept;

    /// Encodes a static array.
    template <std::size_t N>
    static bytes encode(const std::array<uint8_t, N> & data) noexcept {
        if (N == 1 && data[0] <= 0x7f) {
            // Fits in single byte, no header
            return bytes(data.begin(), data.end());
        }

        auto encoded = encodeHeader(data.size(), 0x80, 0xb7);
        encoded.insert(encoded.end(), data.begin(), data.end());
        return encoded;
    }

    /// Encodes a list of elements.
    template <typename T>
    static bytes encodeList(T elements) noexcept {
        auto encodedData = bytes();
        for (const auto & el : elements) {
            auto encoded = encode(el);
            if (encoded.empty()) {
                return {};
            }
            encodedData.insert(encodedData.end(), encoded.begin(), encoded.end());
        }

        auto encoded = encodeHeader(encodedData.size(), 0xc0, 0xf7);
        encoded.insert(encoded.end(), encodedData.begin(), encodedData.end());
        return encoded;
    }

    /// Encodes a list header.
    static bytes encodeHeader(uint64_t size, uint8_t smallTag, uint8_t largeTag) noexcept;

    /// Returns the representation of an integer using the least number of bytes
    /// needed.
    static bytes putVarInt(uint64_t i) noexcept;
    static uint64_t parseVarInt(size_t size, const bytes & data, size_t index);

    struct DecodedItem {
        std::vector<bytes> decoded;
        bytes remainder;
    };

    static DecodedItem decodeList(const bytes & input);
    static uint64_t decodeLength(const bytes & data);
    /// Decodes data, remainder from RLP encoded data
    static DecodedItem decode(const bytes & data);
};
}  // namespace rlp
}  // namespace evm_common
}  // namespace top
