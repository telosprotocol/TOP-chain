// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent.h"
#include "xstatistic/xbasic_size.hpp"
#include "xstatistic/xstatistic.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, mbus)

// <editor-fold defaultstate="collapsed" desc="event type timer">

class xevent_timer_t : public xbus_event_t, public xstatistic::xstatistic_obj_face_t {
public:

    xevent_timer_t(uint32_t ms_interval = 1000,
            bool _sync = false) :
    xbus_event_t(xevent_major_type_timer,
    0,
    to_listener,
    _sync),
    xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_event_timer),
    interval(ms_interval) {
    }

    ~xevent_timer_t() {statistic_del();}

    uint32_t interval;

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_event_timer;}
private:
    virtual size_t get_object_size_real() const override {
        return sizeof(*this) + get_size(get_result_data());
    }
};

using xevent_timer_ptr_t = xobject_ptr_t<xevent_timer_t>;

class xevent_chain_timer_t : public xbus_event_t, public xstatistic::xstatistic_obj_face_t {
public:

    xevent_chain_timer_t(base::xvblock_t* _time_block) :
    xbus_event_t(xevent_major_type_chain_timer,
    0,
    to_listener,
    false),
    xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_event_chain_timer),
    time_block(_time_block) {
        if(time_block != nullptr) {
            time_block->add_ref();
        }
    }

    virtual ~xevent_chain_timer_t() {
        statistic_del();
        if(time_block != nullptr) {
            time_block->release_ref();
        }
    }

    base::xvblock_t* time_block{};

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_event_chain_timer;}
private:
    virtual size_t get_object_size_real() const override {
        return sizeof(*this) + get_size(get_result_data());
    }
};

using xevent_chain_timer_ptr_t = xobject_ptr_t<xevent_chain_timer_t>;

// </editor-fold>

NS_END2
