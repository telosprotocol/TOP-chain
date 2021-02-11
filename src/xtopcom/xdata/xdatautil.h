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

}  // namespace data
}  // namespace top
