// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include "xmbus/xevent.h"
#include "xvnetwork/xaddress.h"
#include "xbase/xobject_ptr.h"
#include "xstatistic/xbasic_size.hpp"
#include "xstatistic/xstatistic.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, mbus)

using namespace top;

class xevent_consensus_t : public xbus_event_t, public xstatistic::xstatistic_obj_face_t {
public:

    enum _minor_type_ {
        data
    };

    xevent_consensus_t(
            int minor_type,
            direction_type dir = to_listener,
            bool _sync = true) :
    xbus_event_t(
    xevent_major_type_consensus,
    minor_type,
    dir,
    _sync),
    xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_event_consensus) {
    }

    ~xevent_consensus_t() {statistic_del();}

    virtual int32_t get_class_type() const override {return xstatistic::enum_statistic_event_consensus;}
private:
    virtual int32_t get_object_size_real() const override {
        return sizeof(*this) + get_size(get_result_data());
    }
};

using xevent_consensus_ptr_t = xobject_ptr_t<xevent_consensus_t>;

class xevent_consensus_data_t : public xevent_consensus_t {
public:

    xevent_consensus_data_t(
            base::xvblock_t* _vblock,
            bool _is_leader,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_consensus_t(
    data,
    dir,
    _sync),
    vblock_ptr(_vblock),
    is_leader(_is_leader) {
    }

    base::xauto_ptr<base::xvblock_t> vblock_ptr;
    bool is_leader;
};

using xevent_consensus_data_ptr_t = xobject_ptr_t<xevent_consensus_data_t>;

NS_END2
