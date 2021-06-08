// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xmessage_bus.h"

using top::mbus::xevent_ptr_t;
using top::mbus::xevent_queue_cb_t;
using top::mbus::xevent_queue_ptr_t;

NS_BEG3(top, tests, vnode)

class xtop_dummy_message_bus : public top::mbus::xmessage_bus_face_t {

    uint32_t add_sourcer(int major_type, xevent_queue_cb_t cb) override { return 0; };
    void remove_sourcer(int major_type, uint32_t id) override{};
    uint32_t add_listener(int major_type, xevent_queue_cb_t cb) override { return 0; };
    void remove_listener(int major_type, uint32_t id) override{};

    void push_event(const xevent_ptr_t & e) override{};
    void clear() override{};

    int size() override { return 0; };
    int sourcers_size() override { return 0; };
    int listeners_size() override { return 0; };
    xevent_queue_ptr_t get_queue(int major_type) override { return {}; };

    xevent_ptr_t  create_event_for_store_index_to_db(base::xvbindex_t * target_index) override {
        return nullptr;
    }
    xevent_ptr_t  create_event_for_revoke_index_to_db(base::xvbindex_t *) override {
        return nullptr;
    }
    xevent_ptr_t  create_event_for_store_block_to_db(base::xvblock_t *) override {
        return nullptr;
    }
    xevent_ptr_t  create_event_for_store_committed_block(base::xvbindex_t * target_index) override {
        return nullptr;
    }
};

using xdummy_message_bus_t = xtop_dummy_message_bus;

extern xdummy_message_bus_t xdummy_message_bus;

NS_END3
