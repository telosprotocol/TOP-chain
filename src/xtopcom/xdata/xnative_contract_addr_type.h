// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <string>
#include <unordered_map>
#include "xdata_common.h"
#include "xcommon/xnode_type.h"
#include "xbasic/xmulti_arg_macro.h"

NS_BEG2(top, data)
using std::string;
class xcontract_addr_type {
public:
    static std::unordered_map<string, common::xnode_type_t>& get_addr_type_map() {
        static std::unordered_map<string, common::xnode_type_t> addr_type_map;
        return addr_type_map;
    }
    xcontract_addr_type(const string& name, common::xnode_type_t node_type) {
        get_addr_type_map()[name]= node_type;
    }

    static common::xnode_type_t get_type(const std::string& name) {
        auto node_type = get_addr_type_map().find(name);
        if (node_type != get_addr_type_map().end()) {
            return node_type->second;
        }
        return common::xnode_type_t::invalid;
    }
};

#define REGISTER_ADDR_TYPE(addr, type) static xcontract_addr_type COMBINE_NAME(__addr__, __COUNTER__) (addr, type)

NS_END2
