// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <string>
#include "xmbus/xevent.h"
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"
#include "xvnetwork/xaddress.h"

NS_BEG2(top, mbus)

// <editor-fold defaultstate="collapsed" desc="event type store">
// published by store, common to ask missed things

enum enum_behind_type {
    enum_behind_type_cold,
    enum_behind_type_common,
    enum_behind_type_nodeserv,
};

enum enum_behind_source {
    enum_behind_source_none,
    enum_behind_source_consensus,
    enum_behind_source_gossip,
    enum_behind_source_newblock,
    enum_behind_source_newblockhash,
};

class xevent_behind_t : public xevent_t {
public:

    enum _minor_type_ {
        type_known,
        type_origin,
    };

    xevent_behind_t(_minor_type_ sub_type, const std::string &_reason,
            direction_type dir = to_listener, bool _sync = true)
    : xevent_t(xevent_major_type_behind, (int) sub_type, dir, _sync)
    , reason(_reason) {
    }

    std::string reason;
};

DEFINE_SHARED_PTR(xevent_behind);

class xevent_behind_block_t : public xevent_behind_t {
public:

    xevent_behind_block_t(
            const data::xblock_ptr_t &_successor_block,
            enum_behind_source _source_type,
            const std::string &_reason,
            vnetwork::xvnode_address_t _self_addr,
            vnetwork::xvnode_address_t _from_addr,
            direction_type dir = to_listener,
            bool _sync = true)
    : xevent_behind_t(type_known, _reason, dir, _sync)
    , successor_block(_successor_block)
    , source_type(_source_type)
    , self_addr(_self_addr)
    , from_addr(_from_addr) {
    }

    data::xblock_ptr_t successor_block;
    enum_behind_source source_type;
    vnetwork::xvnode_address_t self_addr{};
    vnetwork::xvnode_address_t from_addr{};
};

DEFINE_SHARED_PTR(xevent_behind_block);

class xevent_behind_origin_t : public xevent_behind_t {
public:

    xevent_behind_origin_t(
            const std::string& _address,
            enum_behind_type _behind_type,
            const std::string &_reason,
            direction_type dir = to_listener,
            bool _sync = true):
    xevent_behind_t(type_origin, _reason, dir, _sync),
    address(_address),
    behind_type(_behind_type) {
    }

    std::string address;
    enum_behind_type behind_type;
};

NS_END2
