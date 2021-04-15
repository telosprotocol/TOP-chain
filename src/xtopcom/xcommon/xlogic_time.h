// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <limits>
#include <cstdint>

NS_BEG2(top, common)

using xlogic_time_t = std::uint64_t;

XINLINE_CONSTEXPR xlogic_time_t xjudgement_day = std::numeric_limits<xlogic_time_t>::max();

NS_END2
