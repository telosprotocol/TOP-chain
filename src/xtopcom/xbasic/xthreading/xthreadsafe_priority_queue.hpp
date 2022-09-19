// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xthreading/xutility.h"

#include <algorithm>
#include <functional>
#include <mutex>
#include <queue>
#include <vector>

NS_BEG2(top, threading)

/**
 * @brief Thread-safe priority queue.
 *        Wrapped std priority queue, add thread mutex
 *        seprate out priority type (default std::size_t)
 *
 * @tparam T element type
 * @tparam P prio (number or anything has a strict ordering)
 */
template <typename T, typename P = std::size_t>
class xthreadsafe_priority_queue final {
private:
    class xwrapped_type_t {
    public:
        T elements;
        P priority;
    };

    struct comparator {
        bool operator()(xwrapped_type_t const & lhs, xwrapped_type_t const & rhs) const {
            return lhs.priority < rhs.priority;
        }
    };

private:
    // std::size_t m_max_size; // todo?
    std::priority_queue<xwrapped_type_t, std::vector<xwrapped_type_t>, comparator> m_pq;
    mutable std::mutex m_priority_queue_mutex;

public:
    xthreadsafe_priority_queue(xthreadsafe_priority_queue const &) = delete;
    xthreadsafe_priority_queue & operator=(xthreadsafe_priority_queue const &) = delete;
    xthreadsafe_priority_queue(xthreadsafe_priority_queue &&) = default;
    xthreadsafe_priority_queue & operator=(xthreadsafe_priority_queue &&) = default;
    ~xthreadsafe_priority_queue() = default;

    explicit xthreadsafe_priority_queue() {
    }

    bool empty() const {
        std::unique_lock<std::mutex> lock{m_priority_queue_mutex};
        return m_pq.empty();
    }

    std::size_t size() const {
        std::unique_lock<std::mutex> lock{m_priority_queue_mutex};
        return m_pq.size();
    }

    void push(T const & e, P const & prio) {
        std::unique_lock<std::mutex> lock{m_priority_queue_mutex};
        m_pq.push(xwrapped_type_t{e, prio});
    }

    void pop() {
        std::unique_lock<std::mutex> lock{m_priority_queue_mutex};
        m_pq.pop();
    }

    // get the top element with pop it out.
    std::pair<T, P> top() const {
        std::unique_lock<std::mutex> lock{m_priority_queue_mutex};
        xwrapped_type_t r = m_pq.top();
        return std::make_pair(r.elements, r.priority);
    }

    std::pair<T, P> pop_top() {
        std::unique_lock<std::mutex> lock{m_priority_queue_mutex};
        xwrapped_type_t r = m_pq.top();
        m_pq.pop();
        return std::make_pair(r.elements, r.priority);
    }
};

NS_END2