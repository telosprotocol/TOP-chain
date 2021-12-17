// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <limits>
#include "xbasic/xoptional.hpp"

namespace top {
    namespace chain_fork {
       /**
         * @brief chain fork point type
         *
         */
        enum class xtop_enum_fork_point_type: uint8_t {
            invalid,
            logic_time,
            drand_height,
            block_height
        };
        using xfork_point_type_t = xtop_enum_fork_point_type;

        /**
         * @brief chain fork point
         *
         */
        struct xtop_fork_point {
            xtop_fork_point() = default;
            xtop_fork_point(xfork_point_type_t type, uint64_t point, std::string str) : fork_type{type}, point{point}, description{std::move(str)} {
            }

            xfork_point_type_t fork_type{xfork_point_type_t::invalid};
            uint64_t point{std::numeric_limits<uint64_t>::max()};
            std::string description{};
        };
        using xfork_point_t = xtop_fork_point;


         /**
         * @brief chain fork config
         *
         * example:
         * 1. currrent:
         *      struct xtop_chain_fork_config {
         *           top::optional<xtop_fork_point_t>  origin_point;
         *       };
         *
         *     xchain_fork_config_t  chain_config{xtop_fork_point_type_t::drand_height, 199999881, "update tableblock for some reason"}
         *
         * 2. add a fork config
         *      struct xtop_chain_fork_config {
         *           top::optional<xtop_fork_point_t>  origin_point;
         *           top::optional<xtop_fork_point_t>  update_contract;
         *       };
         *
         *     xchain_fork_config_t  chain_config{
         *              {xtop_fork_point_type_t::drand_height, 199999881, "original fork point"},
         *              {xtop_fork_point_type_t::drand_height, 999999889, "update contract"}
         *      }
         *
         *
         *
         */
        struct xtop_chain_fork_config {
            top::optional<xfork_point_t> block_fork_point;  // should always keep block fork point
            top::optional<xfork_point_t> table_statistic_info_fork_point;
            top::optional<xfork_point_t> blacklist_function_fork_point;
            top::optional<xfork_point_t> node_initial_credit_fork_point;
        };
        using xchain_fork_config_t = xtop_chain_fork_config;
    }
}
