// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xversion.hpp"

NS_BEG2(top, common)

struct xtag_election{};
using xelection_round_t = top::xepoch_t<xtag_election, std::uint64_t>;

NS_END2
