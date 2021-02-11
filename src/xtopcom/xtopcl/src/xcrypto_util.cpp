// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcrypto_util.h"

#include "base/utility.h"
#include "xcrypto/xckey.h"

#include <string>

//#include "xbase/xlog.h"

namespace top {
namespace utl {

void xcrypto_util::make_private_key(std::array<uint8_t, PRI_KEY_LEN> & private_key) {
    xecprikey_t pri_key_obj;
    memcpy(private_key.data(), pri_key_obj.data(), pri_key_obj.size());
}

std::string xcrypto_util::make_address_by_assigned_key(std::array<uint8_t, PRI_KEY_LEN> & private_key, uint8_t addr_type, uint16_t ledger_id) {
    xecprikey_t pri_key_obj(private_key.data());
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    return pub_key_obj.to_address(addr_type, ledger_id);
}

std::string xcrypto_util::make_child_address_by_assigned_key(const std::string & parent_addr,
                                                             std::array<uint8_t, PRI_KEY_LEN> & private_key,
                                                             uint8_t addr_type,
                                                             uint16_t ledger_id) {
    xecprikey_t pri_key_obj(private_key.data());
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    return pub_key_obj.to_address(parent_addr, addr_type, ledger_id);
}

std::string xcrypto_util::make_address_by_random_key(uint8_t addr_type, uint16_t ledger_id) {
    xecprikey_t pri_key_obj;
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    return pub_key_obj.to_address(addr_type, ledger_id);
}

std::string xcrypto_util::get_base64_public_key(const std::array<uint8_t, PRI_KEY_LEN> & private_key) {
    xecprikey_t pri_key_obj((uint8_t *)private_key.data());
    xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
    auto base64_pub = xChainSDK::utility::base64_encode(pub_key_obj.data(), pub_key_obj.size());
    return base64_pub;
}

std::string xcrypto_util::digest_sign(const top::uint256_t & hash, const std::array<uint8_t, PRI_KEY_LEN> & private_key) {
    xecprikey_t pri_key_obj((uint8_t *)private_key.data());
    xecdsasig_t signature_obj = pri_key_obj.sign(hash);
    return std::string(reinterpret_cast<char *>(signature_obj.get_compact_signature()), signature_obj.get_compact_signature_size());
}

bool xcrypto_util::verify_sign(const top::uint256_t & hash, const std::string & signature) {
    /*        xecdsasig_t signature_obj((uint8_t*)signature.c_str());

            if (false == xsecp256k1_t::verify_sign(signature_obj, hash)) {
                return false;
            }
    */
    return true;
}

bool xcrypto_util::verify_sign(const top::uint256_t & hash, const std::string & signature, const std::string & address) {
    /*    xecdsasig_t signature_obj((uint8_t*)signature.c_str());
        uint8_t pubkey[65]={0};
        xecpubkey_t pubk(pubkey);
        if (false == xsecp256k1_t::verify_sign(signature_obj, hash, pubk)) {
            return false;
        }

        std::string accountstr = pubk.to_address();
        if (accountstr != address) {
            return false;
        }*/
    return true;
}
}  // namespace utl
}  // namespace top
