// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_contract_runtime/xevm_storage_base.h"

NS_BEG3(top, contract_runtime, evm)
storage_key xtop_evm_storage_base::decode_key_type(xbytes_t const & key) {
    storage_key res;
    assert(key.size() >= 22);
    res.key_type = storage_key_type(key[1]);
    res.address = "T60004";
    res.address.resize(46);  // 'T60004' + [hex;40]
    static constexpr char hex[] = "0123456789abcdef";
    for (std::size_t index = 0; index < 20; ++index) {
        res.address[6 + 2 * index] = hex[key[index + 2] / 16];
        res.address[6 + 2 * index + 1] = hex[key[index + 2] % 16];
    }
    if (key.size() > 22) {
        res.extra_key.resize(2 * (key.size() - 22));  // max 72
        for (std::size_t index = 20; index < key.size() - 2; ++index) {
            res.extra_key[2 * (index - 20)] = hex[key[index + 2] / 16];
            res.extra_key[2 * (index - 20) + 1] = hex[key[index + 2] % 16];
        }
    }
    return res;
}

NS_END3