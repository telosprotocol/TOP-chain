// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xtransaction.h"
// #include "xtransaction_v2.h"
namespace top { namespace data {

class xtx_factory {
public:
    static xtransaction_ptr_t create_tx(const enum_xtransaction_version tx_version = xtransaction_version_2);
    static xtransaction_ptr_t create_genesis_tx_with_balance(const std::string & account, int64_t top_balance);
    static xtransaction_ptr_t create_genesis_tx_with_sys_contract(const std::string & account);
};

}  // namespace data
}  // namespace top
