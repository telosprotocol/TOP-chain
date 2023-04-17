// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#include "xchain_timer/xchain_timer.h"

#include "xbase/xcontext.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xutl.h"
#include "xbasic/xutility.h"
#include "xconfig/xutility.h"
#include "xmetrics/xmetrics.h"
#include "xvledger/xvblock.h"

#include "xbasic/xtimer_driver.h"

#include <cassert>
#include <cinttypes>

NS_BEG2(top, time)

xchain_timer_t::xchain_timer_t(std::shared_ptr<xbase_timer_driver_t> const & timer_driver) noexcept : m_timer_driver{make_observer(timer_driver.get())} {
    assert(m_timer_driver != nullptr);
}

void xchain_timer_t::process(common::xlogic_time_t const old_time, common::xlogic_time_t const new_time, xlogic_timer_update_strategy_t update_strategy) {
    assert(update_strategy == xlogic_timer_update_strategy_t::force ||
           (update_strategy == xlogic_timer_update_strategy_t::discard_old_value && old_time < new_time));
    // copy all callbacks into a temp local variable.
    // there is an issue when the callback (a std::function object) wrappers the xobject_t object.
    // the issue is when callbacks are copied into the local variable and right before a callback is called,
    // the corresponding xobject_t is calling unwatch, then the callback may be invalid (the xobject_t's
    // reference count may reach zero and the object is free by itself).
    // std::function object is easy to make sure the object, inside it, is still alive via std::shared_ptr,
    // but is hard to achieve the same result if the internal object is of type xobject_t.
    // when the std::function object is constructed from lambda which captures an xobject_t instance,
    // it may be easy to achieve the same result by wrapping the xobject_t pointer into an xobject_ptr_t instance.
    // if the construction is from std::bind, it's hard.
    std::map<std::string, time_watcher_item> callbacks;
    {
        xdbg("xchain_timer_t m_mutex notify_all begin");
        std::lock_guard<std::mutex> lock(m_mutex);
        callbacks = m_watch_map;

        xdbg("xchain_timer_t m_mutex notify_all end, %d", m_watch_map.size());
    }

    auto const do_missing = update_strategy != xlogic_timer_update_strategy_t::force && (old_time + 10 >= new_time); // compensate up to 10 logic time.
    if (do_missing) {
        for (auto & callback_datum : callbacks) {
            auto const & item = top::get<time_watcher_item>(callback_datum);
            for (auto time = old_time + 1; time <= new_time; ++time) {
                xinfo("notify_all:%s,%" PRIu64, top::get<std::string const>(callback_datum).c_str(), time);
                if (time % item.interval == 0) {
                    auto const start = base::xtime_utl::gmttime_ms();
                    item.watcher(time);
                    auto const end = base::xtime_utl::gmttime_ms();
                    if (end - start > 1000) {
                        xwarn("[xchain_timer] watcher: %s cost long time:%d", top::get<std::string const>(callback_datum).c_str(), end - start);
                    }
                }
            }
        }
    } else {
        for (auto & callback_datum : callbacks) {
            auto const & item = top::get<time_watcher_item>(callback_datum);
            xinfo("notify_all:%s,%" PRIu64, top::get<std::string const>(callback_datum).c_str(), new_time);
            if (new_time % item.interval == 0) {
                auto const start = base::xtime_utl::gmttime_ms();
                item.watcher(new_time);
                auto const end = base::xtime_utl::gmttime_ms();
                if (end - start > 1000) {
                    xwarn("[xchain_timer] watcher: %s cost long time:%d", top::get<std::string const>(callback_datum).c_str(), end - start);
                }
            }
        }
    }
}

void xchain_timer_t::update_time(common::xlogic_time_t new_time, xlogic_timer_update_strategy_t update_strategy) {
    if (update_strategy != xlogic_timer_update_strategy_t::discard_old_value && update_strategy != xlogic_timer_update_strategy_t::force) {
        assert(false);
        return;
    }

    auto const current_time = m_curr_time.load(std::memory_order_relaxed);

    // for debug purpose
    bool const not_continuous = current_time + 1 < new_time;
#ifdef ENABLE_METRICS
    if (not_continuous) {
        XMETRICS_GAUGE(metrics::chaintimer_clock_discontinuity, 1);
    }
#endif

    std::chrono::steady_clock::time_point curr_time_update_time_point;
    {
        std::lock_guard<std::mutex> lock{ m_update_mutex };
        curr_time_update_time_point = m_curr_time_update_time_point;
    }
    xinfo("logic_timer: update timer: input: %" PRIu64 "; current: %" PRIu64 "; last update time %" PRIi64 " current steady time %" PRIi64 " timer object: %p; discontinuity=%d",
          new_time,
          current_time,
          static_cast<int64_t>(curr_time_update_time_point.time_since_epoch().count()),
          static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
          static_cast<void *>(this),
          not_continuous);

    if (update_strategy == xlogic_timer_update_strategy_t::discard_old_value && current_time >= new_time) {
        return;
    }

    xinfo("logic_timer: updating timer: input: %" PRIu64 "; current: %" PRIu64 "; last update time %" PRIi64 " current steady time %" PRIi64 " timer object: %p",
          new_time,
          current_time,
          static_cast<int64_t>(curr_time_update_time_point.time_since_epoch().count()),
          static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
          static_cast<void *>(this));
    m_curr_time.store(new_time, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock{m_update_mutex};
        m_curr_time_update_time_point = std::chrono::steady_clock::now();
    }

    // this object is a global singleton object. Thus, directly pass 'this' to lambda object without add_ref() operation.
    auto func = [this, current_time, new_time, update_strategy](base::xcall_t &, const int32_t, const uint64_t) -> bool {
        process(current_time, new_time, update_strategy);
        return true;
    };

    base::xcall_t c(func);
    m_timer_thread->send_call(c);
}

void xchain_timer_t::do_check_logic_time() {
    m_timer_driver->schedule(std::chrono::minutes{1}, [this](std::chrono::milliseconds) {
        auto const curr_time = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point last_update_time_point{};
        {
            std::lock_guard<std::mutex> lock{m_update_mutex};
            last_update_time_point = m_curr_time_update_time_point;
        }

        auto const diff = std::chrono::duration_cast<std::chrono::minutes>(curr_time - last_update_time_point);
        if (diff >= std::chrono::minutes{2}) {
            common::xlogic_time_t time = config::gmttime_to_logic_time(top::base::xtime_utl::gmttime());
            xinfo("logic_timer: locally update timer: input: %" PRIu64 "; last update time %" PRIi64 " current steady time %" PRIi64 " timer object: %p",
                  time,
                  static_cast<int64_t>(last_update_time_point.time_since_epoch().count()),
                  static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
                  static_cast<void *>(this));
            update_time(time, xlogic_timer_update_strategy_t::discard_old_value);
        }

        do_check_logic_time();
    });
}

void xchain_timer_t::start() {
    assert(m_timer_driver != nullptr);
    init();
    do_check_logic_time();
}

void xchain_timer_t::stop() {
    m_timer_thread->close();
}

void xchain_timer_t::init() {
    m_timer_thread = base::xiothread_t::create_thread(base::xcontext_t::instance(), 0, -1);
}

uint64_t xchain_timer_t::logic_time() const noexcept {
    return m_curr_time;
}

bool xchain_timer_t::watch(const std::string & key, std::uint64_t interval, xchain_time_watcher cb) {
    xdbg("xchain_timer_t m_mutex watch : %s", key.c_str());
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watch_map[key] = {interval, cb};
    return true;
}

bool xchain_timer_t::unwatch(const std::string & key) {
    xdbg("xchain_timer_t m_mutex unwatch : %s", key.c_str());
    std::lock_guard<std::mutex> lock(m_mutex);
    m_watch_map.erase(key);
    return true;
}

base::xiothread_t * xchain_timer_t::get_iothread() const noexcept {
    return m_timer_thread;
}

void xchain_timer_t::close() {
    if (m_timer_thread != nullptr) {
        m_timer_thread->close();
        m_timer_thread->release_ref();
        m_timer_thread = nullptr;
    }
}

NS_END2
