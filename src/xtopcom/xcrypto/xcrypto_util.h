// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xint.h"

#include <string>

namespace top {
namespace utl {
class xcrypto_util {
public:
    static void make_private_key(uint8_t private_key[32]);
    static std::string make_address_by_assigned_key(uint8_t private_key[32], uint8_t addr_type = 0, uint16_t net_id = 65535);
    static std::string make_address_by_random_key(uint8_t addr_type = 0, uint16_t net_id = 65535);
    static std::string digest_sign(const uint256_t & hash, uint8_t private_key[32]);
    static bool verify_sign(const uint256_t & hash, const std::string & signature, const std::string & address);

    static uint8_t get_address_type(const std::string & address);
};
};  // namespace utl
};  // namespace top
