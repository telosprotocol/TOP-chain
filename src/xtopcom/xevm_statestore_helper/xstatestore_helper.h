// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

// todo
NS_BEG2(top, evm_statestore)

class xtop_evm_statestore_helper {
public:
    xtop_evm_statestore_helper() = default;
    xtop_evm_statestore_helper(xtop_evm_statestore_helper const &) = delete;
    xtop_evm_statestore_helper & operator=(xtop_evm_statestore_helper const &) = delete;
    xtop_evm_statestore_helper(xtop_evm_statestore_helper &&) = default;
    xtop_evm_statestore_helper & operator=(xtop_evm_statestore_helper &&) = default;
    ~xtop_evm_statestore_helper() = default;
};
using xevm_statestore_helper_t = xtop_evm_statestore_helper;

NS_END2