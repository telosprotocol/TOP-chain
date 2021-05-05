// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate/xunit_state.h"
#include "xvledger/xvstate.h"

NS_BEG2(top, state)

class xtop_unit_account : public xunit_state_t {
    xobject_ptr_t<base::xvbstate_t> m_state;

    xobject_ptr_t<base::xtokenvar_t> m_balance;
    xobject_ptr_t<base::xtokenvar_t> m_locked_balance;
    xobject_ptr_t<base::xnoncevar_t> m_nonce;
    std::unordered_map<std::string, xobject_ptr_t<base::xvproperty_t>> m_properties;

public:
    base::xvaccount_t address() const override;
    uint64_t balance() const override;
    uint64_t nonce() const override;
    uint64_t locked_balance() const override;
    xobject_ptr_t<base::xdataunit_t> get_property(std::string const & property_name) const override;
};
using xunit_account_t = xtop_unit_account;

NS_END2
