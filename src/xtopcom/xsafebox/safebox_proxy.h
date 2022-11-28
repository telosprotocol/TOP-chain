// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xint.h"
#include "xbase/xns_macro.h"
#include "xbasic/xcrypto_key.h"
#include "xbasic/xmemory.hpp"

#include <map>
#include <memory>
#include <string>

NS_BEG2(top, safebox)

// wrapper type, do NOT use this outside of safebox implement.
class xsafebox_private_key;
class xsafebox_proxy {
private:
    std::map<std::string, std::unique_ptr<xsafebox_private_key>> m_key_map;

public:
    static xsafebox_proxy & get_instance();

public:
    void add_key_pair(top::xpublic_key_t const & public_key, std::string && sign_key);

    /// @brief return `<randon ecpoint, signature data>`
    /// @param msg message to be signed.
    std::pair<std::string, std::string> get_proxy_signature(std::string const & public_key, std::string const & msg);

    /// @brief return signature_str of secp256k1 signature
    std::string get_proxy_secp256_signature(std::string const & public_key, top::uint256_t const & hash);

private:
    xsafebox_proxy() = default;
};

NS_END2