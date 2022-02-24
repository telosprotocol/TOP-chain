// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <sstream>

#include "xbase/xint.h"
#include "xbase/xbase.h"

namespace top { namespace data {

#define TOP_ADDR_TABLE_ID_SUFFIX_LENGTH  (5)

class xdatautil {
 public:
    static std::string serialize_owner_str(const std::string & prefix, uint32_t table_id);
    static bool deserialize_owner_str(const std::string & address, std::string & prefix, uint32_t & table_id);
    static bool extract_table_id_from_address(const std::string & address, uint32_t & table_id);
    static bool extract_parts(const std::string& address, std::string& base_addr, uint32_t& table_id);
    static std::string base_addr(std::string const& address);
    static std::string xip_to_hex(const xvip2_t & xip);
};

inline std::string to_hex_str(const char* p_data, size_t n_len) {
    std::ostringstream out;
    out << std::hex;
    for (size_t i = 0; i < n_len; ++i) {
        out.fill('0');
        out.width(2);
        out << (static_cast<int16_t>(*(p_data + i)) & 0xff);
    }
    return out.str();
}

inline std::string to_hex_str(const std::string& str) {
    return to_hex_str(str.c_str(), str.length());
}

inline std::string to_hex_str(const uint256_t& hash) {
    return to_hex_str((const char*)hash.data(), hash.size());
}

inline std::string uint_to_str(const uint8_t* data, size_t size) {
    std::ostringstream out;
    out << std::hex;
    out << "0x";
    for (size_t i = 0; i < size; ++i) {
        out.fill('0');
        out.width(2);
        out << (static_cast<uint16_t>(*(data + i)) & 0xff);
    }
    return out.str();
}

inline std::string uint_to_str(const char* data, size_t size) {
    std::ostringstream out;
    if(size > 0){
        out << std::hex;
        out << "0x";
    }
    for (size_t i = 0; i < size; ++i) {
        out.fill('0');
        out.width(2);
        out << (static_cast<uint16_t>(*(data + i)) & 0xff);
    }
    return out.str();
}

inline std::string uint64_to_str(const uint64_t data) {
    std::ostringstream message;
    message << "0x" << std::hex << data << std::dec;
    return message.str();
}

inline int hex_to_dec(char c) {
    if ('0' <= c && c <= '9') {
        return (c - '0');
    } else if ('a' <= c && c <= 'f') {
        return (c - 'a' + 10);
    } else if ('A' <= c && c <= 'F') {
        return (c - 'A' + 10);
    } else {
        return -1;
    }
}

inline std::vector<uint8_t> hex_to_uint(std::string const& str) {
    if (str.size() <= 2 || str.size() % 2 || str[0] != '0' || str[1] != 'x') {
        return {};
    }
    std::vector<uint8_t> ret_vec;
    for (size_t i = 2; i < str.size(); i += 2) {
        int hh = hex_to_dec(str[i]);
        int ll = hex_to_dec(str[i + 1]);
        if (-1 == hh || -1 == ll) {
            return {};
        } else {
            ret_vec.emplace_back((hh << 4) + ll);
        }
    }
    return ret_vec;
}

inline uint64_t hex_to_uint64(std::string& str) {
    if (str.size() <= 2 || str[0] != '0' || str[1] != 'x') {
        return {};
    }
    if (1 == str.size() % 2) {
        str.insert(2, "0");
    }
    uint64_t ret = 0;
    uint32_t first_index = str.size() - 1;
    for (size_t i = first_index; i >= 2; --i) {
        uint64_t n = hex_to_dec(str[i]);
        ret += n * (uint64_t)(pow(16,first_index - i));
    }
    return ret;
}

inline uint256_t hex_to_uint256(const std::string& str) {
    std::vector<uint8_t> ret_vec = std::move(hex_to_uint(str));
    if (ret_vec.size() != 32) {
        return {};
    }
    return uint256_t(ret_vec.data());
}

}  // namespace data
}  // namespace top
