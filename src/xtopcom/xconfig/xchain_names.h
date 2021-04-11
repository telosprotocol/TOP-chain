// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvaccount.h"
#include "xbasic/xns_macro.h"

#include <string>

NS_BEG2(top, config)

XINLINE_CONSTEXPR char const * chain_name_mainnet{"new_horizons"};
XINLINE_CONSTEXPR char const * chain_name_testnet{"galileo"};

base::enum_xchain_id to_chainid(std::string const & chain_name);

NS_END2
