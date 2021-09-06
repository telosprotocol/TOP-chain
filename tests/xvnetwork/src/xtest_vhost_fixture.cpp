// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnetwork/xtest_vhost_fixture.h"

#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_chain_timer.h"
#include "tests/xvnetwork/xdummy_data_accessor.h"
#include "tests/xvnetwork/xdummy_message_filter_manager.h"
#include "xvnetwork/xmessage_filter_manager_face.h"
#include "xvnetwork/xvhost.h"

NS_BEG3(top, tests, vnetwork)

top::vnetwork::xvnode_address_t xvnetwork_fixture_t::get_address(common::xelection_round_t version, common::xnetwork_id_t network_id) {
    common::xsharding_address_t cluster_addr(network_id);

    common::xnode_id_t                  node_id{std::string("test1")};
    common::xaccount_election_address_t account_address{node_id, common::xslot_id_t{}};

    return common::xnode_address_t(cluster_addr, account_address, version, std::uint16_t{1024}, std::uint16_t{0});
}

top::common::xip2_t xvnetwork_fixture_t::get_xip2_address(common::xnetwork_id_t network_id) {
    return top::common::xip2_t(network_id, common::xzone_id_t{1}, common::xcluster_id_t{1}, common::xgroup_id_t{1}, common::xslot_id_t{1});
}

top::common::xip2_t xvnetwork_fixture_t::get_xip2_address(common::xnetwork_id_t      network_id,
                                                          common::xzone_id_t         zone_id,
                                                          common::xcluster_id_t      cluster_id,
                                                          common::xgroup_id_t        group_id) {
    return top::common::xip2_t{network_id, zone_id, cluster_id, group_id, common::xslot_id_t{1}};
}

top::vnetwork::xvnode_address_t xvnetwork_fixture_t::get_address(common::xelection_round_t    version,
                                                                 common::xnetwork_id_t network_id,
                                                                 common::xzone_id_t    zone_id,
                                                                 common::xcluster_id_t cluster_id,
                                                                 common::xgroup_id_t   group_id) {
    common::xsharding_address_t         cluster_addr(network_id, zone_id, cluster_id, group_id);
    common::xnode_id_t                  node_id{std::string("test1")};
    common::xaccount_election_address_t account_address{node_id, common::xslot_id_t{}};
    return common::xnode_address_t(cluster_addr, account_address, version, std::uint16_t{1024}, std::uint16_t{0});
}

top::vnetwork::xvnode_address_t xvnetwork_fixture_t::get_dst_group_address(common::xelection_round_t    version,
                                                                           common::xnetwork_id_t network_id,
                                                                           common::xzone_id_t    zone_id,
                                                                           common::xcluster_id_t cluster_id,
                                                                           common::xgroup_id_t   group_id) {
    common::xsharding_address_t cluster_addr(network_id, zone_id, cluster_id, group_id);
    return common::xnode_address_t(cluster_addr, version, std::uint16_t{1024}, std::uint16_t{0});
}

// xvhost_fixture : test vhost function
void xvhost_fixture_t::SetUp() {
    std::unique_ptr<top::vnetwork::xmessage_filter_manager_face_t> dummy_message_filter_ptr = top::make_unique<xdummy_message_filter_manager_t>();

    vhost_test_ptr = std::make_shared<top::vnetwork::xvhost_t>(make_observer(&tests::network::xdummy_network_driver),
                                                               make_observer(tests::vnetwork::xdummy_chain_timer),
                                                               common::xnetwork_id_t{1},
                                                               make_observer(&tests::vnetwork::xdummy_network_data_accessor),
                                                               std::move(dummy_message_filter_ptr));

    vhost_test_ptr->start();
}

void xvhost_fixture_t::TearDown() {
    vhost_test_ptr->stop();
}

// xvnetwork_driver_fixture: test vnetwork_driver function
void xvnetwork_driver_fixture_t::SetUp() {
    std::unique_ptr<top::vnetwork::xmessage_filter_manager_face_t> dummy_message_filter_ptr = top::make_unique<xdummy_message_filter_manager_t>();

    vhost_test_ptr = std::make_shared<top::vnetwork::xvhost_t>(make_observer(&tests::network::xdummy_network_driver),
                                                               make_observer(tests::vnetwork::xdummy_chain_timer),
                                                               common::xnetwork_id_t{1},
                                                               make_observer(&tests::vnetwork::xdummy_network_data_accessor),
                                                               std::move(dummy_message_filter_ptr));

    vnetwork_driver_test_ptr = std::make_shared<top::vnetwork::xvnetwork_driver_t>(
        make_observer(vhost_test_ptr), get_address(test_version1, test_network_id, test_zone_id, test_cluster_id, test_group_id), common::xelection_round_t{0});

    vnetwork_driver_test_ptr->start();
    vhost_test_ptr->start();
}

void xvnetwork_driver_fixture_t::TearDown() {
    vnetwork_driver_test_ptr->stop();
    vhost_test_ptr->stop();
}

NS_END3
