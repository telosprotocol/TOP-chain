#include <set>
#include <unordered_map>

#include "../mock/xbft_elect_mock.hpp"
#include "../mock/xempty_block_maker.hpp"
#include "../mock/xnetwork_mock.h"
// #include "../mock/xtestca.hpp"
#include "../mock/xunitblock.hpp"
#include "../mock/xvblockstore_mock.hpp"
#include "xblock_common.hpp"
#include "xunit_service/xcons_face.h"
#include "xunit_service/xcons_service_mgr.h"
#include "xunit_service/xcons_service_para.h"
#include "xunit_service/xleader_election.h"
#include "xunit_service/xnetwork_proxy.h"
#include "xunit_service/xtimer_block_maker.h"
#include "xunit_service/xtimer_dispatcher.h"
#include "xunit_service/xtimer_service.h"
#include "xchain_timer/xchain_timer.h"
#include "gtest/gtest.h"
namespace top {
using namespace test;
using namespace mock;

using namespace xunit_service;

class xtimer_test : public testing::Test {
protected:
    top::common::xnode_address_t create_address(int index, const std::string &account) {
        std::size_t                              id = 1;
        top::common::xelection_round_t                  ver(id);
        common::xnetwork_id_t                    nid{common::xtestnet_id};
        common::xslot_id_t                       slot_id{index};
        top::common::xcluster_address_t          cluster_addr(nid, common::xcommittee_zone_id, common::xcommittee_cluster_id, common::xcommittee_group_id);
        top::common::xaccount_election_address_t account_address{common::xnode_id_t{account}, slot_id};
        top::common::xnode_address_t             addr(cluster_addr, account_address, ver, 1023, 0);
        return addr;
    }

    void SetUp() override {
        // auto size = 4;
        // m_certauth = make_object_ptr<xschnorrcert_t>(size);
        // global_worker_pool = make_object_ptr<base::xworkerpool_t_impl<4>>(top::base::xcontext_t::instance());
        // auto timer = make_observer(new time::xchain_timer_t());
        // std::vector<std::string>  acccount_set;
        // xelection_cache_face::elect_set elect_set;
        // for (int i = 0; i < size; i++) {
        //     auto account = "T-xxx" + std::to_string(i);
        //     auto address = create_address(i + 1, account);
        //     address_set.push_back(address);
        //     acccount_set.push_back(account);
        //     elect_set.push_back(xcons_utl::to_xip2(address));
        // }

        // m_face = std::make_shared<xelection_mock>(elect_set);
        // m_certauth = make_object_ptr<xschnorrcert_t>(size);
        // global_worker_pool = make_object_ptr<base::xworkerpool_t_impl<1>>(top::base::xcontext_t::instance());
        // for (int i = 0; i < size; i++) {
        //     std::shared_ptr<xnetwork_proxy_face> network = std::make_shared<xnetwork_proxy>();
        //     xobject_ptr_t<base::xvblockstore_t>  store = make_object_ptr<xblockstore_mock>();
        //     auto                                 pelection = std::make_shared<xrandom_leader_election>(store, m_face);
        //     xresources_face *                    p_res = new xresources(acccount_set[i], global_worker_pool, m_certauth, store, network, pelection, timer);
        //     xconsensus_para_face *               p_para = new xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type_timeout_cert,
        //                                                        base::enum_xconsensus_threshold_2_of_3);
        //     auto                                 p_srv_para = std::make_shared<xcons_service_para>(p_res, p_para);
        //     para_set.push_back(p_srv_para);
        //     auto block_maker = std::make_shared<xtimer_block_maker_t>(p_srv_para);
        //     p_para->add_block_maker(xunit_service::e_timer, block_maker);
        //     // auto p_dispatcher = std::make_shared<xtimer_dispatcher_t>(p_srv_para, block_maker, 5);
        //     // auto                                               block_srv = new xtimer_service_t(p_srv_para, p_dispatcher);
        //     // std::vector<xcons_dispatcher_ptr> dispatchers;
        //     // dispatchers.push_back(std::make_shared<xworkpool_dispatcher>(p_srv_para, block_maker, "T-table"));
        //     // auto ptr = new xtimer_dispatcher_t(p_srv_para, block_maker, 10000);
        //     // dispatchers.push_back(std::shared_ptr<xtimer_dispatcher_t>(p_dispatcher));
        //     auto cons_mgr_ptr = std::make_shared<xcons_service_mgr>(network, std::make_shared<xdispatcher_builder_mock>(), p_srv_para);
        //     m_cons_mgrs.push_back(cons_mgr_ptr);
        // }
    }

    void TearDown() override {
        // delete m_block_srv;
    }

public:
    std::vector<top::common::xnode_address_t>             address_set;
    std::vector<std::shared_ptr<xcons_service_para_face>> para_set;
    xobject_ptr_t<base::xvcertauth_t>                     m_certauth{};
    xobject_ptr_t<base::xworkerpool_t>                    global_worker_pool{};
    std::vector<std::shared_ptr<xcons_service_mgr>>       m_cons_mgrs;
    std::shared_ptr<xelection_cache_face>                       m_face{};
    xobject_ptr_t<base::xvblockstore_t>                   m_store{};
    std::shared_ptr<xleader_election_face>                m_pelection{};
};

TEST_F(xtimer_test, run) {
    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(address_set[i]);
        auto                                               cons_proxy = m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("create finish!");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (size_t i = 0; i < m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("start finish!");
    std::this_thread::sleep_for(std::chrono::minutes(5));
    xinfo("time out fade!");
    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    xinfo("time out destroy!");
    for (size_t i = 0; i < m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(address_set[i]);
        EXPECT_TRUE(m_cons_mgrs[i]->destroy(xip));
    }
}
} // namespace top
