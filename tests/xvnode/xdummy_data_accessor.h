// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "tests/xelection/xdummy_election_cache_data_accessor.h"
#include "xelection/xcache/xnetwork_element.h"
using top::election::cache::xcluster_element_t;
using top::election::cache::xgroup_element_t;
using top::election::cache::xnetwork_element_t;
using top::election::cache::xzone_element_t;
NS_BEG3(top, tests, vnode)

class xtop_dummy_vnetwork_temp_data_accessor : public top::tests::election::xdummy_election_cache_data_accessor_t {
    common::xnetwork_id_t                                   network_id() const noexcept override { return common::xnetwork_id_t{1}; }
    std::shared_ptr<top::election::cache::xgroup_element_t> group_element_by_logic_time(common::xsharding_address_t const & sharding_address,
                                                                                        common::xlogic_time_t const         logic_time,
                                                                                        std::error_code &                   ec) const override {
        common::xelection_round_t    test_version1{1}, test_version0{0};
        common::xnetwork_id_t test_network_id{1}, test_network_id2{2};
        common::xzone_id_t    test_zone_id{1};
        common::xcluster_id_t test_cluster_id{1};
        common::xgroup_id_t   test_group_id{65};

        xnetwork_element_t                  network_elem{test_network_id};
        std::uint16_t                       sharding_size{10};
        std::shared_ptr<xnetwork_element_t> network_element_ptr(new xnetwork_element_t{test_network_id});
        std::shared_ptr<xzone_element_t>    zone_element_ptr(new xzone_element_t{test_zone_id, network_element_ptr});
        std::shared_ptr<xcluster_element_t> cluster_element_ptr(new xcluster_element_t{test_cluster_id, zone_element_ptr});
        std::shared_ptr<xgroup_element_t>   group_element_ptr(new xgroup_element_t{test_version1, test_group_id, sharding_size, 10, cluster_element_ptr});
        return group_element_ptr;
    }
    common::xnode_id_t account_address_from(common::xip2_t const & xip2, std::error_code & ec) const override { return common::xnode_id_t{"test1"}; }
    common::xelection_round_t election_epoch_from(common::xip2_t const & xip2, std::error_code & ec) const override {
        return common::xelection_round_t{1};
    }
};
using xdummy_data_accessor_t = xtop_dummy_vnetwork_temp_data_accessor;

extern xdummy_data_accessor_t xdummy_network_data_accessor;

NS_END3
