// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <mutex>
#include <unordered_map>
#include "xbase/xns_macro.h"

NS_BEG2(top, utl)

template<typename T, class CB>
class xcallback_mgr {
 public:
    bool register_cb(T type, const CB & cb) {
        std::lock_guard<std::mutex> l(m_lock);
        auto iter1 = m_cbs.find(type);
        if (iter1 == m_cbs.end()) {
            m_cbs[type] = cb;
            return true;
        }

        assert(0);
        return false;
    }

    bool unregister_cb(T type) {
        std::lock_guard<std::mutex> l(m_lock);
        auto iter1 = m_cbs.find(type);
        if (iter1 != m_cbs.end()) {
            m_cbs.erase(iter1);
            return true;
        }

        assert(0);
        return false;
    }

    CB get_cb(T type) {
        std::lock_guard<std::mutex> l(m_lock);
        auto iter1 = m_cbs.find(type);
        if (iter1 != m_cbs.end()) {
            CB cb = iter1->second;
            return cb;
        }

        return {};
    }

 private:
    std::mutex m_lock;
    std::unordered_map<std::string, CB> m_cbs;
};


NS_END2
