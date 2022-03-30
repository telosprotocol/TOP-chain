// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/rlp.h"

#include "xevm_common/binary_coding.h"
#include "xevm_common/data.h"

#include <algorithm>
#include <tuple>

namespace top {
namespace evm_common {
namespace rlp {
bytes RLP::encode(const uint64_t & value) noexcept {
    bytes bytes = data((const byte *)&value, sizeof(value));
    while (!bytes.empty()) {
        if (bytes.back() == 0)
            bytes.pop_back();
        else
            break;
    }
    std::reverse(bytes.begin(), bytes.end());

    if (bytes.empty() || (bytes.size() == 1 && bytes[0] == 0)) {
        return {0x80};
    }

    return encode(bytes);
}

bytes RLP::encodeList(const bytes & encoded) noexcept {
    auto result = encodeHeader(encoded.size(), 0xc0, 0xf7);
    result.reserve(result.size() + encoded.size());
    result.insert(result.end(), encoded.begin(), encoded.end());
    return result;
}

bytes RLP::encode(const bytes & data) noexcept {
    if (data.size() == 1 && data[0] <= 0x7f) {
        // Fits in single byte, no header
        return data;
    }

    auto encoded = encodeHeader(data.size(), 0x80, 0xb7);
    encoded.insert(encoded.end(), data.begin(), data.end());
    return encoded;
}

bytes RLP::encodeHeader(uint64_t size, uint8_t smallTag, uint8_t largeTag) noexcept {
    if (size < 56) {
        return {static_cast<uint8_t>(smallTag + size)};
    }

    const auto sizeData = putint(size);

    auto header = bytes();
    header.reserve(1 + sizeData.size());
    header.push_back(largeTag + static_cast<uint8_t>(sizeData.size()));
    header.insert(header.end(), sizeData.begin(), sizeData.end());
    return header;
}

bytes RLP::putint(uint64_t i) noexcept {
    // clang-format off
    if (i < (1ULL << 8))
        return {static_cast<uint8_t>(i)};
    if (i < (1ULL << 16))
        return {
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i),
        };
    if (i < (1ULL << 24))
        return {
            static_cast<uint8_t>(i >> 16),
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i),
        };
    if (i < (1ULL << 32))
        return {
            static_cast<uint8_t>(i >> 24),
            static_cast<uint8_t>(i >> 16),
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i),
        };
    if (i < (1ULL << 40))
        return {
            static_cast<uint8_t>(i >> 32),
            static_cast<uint8_t>(i >> 24),
            static_cast<uint8_t>(i >> 16),
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i),
        };
    if (i < (1ULL << 48))
        return {
            static_cast<uint8_t>(i >> 40),
            static_cast<uint8_t>(i >> 32),
            static_cast<uint8_t>(i >> 24),
            static_cast<uint8_t>(i >> 16),
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i),
        };
    if (i < (1ULL << 56))
        return {
            static_cast<uint8_t>(i >> 48),
            static_cast<uint8_t>(i >> 40),
            static_cast<uint8_t>(i >> 32),
            static_cast<uint8_t>(i >> 24),
            static_cast<uint8_t>(i >> 16),
            static_cast<uint8_t>(i >> 8),
            static_cast<uint8_t>(i),
        };

    return {
        static_cast<uint8_t>(i >> 56),
        static_cast<uint8_t>(i >> 48),
        static_cast<uint8_t>(i >> 40),
        static_cast<uint8_t>(i >> 32),
        static_cast<uint8_t>(i >> 24),
        static_cast<uint8_t>(i >> 16),
        static_cast<uint8_t>(i >> 8),
        static_cast<uint8_t>(i),
    };
    // clang-format on
}

RLP::DecodedItem RLP::decodeList(const bytes & input) {
    RLP::DecodedItem item;
    auto remainder = input;
    while (true) {
        auto listItem = RLP::decode(remainder);
        // ??? bug 
        // 1. struct as list[list[], list[]] will lost rest of member
        // 2. if null will throw at(0)
        // item.decoded.push_back(listItem.decoded[0]);

        for (auto & data : listItem.decoded) {
            item.decoded.push_back(data);
        }

        if (listItem.remainder.size() == 0) {
            break;
        } else {
            remainder = listItem.remainder;
        }
    }
    return item;
}

uint64_t RLP::decodeLength(const bytes & data) {
    size_t index = 0;
    auto decodedLen = decodeVarInt(data, index);
    if (!std::get<0>(decodedLen)) {
        throw std::invalid_argument("can't decode length of string/list length");
    }
    return std::get<1>(decodedLen);
}

RLP::DecodedItem RLP::decode(const bytes & input) {
    if (input.size() == 0) {
        throw std::invalid_argument("can't decode empty rlp data");
    }
    RLP::DecodedItem item;
    uint32_t inputLen = input.size();
    auto prefix = input[0];
    if (prefix <= 0x7f) {
        // a single byte whose value is in the [0x00, 0x7f] range, that byte is its own RLP encoding.
        item.decoded.push_back(bytes{input[0]});
        item.remainder = bytes(input.begin() + 1, input.end());
        return item;
    }
    if (prefix <= 0xb7) {
        // short string
        // string is 0-55 bytes long. A single byte with value 0x80 plus the length of the string followed by the string
        // The range of the first byte is [0x80, 0xb7]

        // empty string
        if (prefix == 0x80) {
            item.decoded.push_back(bytes());
            item.remainder = bytes(input.begin() + 1, input.end());
            return item;
        }

        auto strLen = prefix - 0x80;
        if (strLen == 1 && input[1] <= 0x7f) {
            throw std::invalid_argument("single byte below 128 must be encoded as itself");
        }

        item.decoded.push_back(subData(input, 1, strLen));
        item.remainder = bytes(input.begin() + 1 + strLen, input.end());

        return item;
    }
    if (prefix <= 0xbf) {
        // long string
        uint32_t lenOfStrLen = prefix - 0xb7;
        uint32_t strLen = static_cast<size_t>(decodeLength(subData(input, 1, lenOfStrLen)));
        if (inputLen < lenOfStrLen || inputLen < lenOfStrLen + strLen) {
            throw std::invalid_argument("Invalid rlp encoding length");
        }
        auto data = subData(input, 1 + lenOfStrLen, strLen);
        item.decoded.push_back(data);
        item.remainder = bytes(input.begin() + 1 + lenOfStrLen + strLen, input.end());
        return item;
    }
    if (prefix <= 0xf7) {
        // a list between  0-55 bytes long
        uint32_t listLen = prefix - 0xc0;
        if (inputLen < listLen) {
            throw std::invalid_argument("Invalid rlp string length");
        }
        // empty list
        if (listLen == 0) {
            item.remainder = bytes(input.begin() + 1, input.end());
            return item;
        }

        // decode list
        auto listItem = decodeList(subData(input, 1, listLen));
        for (auto & data : listItem.decoded) {
            item.decoded.push_back(data);
        }
        item.remainder = bytes(input.begin() + 1 + listLen, input.end());
        return item;
    }
    if (prefix <= 0xff) {
        uint32_t lenOfListLen = prefix - 0xf7;
        uint32_t listLen = static_cast<size_t>(decodeLength(subData(input, 1, lenOfListLen)));
        if (inputLen < lenOfListLen || inputLen < lenOfListLen + listLen) {
            throw std::invalid_argument("Invalid rlp list length");
        }
        if (input[1] == 0) {
            throw std::invalid_argument("multi-byte length must have no leading zero");
        }
        if (listLen < 56) {
            throw std::invalid_argument("length below 56 must be encoded in one byte");
        }
        // decode list
        auto listItem = decodeList(subData(input, 1 + lenOfListLen, listLen));
        for (auto & data : listItem.decoded) {
            item.decoded.push_back(data);
        }
        item.remainder = bytes(input.begin() + 1 + lenOfListLen + listLen, input.end());
        return item;
    }
    throw std::invalid_argument("input don't conform RLP encoding form");
}
}  // namespace rlp
}  // namespace evm_common
}  // namespace top