// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate/xunit_account.h"

#include <cassert>

namespace top {
namespace state {

base::xvaccount_t xtop_unit_account::address() const {
    assert(m_state != nullptr);

    return base::xvaccount_t{ m_state->get_account_addr() };
}

uint64_t xtop_unit_account::balance() const noexcept {
    assert(m_state != nullptr);

    std::string const balance_property_name{ "balance" };

    xobject_ptr_t<base::xtokenvar_t> balance_prop = m_state->load_token_var(balance_property_name);
    if (balance_prop == nullptr) {
        balance_prop = m_state->new_token_var(balance_property_name);
    }
    assert(balance_prop != nullptr);
    return static_cast<uint64_t>(balance_prop->get_balance());
}

uint64_t xtop_unit_account::nonce() const {
    assert(m_state != nullptr);

    std::string const nonce_property_name{ "nonce" };

    xobject_ptr_t<base::xnoncevar_t> nonce_prop = m_state->load_nonce_var(nonce_property_name);
    if (nonce_prop == nullptr) {
        nonce_prop = m_state->new_nonce_var(nonce_property_name);
    }
    assert(nonce_prop != nullptr);
    return static_cast<uint64_t>(nonce_prop->get_nonce());
}

uint64_t xtop_unit_account::locked_balance() const {
    return 0;
}

xobject_ptr_t<base::xdataunit_t> xtop_unit_account::get_property(std::string const & property_name) const {
    return nullptr;
}

}
}


namespace std {

}
