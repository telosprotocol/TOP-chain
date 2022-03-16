// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include "xbase/xthread.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xbasic/xtimer_driver_fwd.h"
#include "xbasic/xmemory.hpp"

#include <chrono>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <utility>

NS_BEG2(top, time)

class xchain_timer_t final : public xchain_time_face_t {
private:
    observer_ptr<xbase_timer_driver_t> m_timer_driver;

    struct time_watcher_item {
        uint64_t interval;
        xchain_time_watcher watcher;
    };
    std::mutex m_mutex{};
    std::map<std::string, time_watcher_item> m_watch_map{};
    base::xiothread_t * m_timer_thread{nullptr};
    std::atomic<common::xlogic_time_t> m_curr_time{0};
    std::mutex m_update_mutex{};
    std::chrono::steady_clock::time_point m_curr_time_update_time_point{std::chrono::steady_clock::now()};

public:
    xchain_timer_t(xchain_timer_t const &) = delete;
    xchain_timer_t & operator=(xchain_timer_t const &) = delete;
    xchain_timer_t(xchain_timer_t &&) = default;
    xchain_timer_t & operator=(xchain_timer_t &&) = default;

    explicit xchain_timer_t(std::shared_ptr<xbase_timer_driver_t> const & timer_driver) noexcept;

protected:
    ~xchain_timer_t() override = default;

public:
    void start() override;
    void stop() override;

    void init() override;
    void update_time(common::xlogic_time_t time, xlogic_timer_update_strategy_t update_strategy) override;
    common::xlogic_time_t logic_time() const noexcept override;

    // note: interval is 10s/round, not second!!
    bool watch(const std::string & key, uint64_t interval, xchain_time_watcher cb) override;
    bool unwatch(const std::string & key) override;
    void close() override;
    base::xiothread_t * get_iothread() const noexcept override;

protected:
    void process(common::xlogic_time_t const old_time, common::xlogic_time_t const new_time, xlogic_timer_update_strategy_t update_strategy);
    void do_check_logic_time();
};

NS_END2
