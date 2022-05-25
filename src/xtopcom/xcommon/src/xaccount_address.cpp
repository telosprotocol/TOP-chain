// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaccount_address.h"

NS_BEG2(top, common)

bool is_t0(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_secp256k1_user_account;
}

bool is_t2(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_native_contract;
}

bool is_t8(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_secp256k1_eth_user_account;
}

bool is_t6(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_secp256k1_evm_user_account;
}

NS_END2
