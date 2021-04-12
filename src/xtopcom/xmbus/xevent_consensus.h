// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include "xmbus/xevent.h"
#include "xvnetwork/xaddress.h"
#include "xbase/xobject_ptr.h"
#include "xvledger/xvblock.h"

NS_BEG2(top, mbus)

using namespace top;

class xevent_consensus_t : public xevent_t {
public:

    enum _minor_type_ {
        data
    };

    xevent_consensus_t(
            int minor_type,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_t(
    xevent_major_type_consensus,
    minor_type,
    dir,
    _sync) {
    }
};

DEFINE_SHARED_PTR(xevent_consensus);

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

DEFINE_SHARED_PTR(xevent_consensus_data);

NS_END2
