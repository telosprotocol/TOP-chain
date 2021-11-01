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
#include "xmetrics/xmetrics.h"

NS_BEG2(top, mbus)

class xevent_behind_t : public xbus_event_t {
public:

    enum _minor_type_ {
        type_download,
        type_check,
        type_on_demand,
        type_on_demand_by_hash,
    };

    xevent_behind_t(_minor_type_ sub_type,
            direction_type dir = to_listener, bool _sync = true)
    : xbus_event_t(xevent_major_type_behind, (int) sub_type, dir, _sync) {

    }
};

using xevent_behind_ptr_t = xobject_ptr_t<xevent_behind_t>;

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
        XMETRICS_GAUGE(metrics::xsync_behind_download, 1);
    }

    std::string address;
    uint64_t start_height;
    uint64_t end_height;
    sync::enum_chain_sync_policy sync_policy;
    vnetwork::xvnode_address_t self_addr{};
    vnetwork::xvnode_address_t from_addr{};
    std::string reason;
};

//using xevent_behind_download_ptr_t = xobject_ptr_t<xevent_behind_download_t>;

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
        XMETRICS_GAUGE(metrics::xsync_behind_check, 1);
    }

    std::string address;
    std::string reason;
};

class xevent_behind_on_demand_t : public xevent_behind_t {
public:
    xevent_behind_on_demand_t(
            const std::string& _address,
            const uint64_t _start_height,
            const uint32_t _count,
            const bool _is_consensus,
            const std::string &_reason,
            bool _unit_proof = false,
            direction_type dir = to_listener,
            bool _sync = true):
    xevent_behind_t(type_on_demand, dir, _sync),
    address(_address),
    start_height(_start_height),
    count(_count),
    is_consensus(_is_consensus),
    reason(_reason),
    unit_proof(_unit_proof) {
        XMETRICS_GAUGE(metrics::xsync_behind_on_demand, 1);
    }

    std::string address;
    uint64_t start_height;
    uint32_t count;
    bool is_consensus;
    std::string reason;
    bool unit_proof;
};

class xevent_behind_on_demand_by_hash_t : public xevent_behind_t {
public:
    xevent_behind_on_demand_by_hash_t(
            const std::string& _address,
            const std::string& _hash,
            const std::string &_reason,
            direction_type dir = to_listener,
            bool _sync = true):
    xevent_behind_t(type_on_demand_by_hash, dir, _sync),
    address(_address),
    hash(_hash),
    reason(_reason) {
        XMETRICS_GAUGE(metrics::xsync_behind_on_demand_by_hash, 1);
    }

    std::string address;
    std::string hash;
    std::string reason;
};

// class sync_condition{
// public:
//     enum sync_condition_type
//     {
//         enum_sync_condition_none        = 0,
//         enum_sync_condition_block_class = 0,
//         enum_sync_condition_hash        = 1, 
//         enum_sync_condition_height      = 2, 
//     };
    
// public:
//     sync_condition() {
//         memset((uint8_t *)&m_content, 0, sizeof(m_content));
//         m_conditon_type = enum_sync_condition_block_class;
//     }

//     sync_condition(const sync_condition & obj) {
//         memcpy((uint8_t *)&m_content, (uint8_t *)&obj.m_content, sizeof(m_content));
//         m_conditon_type = obj.m_conditon_type;
//     }
    
//     sync_condition & operator = (const sync_condition & right) {
//         memcpy((uint8_t *)&m_content, (uint8_t *)&right.m_content, sizeof(m_content));
//         m_conditon_type = right.m_conditon_type;
//         return *this;
//     }

//     union {
//         struct {
//             uint8_t m_none[0];
//         } none;

//         struct {
//             uint8_t m_xvblock_class[2];
//         } block_class; //relation to sync_condition_type.enum_sync_condition_block_class

//         struct {
//             uint8_t m_hash[32]; 
//         } hash; //relation to sync_condition_type.enum_sync_condition_hash

//         struct {
//             uint8_t m_height[8];
//         } height; //relation to sync_condition_type.enum_sync_condition_height
//     } m_content;
//     sync_condition_type m_conditon_type{enum_sync_condition_block_class};
// };

// class xevent_command_on_demand_t : public xevent_behind_t {
// public:
//     xevent_command_on_demand_t(
//             const std::string& _address,
//             const sync_condition &left_of_interval,
//             const sync_condition &right_of_interval,
//             const std::string &_reason,
//             const direction_type dir = to_listener,
//             const bool _sync = true):
//     xevent_behind_t(type_on_demand, dir, _sync),
//     m_address(_address),
//     m_left_of_interval(left_of_interval),
//     m_right_of_interval(right_of_interval),
//     m_reason(_reason) {
//     }

//     std::string m_address;
//     sync_condition m_left_of_interval;
//     sync_condition m_right_of_interval;
//     std::string m_reason;
// };

NS_END2
