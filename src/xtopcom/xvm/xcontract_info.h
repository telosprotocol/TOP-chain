// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <cassert>
#include <string>
#include <unordered_map>
#include <vector>

#include "xbase/xlog.h"
#include "xcommon/xaddress.h"
#include "xcommon/xnode_type.h"
#include "xconfig/xconfig_register.h"

NS_BEG2(top, contract)

enum class enum_call_action_way_t {
    consensus, // nomal consensus way
    direct // no consensus way
};

enum class enum_monitor_type_t {
    timer,
    normal
};

enum class enum_broadcast_policy_t {
    invalid,
    normal,
    fullunit
};

struct xblock_monitor_info_t {
    xblock_monitor_info_t(common::xaccount_address_t const & _monitor_address,
            const std::string& _action,
            enum_call_action_way_t _call_way,
            enum_monitor_type_t _type = enum_monitor_type_t::normal) :
    type(_type),
    monitor_address(_monitor_address),
    action(_action),
    call_way(_call_way) {}

    enum_monitor_type_t type;
    common::xaccount_address_t const monitor_address {};
    std::string action {};
    enum_call_action_way_t call_way{enum_call_action_way_t::consensus};

    virtual xblock_monitor_info_t* clone() {
        return new xblock_monitor_info_t(monitor_address, action, call_way, type);
    }
    virtual ~xblock_monitor_info_t() {}
};

struct xtimer_block_monitor_info_t : public xblock_monitor_info_t {
public:
    xtimer_block_monitor_info_t(common::xaccount_address_t const & _monitor_address,
            const std::string& _action, enum_call_action_way_t _call_way, uint32_t interval, const std::string& _conf_interval = "") :
    xblock_monitor_info_t(_monitor_address, _action, _call_way, enum_monitor_type_t::timer),
    timer_interval(interval),
    conf_interval(_conf_interval) {}

    uint32_t get_interval() {
        if(timer_interval != 0) return timer_interval;
        if(!conf_interval.empty()) {
            uint32_t interval{};
            if(config::xconfig_register_t::get_instance().get(conf_interval, interval)) {
                xwarn("[xtimer_block_monitor_info_t::get_interval] key (%s) %d", conf_interval.c_str(), interval);
                return interval;
            }
        }
        //xwarn("[xtimer_block_monitor_info_t::get_interval] key (%s) not exist in config register", conf_interval.c_str());
        assert(false);
        return 0;
    }

    virtual ~xtimer_block_monitor_info_t() {}

    xblock_monitor_info_t* clone() override {
        return new xtimer_block_monitor_info_t(monitor_address, action, call_way, timer_interval, conf_interval);
    }

protected:
    uint32_t timer_interval{};   // if timer_interval = 0, then get interval from config register
    std::string conf_interval{};
};

struct xcontract_info_t {

    xcontract_info_t(
            common::xaccount_address_t const & _address,
            common::xnode_type_t _roles,
            common::xnode_type_t _broadcast_types = common::xnode_type_t::invalid,
            enum_broadcast_policy_t _broadcast_policy = enum_broadcast_policy_t::invalid) :
    address(_address),
    roles(_roles),
    broadcast_types(_broadcast_types),
    broadcast_policy(_broadcast_policy){
    }

    xcontract_info_t(const xcontract_info_t& info) {
        address = info.address;
        roles = info.roles;
        broadcast_types = info.broadcast_types;
        broadcast_policy = info.broadcast_policy;
        for(auto& pair : info.monitor_map) {
            monitor_map[pair.first] = pair.second->clone();
        }
    }

    virtual ~xcontract_info_t() {
        for (auto& pair : monitor_map) {
            delete pair.second;
        }
        monitor_map.clear();
    }

    inline
    void add_block_monitor(common::xaccount_address_t const & monitor_address,
            const std::string& action, enum_call_action_way_t call_way) {
        monitor_map[monitor_address] = new xblock_monitor_info_t(monitor_address, action, call_way);
    }

    inline
    void add_timer_monitor(common::xaccount_address_t const & monitor_address,
            const std::string& action, enum_call_action_way_t call_way, uint32_t interval, const std::string& conf_interval) {
        assert(interval != 0 || !conf_interval.empty());
        monitor_map[monitor_address] = new xtimer_block_monitor_info_t(monitor_address, action, call_way, interval, conf_interval);
    }

    // only timer blockchain allowed
    inline
    void add_local_timer_monitor(common::xaccount_address_t const & monitor_address, const std::string& action) {
        monitor_map[monitor_address] = new xblock_monitor_info_t(monitor_address, action, enum_call_action_way_t::consensus);
    }
    inline
    xblock_monitor_info_t* find(common::xaccount_address_t const & monitor_address) {
        auto it = monitor_map.find(monitor_address);
        if(it != monitor_map.end()) {
            return it->second;
        }
        return nullptr;
    }

    inline
    bool has_monitors() {
        return has_block_monitors() || has_broadcasts();
    }

    inline
    bool has_block_monitors() {
        return !monitor_map.empty();
    }

    inline
    bool has_broadcasts() {
        return broadcast_types != common::xnode_type_t::invalid;
    }

    common::xaccount_address_t address{}; // contract address
    common::xnode_type_t roles{ common::xnode_type_t::invalid }; // relative roles
    common::xnode_type_t broadcast_types{ common::xnode_type_t::invalid };
    enum_broadcast_policy_t broadcast_policy{ enum_broadcast_policy_t::invalid };


    std::unordered_map<common::xaccount_address_t, xblock_monitor_info_t*> monitor_map;
};

NS_END2
