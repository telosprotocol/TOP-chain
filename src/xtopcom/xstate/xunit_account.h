// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate/xunit_state.h"
#include "xvledger/xvaccount.h"

NS_BEG2(top, state)

class xtop_unit_account : public xunit_state_t {
public:
    base::xvaccount_t address() const;
};

NS_END2
