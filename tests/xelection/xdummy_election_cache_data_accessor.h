// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xelection/xcache/xdata_accessor_face.h"

NS_BEG3(top, tests, election)

class xtop_dummy_election_cache_data_accessor : public top::election::cache::xdata_accessor_face_t {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_dummy_election_cache_data_accessor);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_dummy_election_cache_data_accessor);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_election_cache_data_accessor);

    common::xnetwork_id_t
    network_id() const noexcept override {
        return {};
    }

    std::unordered_map<common::xsharding_address_t, top::election::cache::xgroup_update_result_t>
    update_zone(common::xzone_id_t const &,
                data::election::xelection_result_store_t const &,
                std::uint64_t const,
                std::error_code &) override {
        return {};
    }

    std::map<common::xslot_id_t, data::xnode_info_t>
    sharding_nodes(common::xsharding_address_t const &,
                   common::xelection_round_t const &,
                   std::error_code &) const override {
        return {};
    }

    common::xnode_address_t
    parent_address(common::xsharding_address_t const &,
                   common::xelection_round_t const &,
                   std::error_code &) const noexcept override {
        return {};
    }

    std::shared_ptr<top::election::cache::xnode_element_t>
    node_element(common::xgroup_address_t const &,
                 common::xlogic_epoch_t const &,
                 common::xslot_id_t const &,
                 std::error_code &) const override {
        return {};
    }

    std::shared_ptr<top::election::cache::xgroup_element_t>
    group_element(common::xsharding_address_t const &,
                  common::xelection_round_t const &,
                  std::error_code &) const override {
        return {};
    }

    std::shared_ptr<top::election::cache::xgroup_element_t> group_element(common::xgroup_address_t const &, common::xlogic_epoch_t const &, std::error_code &) const override {
        return nullptr;
    }

    std::shared_ptr<top::election::cache::xgroup_element_t>
    group_element_by_logic_time(common::xsharding_address_t const &,
                  common::xlogic_time_t const,
                  std::error_code &) const override {
        return {};
    }

    std::shared_ptr<top::election::cache::xgroup_element_t> group_element_by_height(common::xgroup_address_t const &,
                                                                                    uint64_t const,
                                                                                    std::error_code &) const override {
        return {};
    }

    std::shared_ptr<top::election::cache::xgroup_element_t>
    parent_group_element(common::xsharding_address_t const &,
                         common::xelection_round_t const &,
                         std::error_code &) const override {
        return {};
    }

    std::shared_ptr<top::election::cache::xgroup_element_t> parent_group_element(common::xgroup_address_t const &, common::xlogic_epoch_t const &, std::error_code &) const override {
        return {};
    }

    common::xelection_round_t election_epoch_from(common::xip2_t const & xip2, std::error_code & ec) const override {
        return common::xelection_round_t{};
    }

    common::xnode_id_t
    account_address_from(common::xip2_t const & xip2, std::error_code & ec) const override {
        return common::xnode_id_t {};
    }

    common::xnode_address_t parent_address(common::xgroup_address_t const & child_address, common::xlogic_epoch_t const & child_logic_epoch, std::error_code & ec) const noexcept override {
        return {};
    }

    std::map<common::xslot_id_t, data::xnode_info_t> group_nodes(common::xgroup_address_t const &,
                                                                 common::xlogic_epoch_t const &,
                                                                 std::error_code &) const override {
        return {};
    }

    std::vector<common::xnode_address_t> child_addresses(common::xgroup_address_t const &,
                                                         common::xlogic_epoch_t const &,
                                                         std::error_code & ec) const noexcept {
        return {};
    }
};
using xdummy_election_cache_data_accessor_t = xtop_dummy_election_cache_data_accessor;

NS_END3
