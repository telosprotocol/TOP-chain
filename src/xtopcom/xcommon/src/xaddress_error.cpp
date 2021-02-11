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
            return u8"invalid argument";

        case xaddress_errc_t::invalid_vhost_role:
            return u8"invalid vhost role";

        case xaddress_errc_t::vnode_not_found:
            return u8"virtual node not found";

        case xaddress_errc_t::vnode_address_empty:
            return u8"virtual address empty";

        case xaddress_errc_t::account_address_empty:
            return u8"account address empty";

        case xaddress_errc_t::vnode_type_invalid:
            return u8"vnode type invalid";

        case xaddress_errc_t::version_not_exist:
            return u8"version not exist";

        case xaddress_errc_t::version_mismatch:
            return u8"version mismatch";

        case xaddress_errc_t::version_empty:
            return u8"version is empty";

        case xaddress_errc_t::version_not_empty:
            return u8"version not empty";

        case xaddress_errc_t::virtual_network_not_exist:
            return u8"virtual network not exist";

        case xaddress_errc_t::associated_parent_group_not_exist:
            return u8"associated parent group not exist";

        case xaddress_errc_t::associated_child_group_not_exist:
            return u8"associated child groups not exist";

        case xaddress_errc_t::cluster_has_no_child:
            return u8"cluster has no child";

        case xaddress_errc_t::cluster_id_empty:
            return u8"cluster id empty";

        case xaddress_errc_t::cluster_not_exist:
            return u8"cluster not exist";

        case xaddress_errc_t::cluster_address_empty:
            return u8"cluster address empty";

        case xaddress_errc_t::cluster_address_not_empty:
            return u8"cluster address not empty";

        case xaddress_errc_t::cluster_address_not_exist:
            return u8"cluster address not exist";

        case xaddress_errc_t::cluster_address_format_error:
            return u8"cluster address format error";

        case xaddress_errc_t::cluster_address_not_match:
            return u8"cluster address of src and dst not match";

        case xaddress_errc_t::zone_has_no_child:
            return u8"zone has no child";

        case xaddress_errc_t::zone_id_empty:
            return u8"zone id empty";

        case xaddress_errc_t::zone_not_exist:
            return u8"zone not exist";

        case xaddress_errc_t::empty_result:
            return u8"empty result";

        case xaddress_errc_t::sharding_info_not_exist:
            return u8"sharding info not exist";

        case xaddress_errc_t::sharding_info_empty:
            return u8"sharding info empty";

        case xaddress_errc_t::node_id_empty:
            return u8"node id empty";

        case xaddress_errc_t::network_id_mismatch:
            return u8"network id mismatch";

        case xaddress_errc_t::rumor_manager_not_exist:
            return u8"rumor manager not exist";

        case xaddress_errc_t::bad_address_cast:
            return u8"bad address_cast";

        case xaddress_errc_t::vhost_empty:
            return u8"vhost empty";

        case xaddress_errc_t::vnetwork_driver_not_found:
            return u8"vnetwork driver not found";

        case xaddress_errc_t::missing_crypto_keys:
            return u8"missing crypto keys";

        case xaddress_errc_t::group_id_empty:
            return u8"group id empty";

        case xaddress_errc_t::group_not_exist:
            return u8"group not exist";

        case xaddress_errc_t::group_address_format_error:
            return u8"group address format error";

        case xaddress_errc_t::group_address_not_match:
            return u8"group address not match";

        case xaddress_errc_t::protocol_error:
            return u8"protocol error";

        default:
            return u8"virtual network unknown error";
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
        return u8"[address] ";
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

xtop_address_error::xtop_address_error(xaddress_errc_t const errc,
                                       std::size_t const line,
                                       char const * file)
    : xtop_address_error{std::string{}, make_error_code(errc), line, file }
{
}

xtop_address_error::xtop_address_error(std::string msg,
                                       xaddress_errc_t const errc,
                                       std::size_t const line,
                                       char const * file)
    : xtop_address_error{ std::move(msg), make_error_code(errc), line, file }
{
}

xtop_address_error::xtop_address_error(std::string msg,
                                       std::error_code ec,
                                       std::size_t const line,
                                       std::string file)
    : base_t{ ec, file + u8":" + std::to_string(line) + (msg.empty() ? msg : (": extra info: " + msg)) } {
}

NS_END2
