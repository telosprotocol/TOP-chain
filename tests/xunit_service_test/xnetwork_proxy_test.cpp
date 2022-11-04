// #include "../mock/xnetwork_mock.h"
// #include "gtest/gtest.h"
// #include "xunit_service/xnetwork_proxy.h"

// #include <functional>
// namespace top {
// using namespace xunit_service;

// class xnetwork_proxy_mock : public xnetwork_proxy {
// public:
//     virtual bool exist(const xvip2_t & xip) {
//         auto p_net = find(xip);
//         return p_net != nullptr;
//     }
// };

// class xnetwork_proxy_test : public testing::Test {
// protected:
//     void SetUp() override {}

//     void TearDown() override {}

//     top::common::xnode_address_t create_address(int index, const std::string & account, std::size_t version = 1, bool bbeacon = false) {
//         common::xelection_round_t              ver(version);
//         common::xnetwork_id_t           nid{1};
//         common::xslot_id_t              slot_id{index};
//         top::common::xcluster_address_t cluster_addr;
//         if (bbeacon) {
//             top::common::xcluster_address_t cluster_addr1(nid, common::xcommittee_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id);
//             cluster_addr = cluster_addr1;
//         } else {
//             top::common::xcluster_address_t cluster_addr1(nid, common::xzec_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id);
//             cluster_addr = cluster_addr1;
//         }
//         top::common::xaccount_election_address_t account_address{common::xnode_id_t{account}, slot_id};
//         top::common::xnode_address_t             addr(cluster_addr, account_address, ver, 1, 1);
//         return addr;
//     }

// public:
// };

// TEST_F(xnetwork_proxy_test, elect) {
//     const std::string test_node_address = top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account, 0, "node1234567890abcdef");

//     auto                                               address = create_address(0, test_node_address, 1);
//     auto                                               net = new mock::network_mock(address);
//     std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<mock::network_mock>(address);
//     xnetwork_proxy_mock                                proxy;
//     proxy.add(pnet);
//     auto xip = xcons_utl::to_xip2(pnet->address(), true);
//     EXPECT_TRUE(proxy.exist(xip));
//     xvip2_t head_xip = xip;
//     xvip2_t prev_xip = xip;
//     for (int i = 2; i < 10; i++) {
//         auto                                               address_1 = create_address(0, test_node_address, i);
//         std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet2 = std::make_shared<mock::network_mock>(address_1);
//         proxy.add(pnet2);
//         auto xip_1 = xcons_utl::to_xip2(pnet2->address(), true);
//         EXPECT_TRUE(proxy.exist(xip_1));
//         if (i == 2) {
//             prev_xip = xip_1;
//         }
//         if (i > 2) {
//             proxy.erase(head_xip);
//             EXPECT_FALSE(proxy.exist(head_xip));
//             head_xip = prev_xip;
//             prev_xip = xip_1;
//         }
//     }

//     proxy.erase(head_xip);
//     EXPECT_FALSE(proxy.exist(head_xip));
//     EXPECT_TRUE(proxy.exist(prev_xip));
//     proxy.erase(prev_xip);
//     EXPECT_FALSE(proxy.exist(prev_xip));
// }
// }  // namespace top
