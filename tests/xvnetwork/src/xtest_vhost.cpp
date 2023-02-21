// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_chain_timer.h"
#include "tests/xvnetwork/xdummy_data_accessor.h"
#include "tests/xvnetwork/xtest_vhost_fixture.h"
#include "xbasic/xsimple_message.hpp"
#include "xcommon/xmessage_id.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_error.h"
#include "xvnetwork/xvnetwork_error2.h"

using top::vnetwork::xvnetwork_errc2_t;

NS_BEG3(top, tests, vnetwork)

TEST(test_, vhost_not_run) {
    std::shared_ptr<top::vnetwork::xvhost_t> vhost_test_ptr = std::make_shared<top::vnetwork::xvhost_t>(make_observer(&tests::network::xdummy_network_driver),
                                                                                                        make_observer(tests::vnetwork::xdummy_chain_timer),
                                                                                                        common::xnetwork_id_t{1},
                                                                                                        make_observer(&tests::vnetwork::xdummy_network_data_accessor));

    std::error_code           ec = xvnetwork_errc2_t::success;
    top::vnetwork::xmessage_t msg(xbyte_buffer_t{}, xmessage_id_sync_blocks);

    common::xaccount_election_address_t account_address{common::xnode_id_t{std::string{"test1"}}, common::xslot_id_t{}};

    std::uint16_t const sharding_size{1024};
    std::uint16_t const associated_blk_height{0};

    common::xnode_address_t src_v1(common::xsharding_address_t{common::xnetwork_id_t{1}}, account_address, common::xelection_round_t{1}, sharding_size, associated_blk_height);
    common::xnode_address_t dst_v1(common::xsharding_address_t{common::xnetwork_id_t{2}}, account_address, common::xelection_round_t{1}, sharding_size, associated_blk_height);

    common::xnode_address_t dst_group_address(common::xsharding_address_t{common::xnetwork_id_t{1}, common::xzone_id_t{1}, common::xcluster_id_t{1}, common::xgroup_id_t{64}},
                                              account_address,
                                              common::xelection_round_t{1},
                                              sharding_size,
                                              associated_blk_height);

    top::common::xip2_t dst_ec(common::xnetwork_id_t{1}, common::xzone_id_t{1}, common::xcluster_id_t{1});
    vhost_test_ptr->send(src_v1, dst_ec, msg, ec);
    EXPECT_EQ(ec, xvnetwork_errc2_t::vhost_not_run);

    ec = xvnetwork_errc2_t::success;
    vhost_test_ptr->broadcast(src_v1, dst_ec, msg, ec);
    EXPECT_EQ(ec, xvnetwork_errc2_t::vhost_not_run);
}

TEST_F(xvhost_fixture_t, func_host_node_id) {
    EXPECT_TRUE(vhost_test_ptr->host_node_id() == common::xnode_id_t{"test1"});
}

TEST_F(xvhost_fixture_t, broadcast_address_empty) {
    common::xnode_address_t                src_v1 = get_address(test_version1, test_network_id);
    common::xnode_address_t                dst_empty_account_address = get_dst_group_address(test_version1, test_network_id, test_zone_id, test_cluster_id, test_group_id);

    EXPECT_TRUE(dst_empty_account_address.account_address().empty());
    EXPECT_FALSE(dst_empty_account_address.empty());
    EXPECT_THROW(vhost_test_ptr->send(test_msg, src_v1, dst_empty_account_address), top::error::xtop_error_t);
}

TEST_F(xvhost_fixture_t, func_msg) {
    common::xnode_address_t src_v1 = get_address(test_version1, test_network_id);
    common::xnode_address_t src_v2 = get_address(test_version0, test_network_id);
    common::xnode_address_t dst_v1 = get_address(test_version1, test_network_id2);
    common::xnode_address_t empty_dst;
    common::xnode_address_t dst_group_address = get_dst_group_address(test_version1, test_network_id, test_zone_id, test_cluster_id, test_group_id);
    int &                   m_cnt = tests::network::xdummy_network_driver.m_counter;
    m_cnt = 0;

    vhost_test_ptr->send(test_msg, src_v2, dst_v1);  // xinfo : version different send + 1
    vhost_test_ptr->send(test_msg, src_v1, dst_v1);  // normal send from src to dst send + 1
    EXPECT_EQ(m_cnt, 2);
    vhost_test_ptr->send(test_msg, src_v1, empty_dst);  // empty_dst : broadcast
    EXPECT_EQ(m_cnt, 3);
}

TEST_F(xvhost_fixture_t, func_msg_ec) {
    common::xnode_address_t    src_v1 = get_address(test_version1, test_network_id);
    common::xnode_address_t    src_v2 = get_address(test_version0, test_network_id);
    common::xnode_address_t    dst_v1 = get_address(test_version1, test_network_id2);
    common::xip2_t             dst_xip2_v1 = get_xip2_address(test_network_id2);

    common::xip2_t          empty_dst;
    common::xnode_address_t dst_group_address = get_dst_group_address(test_version1, test_network_id, test_zone_id, test_cluster_id, test_group_id);
    int &                   m_cnt = tests::network::xdummy_network_driver.m_counter;
    int &                   m_cnt_send_to = tests::network::xdummy_network_driver.m_counter_send_to;
    int &                   m_cnt_spread_rumor = tests::network::xdummy_network_driver.m_counter_spread_rumor;
    int &                   m_cnt_forward_broadcast = tests::network::xdummy_network_driver.m_counter_forward_broadcast;
    m_cnt = m_cnt_spread_rumor = m_cnt_forward_broadcast = m_cnt_send_to = 0;

    std::error_code ec = xvnetwork_errc2_t::success;

    vhost_test_ptr->send(src_v2, dst_xip2_v1, test_msg, ec);  // xinfo : version different send + 1
    vhost_test_ptr->send(src_v1, dst_xip2_v1, test_msg, ec);  // normal send from src to dst send + 1
    EXPECT_EQ(m_cnt_send_to, 2);
    EXPECT_EQ(ec, xvnetwork_errc2_t::success);

    // broadcast message in the same sharding
    common::xip2_t broadcast_dst_xip2_v1 =
        top::common::xip2_t{common::xnetwork_id_t{1}, common::xzone_id_t{1}, common::xcluster_id_t{1}, common::xgroup_id_t{1}, common::xslot_id_t{1023}};
    common::xnode_address_t src = get_address(common::xelection_round_t{1}, common::xnetwork_id_t{1}, common::xzone_id_t{1}, common::xcluster_id_t{1}, common::xgroup_id_t{1});
    vhost_test_ptr->broadcast(src, broadcast_dst_xip2_v1, test_msg, ec);
    EXPECT_EQ(m_cnt_spread_rumor, 1);

    // broadcast in the specified network
    common::xip2_t broadcast_dst_xip2_v1_net_broadcast =
        top::common::xip2_t{common::xnetwork_id_t{1}, common::xzone_id_t{127}, common::xcluster_id_t{1}, common::xgroup_id_t{1}, common::xslot_id_t{1023}};
    vhost_test_ptr->broadcast(src, broadcast_dst_xip2_v1_net_broadcast, test_msg, ec);
    EXPECT_EQ(m_cnt_spread_rumor, 2);

    // broadcast between different shradings
    EXPECT_EQ(m_cnt_spread_rumor, 2);
    vhost_test_ptr->broadcast(src_v1, broadcast_dst_xip2_v1, test_msg, ec);
    EXPECT_EQ(m_cnt_spread_rumor, 3);
    EXPECT_EQ(m_cnt, 5);
    EXPECT_EQ(ec, xvnetwork_errc2_t::success);
}

NS_END3
