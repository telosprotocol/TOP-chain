// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xaddress_error.h"

#include <string>

NS_BEG2(top, common)

static
char const *
xaddress_errc_map(int const errc) noexcept {
    auto const ec = static_cast<xaddress_errc_t>(errc);
    switch (ec) {
        case xaddress_errc_t::invalid_argument:
            return "invalid argument";

        case xaddress_errc_t::invalid_vhost_role:
            return "invalid vhost role";

        case xaddress_errc_t::vnode_not_found:
            return "virtual node not found";

        case xaddress_errc_t::vnode_address_empty:
            return "virtual address empty";

        case xaddress_errc_t::account_address_empty:
            return "account address empty";

        case xaddress_errc_t::vnode_type_invalid:
            return "vnode type invalid";

        case xaddress_errc_t::version_not_exist:
            return "version not exist";

        case xaddress_errc_t::version_mismatch:
            return "version mismatch";

        case xaddress_errc_t::version_empty:
            return "version is empty";

        case xaddress_errc_t::version_not_empty:
            return "version not empty";

        case xaddress_errc_t::virtual_network_not_exist:
            return "virtual network not exist";

        case xaddress_errc_t::associated_parent_group_not_exist:
            return "associated parent group not exist";

        case xaddress_errc_t::associated_child_group_not_exist:
            return "associated child groups not exist";

        case xaddress_errc_t::cluster_has_no_child:
            return "cluster has no child";

        case xaddress_errc_t::cluster_id_empty:
            return "cluster id empty";

        case xaddress_errc_t::cluster_not_exist:
            return "cluster not exist";

        case xaddress_errc_t::cluster_address_empty:
            return "cluster address empty";

        case xaddress_errc_t::cluster_address_not_empty:
            return "cluster address not empty";

        case xaddress_errc_t::cluster_address_not_exist:
            return "cluster address not exist";

        case xaddress_errc_t::cluster_address_format_error:
            return "cluster address format error";

        case xaddress_errc_t::cluster_address_not_match:
            return "cluster address of src and dst not match";

        case xaddress_errc_t::zone_has_no_child:
            return "zone has no child";

        case xaddress_errc_t::zone_id_empty:
            return "zone id empty";

        case xaddress_errc_t::zone_not_exist:
            return "zone not exist";

        case xaddress_errc_t::empty_result:
            return "empty result";

        case xaddress_errc_t::sharding_info_not_exist:
            return "sharding info not exist";

        case xaddress_errc_t::sharding_info_empty:
            return "sharding info empty";

        case xaddress_errc_t::node_id_empty:
            return "node id empty";

        case xaddress_errc_t::network_id_mismatch:
            return "network id mismatch";

        case xaddress_errc_t::rumor_manager_not_exist:
            return "rumor manager not exist";

        case xaddress_errc_t::bad_address_cast:
            return "bad address_cast";

        case xaddress_errc_t::vhost_empty:
            return "vhost empty";

        case xaddress_errc_t::vnetwork_driver_not_found:
            return "vnetwork driver not found";

        case xaddress_errc_t::missing_crypto_keys:
            return "missing crypto keys";

        case xaddress_errc_t::group_id_empty:
            return "group id empty";

        case xaddress_errc_t::group_not_exist:
            return "group not exist";

        case xaddress_errc_t::group_address_format_error:
            return "group address format error";

        case xaddress_errc_t::group_address_not_match:
            return "group address not match";

        case xaddress_errc_t::protocol_error:
            return "protocol error";

        default:
            return "virtual network unknown error";
    }
}

std::error_code
make_error_code(xaddress_errc_t const errc) {
    return std::error_code{ static_cast<int>(errc), address_category() };
}

std::error_condition
make_error_condition(xaddress_errc_t const errc) {
    return std::error_condition{ static_cast<int>(errc), address_category() };
}

class xtop_address_category final : public std::error_category
{
    char const *
    name() const noexcept override {
        return "[address] ";
    }

    std::string
    message(int errc) const override {
        return xaddress_errc_map(errc);
    }
};
using xaddress_category_t = xtop_address_category;

std::error_category const &
address_category() {
    static xaddress_category_t category{};
    return category;
}

//xtop_address_error::xtop_address_error(xaddress_errc_t const errc,
//                                       std::size_t const line,
//                                       char const * file)
//    : xtop_address_error{std::string{}, make_error_code(errc), line, file }
//{
//}
//
//xtop_address_error::xtop_address_error(std::string msg,
//                                       xaddress_errc_t const errc,
//                                       std::size_t const line,
//                                       char const * file)
//    : xtop_address_error{ std::move(msg), make_error_code(errc), line, file }
//{
//}
//
//xtop_address_error::xtop_address_error(std::string msg,
//                                       std::error_code ec,
//                                       std::size_t const line,
//                                       std::string file)
//    : base_t{ ec, file + ":" + std::to_string(line) + (msg.empty() ? msg : (": extra info: " + msg)) } {
//}

NS_END2
