// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <string>
#include <system_error>
#include <type_traits>

NS_BEG2(top, election)

enum class xtop_data_accessor_errc {
    success = 0,
    node_id_empty,
    node_id_mismatch,
    node_not_found,
    node_already_exist,
    node_joined_version_empty,
    node_joined_version_mismatch,
    node_staking_mismatch,
    slot_id_empty,
    slot_not_exist,
    zone_id_mismatch,
    zone_id_empty,
    zone_already_exist,
    zone_not_exist,
    cluster_id_mismatch,
    cluster_id_empty,
    cluster_already_exist,
    cluster_not_exist,
    group_type_mismatch,
    group_id_empty,
    group_not_exist,
    group_already_exist,
    group_version_empty,
    group_version_mismatch,
    group_association_failed,
    network_id_mismatch,
    associate_parent_group_twice,
    associate_to_different_parent_group,
    associate_child_group_twice,
    associate_child_group_failed,
    associated_group_not_exist,
    invalid_node_type,
    election_data_empty,
    election_data_historical,
    election_data_partially_updated,
    address_empty,
    block_height_error,
    block_is_empty,
    lightunit_state_is_empty,
    election_result_is_empty,
    no_children,
    unknown_std_exception,
    unknown_error,
};
using xdata_accessor_errc_t = xtop_data_accessor_errc;

std::error_code
make_error_code(xdata_accessor_errc_t const errc);

std::error_condition
make_error_condition(xdata_accessor_errc_t const errc);

std::error_category const &
data_accessor_category();

NS_END2

NS_BEG1(std)

template <>
struct is_error_code_enum<top::election::xdata_accessor_errc_t> : std::true_type {};

NS_END1
