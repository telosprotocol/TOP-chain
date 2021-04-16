// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <list>
#include <queue>
#include <cassert>
#include "xbase/xns_macro.h"

NS_BEG2(top, utl)

template<class T>
class xpriority_queue_t {   
public:
    
    using xqueue_wrap_t = std::shared_ptr<std::list<T>>;
    
    xpriority_queue_t(uint32_t max_priority) {
        for(uint32_t i = 0; i< max_priority; i++) {
            m_list.push_back(std::make_shared<std::list<T>>());
        }
    }
    
    void add(uint32_t priority, const T& t) {
        assert(priority >= 0 && priority < m_list.size());
        
        m_list[priority]->push_back(t);
    }
    
    bool peek(uint32_t priority, T& t) {
        return get_value(priority, t, false);
    }
    
    bool pop(uint32_t priority, T& t) {
        return get_value(priority, t, true);
    }
    
    bool peek(T& t) {
        return get_value(t, false);
    }
    
    bool pop(T& t) {
        return get_value(t, true);
    }
    
    std::vector<xqueue_wrap_t>& get_queue() {
        return m_list;
    }
    
    // monitor
    int size() {
        int count = 0;
        for(auto& l : m_list) {
            count += l->size();
        }
        return count;
    }
    
private:
    
    bool get_value(uint32_t priority, T& t, bool pop) {
        assert(priority >= 0 && priority < m_list.size());
        
        if(!m_list[priority]->empty()) {
            t = m_list[priority]->front();
            if(pop) {
                m_list[priority]->pop_front();
            }
            return true;
        }
        return false;
    }
    
    bool get_value(T& t, bool pop) {
        for(auto& q : m_list) {
            if(!q->empty()) {
                t = q->front();
                if(pop) {
                    q->pop_front();
                }
                return true;
            }
        }
        return false;
    }
    
private:
    std::vector<xqueue_wrap_t> m_list;
};
        


NS_END2