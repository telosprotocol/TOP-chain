// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG2(top, common)

struct xtop_election_result_keepalive_strategy {
    std::size_t outdated_threshold_count;
    std::size_t faded_threshold_count;
};
using xelection_result_keepalive_strategy_t = xtop_election_result_keepalive_strategy;

XINLINE_CONSTEXPR xelection_result_keepalive_strategy_t xdefault_election_result_keepalive_strategy{ 2, 1 };
// XINLINE_CONSTEXPR xelection_result_keepalive_strategy_t xnonconsensus_election_result_keepalive_strategy{ 1, 1 };

NS_END2
