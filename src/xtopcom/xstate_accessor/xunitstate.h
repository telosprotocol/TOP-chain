// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xbase/xobject_ptr.h"
#include "xstate_accessor/xunitstate_face.h"
#include "xstore/xstore_face.h"
#include "xvledger/xvstate.h"

#include <cstdint>

namespace top {
namespace state_accessor {

class xtop_unit_state : public xunit_state_face_t {
private:
    xobject_ptr_t<base::xvbstate_t> m_state{ nullptr };
    observer_ptr<store::xstore_face_t> m_store{ nullptr };

public:
    xtop_unit_state() = default;
    xtop_unit_state(xtop_unit_state const &) = delete;
    xtop_unit_state & operator=(xtop_unit_state const &) = delete;
    xtop_unit_state(xtop_unit_state &&) = default;
    xtop_unit_state & operator=(xtop_unit_state &&) = default;
    virtual ~xtop_unit_state() = default;

    uint64_t balance() const noexcept override;
    uint64_t balance(std::string const & symbol) const noexcept override;
    uint64_t nonce() const noexcept override;

    xtoken_t withdraw(uint64_t const amount, std::error_code & ec) override;
    xtoken_t withdraw(std::string const & symbol, uint64_t const amount, std::error_code & ec) override;

    // ============================== account context related APIs ==============================
    void string_create(std::string const & property_name, std::error_code & ec);
    xobject_ptr_t<base::xvbstate_t> internal_state_object(common::xaccount_address_t const & account_address, std::error_code & ec) const;
};
using xunit_state_t = xtop_unit_state;

}
}
