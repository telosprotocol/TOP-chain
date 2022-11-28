// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xoptional.hpp"
#include "xchain_fork/xfork_points.h"

#include <cstdint>

NS_BEG2(top, chain_fork)

/**
 * @brief chain fork config center
 *
 */
class xtop_utility {
public:
    static bool is_forked(top::optional<fork_points::xfork_point_t> const& fork_point, uint64_t target) noexcept;
    static bool is_block_forked(uint64_t target) noexcept;
};
using xutility_t = xtop_utility;

NS_END2
