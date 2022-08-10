// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG4(top, contract_runtime, evm, sys_contract)

XINLINE_CONSTEXPR char const * event_hex_string_transfer{"0xddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef"};
XINLINE_CONSTEXPR char const * event_hex_string_approve{"0x8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925"};
// OwnershipTransferred(address indexed oldOwner, address indexed newOwner)
XINLINE_CONSTEXPR char const * event_hex_string_ownership_transferred{"0x8be0079c531659141344cd1fd0a4f28419497f9722a3daafe3b4186f6b6457e0"};
// ControllerSet(address indexed oldController, address indexed newController)
XINLINE_CONSTEXPR char const * event_hex_string_controller_set{"0xea4ba01507e54b7b5990927a832da3ab6a71e981b5d53ffad97def0f85fcfb20"};

// ERC20 method ids:
//--------------------------------------------------
// decimals()                            => 0x313ce567
// totalSupply()                         => 0x18160ddd
// balanceOf(address)                    => 0x70a08231
// transfer(address,uint256)             => 0xa9059cbb
// transferFrom(address,address,uint256) => 0x23b872dd
// approve(address,uint256)              => 0x095ea7b3
// allowance(address,address)            => 0xdd62ed3e
// mint(address,uint256)                 => 0x40c10f19
// burnFrom(address,uint256)             => 0x79cc6790
// transferOwnership(address)            => 0xf2fde38b
// setController(address)                => 0x92eefe9b
// owner()                               => 0x8da5cb5b
// controller()                          => 0xf77c4791
//--------------------------------------------------
XINLINE_CONSTEXPR uint32_t method_id_decimals{0x313ce567};
XINLINE_CONSTEXPR uint32_t method_id_total_supply{0x18160ddd};
XINLINE_CONSTEXPR uint32_t method_id_balance_of{0x70a08231};
XINLINE_CONSTEXPR uint32_t method_id_transfer{0xa9059cbb};
XINLINE_CONSTEXPR uint32_t method_id_transfer_from{0x23b872dd};
XINLINE_CONSTEXPR uint32_t method_id_approve{0x095ea7b3};
XINLINE_CONSTEXPR uint32_t method_id_allowance{0xdd62ed3e};
XINLINE_CONSTEXPR uint32_t method_id_mint{0x40c10f19};
XINLINE_CONSTEXPR uint32_t method_id_burn_from{0x79cc6790};
XINLINE_CONSTEXPR uint32_t method_id_transfer_ownership{0xf2fde38b};
XINLINE_CONSTEXPR uint32_t method_id_set_controller{0x92eefe9b};
XINLINE_CONSTEXPR uint32_t method_id_owner{0x8da5cb5b};
XINLINE_CONSTEXPR uint32_t method_id_controller{0xf77c4791};

NS_END4
