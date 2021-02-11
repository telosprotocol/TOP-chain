// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xmbus/xevent.h"
#include "xdata/xblock.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

class xevent_sync_executor_t : public xevent_t {
public:

    enum _minor_type_ {
        none,
        blocks,
    };

    xevent_sync_executor_t(_minor_type_ mt = none,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_t(xevent_major_type_sync_executor,
    (int) mt,
    dir,
    _sync) {
    }
};

DEFINE_SHARED_PTR(xevent_sync_executor);

class xevent_sync_response_blocks_t : public xevent_sync_executor_t {
public:

    xevent_sync_response_blocks_t(
            const std::vector<data::xblock_ptr_t> &_blocks,
            const vnetwork::xvnode_address_t& _self_address,
            const vnetwork::xvnode_address_t& _from_address,
            direction_type dir = to_listener,
            bool _sync = true) :
    xevent_sync_executor_t(xevent_sync_executor_t::blocks, dir, _sync),
    blocks(_blocks),
    self_address(_self_address),
    from_address(_from_address) {
    }

    std::vector<data::xblock_ptr_t> blocks;
    vnetwork::xvnode_address_t self_address;
    vnetwork::xvnode_address_t from_address;
};

DEFINE_SHARED_PTR(xevent_sync_response_blocks);

NS_END2
