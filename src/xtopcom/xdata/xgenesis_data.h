// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <map>
#include <string>
#include <vector>

#include "xcommon/xaddress.h"
#include "xcommon/xsharding_info.h"
#include <unordered_map>
#include "xdata/xdata_common.h"
#include "xdata/xblock.h"
#include "xdata/xtransaction.h"
#include "xdata/xnative_contract_address.h"
#include "xbase/xobject_ptr.h"
#include "xcommon/xip.h"

namespace top { namespace data {

// reserved address for local timer and inner used
#define sys_local_timer_addr                        "__local_timer_addr__"

// load genesis beacon block
common::xaccount_address_t make_address_by_prefix_and_subaddr(const std::string & prefix, uint16_t subaddr);
bool is_account_address(common::xaccount_address_t const & addr);
bool is_sub_account_address(common::xaccount_address_t const & addr);
bool is_sys_contract_address(common::xaccount_address_t const & addr);
bool is_black_hole_address(common::xaccount_address_t const & addr);
bool is_drand_address(common::xaccount_address_t const & addr);
bool is_user_contract_address(common::xaccount_address_t const & addr);
bool is_contract_address(common::xaccount_address_t const & addr);
bool is_beacon_contract_address(common::xaccount_address_t const & addr);
bool is_zec_contract_address(common::xaccount_address_t const & addr);
bool is_sys_sharding_contract_address(common::xaccount_address_t const & addr);
bool is_block_contract_address(common::xaccount_address_t const & addr);
bool has_contract_suffix(const std::string& addr);
base::xtable_index_t account_map_to_table_id(common::xaccount_address_t const & addr);
std::string account_address_to_block_address(common::xaccount_address_t const & addr);
bool is_table_address(common::xaccount_address_t const & addr);
bool is_unit_address(common::xaccount_address_t const & addr);


}  // namespace data
}  // namespace top
