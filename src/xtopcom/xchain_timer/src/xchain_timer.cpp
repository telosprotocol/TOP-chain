// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#include "xchain_timer/xchain_timer.h"

#include "xbase/xcontext.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xutl.h"
#include "xbase/xvblock.h"

#include "xbasic/xtimer_driver.h"

#include <cassert>
#include <cinttypes>

NS_BEG2(top, time)

xchain_timer_t::xchain_timer_t(std::shared_ptr<xbase_timer_driver_t> const & timer_driver) noexcept : m_timer_driver{make_observer(timer_driver.get())} {
    assert(m_timer_driver != nullptr);
}

void xchain_timer_t::process(common::xlogic_time_t time) {
    // time::xchain_time_st current_time(timer_block, recv_ms);
    {
        xdbg("m_one_timer_mutex notify_all begin");
        std::lock_guard<std::mutex> lock(m_one_timer_mutex);
        for (auto iter = m_watch_one_map.begin(); iter != m_watch_one_map.end();) {
            xdbg("m_one_timer_mutex notify_all, %d", m_watch_one_map.size());
            auto const & item = iter->second;
            if (time >= item.interval) {  // use >= for exception
                auto start = base::xtime_utl::gmttime_ms();
                item.watcher(time);
                auto end = base::xtime_utl::gmttime_ms();
                if (end - start > 1000) {
                    xwarn("[xchain_timer] watcher one: %ld cost long time:%d", iter->first, end - start);
                }
                iter = m_watch_one_map.erase(iter);
            } else {
                ++iter;
            }
        }
        xdbg("m_one_timer_mutex notify_all end, %d", m_watch_one_map.size());
    }

    {
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

        for (auto & iter : callbacks) {
            time_watcher_item const & item = iter.second;
            xinfo("notify_all:%s,%lld", iter.first.c_str(), time);
            if (time % item.interval == 0) {
                auto start = base::xtime_utl::gmttime_ms();
                item.watcher(time);
                auto end = base::xtime_utl::gmttime_ms();
                if (end - start > 1000) {
                    xwarn("[xchain_timer] watcher: %s cost long time:%d", iter.first.c_str(), end - start);
                }
            }
        }
    }
}

void xchain_timer_t::update_time(common::xlogic_time_t time, xlogic_timer_update_strategy_t update_strategy) {
    if (update_strategy != xlogic_timer_update_strategy_t::discard_old_value && update_strategy != xlogic_timer_update_strategy_t::force) {
        assert(false);
        return;
    }

    auto const current_time = m_curr_time.load(std::memory_order_relaxed);

    std::chrono::steady_clock::time_point curr_time_update_time_point;
    {
        std::lock_guard<std::mutex> lock{m_update_mutex};
        curr_time_update_time_point = m_curr_time_update_time_point;
    }
    xinfo("logic_timer: update timer: input: %" PRIu64 "; current: %" PRIu64 "; last update time %" PRIi64 " current steady time %" PRIi64 " timer object: %p",
          time,
          current_time,
          static_cast<int64_t>(curr_time_update_time_point.time_since_epoch().count()),
          static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
          static_cast<void *>(this));

    if (update_strategy == xlogic_timer_update_strategy_t::discard_old_value && current_time >= time) {
        return;
    }

    xinfo("logic_timer: updating timer: input: %" PRIu64 "; current: %" PRIu64 "; last update time %" PRIi64 " current steady time %" PRIi64 " timer object: %p",
          time,
          current_time,
          static_cast<int64_t>(curr_time_update_time_point.time_since_epoch().count()),
          static_cast<int64_t>(std::chrono::steady_clock::now().time_since_epoch().count()),
          static_cast<void *>(this));
    m_curr_time.store(time, std::memory_order_relaxed);
    {
        std::lock_guard<std::mutex> lock{m_update_mutex};
        m_curr_time_update_time_point = std::chrono::steady_clock::now();
    }

    auto func = [](base::xcall_t & call, const int32_t thread_id, const uint64_t timenow_ms) -> bool {
        auto _this = (xchain_timer_t *)call.get_param1().get_object();
        auto time = call.get_param2().get_uint64();
        auto recv_ms = call.get_param3().get_int64();
        _this->process(time);
        return true;
    };

    base::xcall_t c((base::xcallback_t)func, this, time, base::xtime_utl::gettimeofday_ms());
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
            common::xlogic_time_t time = (base::xtime_utl::gmttime() - base::TOP_BEGIN_GMTIME) / 10;
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
    m_watch_map.insert({key, {interval, cb}});
    return true;
}

bool xchain_timer_t::watch_one(uint64_t interval, xchain_time_watcher cb) {
    xdbg("m_one_timer_mutex watch_one");
    std::lock_guard<std::mutex> lock(m_one_timer_mutex);
    static uint64_t             uuid{0};
    m_watch_one_map.insert({++uuid, {interval, cb}});
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
