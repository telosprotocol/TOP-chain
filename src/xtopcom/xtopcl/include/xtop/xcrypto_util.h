// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "user_info.h"
#include "xbase/xint.h"

#include <array>
#include <string>

namespace top {
namespace utl {
using xChainSDK::PRI_KEY_LEN;
class xcrypto_util {
public:
    static void make_private_key(std::array<uint8_t, PRI_KEY_LEN> & private_key);
    static std::string make_address_by_assigned_key(std::array<uint8_t, PRI_KEY_LEN> & private_key, uint8_t addr_type = '0', uint16_t ledger_id = 0);
    static std::string make_child_address_by_assigned_key(const std::string & parent_addr,
                                                          std::array<uint8_t, PRI_KEY_LEN> & private_key,
                                                          uint8_t addr_type,
                                                          uint16_t ledger_id = 0);
    static std::string make_address_by_random_key(uint8_t addr_type = 0, uint16_t ledger_id = 0);

    static std::string get_base64_public_key(const std::array<uint8_t, PRI_KEY_LEN> & private_key);
    static std::string get_hex_public_key(const std::array<uint8_t, PRI_KEY_LEN> & private_key);    
    static std::string digest_sign(const top::uint256_t & hash, const std::array<uint8_t, PRI_KEY_LEN> & private_key);
    static bool verify_sign(const top::uint256_t & hash, const std::string & signature);
    static bool verify_sign(const top::uint256_t & hash, const std::string & signature, const std::string & address);
};

}  // namespace utl
}  // namespace top
