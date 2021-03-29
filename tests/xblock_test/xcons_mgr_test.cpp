#include "xblock_common.hpp"
#include "xunit_service/xnetwork_proxy.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"
namespace top {
namespace test {
using namespace mock;
using namespace xunit_service;

class xcons_service_mgr_test : public xcons_service_mgr {
public:
    xcons_service_mgr_test(const std::shared_ptr<xnetwork_proxy_face> &     network_proxy,
                           const xcons_dispatcher_builder_ptr &             dispatcher_builder,
                           const std::shared_ptr<xcons_service_para_face> & para)
      : xcons_service_mgr(nullptr, network_proxy, dispatcher_builder, para) {}

public:
    size_t get_size(const xvip2_t & xip) {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto                        xip_ = xcons_utl::erase_version(xip);
        auto                        iter = m_cons_map.find(xip_);
        return iter->second.size();
    }
};

class xcons_mgr_test : public testing::Test {
protected:
    void SetUp() override {}

    void TearDown() override {
        // delete m_engines;
    }

    std::shared_ptr<xcons_service_para_face> create_para(const std::string & account, const observer_ptr<time::xchain_timer_t> & timer) {
        return nullptr;
        // auto                                certauth = make_object_ptr<xschnorrcert_t>(4);
        // auto                                worker_pool = make_object_ptr<base::xworkerpool_t_impl<4>>(top::base::xcontext_t::instance());
        // xobject_ptr_t<base::xvblockstore_t> store = make_object_ptr<xblockstore_mock>();

        // // std::shared_ptr<xnetwork_proxy_face> network = std::make_shared<xnetwork_proxy>();
        // xelection_cache_face::elect_set              elect_set;
        // auto                                   face = std::make_shared<xelection_mock>(elect_set);
        // std::shared_ptr<xleader_election_face> pelection = std::make_shared<xrandom_leader_election>(store, face);
        // xresources_face *                      p_res = new xresources(account, worker_pool, certauth, store, m_netproxy, pelection, timer);
        // xconsensus_para_face *                 p_para = new xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type_clock_cert, base::enum_xconsensus_threshold_2_of_3);
        // return std::make_shared<xcons_service_para>(p_res, p_para);
    }

    std::vector<xcons_dispatcher_ptr> create_dispatcher(std::shared_ptr<xcons_service_para_face> para, bool bwithtimer) {
        xelection_cache_face::elect_set         elect_set;
        auto                              block_maker = std::make_shared<xempty_block_maker>(para);
        auto                              store = para->get_resources()->get_vblockstore();
        std::vector<xcons_dispatcher_ptr> dispatchers;
        dispatchers.push_back(std::make_shared<xtimer_work_pool_dispatcher>(para, block_maker, "T-table"));
        if (bwithtimer) {
            dispatchers.push_back(std::make_shared<xtimer_dispatcher_t>(para, block_maker));
            // m_dispatchers.insert(m_dispatchers.end(), dispatchers.begin(), dispatchers.end());
        }
        return dispatchers;
    }

protected:
    std::shared_ptr<xnetwork_proxy_face> m_netproxy = nullptr;//std::make_shared<xnetwork_proxy>();
};

TEST_F(xcons_mgr_test, create) {
    xblock_common                                      comm;
    auto                                               addr = comm.create_address(0, "T_test");
    auto                                               addr2 = comm.create_address(0, "T_test", 2);
    std::shared_ptr<top::xbase_io_context_wrapper_t> io_object = std::make_shared<top::xbase_io_context_wrapper_t>();
    std::shared_ptr<top::xbase_timer_driver_t> timer_driver = std::make_shared<top::xbase_timer_driver_t>(io_object);
    auto                                               timer = make_observer(new time::xchain_timer_t(timer_driver));
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(addr);
    std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet2 = std::make_shared<network_mock>(addr2);
    auto                                               para = create_para("T_test0", timer);
    // auto                                               dispacthers = create_dispatcher(para, false);
    auto                                               cons_mgr = std::make_shared<xcons_service_mgr_test>(m_netproxy, std::make_shared<xdispatcher_builder_mock>(), para);
    auto                                               cons_proxy = cons_mgr->create(pnet);
    EXPECT_TRUE(cons_proxy != nullptr);
    EXPECT_TRUE(cons_proxy->start(10));
    std::this_thread::sleep_for(std::chrono::seconds(5));
    auto cons_proxy2 = cons_mgr->create(pnet2);
    EXPECT_EQ(cons_mgr->get_size(xcons_utl::to_xip2(addr)), 1);
    EXPECT_NE(cons_proxy->get_ip().low_addr, cons_proxy2->get_ip().low_addr);
    EXPECT_TRUE(cons_proxy2 != nullptr);
    EXPECT_TRUE(cons_proxy->fade());
    EXPECT_TRUE(cons_proxy2->start(10));
    std::this_thread::sleep_for(std::chrono::seconds(10));
    // delete cons_mgr;
}

TEST_F(xcons_mgr_test, mapcompare) {
    std::map<xvip2_t, int, xvip2_compare> _map;
    xvip2_t                              ip{-1, -1};
    xvip2_t                              ip2{1, -1};
    _map[ip] = 1;
    _map[ip] = 2;
    EXPECT_EQ(_map[ip], 2);
    _map[ip2] = 3;
    EXPECT_EQ(_map[ip2], 3);
    // delete cons_mgr;
}
}  // namespace test
}  // namespace top
