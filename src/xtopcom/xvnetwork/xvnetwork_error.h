// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress_error.h"

#include <cstdint>
#include <string>
#include <system_error>

NS_BEG2(top, vnetwork)
//enum class xenum_vnetwork_errc : std::uint32_t {
//    success                             = 0,
//    invalid_argument,
//    invalid_vhost_role,
//    vnode_not_found,
//    vnode_address_empty,
//    vnode_type_invalid,
//    account_address_empty,
//    version_not_exist,
//    version_mismatch,
//    version_empty,
//    version_not_empty,
//    virtual_network_not_exist,
//    associated_parent_group_not_exist,
//    associated_child_group_not_exist,
//    cluster_has_no_child,
//    cluster_id_empty,
//    cluster_not_exist,
//    cluster_address_empty,
//    cluster_address_not_empty,
//    cluster_address_not_exist,
//    cluster_address_format_error,
//    cluster_address_not_match,
//    zone_has_no_child,
//    zone_id_empty,
//    zone_not_exist,
//    empty_result,
//    sharding_info_not_exist,
//    sharding_info_empty,
//    node_id_empty,
//    network_id_mismatch,
//    rumor_manager_not_exist,
//    bad_address_cast,
//    vhost_empty,
//    vnetwork_driver_not_found,
//    missing_crypto_keys
//};
using xvnetwork_errc_t = top::common::xaddress_errc_t;

//std::error_code
//make_error_code(xvnetwork_errc_t const errc);
//
//std::error_condition
//make_error_condition(xvnetwork_errc_t const errc);

using top::common::make_error_code;
using top::common::make_error_condition;

std::error_category const &
vnetwork_category();

//class xtop_vnetwork_error final : public std::runtime_error
//{
//private:
//    std::error_code m_ec;
//
//public:
//    xtop_vnetwork_error(xvnetwork_errc_t const errc, std::size_t const line, char const * file);
//    xtop_vnetwork_error(std::string msg, xvnetwork_errc_t const errc, std::size_t const line, char const * file);
//
//    std::error_code const &
//    code() const noexcept;
//
//    char const *
//    what() const noexcept override;
//
//private:
//    xtop_vnetwork_error(std::string msg, std::error_code ec, std::size_t const line, std::string file);
//};

// using xvnetwork_error_t = top::common::xaddress_error_t;

NS_END2

NS_BEG1(std)

//template <>
//struct is_error_code_enum<top::vnetwork::xvnetwork_errc_t> : std::true_type {
//};

NS_END1
