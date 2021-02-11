// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include <map>
#include <list>
#include <memory>
#include <functional>
#include "xmbus/xevent_ports.h"

NS_BEG2(top, mbus)

class xevent_queue_t {
public:
    uint32_t add_sourcer(xevent_queue_cb_t cb);
    void remove_sourcer(uint32_t id);
    uint32_t add_listener(xevent_queue_cb_t cb);
    void remove_listener(uint32_t id);

    void dispatch_event(const xevent_ptr_t& e);
    void clear();
    
    // monitor functions
    int sourcers_size();
    int listeners_size();
    
private:
    xevent_ports_t m_sourcers {};
    xevent_ports_t m_listeners {};
};
        
using xevent_queue_ptr_t = std::shared_ptr<xevent_queue_t>;

NS_END2

