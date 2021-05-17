// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stdint.h>

#include "xmbus/xevent_queue.h"
#include "xbase/xns_macro.h"
#include "xbase/xvevent.h"
#include "xmbus/xevent.h"
#include "xmbus/xevent_ports.h"

NS_BEG2(top, mbus)

uint32_t xevent_queue_t::add_sourcer(xevent_queue_cb_t cb) {
    return m_sourcers.add_port(cb);
}

void xevent_queue_t::remove_sourcer(uint32_t id) {
    m_sourcers.remove_port(id);
}

uint32_t xevent_queue_t::add_listener(xevent_queue_cb_t cb) {
    return m_listeners.add_port(cb);
}

void xevent_queue_t::remove_listener(uint32_t id) {
    return m_listeners.remove_port(id);
}

void xevent_queue_t::dispatch_event(const xevent_ptr_t& e) {
    if (e->direction == xevent_t::to_listener) {
        m_listeners.dispatch_event(e);
    } else {
        m_sourcers.dispatch_event(e);
    }
}

void xevent_queue_t::clear() {
    m_listeners.clear();
}

int xevent_queue_t::sourcers_size() {
    return m_sourcers.size();
}

int xevent_queue_t::listeners_size() {
    return m_listeners.size();
}

NS_END2
