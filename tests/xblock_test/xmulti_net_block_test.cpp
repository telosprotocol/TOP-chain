#include "xblock_common.hpp"
namespace top {
namespace test {
    //
class xmulti_net_block_test : public testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {
        // delete m_engines;
    }

    top::common::xnode_address_t create_address(int index, const std::string &account, common::xnode_type_t node_type, std::size_t version = 1) {
        // std::size_t                       id = 1;
        top::common::xelection_round_t           ver(version);
        common::xnetwork_id_t             nid{common::xtestnet_id};
        common::xslot_id_t                slot_id{index};
        top::common::xcluster_address_t cluster_addr;
        switch (node_type) {
            case common::xnode_type_t::auditor:
                cluster_addr = top::common::xcluster_address_t {
                    nid, common::xconsensus_zone_id, common::xdefault_cluster_id, common::xauditor_group_id_begin
                };
            break;
            case common::xnode_type_t::validator:
                cluster_addr = top::common::xcluster_address_t {
                    nid, common::xconsensus_zone_id, common::xdefault_cluster_id, common::xvalidator_group_id_begin
                };
            break;
            case common::xnode_type_t::rec:
                cluster_addr = top::common::xcluster_address_t {
                    nid, common::xcommittee_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id
                };
            break;
            case common::xnode_type_t::zec:
                cluster_addr = top::common::xcluster_address_t {
                    nid, common::xzec_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id
                };
            break;
            default:
            break;
        }

        top::common::xaccount_election_address_t account_address{common::xnode_id_t{account}, slot_id};
        top::common::xnode_address_t   addr(cluster_addr, account_address, ver, 1023, 0);
        return addr;
    }

    std::vector<common::xnode_type_t> parse_node_type(common::xnode_type_t node_types) {
        std::vector<common::xnode_type_t> node_type_set;
        for (int i = 0; i < 5; i++) {
            auto node_type = ((common::xnode_type_t)(1 << i)) & node_types;
            if (node_type != common::xnode_type_t::invalid) {
                node_type_set.push_back(node_type);
            }
        }
        return node_type_set;
    }

    std::vector<top::common::xnode_address_t> build_cross_networks(size_t node_size,
                                                common::xnode_type_t node_types, const std::string & account_prefix,
                                                std::size_t version = 1) {
        std::vector<top::common::xnode_address_t> address_set;
        auto parse_node_types = parse_node_type(node_types);
        for (size_t i = 0; i < parse_node_types.size(); i++) {
            for (size_t j = 0; j < node_size; j++) {
                auto account = account_prefix;
                account = account.append("_").append(std::to_string(i)).append("_").append(std::to_string(j));
                auto node_address = create_address(j, account, parse_node_types[i], version);
                auto xip = xcons_utl::to_xip2(node_address);
                xinfo("[xunitservice] create account %s with xip %" PRIu64 "", account.c_str(), xip.low_addr);
                address_set.push_back(node_address);
            }
        }
        return address_set;
    }

};

// TEST_F(xmulti_net_block_test, run) {
//     std::vector<xcons_dispatcher_builder_ptr>   m_dispatcher_builders;
//     std::string node_publick_addr = "node1234567890abcdef";
//     const std::string node_address = top::base::xvaccount_t::make_account_address(
//                                         top::base::enum_vaccount_addr_type_secp256k1_user_account,
//                                         0, node_publick_addr);
//     auto address_set = build_cross_networks(4, common::xnode_type_t::validator|common::xnode_type_t::auditor, node_address);
//     EXPECT_EQ(address_set.size(), 8);
//     auto certauth = make_object_ptr<xschnorrcert_t>(address_set.size());

//     std::string table_publick_addr = "table1234567890abcdef";
//     const std::string table_address = top::base::xvaccount_t::make_account_address(
//                                         top::base::enum_vaccount_addr_type_secp256k1_user_account,
//                                         0, table_publick_addr);

//     auto timer = make_observer(new time::xchain_timer_t());

//     std::vector<xcons_dispatcher_ptr> m_dispatchers;
//     std::vector<std::shared_ptr<xcons_service_mgr_face>> m_cons_mgrs;

//     for (size_t j = 0; j < 2; j++) {
//         xelection_cache_face::elect_set elect_set;
//         for (size_t i = 0; i < 4; i++) {
//             elect_set.push_back(xcons_utl::to_xip2(address_set[j * 2 + i]));
//         }
//         for (size_t i = 0; i < 4; i++) {
//             auto global_worker_pool = make_object_ptr<base::xworkerpool_t_impl<1>>(top::base::xcontext_t::instance());
//             xobject_ptr_t<base::xvblockstore_t> store = make_object_ptr<xblockstore_mock>();
//             auto face = std::make_shared<xelection_mock>(elect_set);
//             std::shared_ptr<xleader_election_face> pelection = std::make_shared<xrandom_leader_election>(store, face);

//             std::shared_ptr<xnetwork_proxy_face> network = std::make_shared<xnetwork_proxy>();
//             xresources_face *                  p_res = new xresources(address_set[j * 2 + i].account_address().value(),
//                                                     global_worker_pool, certauth, store, network, pelection, timer);
//             xconsensus_para_face *             p_para = new xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type_clock_cert,
//                                                                 base::enum_xconsensus_threshold_2_of_3);
//             std::shared_ptr<xcons_service_para_face> p_srv_para = std::make_shared<xcons_service_para>(p_res, p_para);
//             auto block_maker = std::make_shared<xempty_block_maker>(p_srv_para);
//             // para_set.push_back(p_srv_para);
//             // std::vector<xcons_dispatcher_ptr> dispatchers;
//             // dispatchers.push_back(std::make_shared<xtimer_work_pool_dispatcher>(p_srv_para, block_maker, table_address));

//             // m_dispatchers.insert(m_dispatchers.end(), dispatchers.begin(), dispatchers.end());
//             p_para->add_block_maker(xunit_service::e_table, block_maker);
//             auto builder = std::make_shared<xdispatcher_builder_mock>();
//             m_dispatcher_builders.push_back(builder);
//             std::shared_ptr<xcons_service_mgr_face> cons_mgr_ptr = std::make_shared<xcons_service_mgr>(network, builder, p_srv_para);
//             m_cons_mgrs.push_back(cons_mgr_ptr);
//         }
//     }

//     auto global_worker_pool1 = make_object_ptr<base::xworkerpool_t_impl<1>>(top::base::xcontext_t::instance());
//     auto local_time = make_object_ptr<xlocal_time>(m_dispatcher_builders,
//                                     top::base::xcontext_t::instance(),
//                                     global_worker_pool1->get_thread_ids()[0]);

//     std::vector<xcons_proxy_face_ptr> cons_proxies;
//     for (size_t i = 0; i < m_cons_mgrs.size(); i++) {
//         std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(address_set[i]);
//         auto cons_proxy = m_cons_mgrs[i]->create(pnet);
//         EXPECT_NE(cons_proxy, nullptr);
//         cons_proxies.push_back(cons_proxy);
//     }
//     xinfo("***************create finish!***************");
//     std::this_thread::sleep_for(std::chrono::seconds(3));
//     for (size_t i = 0; i < m_cons_mgrs.size(); i++) {
//         EXPECT_TRUE(cons_proxies[i]->start());
//     }
//     xinfo("***************start finish!***************");
//     std::this_thread::sleep_for(std::chrono::minutes(30000));
// }

}  // namespace test
}  // namespace top
