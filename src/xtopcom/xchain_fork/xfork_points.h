// Copyright (c) 2022 - present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xoptional.hpp"

#include <cstdint>

NS_BEG2(top, fork_points)

/// @brief chain fork point type
enum class xtop_enum_fork_point_type: uint8_t {
    invalid,
    logic_time,
    drand_height,
    block_height
};
using xfork_point_type_t = xtop_enum_fork_point_type;

/// @brief chain fork point
struct xtop_fork_point {
    xtop_fork_point() = default;
    xtop_fork_point(xfork_point_type_t const type, uint64_t const point, std::string str) : fork_type{type}, point{point}, description{std::move(str)} {
    }

    xfork_point_type_t fork_type{xfork_point_type_t::invalid};
    uint64_t point{std::numeric_limits<uint64_t>::max()};
    std::string description{};
};
using xfork_point_t = xtop_fork_point;

extern top::optional<xfork_point_t> block_fork_point;  // should always keep block fork point
extern top::optional<xfork_point_t> V3_0_0_0_block_fork_point;
extern top::optional<xfork_point_t> tx_v2_fee_fork_point;
extern top::optional<xfork_point_t> partly_remove_confirm;
extern top::optional<xfork_point_t> add_rsp_id;
extern top::optional<xfork_point_t> inner_table_tx;
extern top::optional<xfork_point_t> eth_fork_point;
extern top::optional<xfork_point_t> relay_fork_point;
extern top::optional<xfork_point_t> v1_6_0_version_point;  // for v1.6.0 version control
extern top::optional<xfork_point_t> v1_7_0_block_fork_point;
extern top::optional<xfork_point_t> v1_7_0_sync_point;
extern top::optional<xfork_point_t> TEST_FORK;

NS_END2
