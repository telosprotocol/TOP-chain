// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xthreading/xutility.h"

#include <algorithm>
#include <condition_variable>
#include <iterator>
#include <mutex>
#include <list>

NS_BEG2(top, threading)

template <typename T, typename Container = std::list<T>>
class xthreadsafe_queue final
{
public:
    xthreadsafe_queue(xthreadsafe_queue const &)             = delete;
    xthreadsafe_queue & operator=(xthreadsafe_queue const &) = delete;
    xthreadsafe_queue(xthreadsafe_queue &&)                  = default;
    xthreadsafe_queue & operator=(xthreadsafe_queue &&)      = default;
    ~xthreadsafe_queue()                                     = default;

    explicit
    xthreadsafe_queue(std::size_t const max_size = std::numeric_limits<std::size_t>::max())
        : m_max_size{ max_size }
    {
        // if (m_max_size != std::numeric_limits<std::size_t>::max()) {
        //     m_queue.reserve(m_max_size);
        // }
    }

    void
    push(T && o) {
        XLOCK_GUARD(m_queue_mutex) {
            if (m_queue.size() >= m_max_size) {
                return;
            }
            m_queue.push_back(std::move(o));
        }

        m_cv.notify_one();
    }

    void
    push(T const & o) {
        XLOCK_GUARD(m_queue_mutex) {
            if (m_queue.size() >= m_max_size) {
                return;
            }
            m_queue.push_back(o);
        }

        m_cv.notify_one();
    }

    T
    wait_and_pop() {
        std::unique_lock<std::mutex> lock{ m_queue_mutex };
        m_cv.wait(lock, [this]() { return !m_queue.empty(); });

        T ret = std::move(m_queue.front());
        m_queue.pop_front();

        return ret;
    }

    Container
    wait_and_pop_all() {
        std::unique_lock<std::mutex> lock{ m_queue_mutex };
        m_cv.wait(lock, [this]() { return !m_queue.empty(); });

        Container ret;
        ret.swap(m_queue);

        return ret;
    }

    bool
    try_pop(T &value) {
        std::lock_guard<std::mutex> lk{ m_queue_mutex };
        if (m_queue.empty())
        {
            return false;
        }

        value = std::move(m_queue.front());
        m_queue.pop_front();
        return true;
    }

    std::size_t
    unsafe_size() const {
        std::lock_guard<std::mutex> lk{ m_queue_mutex };
        return m_queue.size();
    }

  private:
    mutable std::mutex m_queue_mutex{};
    std::condition_variable m_cv{};
    Container m_queue{};
    std::size_t m_max_size;
};

NS_END2
