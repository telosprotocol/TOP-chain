// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchain_fork/xutility.h"

#include "xvledger/xvblock.h"   // for base::TOP_BEGIN_GMTIME

#include <cinttypes>

NS_BEG2(top, chain_fork)

bool xtop_utility::is_forked(top::optional<fork_points::xfork_point_t> const & fork_point, uint64_t const target) noexcept {
    if (!fork_point.has_value()) {
        return false;
    }

    xdbg("xtop_utility::is_forked target:%" PRIu64 ", fork point:%" PRIu64 ", %s", target, fork_point.value().point, fork_point->description.c_str());
    return target >= fork_point.value().point;
}

bool xtop_utility::is_block_forked(uint64_t const target) noexcept {
    return xtop_utility::is_forked(fork_points::v1_7_0_block_fork_point, target);
}

bool xtop_utility::is_tx_forked_by_timestamp(uint64_t const fire_timestamp) noexcept {
    auto const clock = fork_points::block_fork_point.value().point;
    auto const clock_time_stamp = clock * 10 + base::TOP_BEGIN_GMTIME;
    return fire_timestamp >= clock_time_stamp;
}

NS_END2
