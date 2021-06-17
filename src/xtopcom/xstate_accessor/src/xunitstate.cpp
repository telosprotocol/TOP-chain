// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xunitstate.h"

#include <cstdint>

namespace top {
namespace state_accessor {

uint64_t xtop_unit_state::balance() const noexcept {
    return 0;
}

uint64_t xtop_unit_state::balance(std::string const & symbol) const noexcept {
    return 0;
}

uint64_t xtop_unit_state::nonce() const noexcept {
    return 0;
}

xtoken_t xtop_unit_state::withdraw(uint64_t const amount, std::error_code & ec) {
    return xtoken_t{};
}
xtoken_t xtop_unit_state::withdraw(std::string const & symbol, uint64_t const amount, std::error_code & ec) {
    return xtoken_t{};
}

// ============================== account context related APIs ==============================
xobject_ptr_t<base::xvbstate_t> xtop_unit_state::internal_state_object(common::xaccount_address_t const & account_address, std::error_code & ec) const {
    if (account_address.empty()) {
        return m_state;
    }

    auto other_account = m_store->query_account(account_address.value());
    if (other_account == nullptr) {
        ec = error::xerrc_t::load_account_state_failed;
        return nullptr;
    }

    auto other_state = other_account->get_bstate();
    if (other_state == nullptr) {
        ec = error::xerrc_t::load_account_state_failed;
        return nullptr;
    }

    return other_state;
}


}
}
