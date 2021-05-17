// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cstdint>
#include <functional>
#include <map>
#include <utility>

#include "xmbus/xevent_ports.h"
#include "xbase/xlock.h"
#include "xbase/xns_macro.h"
#include "xbase/xvevent.h"
#include "xmbus/xevent.h"

NS_BEG2(top, mbus)

uint32_t xevent_ports_t::add_port(xevent_queue_cb_t cb) {
    m_lock.lock_write();
    m_ports[++m_cur_index] = cb;
    auto r = m_cur_index;
    m_lock.release_write();
    return r;
}

void xevent_ports_t::remove_port(uint32_t id) {
    m_lock.lock_write();
    m_ports.erase(id);
    m_lock.release_write();
}

void xevent_ports_t::dispatch_event(const xevent_ptr_t& e) {
    m_lock.lock_read();
    if (m_ports.empty()) {
        e->err = xevent_t::fail;
    } else {
        for (auto& entry : m_ports) {
            entry.second(e);
        }
    }
    m_lock.release_read();
}

void xevent_ports_t::clear() {
    m_lock.lock_write();
    m_ports.clear();
    m_lock.release_write();
}

int xevent_ports_t::size() {
    m_lock.lock_read();
    auto s = m_ports.size();
    m_lock.release_read();
    return s;
}

NS_END2
