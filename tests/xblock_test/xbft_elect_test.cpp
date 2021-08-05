#include "gtest/gtest.h"
#include "xunit_service/xleader_election.h"
#include "../mock/xbft_elect_mock.hpp"
#include "../mock/xvblockstore_mock.hpp"
#include "../mock/xnetwork_mock.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xleader_election.h"
#include <functional>

namespace top {
using xunit_service::xelection_cache_face;
using xunit_service::xleader_election_face;
using xunit_service::xrandom_leader_election;
using xunit_service::xelection_wrapper;
// using xunit_service::xelection;
using mock::xelection_mock;
using mock::xblockstore_mock;

class xbft_elect_test : public testing::Test {
 protected:
    void SetUp() override {
        xelection_cache_face::elect_set elect_set;
        elect_set.push_back({1, 1});
        elect_set.push_back({2, 1});
        // m_face = std::make_shared<xelection_mock>(elect_set);
        // m_store = make_object_ptr<xblockstore_mock>();
     }

    void TearDown() override {
    }
 public:
    std::shared_ptr<xelection_cache_face> m_face;
    xobject_ptr_t<base::xvblockstore_t> m_store;
};

class xmock_network_2 : public mock::network_mock {
 public:
    xmock_network_2(top::common::xnode_address_t address) : mock::network_mock(address) {

    }
    std::vector<std::uint16_t> table_ids() const override {
        std::vector<std::uint16_t> tables{1,2,3,4,5,6,7,8,9,10};
        return tables;
    }

    std::map<common::xslot_id_t, data::xnode_info_t>
    neighbors_info2() const override {
        std::map<common::xslot_id_t, data::xnode_info_t> slot_map;
        // auto version = m_address.version();
        // for (int i = 0; i < 10; i++) {
        //     auto address = create_address(i, "T-1-xxxxdeawed");
        //     data::xnode_info_t node {
        //         data::election::xelection_info_t {},
        //         address
        //     };
        //     slot_map[address.slot_id()] = node;
        // }
        return slot_map;
    }

    static top::common::xnode_address_t create_address(int index, const std::string &account, std::size_t version = 1, bool bbeacon = false) {
        top::common::xelection_round_t           ver(version);
        common::xnetwork_id_t             nid{common::xtestnet_id};
        common::xslot_id_t                slot_id{index};
        top::common::xcluster_address_t cluster_addr;
        if (!bbeacon) {
            top::common::xcluster_address_t cluster_addr1(
                    common::xtestnet_id,
                    common::xcommittee_zone_id,
                    common::xcommittee_cluster_id,
                    common::xcommittee_group_id);
            cluster_addr = cluster_addr1;
        } else {
            top::common::xcluster_address_t cluster_addr1(
                common::xtestnet_id,
                common::xzec_zone_id,
                common::xcommittee_cluster_id,
                common::xcommittee_group_id);
            cluster_addr = cluster_addr1;
        }
        top::common::xaccount_election_address_t account_address{ common::xnode_id_t { account }, slot_id };
        top::common::xnode_address_t addr(cluster_addr, account_address, ver, 1023, 0);
        return addr;
    }
};

// class xelection_mock2 : public xelection {
// public:
//     size_t get_size() {
//         return m_elect_data.size();
//     }
// };

// TEST_F(xbft_elect_test, network_start) {
//     auto address = xmock_network_2::create_address(1, "T-1-xxxxdeawed");
//     auto net = std::make_shared<xmock_network_2>(address);
//     auto xip = xunit_service::xcons_utl::to_xip2(address);
//     auto pelection = new xelection_mock2();
//     auto ret = xelection_wrapper::on_network_start(pelection, xip, net);
//     EXPECT_TRUE(ret);
//     std::vector<uint16_t> tables;
//     EXPECT_EQ(pelection->get_tables(xip, &tables), 10);
//     xelection_cache_face::elect_set elect_set_;
//     EXPECT_EQ(pelection->get_election(xip, &elect_set_), 10);
//     auto address2 = xmock_network_2::create_address(2, "T-1-xxxxdeawed");
//     auto net2 = std::make_shared<xmock_network_2>(address2);
//     auto xip2 = xunit_service::xcons_utl::to_xip2(address2);
//     ret = xelection_wrapper::on_network_start(pelection, xip2, net2);
//     EXPECT_TRUE(ret);
//     std::vector<uint16_t> tables2;
//     EXPECT_EQ(pelection->get_tables(xip2, &tables2), 10);
//     xelection_cache_face::elect_set elect_set_2;
//     EXPECT_EQ(pelection->get_election(xip2, &elect_set_2), 10);
//     EXPECT_EQ(pelection->get_size(), 2);
//     pelection->erase(xip);
//     EXPECT_EQ(pelection->get_size(), 1);
// }

// TEST_F(xbft_elect_test, elect) {
//     xleader_election_face * pelection = new xrandom_leader_election(m_store, m_face);
//     xvip2_t xvip2_t {1, 1};
//     bool bleader = pelection->is_leader(0, "T-xxxx", xvip2_t);
//     bool bleader2 = pelection->is_leader(1, "T-xxxx", xvip2_t);
//     EXPECT_NE(bleader, bleader2);
// }
}  // namespace top
