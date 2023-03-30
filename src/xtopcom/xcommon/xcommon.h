// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xbase/xns_macro.h"

#include <string>
NS_BEG1(top)

// block special heights
using xblock_number_t = uint64_t;
static const xblock_number_t LatestConnectBlock = (xblock_number_t)-3;
static const xblock_number_t LatestBlock = (xblock_number_t)-2;
static const xblock_number_t PendingBlock = (xblock_number_t)-1;

constexpr const char* BlockHeightLatest = "latest";
constexpr const char* BlockHeightEarliest = "earliest";
constexpr const char* BlockHeightPending = "pending";

NS_END1
