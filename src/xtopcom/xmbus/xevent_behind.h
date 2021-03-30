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
#include "xsyncbase/xsync_policy.h"

NS_BEG2(top, mbus)

class xevent_behind_t : public xevent_t {
public:

    enum _minor_type_ {
        type_download,
        type_check,
        type_on_demand,
    };

    xevent_behind_t(_minor_type_ sub_type,
            direction_type dir = to_listener, bool _sync = true)
    : xevent_t(xevent_major_type_behind, (int) sub_type, dir, _sync) {
    }
};

DEFINE_SHARED_PTR(xevent_behind);

// table
class xevent_behind_download_t : public xevent_behind_t {
public:

    xevent_behind_download_t(
            const std::string &_address,
            uint64_t _start_height,
            uint64_t _end_height,
            sync::enum_chain_sync_policy _sync_policy,
            vnetwork::xvnode_address_t _self_addr,
            vnetwork::xvnode_address_t _from_addr,
            const std::string &_reason,
            direction_type dir = to_listener,
            bool _sync = true):
    xevent_behind_t(type_download, dir, _sync),
    address(_address),
    start_height(_start_height),
    end_height(_end_height),
    sync_policy(_sync_policy),
    self_addr(_self_addr),
    from_addr(_from_addr),
    reason(_reason) {
    }

    std::string address;
    uint64_t start_height;
    uint64_t end_height;
    sync::enum_chain_sync_policy sync_policy;
    vnetwork::xvnode_address_t self_addr{};
    vnetwork::xvnode_address_t from_addr{};
    std::string reason;
};

//DEFINE_SHARED_PTR(xevent_behind_download);

class xevent_behind_check_t : public xevent_behind_t {
public:

    xevent_behind_check_t(
            const std::string& _address,
            const std::string &_reason,
            direction_type dir = to_listener,
            bool _sync = true):
    xevent_behind_t(type_check, dir, _sync),
    address(_address),
    reason(_reason) {
    }

    std::string address;
    std::string reason;
};

class xevent_behind_on_demand_t : public xevent_behind_t {
public:

    xevent_behind_on_demand_t(
            const std::string& _address,
            uint64_t _start_height,
            uint32_t _count,
            bool _is_consensus,
            const std::string &_reason,
            direction_type dir = to_listener,
            bool _sync = true):
    xevent_behind_t(type_on_demand, dir, _sync),
    address(_address),
    start_height(_start_height),
    count(_count),
    is_consensus(_is_consensus),
    reason(_reason) {
    }

    std::string address;
    uint64_t start_height;
    uint32_t count;
    bool is_consensus;
    std::string reason;
};

NS_END2
