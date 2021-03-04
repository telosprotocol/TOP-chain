// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#include "xchain_timer/xchain_timer.h"

#include "xbase/xcontext.h"
#include "xbase/xobject.h"
#include "xbase/xthread.h"
#include "xbase/xutl.h"

#include <cassert>
#include <cinttypes>
NS_BEG2(top, time)

void xchain_timer_t::process(data::xblock_t* timer_block, int64_t recv_ms) {
    time::xchain_time_st current_time(timer_block, recv_ms);
    {
        xdbg("m_one_timer_mutex notify_all begin");
        std::lock_guard<std::mutex> lock(m_one_timer_mutex);
        for (auto iter = m_watch_one_map.begin(); iter != m_watch_one_map.end();) {
            xdbg("m_one_timer_mutex notify_all, %d", m_watch_one_map.size());
            time_watcher_item item = iter->second;
            if (current_time.xtime_round >= item.interval) {  // use >= for exception
                auto start = base::xtime_utl::gmttime_ms();
                item.watcher(current_time);
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
            time_watcher_item item = iter.second;
            xinfo("notify_all:%s,%lld", iter.first.c_str(), current_time.xtime_round);
            if (current_time.xtime_round % item.interval == 0) {
                auto start = base::xtime_utl::gmttime_ms();
                item.watcher(current_time);
                auto end = base::xtime_utl::gmttime_ms();
                if (end - start > 1000) {
                    xwarn("[xchain_timer] watcher: %s cost long time:%d", iter.first.c_str(), end - start);
                }
            }
        }
    }
}

bool xchain_timer_t::update_time(data::xblock_t* timer_block, bool force) {
    xinfo("new xchain_timer_t m_mutex update_time,id(%ld, %ld, %p)", timer_block->get_height(), m_latest.load(), this);
    if (timer_block->get_height() > m_latest || force) {
        xinfo("[xchain_timer_t::update_time] notify");
        m_latest = timer_block->get_height(); // update first
        auto func = [](base::xcall_t &call, const int32_t thread_id, const uint64_t timenow_ms) -> bool {
            auto _this = (xchain_timer_t*) call.get_param1().get_object();
            auto block = (data::xblock_t*) call.get_param2().get_object();
            auto recv_ms = call.get_param3().get_int64();
            _this->process(block, recv_ms);
            return true;
        };

        base::xcall_t c((base::xcallback_t) func, this, timer_block, base::xtime_utl::gettimeofday_ms());
        m_timer_thread->send_call(c);
    } else {
        xwarn("[new xchain_timer_t] update_time failed,id(%" PRIu64 ")\n", timer_block->get_height());
    }
    xinfo("new xchain_timer_t m_mutex update_time end");
    return true;
}

void xchain_timer_t::init() {
    m_timer_thread = base::xiothread_t::create_thread(base::xcontext_t::instance(), 0, -1);
}

uint64_t xchain_timer_t::logic_time() const noexcept {
    return m_latest;
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
