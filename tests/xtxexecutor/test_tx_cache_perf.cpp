#include "gtest/gtest.h"

#include "xstore/xaccount_context.h"
#include "xdata/xaction_parse.h"
#include "xdata/xtransaction_v1.h"
#include "xtxexecutor/xtransaction_context.h"
#include "xconfig/xconfig_register.h"
#include "xchain_timer/xchain_timer.h"
#include "xloader/xconfig_onchain_loader.h"
//#include "test_xtxexecutor_util.hpp"
#include "xtxstore/xtransaction_prepare_mgr.h"
#include "xbasic/xtimer_driver_fwd.h"
#include "xdata/xtransaction_cache.h"

using namespace top::txexecutor;
using namespace top::data;
using namespace top;
using namespace top::store;

class test_tx_cache_perf : public testing::Test {
 protected:
    void SetUp() override {
        xinit_log("./cache_test_perf.log", true, true);
        xset_log_level(enum_xlog_level_info);
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
        tx->set_deposit(100000);
        struct timeval val;
        base::xtime_utl::gettimeofday(&val);
        tx->set_different_source_target_address("T80000733b43e6a2542709dc918ef2209ae0fc6503c2f2", "T80000733b43e6a2542709dc918ef2209ae0fc6503c2f1");
        tx->set_fire_timestamp((uint64_t)val.tv_sec);
        tx->set_expire_duration(1);
        tx->set_digest();
        m_trans = make_object_ptr<xcons_transaction_t>(tx.get());
    }

    void TearDown() override {
    }
 public:
    xcons_transaction_ptr_t             m_trans;
    txexecutor::xtransaction_prepare_mgr*      m_tx_prepare_mgr{nullptr};  // not shared_ptr for xxtimer
};
std::shared_ptr<data::xtransaction_cache_t> g_transaction_cache;    

TEST_F(test_tx_cache_perf, test_cache) {
    g_transaction_cache = std::make_shared<data::xtransaction_cache_t>();
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
    tx->set_deposit(100000);
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    tx->set_fire_timestamp((uint64_t)val.tv_sec);
    tx->set_expire_duration(1);
    tx->set_digest();

    std::string tx_hash;
    clock_t start,ends;
    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        g_transaction_cache->tx_add(tx_hash, tx);
    }
    ends=clock();
    std::cout<<"add time:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;

    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        EXPECT_EQ(g_transaction_cache->tx_find(tx_hash), 1);
    }
    ends=clock();
    std::cout<<"find time:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;

    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        g_transaction_cache->tx_erase(tx_hash);
    }
    ends=clock();
    std::cout<<"delete time:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;
    g_transaction_cache->tx_clear();
}

void cache_test_func() {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
    tx->set_deposit(100000);
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    tx->set_fire_timestamp((uint64_t)val.tv_sec);
    tx->set_expire_duration(1);
    tx->set_digest();

    std::string tx_hash;
    clock_t start,ends;
    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        g_transaction_cache->tx_add(tx_hash, tx);
    }
    ends=clock();
    std::cout<<"add time2:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;

    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        EXPECT_EQ(g_transaction_cache->tx_find(tx_hash), 1);
    }
    ends=clock();
    std::cout<<"find time2:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;

    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        g_transaction_cache->tx_erase(tx_hash);
    }
    ends=clock();
    std::cout<<"delete time2:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;    
}
TEST_F(test_tx_cache_perf, test_cache_thread_BENCH) {
    //g_transaction_cache = std::make_shared<data::xtransaction_cache_t>();
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
    tx->set_deposit(100000);
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    tx->set_fire_timestamp((uint64_t)val.tv_sec);
    tx->set_expire_duration(1);
    tx->set_digest();

    std::thread cache_thread(cache_test_func); 

    std::string tx_hash;
    clock_t start,ends;
    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        g_transaction_cache->tx_add(tx_hash, tx);
    }
    ends=clock();
    std::cout<<"add time:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;

    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        EXPECT_EQ(g_transaction_cache->tx_find(tx_hash), 1);
    }
    ends=clock();
    std::cout<<"find time:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;

    cache_thread.join();
    start=clock();
    for (uint64_t i=1000000;i<2000000;i++) {
        tx_hash = std::to_string(i) + "1234567890123456789012345";
        g_transaction_cache->tx_erase(tx_hash);
    }
    ends=clock();
    std::cout<<"delete time:"<<ends-start<<"," << 1000000/((ends-start)*1.0/1000000)<<std::endl;    
}
