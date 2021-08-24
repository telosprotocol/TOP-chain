// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xmessage_bus.h"

#include <cstdint>

NS_BEG3(top, tests, mbus)

class xtop_dummy_message_bus : public top::mbus::xmessage_bus_face_t {
public:
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_message_bus);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_message_bus);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xtop_dummy_message_bus);

    void push_event(top::mbus::xevent_ptr_t const &) override {
    }

    top::mbus::xevent_ptr_t  create_event_for_store_index_to_db(base::xvbindex_t *) override {
        return nullptr;
    }
    top::mbus::xevent_ptr_t  create_event_for_revoke_index_to_db(base::xvbindex_t *) override {
        return nullptr;
    }
    top::mbus::xevent_ptr_t  create_event_for_store_block_to_db(base::xvblock_t *) override {
        return nullptr;
    }
    top::mbus::xevent_ptr_t  create_event_for_store_committed_block(base::xvbindex_t *) override {
        return nullptr;
    }

    uint32_t add_sourcer(int, top::mbus::xevent_queue_cb_t) override {
        return 0;
    }

    void remove_sourcer(int, uint32_t) override {
    }

    uint32_t add_listener(int, top::mbus::xevent_queue_cb_t) override {
        return 0;
    }

    void remove_listener(int, uint32_t) override {
    }

    void clear() override {
    }

    int size() override {
        return 0;
    }

    int sourcers_size() override {
        return 0;
    }

    int listeners_size() override {
        return 0;
    }

    top::mbus::xevent_queue_ptr_t get_queue(int) override {
        return nullptr;
    }
};
using xdummy_message_bus_t = xtop_dummy_message_bus;

NS_END3
