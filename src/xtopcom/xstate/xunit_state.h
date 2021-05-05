// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xstate/xstate.h"
#include "xstate/xtoken.h"
#include "xstate/xunit_state_fwd.h"
#include "xvledger/xvaccount.h"
#include "xvledger/xvproperty.h"

#include <cstdint>

NS_BEG2(top, state)

class xtop_unit_state : public xstate_t {
public:
    virtual base::xvaccount_t address() const = 0;
    virtual uint64_t balance() const noexcept = 0;
    virtual uint64_t nonce() const noexcept = 0;
    virtual uint64_t locked_balance() const noexcept = 0;
    virtual xobject_ptr_t<base::xdataunit_t> get_property(std::string const & property_name) const = 0;
};

NS_END2
