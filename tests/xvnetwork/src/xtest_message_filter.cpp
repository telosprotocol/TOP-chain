// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xnetwork/xdummy_network_driver.h"
#include "tests/xvnetwork/xdummy_chain_timer.h"
#include "tests/xvnetwork/xdummy_data_accessor.h"
#include "tests/xvnetwork/xdummy_vhost.h"
#include "tests/xvnetwork/xtest_vhost_fixture.h"
#include "xcommon/xaddress.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xversion.h"
#include "xvnetwork/xaddress.h"
#include "xvnetwork/xmessage.h"
#include "xvnetwork/xmessage_filter_manager.h"
#include "xvnetwork/xvhost.h"
#include "xvnetwork/xvnetwork_message.h"

#include <gtest/gtest.h>

#include <list>
#include <memory>

using top::common::xaccount_election_address_t;
using top::common::xcluster_id_t;
using top::common::xgroup_id_t;
using top::common::xlogic_time_t;
using top::common::xnetwork_id_t;
using top::common::xnode_id_t;
using top::common::xsharding_address_t;
using top::common::xversion_t;
using top::common::xzone_id_t;
using top::vnetwork::xmessage_filter_manager_t;
using top::vnetwork::xmessage_t;
using top::vnetwork::xvnetwork_message_t;
using top::vnetwork::xvnetwork_message_type_t;
using top::vnetwork::xvnode_address_t;

#define FILT_SUCCESS(msg, filtered_cnt)                                                                                                                                            \
    EXPECT_TRUE((filter_mgr.filt_message(msg), msg.empty()) && filter_mgr.m_filtered_num == filtered_cnt)                                                                                 \
        << "actually filter num is " << filter_mgr.m_filtered_num << " but not " << filtered_cnt

#define FILT_MORE_THAN(msg, filtered_cnt)                                                                                                                                          \
    EXPECT_TRUE(((filter_mgr.filt_message(msg), msg.empty()) || 1) && filter_mgr.m_filtered_num >= filtered_cnt)                                                                          \
        << "passed filter num is " << filter_mgr.m_filtered_num << " no more than " << filtered_cnt

#define FILT_FAILURE(msg) EXPECT_FALSE((filter_mgr.filt_message(msg), msg.empty()))

NS_BEG3(top, tests, vnetwork)

class test_message_filter : public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(test_message_filter);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(test_message_filter);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(test_message_filter);

    void SetUp() {
        vhost_ptr->start();
        filter_mgr.start();
    }

    void TearDown(){};

    std::uint16_t const sharding_size{1024};
    std::uint16_t const associated_blk_height{0};
    xvnode_address_t    get_address(xnetwork_id_t network_id) {
        common::xsharding_address_t cluster_addr(network_id);

        common::xnode_id_t                  node_id{std::string("test1")};
        common::xaccount_election_address_t account_address{node_id, common::xslot_id_t{}};

        return common::xnode_address_t(cluster_addr, account_address);
    }
    xvnode_address_t get_address(common::xversion_t version, xnetwork_id_t network_id) {
        common::xsharding_address_t cluster_addr(network_id);

        common::xnode_id_t                  node_id{std::string("test1")};
        common::xaccount_election_address_t account_address{node_id, common::xslot_id_t{}};

        return common::xnode_address_t(cluster_addr, account_address, version, sharding_size, associated_blk_height);
    }
    xvnode_address_t get_address(common::xversion_t version, common::xsharding_address_t sharding_address) {
        common::xnode_id_t                  node_id{std::string("test1")};
        common::xaccount_election_address_t account_address{node_id, common::xslot_id_t{}};
        return common::xnode_address_t(sharding_address, account_address, version, sharding_size, associated_blk_height);
    }

    common::xversion_t    test_version1{1}, test_version0{0};
    common::xnetwork_id_t test_network_id{1}, test_network_id2{2};
    common::xzone_id_t    test_zone_id{1};
    common::xcluster_id_t test_cluster_id{1};
    common::xgroup_id_t   test_group_id{65};

    common::xnode_address_t adr_v1_net1 = get_address(test_version1, test_network_id);
    common::xnode_address_t adr_v0_net1 = get_address(test_version0, test_network_id);
    common::xnode_address_t adr_v1_net2 = get_address(test_version1, test_network_id2);
    common::xnode_address_t adr_vnull_net1 = get_address(test_network_id);
    common::xnode_address_t adr_validator_ver1 = get_address(test_version1, top::common::build_consensus_sharding_address(test_group_id, test_network_id));
    common::xnode_address_t adr_validator_ver0 = get_address(test_version0, top::common::build_consensus_sharding_address(test_group_id, test_network_id));
    common::xnode_address_t adr_validator_vernull{common::build_consensus_sharding_address(test_group_id, test_network_id),
                                                  common::xaccount_election_address_t{common::xnode_id_t{std::string{"test1"}}, common::xslot_id_t{}}};
    common::xnode_address_t adr_edge_ver1 = get_address(test_version1, top::common::build_edge_sharding_address(test_network_id));
    common::xnode_address_t adr_edge_ver0 = get_address(test_version0, top::common::build_edge_sharding_address(test_network_id));
    common::xnode_address_t adr_edge_net1_vernull{common::build_edge_sharding_address(test_network_id),
                                                  common::xaccount_election_address_t{common::xnode_id_t{std::string{"test1"}}, common::xslot_id_t{}}};
    common::xnode_address_t adr_archieve_ver1 = get_address(test_version1, top::common::build_archive_sharding_address(top::common::xarchive_group_id, test_network_id));
    common::xnode_address_t adr_broadcast_ver1 = get_address(test_version1, common::build_network_broadcast_sharding_address(test_network_id));

    top::xbyte_buffer_t       byte_buf = random_base58_bytes(20);
    top::vnetwork::xmessage_t test_msg{byte_buf, sync::xmessage_id_sync_blocks};
    top::vnetwork::xmessage_t test_empty_msg{xbyte_buffer_t{}, sync::xmessage_id_sync_blocks};

    xvnetwork_message_t get_vnetwork_message(xvnode_address_t send_adr, xvnode_address_t recv_adr, xmessage_t msg, xlogic_time_t logic_time = xlogic_time_t{0}) {
        return xvnetwork_message_t{send_adr, recv_adr, msg, logic_time};
    }

    std::shared_ptr<top::vnetwork::xvhost_t> vhost_ptr = std::make_shared<top::vnetwork::xvhost_t>(make_observer(&tests::network::xdummy_network_driver),
                                                                                                   make_observer(tests::vnetwork::xdummy_chain_timer),
                                                                                                   test_network_id,
                                                                                                   make_observer(&tests::vnetwork::xdummy_network_data_accessor));
    xmessage_filter_manager_t                filter_mgr{make_observer(vhost_ptr.get()), make_observer(&tests::vnetwork::xdummy_network_data_accessor)};
};

#if defined(DEBUG)
TEST_F(test_message_filter, test1) {
    // dummy_vhost_ptr_info
    common::xnode_id_t    hst_node_id = vhost_ptr->host_node_id();
    std::string           hst_node_id_str = hst_node_id.to_string();  // "test1"
    // common::xnetwork_id_t hst_net_id = vhost_ptr->network_id();       // 1

    common::xlogic_time_t vhost_logic_test_time = vhost_ptr->last_logic_time();
    int                   t = static_cast<xlogic_time_t>(vhost_logic_test_time);  // 10

    // filter 1:
    xvnetwork_message_t empty_message{};
    FILT_SUCCESS(empty_message, 1);

    // filter 2:
    xvnode_address_t    diff_host_node_id_vnode_adr = xvnode_address_t{common::xsharding_address_t{test_network_id},
                                                                    xaccount_election_address_t{xnode_id_t{std::string("test_diff_node_id")}, common::xslot_id_t{}},
                                                                    test_version1,
                                                                    sharding_size,
                                                                    associated_blk_height};
    xvnetwork_message_t diff_host_node_id_vmsg = get_vnetwork_message(adr_v1_net1, adr_v1_net2, test_msg);
    FILT_SUCCESS(diff_host_node_id_vmsg, 2);

    // filter 3:
    xlogic_time_t       logic_time2 = xlogic_time_t{2};
    xvnetwork_message_t logic_time_test_vmsg1 = get_vnetwork_message(adr_v1_net1, adr_vnull_net1, test_msg, logic_time2);
    FILT_SUCCESS(logic_time_test_vmsg1, 3);

    xlogic_time_t       logic_time13 = xlogic_time_t{13};
    xvnetwork_message_t logic_time_test_vmsg2 = get_vnetwork_message(adr_v1_net1, adr_vnull_net1, test_msg, logic_time13);
    FILT_SUCCESS(logic_time_test_vmsg2, 3);

    xlogic_time_t       logic_time9 = xlogic_time_t{9};
    xvnetwork_message_t logic_time_test_vmsg3 = get_vnetwork_message(adr_v1_net1, adr_vnull_net1, test_msg, logic_time9);
    FILT_MORE_THAN(logic_time_test_vmsg3, 4);

    // filter 4:
    xvnetwork_message_t validator_version_mismatch_vmsg1 = get_vnetwork_message(adr_validator_ver0, adr_validator_ver1, test_msg);
    FILT_SUCCESS(validator_version_mismatch_vmsg1, 4);
    xvnetwork_message_t validator_version_mismatch_vmsg2 = get_vnetwork_message(adr_validator_vernull, adr_validator_ver1, test_msg);
    FILT_SUCCESS(validator_version_mismatch_vmsg2, 4);
    xvnetwork_message_t validator_version_mismatch_vmsg3 = get_vnetwork_message(adr_validator_ver1, adr_validator_vernull, test_msg);
    FILT_MORE_THAN(validator_version_mismatch_vmsg3, 5);

    // need election_data_accessor return group information.
    // hard to make vnetwork_message filter unit tests.

    // filter 5:
    xvnetwork_message_t edge_to_validator_vmsg = get_vnetwork_message(adr_edge_ver1, adr_validator_ver1, test_msg);
    FILT_SUCCESS(edge_to_validator_vmsg, 5);

    // filter 6:
    xvnetwork_message_t archieve_to_validator_vmsg = get_vnetwork_message(adr_archieve_ver1, adr_validator_vernull, test_msg);
    FILT_MORE_THAN(archieve_to_validator_vmsg, 6);

    // pass all the filter
    FILT_FAILURE(archieve_to_validator_vmsg);

    xvnetwork_message_t test_pass_vmsg1 = get_vnetwork_message(adr_edge_ver1, adr_broadcast_ver1, test_msg);
    FILT_FAILURE(test_pass_vmsg1);
    xvnetwork_message_t test_pass_vmsg2 = get_vnetwork_message(adr_edge_ver1, adr_edge_ver0, test_msg);
    FILT_FAILURE(test_pass_vmsg2);
}
#endif
NS_END3
