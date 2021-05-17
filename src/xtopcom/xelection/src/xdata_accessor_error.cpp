// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <system_error>

#include "xelection/xdata_accessor_error.h"
#include "xbase/xns_macro.h"

NS_BEG2(top, election)

static
char const *
xdata_accessor_errc_map(int const errc) noexcept {
    auto const ec = static_cast<xdata_accessor_errc_t>(errc);
    switch (ec) {
        case xdata_accessor_errc_t::success:
            return "success";
        case xdata_accessor_errc_t::node_not_found:
            return u8"node id not found";
        case xdata_accessor_errc_t::node_already_exist:
            return u8"node already exist";
        case xdata_accessor_errc_t::node_id_empty:
            return u8"node id empty";
        case xdata_accessor_errc_t::node_id_mismatch:
            return u8"node id mismatch";
        case xdata_accessor_errc_t::node_joined_version_empty:
            return u8"node joined version empty";
        case xdata_accessor_errc_t::node_joined_version_mismatch:
            return u8"node joined version mismatch";
        case xdata_accessor_errc_t::node_staking_mismatch:
            return u8"node staking mismatch";
        case xdata_accessor_errc_t::slot_id_empty:
            return u8"slot id empty";
        case xdata_accessor_errc_t::slot_not_exist:
            return u8"slot not exist";
        case xdata_accessor_errc_t::zone_id_mismatch:
            return u8"zone id mismatch";
        case xdata_accessor_errc_t::zone_id_empty:
            return u8"zone id empty";
        case xdata_accessor_errc_t::zone_already_exist:
            return u8"zone already exist";
        case xdata_accessor_errc_t::zone_not_exist:
            return u8"zone not exist";
        case xdata_accessor_errc_t::cluster_id_mismatch:
            return u8"cluster id mismatch";
        case xdata_accessor_errc_t::cluster_id_empty:
            return u8"cluster id empty";
        case xdata_accessor_errc_t::cluster_already_exist:
            return u8"cluster already exist";
        case xdata_accessor_errc_t::cluster_not_exist:
            return u8"cluster not exist";
        case xdata_accessor_errc_t::group_id_empty:
            return u8"group id empty";
        case xdata_accessor_errc_t::group_not_exist:
            return u8"group not exist";
        case xdata_accessor_errc_t::group_already_exist:
            return u8"group already exist";
        case xdata_accessor_errc_t::group_type_mismatch:
            return u8"group type mismatch";
        case xdata_accessor_errc_t::group_version_mismatch:
            return u8"group version mismatch";
        case xdata_accessor_errc_t::group_version_empty:
            return u8"group veresion empty";
        case xdata_accessor_errc_t::group_association_failed:
            return u8"group association failed";
        case xdata_accessor_errc_t::network_id_mismatch:
            return u8"network id mismatch";
        case xdata_accessor_errc_t::associate_parent_group_twice:
            return u8"associate parent group twice";
        case xdata_accessor_errc_t::associate_to_different_parent_group:
            return u8"associate to different parent group";
        case xdata_accessor_errc_t::associate_child_group_twice:
            return u8"associate child group twice";
        case xdata_accessor_errc_t::associate_child_group_failed:
            return u8"associate child group failed";
        case xdata_accessor_errc_t::associated_group_not_exist:
            return u8"associated group not exist";
        case xdata_accessor_errc_t::invalid_node_type:
            return u8"unknown node type";
        case xdata_accessor_errc_t::election_data_empty:
            return u8"election data empty";
        case xdata_accessor_errc_t::election_data_historical:
            return u8"election data historical";
        case xdata_accessor_errc_t::election_data_partially_updated:
            return u8"election data partially updated";
        case xdata_accessor_errc_t::address_empty:
            return u8"address empty";
        case xdata_accessor_errc_t::block_height_error:
            return u8"block height error";
        case xdata_accessor_errc_t::block_is_empty:
            return u8"block is empty";
        case xdata_accessor_errc_t::lightunit_state_is_empty:
            return u8"lightunit_state is empty";
        case xdata_accessor_errc_t::election_result_is_empty:
            return u8"election_result is empty";
        case xdata_accessor_errc_t::no_children:
            return u8"no children";
        case xdata_accessor_errc_t::unknown_std_exception:
            return u8"unknown std::exception error";
        case xdata_accessor_errc_t::unknown_error:
            return u8"unknown error";
        default:
            return u8"unknown error";
    }
}

class xtop_data_accessor_category final : public std::error_category {
public:
    const char *
    name() const noexcept override {
        return u8"[election_data_accessor]";
    }

    std::string
    message(int errc) const override {
        return xdata_accessor_errc_map(errc);
    }
};
using xdata_accessor_category_t = xtop_data_accessor_category;

std::error_code
make_error_code(xdata_accessor_errc_t const errc) {
    return std::error_code{ static_cast<int>(errc), data_accessor_category() };
}

std::error_condition
make_error_condition(xdata_accessor_errc_t const errc) {
    return std::error_condition{ static_cast<int>(errc), data_accessor_category() };
}

std::error_category const &
data_accessor_category() {
    static xdata_accessor_category_t category{};
    return category;
}

NS_END2
