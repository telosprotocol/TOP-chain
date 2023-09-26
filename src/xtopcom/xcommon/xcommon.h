// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <string>
#include <cstddef>

NS_BEG1(top)

// block special heights
using xblock_number_t = uint64_t;

XINLINE_CONSTEXPR xblock_number_t LatestConnectBlock = static_cast<xblock_number_t>(-3);
XINLINE_CONSTEXPR xblock_number_t LatestBlock = static_cast<xblock_number_t>(-2);
XINLINE_CONSTEXPR xblock_number_t PendingBlock = static_cast<xblock_number_t>(-1);

XINLINE_CONSTEXPR char const * BlockHeightLatest = "latest";
XINLINE_CONSTEXPR char const * BlockHeightEarliest = "earliest";
XINLINE_CONSTEXPR char const * BlockHeightPending = "pending";

XINLINE_CONSTEXPR std::size_t ETH_ADDRESS_LENGTH{20};

NS_END1
