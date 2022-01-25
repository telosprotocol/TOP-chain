// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xerror/xerror.h"

#include <cstdint>
#include <string>
#include <stdexcept>
#include <system_error>
#include <type_traits>

NS_BEG2(top, common)

enum class xenum_address_errc {
    success                             = 0,
    invalid_argument,
    invalid_vhost_role,
    vnode_not_found,
    vnode_address_empty,
    vnode_type_invalid,
    account_address_empty,
    version_not_exist,
    version_mismatch,
    version_empty,
    version_not_empty,
    virtual_network_not_exist,
    associated_parent_group_not_exist,
    associated_child_group_not_exist,
    cluster_has_no_child,
    cluster_id_empty,
    cluster_not_exist,
    cluster_address_empty,
    cluster_address_not_empty,
    cluster_address_not_exist,
    cluster_address_format_error,
    cluster_address_not_match,
    zone_has_no_child,
    zone_id_empty,
    zone_not_exist,
    empty_result,
    sharding_info_not_exist,
    sharding_info_empty,
    node_id_empty,
    network_id_mismatch,
    rumor_manager_not_exist,
    bad_address_cast,
    vhost_empty,
    vnetwork_driver_not_found,
    missing_crypto_keys,
    group_id_empty,
    group_not_exist,
    group_address_format_error,
    group_address_not_match,
    protocol_error,
};
using xaddress_errc_t = xenum_address_errc;

std::error_code
make_error_code(xaddress_errc_t const errc);

std::error_condition
make_error_condition(xaddress_errc_t const errc);

std::error_category const &
address_category();

//class xtop_address_error final : public top::error::xtop_error_t
//{
//    using base_t = top::error::xtop_error_t;
//
//public:
//    xtop_address_error(xaddress_errc_t const errc, std::size_t const line, char const * file);
//    xtop_address_error(std::string msg, xaddress_errc_t const errc, std::size_t const line, char const * file);
//
//private:
//    xtop_address_error(std::string msg, std::error_code ec, std::size_t const line, std::string file);
//};
//using xaddress_error_t = xtop_address_error;

NS_END2

NS_BEG1(std)

template <>
struct is_error_code_enum<top::common::xaddress_errc_t> : std::true_type {};

NS_END1
