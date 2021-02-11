// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <list>
#include <memory>
#include <functional>
#include "xmbus/xevent.h"
#include "xbase/xlock.h"

NS_BEG2(top, mbus)
        
using xevent_queue_cb_t = std::function<void(const xevent_ptr_t&)>;

class xevent_ports_t {
public:
    uint32_t add_port(xevent_queue_cb_t cb);
    void remove_port(uint32_t id);
    void dispatch_event(const xevent_ptr_t& e);
    void clear();
    
    // monitor functions
    int size();
    
private:
    std::map<uint32_t, xevent_queue_cb_t> m_ports;
    base::xrwlock_t m_lock;
    uint32_t m_cur_index {0};
};
        
using xevent_ports_ptr_t = std::shared_ptr<xevent_ports_t>;

NS_END2

