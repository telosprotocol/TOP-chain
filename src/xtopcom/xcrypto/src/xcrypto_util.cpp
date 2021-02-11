// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcrypto/xcrypto_util.h"

#include "xcrypto/xckey.h"

#include <string>

namespace top {
namespace utl {
void xcrypto_util::make_private_key(uint8_t private_key[32]) {
    utl::xecprikey_t pri_key_obj;
    memcpy(private_key, pri_key_obj.data(), pri_key_obj.size());
}

std::string xcrypto_util::make_address_by_assigned_key(uint8_t private_key[32], uint8_t addr_type, uint16_t net_id) {
    utl::xecprikey_t pri_key_obj(private_key);
    utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    return pub_key_obj.to_address(addr_type, net_id);
}

std::string xcrypto_util::make_address_by_random_key(uint8_t addr_type, uint16_t net_id) {
    utl::xecprikey_t pri_key_obj;
    utl::xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    return pub_key_obj.to_address(addr_type, net_id);
}

std::string xcrypto_util::digest_sign(const uint256_t & hash, uint8_t private_key[32]) {
    utl::xecprikey_t pri_key_obj(private_key);
    utl::xecdsasig_t signature_obj = pri_key_obj.sign(hash);
    return std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
}

bool xcrypto_util::verify_sign(const uint256_t & hash, const std::string & signature, const std::string & address) {
    utl::xkeyaddress_t key_address(address);
    utl::xecdsasig_t signature_obj((uint8_t *)signature.c_str());
    return key_address.verify_signature(signature_obj, hash);
}

uint8_t xcrypto_util::get_address_type(const std::string & address) {
    utl::xkeyaddress_t key_address(address);
    uint8_t addr_type{255};
    uint16_t network_id{65535};
    key_address.get_type_and_netid(addr_type, network_id);
    return addr_type;
}
};  // namespace utl
};  // namespace top
