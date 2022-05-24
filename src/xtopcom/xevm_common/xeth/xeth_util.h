#pragma once
#include <string>
#include <vector>
#include "xbase/xint.h"
#include "xbasic/xuint.hpp"
#include "xevm_common/common.h"
#include "xevm_common/rlp.h"
#include "xevm_common/fixed_hash.h"
#include "xutility/xhash.h"

NS_BEG3(top, evm_common, eth)
using namespace top::evm_common::rlp;
class util {
#define block_number_of_per_epoch 30000
#define max_epoch 2048
#define dataset_init_bytes       1 << 30
#define dataset_growth_bytes     1 << 23
#define mix_bytes                128
#define hash_bytes               64
public:
    inline static std::string hex_to_hex(std::string& hex) {
        std::string raw_hex;
        if (hex.substr(0,2) == "0x") {
            raw_hex = hex.substr(2);
        } else {
            raw_hex = hex;
        }
        return raw_hex;
    }

    inline static std::vector<uint8_t> hex_to_bytes(std::string &hex) {
        auto parse_digit = [](char d) -> int { return d <= '9' ? (d - '0') : (d - 'a' + 10); };
        std::vector<uint8_t> bytes;
        for (size_t i = 1; i < hex.size(); i += 2)
        {
            int h = parse_digit(hex[i - 1]);
            int l = parse_digit(hex[i]);
            bytes.push_back(uint8_t((h << 4) | l));
        }

        return bytes;
    }

    inline static uint64_t bytes_to_uint64(std::array<byte, 8> &bytes) {
        uint64_t value = 0;
        for (size_t i = 0; i < bytes.size(); i ++)
        {
            value = (value << 8) | bytes[i];
        }

        return value;
    }

    inline static std::vector<uint8_t> fromU256(u256 value) {
        std::vector<uint8_t> v;
        u256 tmp = value;
        int32_t len = 32;
        while (len-- > 0) {
            v.push_back(tmp.convert_to<uint8_t>());
            tmp >= 8;
        }

        return v;
    }

    inline static h256 zeroHash() {
        bytes out;
        std::vector<uint8_t> zeorVec;
        out = RLP::encodeList<std::vector<uint8_t>>(zeorVec);
        auto hashValue = utl::xkeccak256_t::digest(out.data(), out.size());
        h256 hash = FixedHash<32>(hashValue.data(), h256::ConstructFromPointer);
        return hash;
    }
};
NS_END3