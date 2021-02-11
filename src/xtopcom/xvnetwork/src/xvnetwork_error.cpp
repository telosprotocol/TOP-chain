// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvnetwork/xvnetwork_error.h"

NS_BEG2(top, vnetwork)

//static
//char const *
//xvnetwork_errc_map(int const errc) noexcept {
//    auto const ec = static_cast<xvnetwork_errc_t>(errc);
//    switch (ec) {
//        case xvnetwork_errc_t::invalid_argument:
//            return u8"invalid argument";
//
//        case xvnetwork_errc_t::invalid_vhost_role:
//            return u8"invalid vhost role";
//
//        case xvnetwork_errc_t::vnode_not_found:
//            return u8"virtual node not found";
//
//        case xvnetwork_errc_t::vnode_address_empty:
//            return u8"virtual address empty";
//
//        case xvnetwork_errc_t::account_address_empty:
//            return u8"account address empty";
//
//        case xvnetwork_errc_t::vnode_type_invalid:
//            return u8"vnode type invalid";
//
//        case xvnetwork_errc_t::version_not_exist:
//            return u8"version not exist";
//
//        case xvnetwork_errc_t::version_mismatch:
//            return u8"version mismatch";
//
//        case xvnetwork_errc_t::version_empty:
//            return u8"version is empty";
//
//        case xvnetwork_errc_t::version_not_empty:
//            return u8"version not empty";
//
//        case xvnetwork_errc_t::virtual_network_not_exist:
//            return u8"virtual network not exist";
//
//        case xvnetwork_errc_t::associated_parent_group_not_exist:
//            return u8"associated parent cluster not exist";
//
//        case xvnetwork_errc_t::associated_child_group_not_exist:
//            return u8"associated child clusters not exist";
//
//        case xvnetwork_errc_t::cluster_has_no_child:
//            return u8"cluster has no child";
//
//        case xvnetwork_errc_t::cluster_id_empty:
//            return u8"cluster id empty";
//
//        case xvnetwork_errc_t::cluster_not_exist:
//            return u8"cluster not exist";
//
//        case xvnetwork_errc_t::cluster_address_empty:
//            return u8"cluster address empty";
//
//        case xvnetwork_errc_t::cluster_address_not_empty:
//            return u8"cluster address not empty";
//
//        case xvnetwork_errc_t::cluster_address_not_exist:
//            return u8"cluster address not exist";
//
//        case xvnetwork_errc_t::cluster_address_format_error:
//            return u8"cluster address format error";
//
//        case xvnetwork_errc_t::cluster_address_not_match:
//            return u8"cluster address of src and dst not match";
//
//        case xvnetwork_errc_t::zone_has_no_child:
//            return u8"zone has no child";
//
//        case xvnetwork_errc_t::zone_id_empty:
//            return u8"zone id empty";
//
//        case xvnetwork_errc_t::zone_not_exist:
//            return u8"zone not exist";
//
//        case xvnetwork_errc_t::empty_result:
//            return u8"empty result";
//
//        case xvnetwork_errc_t::sharding_info_not_exist:
//            return u8"sharding info not exist";
//
//        case xvnetwork_errc_t::sharding_info_empty:
//            return u8"sharding info empty";
//
//        case xvnetwork_errc_t::node_id_empty:
//            return u8"node id empty";
//
//        case xvnetwork_errc_t::network_id_mismatch:
//            return u8"network id mismatch";
//
//        case xvnetwork_errc_t::rumor_manager_not_exist:
//            return u8"rumor manager not exist";
//
//        case xvnetwork_errc_t::bad_address_cast:
//            return u8"bad address_cast";
//
//        case xvnetwork_errc_t::vhost_empty:
//            return u8"vhost empty";
//
//        case xvnetwork_errc_t::vnetwork_driver_not_found:
//            return u8"vnetwork driver not found";
//
//        case xvnetwork_errc_t::missing_crypto_keys:
//            return u8"missing crypto keys";
//
//        default:
//            return u8"virtual network unknown error";
//    }
//}

//std::error_code
//make_error_code(xvnetwork_errc_t const errc) {
//    return std::error_code{ static_cast<int>(errc), vnetwork_cagegory() };
//}
//
//std::error_condition
//make_error_condition(xvnetwork_errc_t const errc) {
//    return std::error_condition{ static_cast<int>(errc), vnetwork_cagegory() };
//}

//class xtop_vnetwork_category final : public std::error_category
//{
//    char const *
//    name() const noexcept override {
//        return u8"[vnetwork] ";
//    }
//
//    std::string
//    message(int errc) const override {
//        return xvnetwork_errc_map(errc);
//    }
//};
//using xvnetwork_category_t = xtop_vnetwork_category;

std::error_category const &
vnetwork_category() {
    //static xvnetwork_category_t category{};
    //return category;
    return top::common::address_category();
}

//xtop_vnetwork_error::xtop_vnetwork_error(xvnetwork_errc_t const errc,
//                                         std::size_t const line,
//                                         char const * file)
//    : xtop_vnetwork_error{std::string{}, make_error_code(errc), line, file }
//{
//}
//
//xtop_vnetwork_error::xtop_vnetwork_error(std::string msg,
//                                         xvnetwork_errc_t const errc,
//                                         std::size_t const line,
//                                         char const * file)
//    : xtop_vnetwork_error{ std::move(msg), make_error_code(errc), line, file }
//{
//}
//
//xtop_vnetwork_error::xtop_vnetwork_error(std::string msg,
//                                         std::error_code ec,
//                                         std::size_t const line,
//                                         std::string file)
//    : std::runtime_error{ file + u8":" + std::to_string(line) + ":" + ec.message() + (msg.empty() ? msg : (". extra info: " + msg)) }
//    , m_ec{ std::move(ec) }
//{
//}
//
//std::error_code const &
//xtop_vnetwork_error::code() const noexcept {
//    return m_ec;
//}
//
//char const *
//xtop_vnetwork_error::what() const noexcept {
//    return std::runtime_error::what();
//}

NS_END2
