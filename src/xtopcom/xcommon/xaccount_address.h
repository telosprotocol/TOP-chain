// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaccount_address_fwd.h"
#include "xcommon/xaccount_base_address_fwd.h"
#include "xcommon/xnode_id.h"

NS_BEG2(top, common)

bool is_t0(xaccount_address_t const & account_address);
bool is_t2(xaccount_address_t const & account_address);
bool is_t8(xaccount_address_t const & account_address);
bool is_t6(xaccount_address_t const & account_address);

bool address_belongs_to_zone(xaccount_address_t const & account_address, xzone_id_t const & target_zone_id);
xaccount_address_t append_table_id(xaccount_base_address_t const & base_address, xtable_id_t const & table_id);
xaccount_address_t append_table_id(xaccount_address_t const & address, xtable_id_t const & table_id, std::error_code & ec);
xaccount_address_t append_table_id(xaccount_address_t const & address, xtable_id_t const & table_id);

NS_END2
