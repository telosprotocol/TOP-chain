// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <unordered_map>
#include <mutex>
#include <list>
#include "xbase/xns_macro.h"

NS_BEG2(top, utl)

template<typename _Cf>
class xsync_cb_manager_t {
    using func_do_callbacks = std::function<void()>;
public:

    uint32_t add_callback(_Cf func_cb) {
        std::lock_guard<std::recursive_mutex> lock(m_lock);
        m_callbacks[++m_cur_index] = func_cb;
        return m_cur_index;
    }

    void remove_callback(uint32_t id) {
        std::lock_guard<std::recursive_mutex> lock(m_remove_lock);
        m_removed_callbacks.push_back(id);
    }

    void do_callback(func_do_callbacks dc) {
        std::lock_guard<std::recursive_mutex> lock(m_lock);

        if (!m_removed_callbacks.empty()) {
            do_remove_callbacks(); // need both m_lock & m_remove_lock
        }

        dc();

        if (!m_removed_callbacks.empty()) {
            do_remove_callbacks(); // do again
        }
    }

protected:

    void do_remove_callbacks() {
        std::lock_guard<std::recursive_mutex> lock(m_remove_lock);
        for (auto id : m_removed_callbacks) {
            m_callbacks.erase(id);
        }
        m_removed_callbacks.clear();
    }

protected:
    std::recursive_mutex m_lock;
    std::recursive_mutex m_remove_lock;
    std::list<uint32_t> m_removed_callbacks;
    std::unordered_map<uint32_t, _Cf> m_callbacks;
    uint32_t m_cur_index{0};
};

NS_END2
