// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xcommon/xrole_type.h"
#include "xvnetwork/xaddress.h"
#include "xdata/xgenesis_data.h"
#include "xcommon/xnode_type.h"
#include "xcommon/xsharding_info.h"

NS_BEG2(top, contract)

// using namespace top::common;
// using namespace top::data;

class xcontract_address_map_t {
public:
    /**
     * @brief calc cluster address
     *
     * @param address
     * @param table_id
     * @return common::xaccount_address_t
     */
    static common::xaccount_address_t calc_cluster_address(common::xaccount_address_t const &address, const uint32_t table_id) {
        if (data::is_sys_contract_address(address)) {
            return data::make_address_by_prefix_and_subaddr(address.to_string(), (uint16_t)table_id);
        }
        return address;
    }

    /**
     * @brief check if monitor_address is prefix of cluster_address
     *
     * @param cluster_address
     * @param monitor_address
     * @return true
     * @return false
     */
    static bool match(common::xaccount_address_t const &cluster_address, common::xaccount_address_t const &monitor_address) {
        if (cluster_address == monitor_address) {
            return true;
        }

        if (cluster_address.to_string().size() == monitor_address.to_string().size() + TOP_ADDR_TABLE_ID_SUFFIX_LENGTH) {
            return cluster_address.to_string().find(monitor_address.to_string()) == 0;
        }

        return false;
    }

};

NS_END2
