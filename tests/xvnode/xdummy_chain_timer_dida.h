// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "tests/xchain_timer/xdummy_chain_timer.h"

#include <thread>

NS_BEG3(top, tests, vnode)

class xtop_dummy_chain_timer final : public tests::chain_timer::xdummy_chain_timer_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_chain_timer);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_chain_timer);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_chain_timer);

    xtop_dummy_chain_timer(uint16_t count) : m_count{count} {}
    void func(time::xchain_time_watcher cb) {
        half_sec_cnt = 0;
        for (size_t u = 0; u < m_count; u++) {
            usleep(500000);
            half_sec_cnt++;
            cb(half_sec_cnt);
        }
        return;
    }
    bool watch(const std::string & key, std::uint64_t interval, time::xchain_time_watcher cb) override {
        std::thread t(std::mem_fn(&xtop_dummy_chain_timer::func), this, cb);
        t.detach();

        return false;
    }

    int half_sec_cnt;
    uint16_t m_count;
};
using xdummy_chain_timer_t = xtop_dummy_chain_timer;

extern xdummy_chain_timer_t xdummy_chain_timer;

NS_END3
