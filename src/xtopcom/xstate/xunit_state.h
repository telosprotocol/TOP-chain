// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate/xstate.h"
#include "xstate/xtoken.h"
#include "xstate/xunit_state_fwd.h"
#include "xvledger/xvaccount.h"

#include <cstdint>

NS_BEG2(top, state)

using xtoken_t = uint64_t;
using xnonce_t = uint64_t;

class xtop_unit_state : public xstate_t {
public:
    base::xvaccount_t address() const = 0;
    virtual xtoken_t balance() const noexcept = 0;
    virtual xnonce_t nonce() const noexcept = 0;
    virtual xtoken_t locked_balance() const noexcept = 0;
};

NS_END2
