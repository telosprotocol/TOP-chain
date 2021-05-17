// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xcommon/xaddress.h"
#include "xbase/xns_macro.h"
#include "xcommon/xip.h"

NS_BEG3(top, data, election)

std::vector<std::string> get_property_name_by_addr(common::xaccount_address_t const & sys_contract_addr);

std::string get_property_by_group_id(common::xgroup_id_t const & group_id);

NS_END3