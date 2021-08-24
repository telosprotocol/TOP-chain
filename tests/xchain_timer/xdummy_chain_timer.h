// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xchain_timer/xchain_timer_face.h"

NS_BEG3(top, tests, chain_timer)

class xtop_dummy_chain_timer : public time::xchain_time_face_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_chain_timer);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_chain_timer);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_chain_timer);

    void start() override{
    }

    void stop() override {
    }

    void update_time(common::xlogic_time_t, time::xlogic_timer_update_strategy_t) override {
        return;
    }

    bool
    watch(const std::string &, uint64_t, time::xchain_time_watcher) override {
        return false;
    }

    //bool
    //watch_one(uint64_t, time::xchain_time_watcher) override {
    //    return false;
    //}

    bool
    unwatch(const std::string &) override {
        return false;
    }

    void
    init() override {
    }

    common::xlogic_time_t
    logic_time() const noexcept override {
        return 0;
    }

    void
    close() override {
    }

    base::xiothread_t *
    get_iothread() const noexcept override {
        return nullptr;
    }
};
using xdummy_chain_timer_t = xtop_dummy_chain_timer;

NS_END3
