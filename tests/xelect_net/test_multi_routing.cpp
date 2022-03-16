#include "tests/xsystem_contract/xelection_algorithm/xtest_election_data_manager_fixture.h"
#include "xelect_net/include/elect_manager.h"
#include "xpbase/base/kad_key/kadmlia_key.h"
#include "xwrouter/multi_routing/service_node_cache.h"
#include "xwrouter/multi_routing/small_net_cache.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, elect_net)
class xtop_test_elect_net_fixture
  : public election::xtest_election_data_manager_fixture_t
  , public testing::Test {
public:
    XDECLARE_DEFAULTED_DEFAULT_CONSTRUCTOR(xtop_test_elect_net_fixture);
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_elect_net_fixture);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_elect_net_fixture);

public:
    std::shared_ptr<top::elect::ElectManager> test_elect_manager;

protected:
    void SetUp() override;
    void TearDown() override;
};

void xtop_test_elect_net_fixture::SetUp() {
    global_node_id = "test_node_prefix7";
    top::base::Config config;
    config.Set("node", "network_id", 255);
    config.Set("node", "first_node", true);
    config.Set("node", "local_ip", "127.0.0.1");
    config.Set("node", "local_port", 8080);
    config.Set("node", "node_id", global_node_id);
    test_elect_manager = std::make_shared<top::elect::ElectManager>(nullptr, config);

    wrouter::SmallNetNodes::Instance()->Init();
    wrouter::ServiceNodes::Instance();
    base::KadmliaKeyPtr kad_key_ptr = base::GetRootKadmliaKey(global_node_id);
    wrouter::MultiRouting::Instance()->CreateRootRouting(nullptr, config, kad_key_ptr);
    top::kadmlia::CreateGlobalXid(config);
}
void xtop_test_elect_net_fixture::TearDown() {
}

void create_new_routing_table(std::shared_ptr<top::elect::ElectManager> test_elect_manager,
                              const top::data::election::xelection_result_store_t & election_result_store,
                              const top::common::xzone_id_t & zid,
                              uint64_t const associated_blk_height) {
    test_elect_manager->OnElectUpdated(election_result_store, zid,associated_blk_height);
}

void try_get_routing_table(const top::base::ServiceType & service_type) {
    top::kadmlia::ElectRoutingTablePtr rt = nullptr;
    while (!rt) {
        rt = wrouter::MultiRouting::Instance()->GetElectRoutingTable(service_type);
    }
    std::vector<kadmlia::NodeInfoPtr> vec;
    rt->GetRandomNodes(vec, rt->nodes_size());
}

TEST_F(xtop_test_elect_net_fixture, TOP_4008_BENCH) {
    auto const node_type = common::xnode_type_t::consensus_auditor;
    auto const net_id = common::xnetwork_id_t{255};
    auto const zid = common::xconsensus_zone_id;
    auto const cid = common::xdefault_cluster_id;
    auto const gid = common::xdefault_group_id;
    auto const slot_id = common::xslot_id_t{7};
    std::size_t node_size = 10;
    add_nodes_to_election_result(node_size, node_type, cid, gid, "T00000LMZLAYynftsjQiKZ5W7TQnc");
    auto const election_result = election_network_result;
    data::election::xelection_result_store_t e;
    e.result_of(net_id) = election_network_result;

    for (std::size_t index = 0; index < 10000; ++index) {
        common::xip2_t xip2_{net_id, zid, cid, gid, slot_id, node_size, index};
        std::thread t2 = std::thread(&try_get_routing_table, base::GetKadmliaKey(xip2_)->GetServiceType());
        
        e.result_of(net_id).result_of(node_type).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id).group_version() =
            top::common::xelection_round_t{index};
        std::thread t1 = std::thread(&create_new_routing_table, test_elect_manager, e, zid, index *2);

        t2.join();
        t1.join();
        if (index >= 2) {
            common::xip2_t xip2_{net_id, zid, cid, gid, slot_id, node_size, index - 2};
            test_elect_manager->OnElectQuit(xip2_);
        }
    }
}

NS_END3