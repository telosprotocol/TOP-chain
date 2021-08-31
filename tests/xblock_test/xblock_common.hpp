#include "../mock/xbft_elect_mock.hpp"
#include "../mock/xempty_block_maker.hpp"
#include "../mock/xnetwork_mock.h"
// #include "../mock/xtestca.hpp"
#include "../mock/xtimer_workpool_dispatcher.hpp"
#include "../mock/xunitblock.hpp"
#include "../mock/xvblockstore_mock.hpp"
#include "gtest/gtest.h"
#include "xblockstore/xblockstore_face.h"
#include "xchain_timer/xchain_timer.h"
#include "xcommon/xip.h"
#include "xstore/test/xstore_face_mock.h"
#include "xtxpool_v2/xtxpool_face.h"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_service_mgr.h"
#include "xunit_service/xcons_service_para.h"
#include "xunit_service/xcons_utl.h"
#include "xunit_service/xleader_election.h"
#include "xunit_service/xnetwork_proxy.h"
#include "xunit_service/xtableblockservice.h"
#include "xunit_service/xtimer_dispatcher.h"
#include "xunit_service/xworkpool_dispatcher.h"

#include <string>
namespace top {
namespace test {
using namespace mock;
using namespace xunit_service;
using namespace store;
//using namespace txexecutor;
using namespace xtxpool_v2;

class xblock_common {
public:
    top::common::xnode_address_t create_address(int index, const std::string & account, std::size_t version = 1, bool bbeacon = false) {
        // std::size_t                       id = 1;
        common::xelection_round_t              ver(version);
        common::xnetwork_id_t           nid{common::xtestnet_id};
        common::xslot_id_t              slot_id{index};
        top::common::xcluster_address_t cluster_addr;
        if (!bbeacon) {
            top::common::xcluster_address_t cluster_addr1(common::xtestnet_id, common::xcommittee_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id);
            cluster_addr = cluster_addr1;
        } else {
            top::common::xcluster_address_t cluster_addr1(common::xtestnet_id, common::xzec_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id);
            cluster_addr = cluster_addr1;
        }
        top::common::xaccount_election_address_t account_address{common::xnode_id_t{account}, slot_id};
        top::common::xnode_address_t             addr(cluster_addr, account_address, ver, 1023, 0);
        return addr;
    }

    void create_address(const std::string & account = "node1234567890abcdef", std::size_t version = 1, int size = 4, bool bbeacon = false) {
        if (!acccount_set.empty()) {
            acccount_set.clear();
            address_set.clear();
            elect_set.clear();
        }
        for (int i = 0; i < size; i++) {
            std::string       account_publick_addr = account + std::to_string(i);
            const std::string test_node_address = top::base::xvaccount_t::make_account_address(top::base::enum_vaccount_addr_type_secp256k1_user_account, 0, account_publick_addr);
            auto              address = create_address(i, test_node_address, version);
            address_set.push_back(address);
            acccount_set.push_back(test_node_address);
            // elect_set.push_back(xcons_utl::to_xip2(address));
        }
    }

    void init(bool withtimer = false, int size = 4) {
        // auto size = 4;
        // m_certauth = make_object_ptr<xschnorrcert_t>(size);
        // xconsensus::enum_xconsensus_pacemaker_type pacemaker = xconsensus::enum_xconsensus_pacemaker_type_clock_cert;
        // if (withtimer) {
        //     pacemaker = xconsensus::enum_xconsensus_pacemaker_type_timeout_cert;
        // }

        // create_address("node1234567890abcdef");
        // m_timer = make_observer(new time::xchain_timer_t());
        // m_certauth = make_object_ptr<xschnorrcert_t>(size);
        // global_worker_pool = make_object_ptr<base::xworkerpool_t_impl<1>>(top::base::xcontext_t::instance());

        // table_address = data::xblocktool_t::make_address_shard_table_account(100);

        // for (int i = 0; i < size; i++) {
        //     auto              mbus = std::make_shared<top::mbus::xmessage_bus_t>();
        //     xstore_face_ptr_t persist_db;
        //     if (test_xstore_mock_enable) {
        //         // persist_db = make_object_ptr<xstore_face_mock_t>();
        //     } else {
        //         persist_db = xstore_factory::create_store_with_memdb();
        //     }
        //     store_set.push_back(persist_db);

        //     xobject_ptr_t<base::xvblockstore_t> blockstore;
        //     if (test_xblockstore_mock_enable) {
        //         blockstore = make_object_ptr<xblockstore_mock>();
        //     } else {
        //         blockstore.attach(xblockstorehub_t::instance().create_block_store(*persist_db, table_address));
        //     }
        //     blockstore_set.push_back(blockstore);
        //     auto                                   worker_pool = make_object_ptr<base::xworkerpool_t_impl<1>>(top::base::xcontext_t::instance());
        //     auto                                   face = std::make_shared<xelection_mock>(elect_set);
        //     std::shared_ptr<xleader_election_face> pelection = std::make_shared<xrandom_leader_election>(blockstore, face);
        //     m_pelection.push_back(pelection);
        //     std::shared_ptr<xnetwork_proxy_face>     network = std::make_shared<xnetwork_proxy>();
        //     xresources_face *                        p_res = new xresources(acccount_set[i], worker_pool, m_certauth, blockstore, network, pelection, m_timer);
        //     xconsensus_para_face *                   p_para = new xconsensus_para(pacemaker, base::enum_xconsensus_threshold_2_of_3);
        //     std::shared_ptr<xcons_service_para_face> p_srv_para = std::make_shared<xcons_service_para>(p_res, p_para);
        //     para_set.push_back(p_srv_para);

        //     std::shared_ptr<xblock_maker_face> block_maker;
        //     if (test_xtxpool_block_maker_mock_enable) {
        //         block_maker = std::make_shared<xempty_block_maker>(p_srv_para);
        //     } else {
        //         auto txpool = xtxpool_v2::xtxpool_instance::create_xtxpool_inst(make_observer(persist_db), make_observer(mbus.get()));
        //         txpool_set.push_back(txpool);
        //         block_maker = xblockmaker_factory::create_txpool_block_maker(make_observer(persist_db), blockstore.get(), txpool.get());
        //     }
        //     p_para->add_block_maker(xunit_service::e_table, block_maker);
        //     xcons_dispatcher_builder_ptr dispatcher_builder = std::make_shared<xdispatcher_builder_mock>();
        //     m_dispatcher_builders.push_back(dispatcher_builder);
        //     std::shared_ptr<xcons_service_mgr_face> cons_mgr_ptr = std::make_shared<xcons_service_mgr>(network, dispatcher_builder, p_srv_para);
        //     m_cons_mgrs.push_back(cons_mgr_ptr);
        // }
    }

public:
    bool test_xblockstore_mock_enable{true};
    bool test_xstore_mock_enable{true};
    bool test_xtxpool_block_maker_mock_enable{true};

public:
    std::string                                           table_address;
    std::vector<std::string>                              acccount_set;
    std::vector<top::common::xnode_address_t>             address_set;
    std::vector<std::shared_ptr<xcons_service_para_face>> para_set;
    std::vector<std::shared_ptr<xcons_service_mgr_face>>  m_cons_mgrs;
    xobject_ptr_t<base::xvcertauth_t>                     m_certauth;
    xobject_ptr_t<base::xworkerpool_t_impl<1>>            global_worker_pool;
    std::vector<std::shared_ptr<xleader_election_face>>   m_pelection{};
    xobject_ptr_t<xlocal_time>                            m_local_time{};
    std::vector<xcons_dispatcher_ptr>                     m_dispatchers;
    xelection_cache_face::elect_set                             elect_set;
    std::vector<xstore_face_ptr_t>                        store_set;
    std::vector<xobject_ptr_t<xtxpool_face_t>>            txpool_set;
    std::vector<xobject_ptr_t<base::xvblockstore_t>>      blockstore_set;
    observer_ptr<time::xchain_timer_t>                    m_timer;
    std::vector<xcons_dispatcher_builder_ptr>   m_dispatcher_builders;
};

}  // namespace test
}  // namespace top
