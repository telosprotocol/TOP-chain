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

class test_tx_cache : public testing::Test {
 protected:
    void SetUp() override {
        xinit_log("./cache_test.log", true, true);
        xset_log_level(enum_xlog_level_debug);
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
    //txexecutor::xtransaction_prepare_mgr*      m_tx_prepare_mgr{nullptr};  // not shared_ptr for xxtimer
    std::shared_ptr<data::xtransaction_cache_t> m_transaction_cache;    
};

TEST_F(test_tx_cache, test_prepare_mgr) {
/*    m_transaction_cache = std::make_shared<data::xtransaction_cache_t>();
    static top::base::xiothread_t * t1 = top::base::xiothread_t::create_thread(top::base::xcontext_t::instance(), 0, -1);
    if (m_tx_prepare_mgr != nullptr)
        return;
    m_tx_prepare_mgr = new txexecutor::xtransaction_prepare_mgr(nullptr, top::base::xcontext_t::instance(), t1->get_thread_id(), make_observer(m_transaction_cache));
    m_tx_prepare_mgr->start();
    top::base::xtime_utl::sleep_ms(100);
    xinfo("test over.");*/
}
TEST_F(test_tx_cache, test_cache) {
    m_transaction_cache = std::make_shared<data::xtransaction_cache_t>();
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v1_t>();
    tx->set_deposit(100000);
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    tx->set_fire_timestamp((uint64_t)val.tv_sec);
    tx->set_expire_duration(1);
    tx->set_memo("123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890");
    tx->set_digest();
    std::string tx_hash("12345678901234567890123456789012");
    std::string tx_hash2("12345678901234567890123456789099");
    m_transaction_cache->tx_add(tx_hash, tx);
    m_transaction_cache->tx_add(tx_hash2, tx);
    EXPECT_EQ(m_transaction_cache->tx_find(tx_hash), 1);
    xtransaction_cache_data_t cache_data;
    EXPECT_EQ(m_transaction_cache->tx_get(tx_hash, cache_data), 1);
    Json::Value jv;
    m_transaction_cache->tx_set_json(tx_hash, 0, jv);
    m_transaction_cache->tx_set_json(tx_hash, 1, jv);
    m_transaction_cache->tx_erase(tx_hash);
    EXPECT_EQ(m_transaction_cache->tx_find(tx_hash), 0);
    EXPECT_EQ(m_transaction_cache->tx_find(tx_hash2), 1);
    top::base::xtime_utl::sleep_ms(2000);
    m_transaction_cache->tx_clean();
    EXPECT_EQ(m_transaction_cache->tx_find(tx_hash2), 0);
}
