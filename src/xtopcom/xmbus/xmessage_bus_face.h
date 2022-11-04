
// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xcxx_config.h"
#include "xbase/xns_macro.h"
#include "xmbus/xevent_ports.h"
#include "xmbus/xevent_queue.h"
#include "xvledger/xveventbus.h"

NS_BEG2(top, mbus)

class xtop_message_bus_face : public base::xveventbus_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_message_bus_face);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_message_bus_face);
    XDECLARE_DEFAULTED_VIRTULA_DESTRUCTOR(xtop_message_bus_face);

    virtual uint32_t add_sourcer(int major_type, xevent_queue_cb_t cb) = 0;
    virtual void remove_sourcer(int major_type, uint32_t id) = 0;
    virtual uint32_t add_listener(int major_type, xevent_queue_cb_t cb) = 0;
    virtual void remove_listener(int major_type, uint32_t id) = 0;

    virtual void clear() = 0;

    virtual int size() = 0;
    virtual int sourcers_size() = 0;
    virtual int listeners_size() = 0;
    virtual xevent_queue_ptr_t get_queue(int major_type) = 0;
};
using xmessage_bus_face_t = xtop_message_bus_face;

NS_END2