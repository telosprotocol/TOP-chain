// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xsync/xaccount_cache.h"

NS_BEG2(top, sync)

xaccount_cache_t::xaccount_cache_t(uint32_t max_size)
:m_max_size(max_size) {
}

xaccount_cache_t::~xaccount_cache_t() {

}

void xaccount_cache_t::add(std::string const& key, xaccount_face_ptr_t const& account, bool always_stay) {
    std::unique_lock<std::mutex> lock(m_lock);
    if(always_stay) {
        m_always_stay_map[key] = account;
    } else {
        m_active_map[key] = account;
    }
    clear();
}

void xaccount_cache_t::remove(std::string const& key, uint32_t which) {
    std::unique_lock<std::mutex> lock(m_lock);
    if((WALK_MAP_ALWAYS_STAY & which) == WALK_MAP_ALWAYS_STAY) {
        m_always_stay_map.erase(key);
    }

    if((WALK_MAP_ACTIVE & which) == WALK_MAP_ACTIVE) {
        m_active_map.erase(key);
    }

    if((WALK_MAP_DONE & which) == WALK_MAP_DONE) {
        m_done_map.erase(key);
    }
}

void xaccount_cache_t::walk(walk_func _func, uint32_t which) {
    std::unique_lock<std::mutex> lock(m_lock);
    if((WALK_MAP_ALWAYS_STAY & which) == WALK_MAP_ALWAYS_STAY) {
        for(auto const& pair : m_always_stay_map) {
            _func(pair.first, pair.second);
        }
    }

    if((WALK_MAP_ACTIVE & which) == WALK_MAP_ACTIVE) {
        for(auto const& pair : m_active_map) {
            _func(pair.first, pair.second);
        }
    }

    if((WALK_MAP_DONE & which) == WALK_MAP_DONE) {
        for(auto const& pair : m_done_map) {
            _func(pair.first, pair.second);
        }
    }
}

void xaccount_cache_t::walk_remove(walk_remove_func _func, uint32_t which) {

    std::unique_lock<std::mutex> lock(m_lock);

    if((WALK_MAP_ALWAYS_STAY & which) == WALK_MAP_ALWAYS_STAY) {
        std::unordered_map<std::string, xaccount_face_ptr_t>::iterator it = m_always_stay_map.begin();
        for (; it!=m_always_stay_map.end();) {
            if (_func(it->first, it->second)) {
                m_always_stay_map.erase(it++);
            } else {
                ++it;
            }
        }
    }

    if((WALK_MAP_ACTIVE & which) == WALK_MAP_ACTIVE) {
        std::unordered_map<std::string, xaccount_face_ptr_t>::iterator it = m_active_map.begin();
        for (; it!=m_active_map.end();) {
            if (_func(it->first, it->second)) {
                m_active_map.erase(it++);
            } else {
                ++it;
            }
        }
    }

    if((WALK_MAP_DONE & which) == WALK_MAP_DONE) {
        std::unordered_map<std::string, xaccount_face_ptr_t>::iterator it = m_done_map.begin();
        for (; it!=m_done_map.end();) {
            if (_func(it->first, it->second)) {
                m_done_map.erase(it++);
            } else {
                ++it;
            }
        }
    }
}

void xaccount_cache_t::active(std::string const& key) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_done_map.find(key);
    if(it != m_done_map.end()) {
        m_active_map[key] = it->second;
        m_done_map.erase(it);
    }
}

void xaccount_cache_t::done(std::string const& key) {
    std::unique_lock<std::mutex> lock(m_lock);
    auto it = m_active_map.find(key);
    if(it != m_active_map.end()) {
        m_done_map[key] = it->second;
        m_active_map.erase(it);
    }
}

xaccount_face_ptr_t xaccount_cache_t::get(const std::string &key, uint32_t which) {
    std::unique_lock<std::mutex> lock(m_lock);

    if((WALK_MAP_ALWAYS_STAY & which) == WALK_MAP_ALWAYS_STAY) {
        auto it = m_always_stay_map.find(key);
        if (it != m_always_stay_map.end()) {
            return it->second;
        }
    }

    if((WALK_MAP_ACTIVE & which) == WALK_MAP_ACTIVE) {
        auto it = m_active_map.find(key);
        if (it != m_active_map.end()) {
            return it->second;
        }
    }

    if((WALK_MAP_DONE & which) == WALK_MAP_DONE) {
        auto it = m_done_map.find(key);
        if (it != m_done_map.end()) {
            xaccount_face_ptr_t stack = it->second;
            m_active_map[key] = it->second;
            m_done_map.erase(it);
            return stack;
        }
    }

    return nullptr;
}

void xaccount_cache_t::clear() {
    uint32_t size = m_active_map.size() + m_done_map.size();
    if(size > m_max_size) {
        if(!m_done_map.empty()) {
            size -= m_max_size;
            if(m_done_map.size() < size) {
                m_done_map.clear();
            } else {
                for(uint32_t i=0;i<size;i++) {
                    m_done_map.erase(m_done_map.begin());
                }
            }
        }
    }
}

NS_END2