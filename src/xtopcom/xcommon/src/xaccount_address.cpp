// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaccount_address.h"

#include "xcommon/xaccount_base_address.h"
#include "xcommon/xerror/xerror.h"
#include "xdata/xnative_contract_address.h"

#include <cinttypes>

NS_BEG2(top, common)

xaccount_address_t const black_hole_system_address{black_hole_addr};

bool is_t0_address(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_secp256k1_user_account;
}

bool is_t2_address(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_native_contract;
}

bool is_t8_address(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_secp256k1_eth_user_account;
}

bool is_t6_address(xaccount_address_t const & account_address) {
    return account_address.type() == base::enum_vaccount_addr_type_secp256k1_evm_user_account;
}

bool is_black_hold_address(xaccount_address_t const & account_address) {
    assert(account_address.type() == base::enum_vaccount_addr_type_black_hole);
    return account_address == black_hole_system_address;
}

bool address_belongs_to_zone(xaccount_address_t const & account_address, xzone_id_t const & target_zone_id) {
    return account_address.ledger_id().zone_id() == target_zone_id;
}

xaccount_address_t append_table_id(xaccount_base_address_t const & base_address, xtable_id_t const table_id) {
    return xaccount_address_t::build_from(base_address, table_id);
}

xaccount_address_t append_table_id(xaccount_address_t const & address, xtable_id_t const table_id, std::error_code & ec) {
    assert(!ec);

    if (!address.has_assigned_table_id()) {
        return xaccount_address_t::build_from(address.base_address(), table_id);
    }

    if (address.table_id() == table_id) {
        return address;
    }

    ec = error::xerrc_t::table_id_mismatch;
    xerror("append wrong table id %" PRIu16 " to address %s", table_id.value(), address.to_string().c_str());

    return {};
}

xaccount_address_t append_table_id(xaccount_address_t const & address, xtable_id_t const table_id) {
    std::error_code ec;
    auto r = append_table_id(address, table_id, ec);
    top::error::throw_error(ec);
    return r;
}

bool is_eoa_address(xaccount_address_t const & account_address) noexcept {
    // block hole address is not an EOA account since its private key is unknown.
    return is_t0_address(account_address) || is_t6_address(account_address) || is_t8_address(account_address);
}

NS_END2
