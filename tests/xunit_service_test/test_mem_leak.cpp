#if 0
#include "gtest/gtest.h"
#include "xvledger/xvblock.h"
#include "xBFT/xconsaccount.h"
#include "xbase/xthread.h"
#include "xcrypto/xckey.h"
#include "xcertauth/xcertauth_face.h"
#include "tests/mock/xtestca.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xunit_service/xtimer_picker.h"
#include "xunit_service/xcons_service_para.h"
#include "tests/mock/xvblockstore_mock.hpp"

using namespace top;
using namespace top::base;
using namespace top::test;
using namespace top::mock;
using namespace top::xconsensus;
using namespace top::xunit_service;

class test_mem_leak : public testing::Test {
 protected:
    void SetUp() override {

     }

    void TearDown() override {
    }
 public:

};

class test_engine : public xconsensus::xcsaccount_t {
 public:
    test_engine(base::xcontext_t & _context,
                const int32_t target_thread_id,
                enum_xconsensus_pacemaker_type cert_type,
                xvblockstore_t* blockstore,
                xobject_t* store)
    : xconsensus::xcsaccount_t(_context, target_thread_id, "T-a-test12345678912345") {
        set_vblockstore(blockstore);
        xconsensus::xcsaccount_t::register_plugin(blockstore);
        xconsensus::xcsaccount_t::register_plugin(store);  // for test

        base::xauto_ptr<xcscoreobj_t> auto_engine = create_engine(*this, cert_type);
        xinfo("test_engine::test_engine this=%p,this_ref=%d,pacemaker_ref=%d,blockstore_ref=%d",
            this, get_refcount(), auto_engine->get_refcount(), blockstore->get_refcount());
    }
    virtual ~test_engine() {
        xinfo("test_engine::~test_engine this=%p", this);
    }
};

TEST(test_mem_leak, test_engine_test_1_BENCH) {
    base::xauto_ptr<base::xworkerpool_t_impl<1>> work_pool = new base::xworkerpool_t_impl<1>(base::xcontext_t::instance());
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    ASSERT_EQ(store->get_refcount(), 1);
    base::xauto_ptr<xvblockstore_t> blockstore = store::xblockstorehub_t::instance().create_block_store(*store.get(), "T-a-test12345678912345");
    ASSERT_EQ(store->get_refcount(), 2);
    ASSERT_EQ(blockstore->get_refcount(), 2);

    test_engine* engine = new test_engine(base::xcontext_t::instance(), work_pool->get_thread_ids()[0], xconsensus::enum_xconsensus_pacemaker_type_clock_cert, blockstore.get(), store.get());
    ASSERT_EQ(blockstore->get_refcount(), 6);
    engine->close();
    sleep(20);
    auto childnode = engine->get_child_node();
    xassert(childnode == nullptr);
    engine->release_ref();
    sleep(80);  // delay release should sleep wait blockstore acc_t idle close
    ASSERT_EQ(blockstore->get_refcount(), 2);  // fail blockstore->get_refcount() is 3
}

TEST(test_mem_leak, test_engine_test_2_BENCH) {
    base::xauto_ptr<base::xworkerpool_t_impl<1>> work_pool = new base::xworkerpool_t_impl<1>(base::xcontext_t::instance());
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    ASSERT_EQ(store->get_refcount(), 1);
    base::xauto_ptr<xvblockstore_t> blockstore = store::xblockstorehub_t::instance().create_block_store(*store.get(), "T-a-test12345678912345");
    ASSERT_EQ(store->get_refcount(), 2);
    ASSERT_EQ(blockstore->get_refcount(), 2);

    test_engine* engine = new test_engine(base::xcontext_t::instance(), work_pool->get_thread_ids()[0], xconsensus::enum_xconsensus_pacemaker_type_timeout_cert, blockstore.get(), store.get());
    engine->close();
    engine->release_ref();
    sleep(20);
}

std::shared_ptr<xcons_service_para_face> create_cons_service_para(const std::string & account, base::xvblockstore_t* blockstore) {
    auto                                certauth = make_object_ptr<xschnorrcert_t>(4);
    auto                                worker_pool = make_object_ptr<base::xworkerpool_t_impl<4>>(top::base::xcontext_t::instance());
    xobject_ptr_t<base::xvblockstore_t> blockstore_ptr;
    blockstore->add_ref();
    blockstore_ptr.attach(blockstore);

    //std::shared_ptr<xnetwork_proxy_face> network = std::make_shared<xnetwork_proxy>();
    //xelection_cache_face::elect_set              elect_set;
    //auto                                   face = std::make_shared<xelection_mock>(elect_set);
    //std::shared_ptr<xleader_election_face> pelection = std::make_shared<xrandom_leader_election>(store, face);
    xresources_face *                      p_res = new xresources(account, worker_pool, certauth, blockstore_ptr, nullptr, nullptr, nullptr, nullptr, nullptr);
    xconsensus_para_face *                 p_para = new xconsensus_para(xconsensus::enum_xconsensus_pacemaker_type_clock_cert, base::enum_xconsensus_threshold_2_of_3);
    return std::make_shared<xcons_service_para>(p_res, p_para);
}

TEST(test_mem_leak, test_timer_pick_BENCH) {
    base::xauto_ptr<base::xworkerpool_t_impl<1>> work_pool = new base::xworkerpool_t_impl<1>(base::xcontext_t::instance());
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    ASSERT_EQ(store->get_refcount(), 1);
    std::string account = "T-a-test12345678912345";
    base::xauto_ptr<xvblockstore_t> blockstore = store::xblockstorehub_t::instance().create_block_store(*store.get(), account);
    ASSERT_EQ(store->get_refcount(), 2);
    ASSERT_EQ(blockstore->get_refcount(), 2);

    std::shared_ptr<xcons_service_para_face> xcons_service_para = create_cons_service_para(account, blockstore.get());
    auto m_picker = new xtimer_picker_t(base::xcontext_t::instance(), work_pool->get_thread_ids()[0], xcons_service_para, nullptr);
    m_picker->close();
    m_picker->release_ref();
    sleep(20);
}
#endif
