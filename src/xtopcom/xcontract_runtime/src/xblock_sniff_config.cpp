// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_runtime/xblock_sniff_config.h"

NS_BEG2(top, contract_runtime)

xtop_sniff_broadcast_config::xtop_sniff_broadcast_config(xsniff_broadcast_type_t type, xsniff_broadcast_policy_t policy) : type{type}, policy{policy} {
}

xtop_sniff_timer_config::xtop_sniff_timer_config(uint32_t interval, std::string action) : interval{interval}, action{std::move(action)} {
}

xtop_sniff_block_config::xtop_sniff_block_config(common::xaccount_address_t const & sniff_address, common::xaccount_address_t const & action_address, std::string action)
  : sniff_address{sniff_address}, action_address{action_address}, action{std::move(action)} {
}

NS_END2
