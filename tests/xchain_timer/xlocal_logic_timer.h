// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xbase.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xvledger/xvblock.h"

#include <cassert>

NS_BEG3(top, tests, chain_timer)

class xtop_local_logic_timer : public time::xchain_time_face_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_local_logic_timer);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_local_logic_timer);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_local_logic_timer);

    void start() override{
        running(true);
    }

    void stop() override {
        running(false);
    }

    void update_time(common::xlogic_time_t, time::xlogic_timer_update_strategy_t) override {
    }

    bool watch(const std::string &, uint64_t, time::xchain_time_watcher) override {
        return false;
    }

    bool unwatch(const std::string &) override {
        return false;
    }

    void init() override {
    }

    common::xlogic_time_t logic_time() const noexcept override {
        return (base::xtime_utl::gmttime() - base::TOP_BEGIN_GMTIME) / 10;
    }

    void close() override {
    }

    base::xiothread_t * get_iothread() const noexcept override {
        return nullptr;
    }
};
using xlocal_logic_timer_t = xtop_local_logic_timer;

NS_END3
