// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cassert>
#include "xmbus/xmessage_bus.h"
#include "xmbus/xevent_timer.h"
#include "xmbus/xevent_store.h"
#include "xmetrics/xmetrics.h"
#include "xbase/xbase.h"
#include "xdata/xnative_contract_address.h"

NS_BEG2(top, mbus)

xmessage_bus_timer_t::xmessage_bus_timer_t(xmessage_bus_t* bus,
        int timer_interval_milliseconds) :
m_interval_milliseconds(timer_interval_milliseconds),
m_message_bus(bus) {
}

xmessage_bus_timer_t::~xmessage_bus_timer_t() {
    stop();
}

void xmessage_bus_timer_t::start() {
    if (m_running) return;

    m_running = true;
    m_thread = std::thread([this] {
        while (m_running) {
            std::this_thread::sleep_for(std::chrono::milliseconds(
                    this->m_interval_milliseconds));
            this->m_message_bus->push_event(make_object_ptr<xevent_timer_t>(this->m_interval_milliseconds));
        }
    });
}

void xmessage_bus_timer_t::stop() {
    if (!m_running) return;

    m_running = false;
    m_thread.join();
}

xmessage_bus_t::xmessage_bus_t(bool enable_timer,
        int timer_interval_seconds) :
m_timer(this, timer_interval_seconds) {
    xdbg("xmessage_but_t address %p", this);
    for (int i = 0; i < (int) xevent_major_type_max; i++) {
        m_queues.push_back(std::make_shared<xevent_queue_t>());
    }

    if (enable_timer) {
        m_timer.start();
    }
}

xmessage_bus_t::~xmessage_bus_t() {
    m_timer.stop();
}

uint32_t xmessage_bus_t::add_sourcer(int major_type, xevent_queue_cb_t cb) {
    assert((major_type > 0 && major_type < (int) m_queues.size()));
    return m_queues[major_type]->add_sourcer(cb);
}

void xmessage_bus_t::remove_sourcer(int major_type, uint32_t id) {
    assert((major_type > 0 && major_type < (int) m_queues.size()));
    m_queues[major_type]->remove_sourcer(id);
}

uint32_t xmessage_bus_t::add_listener(int major_type, xevent_queue_cb_t cb) {
    assert((major_type > 0 && major_type < (int) m_queues.size()));
    return m_queues[major_type]->add_listener(cb);
}

void xmessage_bus_t::remove_listener(int major_type, uint32_t id) {
    assert((major_type > 0 && major_type < (int) m_queues.size()));
    m_queues[major_type]->remove_listener(id);
}

void xmessage_bus_t::push_event(const xevent_ptr_t& e) {

    XMETRICS_COUNTER_INCREMENT("mbus_push_event_counter", 1);

    XMETRICS_TIME_RECORD("mbus_push_event_timer");
    assert((e->major_type > 0 && e->major_type < (int) m_queues.size()));
    m_queues[e->major_type]->dispatch_event(e);

}

void xmessage_bus_t::clear() {
    for(auto& l : m_queues) {
        l->clear();
    }
}

int xmessage_bus_t::size() {
    return (int) m_queues.size();
}

int xmessage_bus_t::listeners_size() {
    int s = 0;
    for (auto& l : m_queues) {
        s += l->listeners_size();
    }
    return s;
}

int xmessage_bus_t::sourcers_size() {
    int s = 0;
    for (auto& l : m_queues) {
        s += l->sourcers_size();
    }
    return s;
}

xevent_queue_ptr_t xmessage_bus_t::get_queue(int major_type) {
    assert((major_type > 0 && major_type < (int) m_queues.size()));
    return m_queues[major_type];
}

//XTODO,add implmentation for below
xevent_ptr_t  xmessage_bus_t::create_event_for_store_index_to_db(base::xvbindex_t * target_index) {
    xassert(false);  // no use now
    return nullptr;
}

xevent_ptr_t  xmessage_bus_t::create_event_for_revoke_index_to_db(base::xvbindex_t * target_index)
{
    xassert(false);  // no use now
    return nullptr;
}

xevent_ptr_t  xmessage_bus_t::create_event_for_store_block_to_db(base::xvblock_t * this_block_ptr) {
    xassert(false);  // no use now
    return nullptr;
}


xevent_ptr_t  xmessage_bus_t::create_event_for_store_committed_block(base::xvbindex_t * target_index) {
    if (target_index->get_block_level() == base::enum_xvblock_level_table || target_index->get_block_level() == base::enum_xvblock_level_root) {
        return  make_object_ptr<mbus::xevent_store_block_committed_t>(target_index->get_address(), target_index, true);
    } else {
        return nullptr;
    }
}


NS_END2
