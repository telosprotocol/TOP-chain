// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "tests/xchain_timer/xdummy_chain_timer.h"

NS_BEG3(top, tests, vnetwork)

class xtop_dummy_chain_timer final : public tests::chain_timer::xdummy_chain_timer_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_chain_timer);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_chain_timer);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_chain_timer);

    common::xlogic_time_t logic_time() const noexcept override { return 10; }
};
using xdummy_chain_timer_t = xtop_dummy_chain_timer;

extern top::xobject_ptr_t<xdummy_chain_timer_t> xdummy_chain_timer;

NS_END3
