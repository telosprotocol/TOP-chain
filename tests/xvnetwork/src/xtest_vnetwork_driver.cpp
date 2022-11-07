// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_chain_timer.h"
#include "tests/xvnetwork/xdummy_data_accessor.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "tests/xvnetwork/xtest_vhost_fixture.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_error2.h"

using top::common::xip2_t;
using top::vnetwork::xvnetwork_errc2_t;
using top::vnetwork::xvnode_address_t;
NS_BEG3(top, tests, vnetwork)

TEST_F(xvnetwork_driver_fixture_t, test_vhost_not_run_ec) {
    xvnode_address_t           dst = get_address(test_version1, test_network_id);
    xip2_t                     dst_xip2 = get_xip2_address(test_network_id);
    xvnode_address_t           dst_group_address = get_dst_group_address(test_version1, test_network_id, test_zone_id, test_cluster_id, test_group_id);

    int & m_cnt = tests::network::xdummy_network_driver.m_counter;
    m_cnt = 0;

    // test vhost_not_run
    vhost_test_ptr->stop();
    std::error_code ec = xvnetwork_errc2_t::success;
    vnetwork_driver_test_ptr->send_to(dst_xip2, test_msg, ec);
    EXPECT_TRUE(ec == xvnetwork_errc2_t::vhost_not_run);

    ec = xvnetwork_errc2_t::success;
    vnetwork_driver_test_ptr->broadcast(dst_xip2, test_msg, ec);
    EXPECT_TRUE(ec == xvnetwork_errc2_t::vhost_not_run);

    EXPECT_EQ(m_cnt, 0);
    vhost_test_ptr->start();
}

TEST_F(xvnetwork_driver_fixture_t, test_send_to_ec) {
    std::uint32_t max_net_id = 0x00000FFF;
    xip2_t        dst_xip2 = get_xip2_address(test_network_id);
    // test: send_to->send->cnt_send_to
    int & m_cnt1 = tests::network::xdummy_network_driver.m_counter_send_to;
    m_cnt1 = 0;
    std::error_code ec = xvnetwork_errc2_t::success;
    for (auto i = 1u; i < max_net_id; ++i) {
        xvnode_address_t dst = get_address(test_version1, common::xnetwork_id_t{i}, test_zone_id, test_cluster_id, test_group_id);
        vnetwork_driver_test_ptr->send_to(dst_xip2, test_msg, ec);
        vnetwork_driver_test_ptr->send_to(dst, test_msg);
    }
    EXPECT_TRUE(ec == xvnetwork_errc2_t::success);
    EXPECT_EQ(m_cnt1, 2 * (max_net_id - 1));

    // tests: send_to->send->broadcast->cnt_spread_rumor
    top::vnetwork::xvnode_address_t empty_dst;
    int &                           m_cnt2 = tests::network::xdummy_network_driver.m_counter_spread_rumor;
    m_cnt2 = 0;
    ec = xvnetwork_errc2_t::success;
    for (auto i = 1u; i < max_net_id; ++i) {
        vnetwork_driver_test_ptr->send_to(empty_dst, test_msg);
    }
    EXPECT_TRUE(ec == xvnetwork_errc2_t::success);
    EXPECT_EQ(m_cnt2, max_net_id - 1);
}

TEST_F(xvnetwork_driver_fixture_t, test_broadcast_ec) {
    std::uint8_t    max_broadcast_count = 0xFF;
    std::error_code ec = xvnetwork_errc2_t::success;

    common::xnode_address_t src = get_address(common::xelection_round_t{1}, common::xnetwork_id_t{1}, common::xzone_id_t{1}, common::xcluster_id_t{1}, common::xgroup_id_t{1});

    // broadcast in the specified network
    xip2_t broadcast_dst_xip2_v1_net_broadcast{
        common::xnetwork_id_t{1}, common::xzone_id_t{127}, common::xcluster_id_t{1}, common::xgroup_id_t{1}, common::xslot_id_t{1023}};

    int & m_cnt = tests::network::xdummy_network_driver.m_counter_spread_rumor;
    m_cnt = 0;
    for (auto i = 0u; i < max_broadcast_count; ++i) {
        vnetwork_driver_test_ptr->broadcast(broadcast_dst_xip2_v1_net_broadcast, test_msg, ec);
        vnetwork_driver_test_ptr->broadcast(test_msg);
    }
    EXPECT_TRUE(ec == xvnetwork_errc2_t::success);
    EXPECT_EQ(m_cnt, 2 * max_broadcast_count);

    // broadcast message in the same sharding
    xip2_t broadcast_dst_xip2_v1{
        common::xnetwork_id_t{1}, common::xzone_id_t{1}, common::xcluster_id_t{1}, common::xgroup_id_t{1}, common::xslot_id_t{1023}};

    int & m_cnt2 = tests::network::xdummy_network_driver.m_counter_spread_rumor;
    m_cnt2 = 0;
    for (auto i = 0u; i < max_broadcast_count; ++i) {
        vnetwork_driver_test_ptr->broadcast(broadcast_dst_xip2_v1, test_msg, ec);
    }
    EXPECT_TRUE(ec == xvnetwork_errc2_t::success);
    EXPECT_EQ(m_cnt2, max_broadcast_count);
}

TEST(test_vnetwork_driver, init_null_vhost) {
    top::vnetwork::xvnode_address_t adr{common::xsharding_address_t{common::xnetwork_id_t{1}}};
    top::vnetwork::xvhost_face_t *  nullptr_vhost = nullptr;
    common::xlogic_time_t const     start_time = 1;
    EXPECT_THROW(std::shared_ptr<top::vnetwork::xvnetwork_driver_t> vnetwork_driver_test_ptr1 = std::make_shared<top::vnetwork::xvnetwork_driver_t>(make_observer(nullptr_vhost), adr, common::xelection_round_t{0}),
                 top::error::xtop_error_t);
    EXPECT_THROW(std::shared_ptr<top::vnetwork::xvnetwork_driver_t> vnetwork_driver_test_ptr2 =
                     std::make_shared<top::vnetwork::xvnetwork_driver_t>(make_observer(nullptr_vhost), adr, common::xelection_round_t{0}),
                 top::error::xtop_error_t);
}

TEST(test_vnetwork_driver, test_m_value) {
    top::vnetwork::xvnode_address_t                    adr{common::xsharding_address_t{common::xnetwork_id_t{1}},
                                        common::xaccount_election_address_t{common::xnode_id_t{std::string{"test1"}}, common::xslot_id_t{}},
                                        common::xelection_round_t{1},
                                        std::uint16_t{0},
                                        std::uint64_t{0}};
    std::shared_ptr<top::vnetwork::xvnetwork_driver_t> vnetwork_driver_test_ptr =
        std::make_shared<top::vnetwork::xvnetwork_driver_t>(make_observer(&tests::vnetwork::xdummy_vhost), adr, common::xelection_round_t{0});
    vnetwork_driver_test_ptr->start();
    int & m_cnt = tests::vnetwork::xdummy_vhost.m_counter;
    m_cnt = 0;

    common::xnetwork_id_t const net_id{1};
    EXPECT_EQ(vnetwork_driver_test_ptr->network_id(), net_id);
    EXPECT_STREQ(vnetwork_driver_test_ptr->host_node_id().c_str(), "test1");
    EXPECT_EQ(vnetwork_driver_test_ptr->address(), adr);
    EXPECT_EQ(m_cnt, 0);
    vnetwork_driver_test_ptr->parent_group_address();
    EXPECT_EQ(m_cnt, 1);
    vnetwork_driver_test_ptr->neighbors_info2();
    EXPECT_EQ(m_cnt, 2);
    vnetwork_driver_test_ptr->parents_info2();
    EXPECT_EQ(m_cnt, 4);
    vnetwork_driver_test_ptr->children_info2(common::xgroup_id_t{65}, common::xelection_round_t{1});
    EXPECT_EQ(m_cnt, 5);
    EXPECT_TRUE(vnetwork_driver_test_ptr->virtual_host() == make_observer(&tests::vnetwork::xdummy_vhost));
    EXPECT_TRUE(vnetwork_driver_test_ptr->type() == common::real_part_type(adr.type()));
    std::vector<xvnode_address_t> res(2, adr);
    EXPECT_EQ(res.size(), 2);
    res = vnetwork_driver_test_ptr->archive_addresses(common::xnode_type_t::storage_archive);
    EXPECT_EQ(res.size(), 1);
    vnetwork_driver_test_ptr->stop();
}

NS_END3
