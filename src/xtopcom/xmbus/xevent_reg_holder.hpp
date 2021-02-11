// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include "xbasic/xmemory.hpp"
#include "xmbus/xmessage_bus.h"
#include "xmessage_bus.h"

NS_BEG2(top, mbus)

class xevent_reg_holder_t {
public:

    xevent_reg_holder_t(observer_ptr<xmessage_bus_face_t> const & bus) :
    m_message_bus(bus) {
    }

    virtual ~xevent_reg_holder_t() {
        clear();
    }

    void add_listener(int major_type, xevent_queue_cb_t listener) {
        m_listener_ids.push_back(
                m_message_bus->add_listener(major_type, listener));
        m_major_types.push_back(major_type);
    }

    void clear() {
        for (int i = 0; i < (int) m_listener_ids.size(); i++) {
            m_message_bus->remove_listener(
                    m_major_types[i], m_listener_ids[i]);
        }
        m_listener_ids.clear();
        m_major_types.clear();
    }

    observer_ptr<xmessage_bus_face_t> get_mbus() {
        return m_message_bus;
    }

protected:
    observer_ptr<xmessage_bus_face_t> m_message_bus;
    std::vector<uint32_t> m_listener_ids;
    std::vector<int> m_major_types;
};

NS_END2
