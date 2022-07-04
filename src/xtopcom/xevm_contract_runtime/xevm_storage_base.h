// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "assert.h"
#include "xevm_contract_runtime/xevm_storage_face.h"

#include <string>

NS_BEG3(top, contract_runtime, evm)

enum class storage_key_type {
    // Config = 0x0,
    Nonce = 0x1,
    Balance = 0x2,
    Code = 0x3,
    Storage = 0x4,
    Generation = 0x5,

    ALL = 0x6  // show all
};

struct storage_key {
    storage_key_type key_type;
    std::string address;
    std::string extra_key;
};

class xtop_evm_storage_base : public xevm_storage_face_t {
public:
    xtop_evm_storage_base() = default;
    xtop_evm_storage_base(xtop_evm_storage_base const &) = delete;
    xtop_evm_storage_base & operator=(xtop_evm_storage_base const &) = delete;
    xtop_evm_storage_base(xtop_evm_storage_base &&) = default;
    xtop_evm_storage_base & operator=(xtop_evm_storage_base &&) = default;
    ~xtop_evm_storage_base() override = default;

protected:
    /// [1 byte]   -   [1 byte]   - [20 bytes]  - [?? bytes (36 max)]
    //  [version]  - [key perfix] -  [address]  - [extra_key for storage]
    storage_key decode_key_type(xbytes_t const & key);
};

using xevm_storage_base_t = xtop_evm_storage_base;
NS_END3