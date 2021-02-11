#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xaction.h"
#include "xdata/xcons_transaction.h"
#include "xdata/xdata_defines.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xaccountobj.h"
#include "xtxpool/xtxpool_error.h"
#include "xdata/xblocktool.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xtxpool/xtxpool_para.h"
#include "xchain_timer/xchain_timer_face.h"
#include "xtxpool/xtxpool_table.h"

using namespace top::xtxpool;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;

class xchain_timer_mock final : public time::xchain_time_face_t {
public:
    bool update_time(data::xblock_t* timer_block, bool force = false) override { return true; }

    //void restore_last_db_time() override {}

    // time::xchain_time_st const & get_local_time() const noexcept override {
    //     static time::xchain_time_st st;
    //     st.xtime_round = 1;
    //     return st;
    // }

    common::xlogic_time_t logic_time() const noexcept override { return timer_height; }

    bool watch(const std::string &, uint64_t, time::xchain_time_watcher) override { return true; }

    bool watch_one(uint64_t, time::xchain_time_watcher) override { return true; }

    bool unwatch(const std::string &) override { return true; }

    void init() override {}

    void                close() override {}
    base::xiothread_t * get_iothread() const noexcept override { return nullptr; }

    uint64_t timer_height{0};
};

class test_tx_queue : public testing::Test {
protected:
    void SetUp() override {
        auto & config_register = config::xconfig_register_t::get_instance();
        config_register.set<std::string>(config::xtx_send_timestamp_tolerance_onchain_goverance_parameter_t::name, "0");
    }

    void TearDown() override {
        auto & config_register = config::xconfig_register_t::get_instance();
        config_register.set<std::string>(config::xtx_send_timestamp_tolerance_onchain_goverance_parameter_t::name,
                                         std::to_string(config::xtx_send_timestamp_tolerance_onchain_goverance_parameter_t::value));
    }

public:
    std::shared_ptr<xchain_timer_mock> m_chain_timer;
};

TEST_F(test_tx_queue, tx_order_1_send_repeat) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t       last_tx_hash;
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 1, last_tx_hash, &table);
    for (uint64_t i = 1; i < queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, i, now + i, last_tx_hash);
            tx->get_transaction()->set_push_pool_timestamp(now + i);
            last_tx_hash = tx->get_transaction()->digest();
            ASSERT_EQ(0, queue->push_send_tx(tx));
        }
    }

    last_tx_hash = {};
    for (uint64_t i = 1; i < queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, i, now + i, last_tx_hash);
            tx->get_transaction()->set_push_pool_timestamp(now + i);
            last_tx_hash = tx->get_transaction()->digest();
            ASSERT_EQ(xtxpool_error_request_tx_repeat, queue->push_send_tx(tx));
        }
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, send_tx_fail_pop) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t       last_tx_hash;
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 0, last_tx_hash, &table);

    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    for (uint64_t i = 0; i < 10; i++) {
        txs[i]->get_transaction()->set_push_pool_timestamp(now + i);
        ASSERT_EQ(0, queue->push_send_tx(txs[i]));
    }

    queue->pop_tx_by_hash(txs[5]->get_transaction()->digest(), data::enum_transaction_subtype_send, 1);
    {
        ASSERT_EQ(queue->get_queue_size(), 5);  
        auto txs_get = queue->get_cons_txs(5, txs[5]->get_transaction()->digest());
        ASSERT_EQ(0, txs_get.size());      
    }

    queue->pop_tx_by_hash(txs[5]->get_transaction()->digest(), data::enum_transaction_subtype_send, 1);


    queue->pop_tx_by_hash(txs[0]->get_transaction()->digest(), data::enum_transaction_subtype_send, 1);
    {
        auto txs_get = queue->get_cons_txs(0, {});
        ASSERT_EQ(0, txs_get.size());
        ASSERT_EQ(queue->get_queue_size(), 0);   
    }

    queue->release_ref();
}

TEST_F(test_tx_queue, tx_timer_contract_send_repeat) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t(sys_contract_beacon_timer_addr, 10, {}, &table);
    for (uint32_t i = 0; i < queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(sys_contract_beacon_timer_addr, sys_contract_beacon_timer_addr, 10, now + i);
            ASSERT_EQ(0, queue->push_send_tx(tx));
        }
    }

    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(sys_contract_beacon_timer_addr, sys_contract_beacon_timer_addr, 10, now);
        ASSERT_EQ(xtxpool_error_tx_nonce_repeat, queue->push_send_tx(tx));
    }
    std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(10, {});
    ASSERT_EQ(1, txs.size());
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_over_limit1) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t       last_tx_hash;
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 11, {}, &table);
    uint64_t        i = 1;
    for (; i <= queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10 + i, now + i, last_tx_hash);
            tx->get_transaction()->set_push_pool_timestamp(now + i);
            last_tx_hash = tx->get_transaction()->digest();
            ASSERT_EQ(0, queue->push_send_tx(tx));
        }
    }

    for (; i <= 2 * queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10 + i, now + i, last_tx_hash);
            tx->get_transaction()->set_push_pool_timestamp(now + i);
            last_tx_hash = tx->get_transaction()->digest();
            if (i == queue->get_send_tx_queue_max_num() + 1) {
                ASSERT_EQ(xtxpool_error_send_tx_queue_over_upper_limit, queue->push_send_tx(tx));
            } else {
                ASSERT_EQ(xtxpool_error_tx_nonce_incontinuity, queue->push_send_tx(tx));
            }
        }
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_over_limit2) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    uint256_t       last_tx_hash;
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 11, {}, &table);
    auto &          config_register = top::config::xconfig_register_t::get_instance();
    uint32_t unitblock_send_transfer_tx_batch_num = XGET_CONFIG(unitblock_send_transfer_tx_batch_num);
    uint64_t i = 1;
    for (; i <= queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10 + i, now + i, last_tx_hash);
            tx->get_transaction()->set_push_pool_timestamp(now + i);
            last_tx_hash = tx->get_transaction()->digest();
            xdbg("xxxxxxxxxxxxxxxx i:%d", i);
            ASSERT_EQ(0, queue->push_send_tx(tx));
            uint32_t num = (i < unitblock_send_transfer_tx_batch_num) ? i : unitblock_send_transfer_tx_batch_num;
            std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(11, {});
            ASSERT_EQ(num, txs.size());
        }
    }

    last_tx_hash = {};
    for (i = 1; i <= queue->get_send_tx_queue_max_num(); i++) {
        {
            xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10 + i, now + 100 + i, last_tx_hash);
            tx->get_transaction()->set_push_pool_timestamp(now + i);
            last_tx_hash = tx->get_transaction()->digest();
            ASSERT_EQ(0, queue->push_send_tx(tx));
            uint32_t num = (i < unitblock_send_transfer_tx_batch_num) ? i : unitblock_send_transfer_tx_batch_num;
            std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(11, {});
            ASSERT_EQ(num, txs.size());
        }
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_send_nonce_repeat1) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 10, {}, &table);
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now);
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now - 1);
        ASSERT_EQ(xtxpool_error_tx_nonce_repeat, queue->push_send_tx(tx));
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_send_nonce_repeat2) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 10, {}, &table);
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now);
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now + 1);
        ASSERT_EQ(0, queue->push_send_tx(tx));
        ASSERT_EQ(1, queue->get_queue_size());
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_send_nonce_repeat3) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 10, {}, &table);
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now);
        ASSERT_EQ(0, queue->push_send_tx(tx));
        std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(10, {});
        ASSERT_EQ(txs.size(), 1);
        ASSERT_EQ(txs[0]->get_transaction()->get_tx_nonce(), 11);
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now + 1);
        ASSERT_EQ(0, queue->push_send_tx(tx));
        std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(10, {});
        ASSERT_EQ(txs.size(), 1);
        ASSERT_EQ(txs[0]->get_transaction()->get_tx_nonce(), 11);
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, send_tx_push_clear_expired_txs) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 0, {}, &table);
    uint32_t        i;
    uint64_t        fire_timestamp = xverifier::xtx_utl::get_gmttime_s();
    uint256_t       last_tx_hash;
    uint64_t        last_tx_nonce = 0;

    for (i = 0; i < queue->get_send_tx_queue_max_num(); i++) {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, last_tx_nonce, fire_timestamp, last_tx_hash, 1);
        tx->get_transaction()->set_push_pool_timestamp(fire_timestamp);
        last_tx_hash = tx->get_transaction()->digest();
        last_tx_nonce = tx->get_transaction()->get_tx_nonce();
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    ASSERT_EQ(queue->get_send_tx_queue_max_num(), queue->get_queue_size());

    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, last_tx_nonce, fire_timestamp, last_tx_hash, 2);
        tx->get_transaction()->set_push_pool_timestamp(fire_timestamp);
        ASSERT_NE(0, queue->push_send_tx(tx));
        ASSERT_EQ(queue->get_send_tx_queue_max_num(), queue->get_queue_size());
    }
    sleep(3);
    fire_timestamp = fire_timestamp + 3;
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, last_tx_nonce, fire_timestamp, last_tx_hash, 2);
        tx->get_transaction()->set_push_pool_timestamp(fire_timestamp);
        ASSERT_EQ(xtxpool_error_tx_nonce_incontinuity, queue->push_send_tx(tx));
        ASSERT_EQ(0, queue->get_queue_size());
    }
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_order_1_send) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 10, {}, &table);
    uint256_t       last_tx_hash;
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now + 1);
        last_tx_hash = tx->get_transaction()->digest();
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 10, now + 100);
        last_tx_hash = tx->get_transaction()->digest();
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 2, 12, now + 1);
        ASSERT_EQ(xtxpool_error_tx_nonce_incontinuity, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 2, 11, now + 100, last_tx_hash);
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 11, now + 200, last_tx_hash);
        ASSERT_EQ(0, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0 , 2, 9, now + 100);
        ASSERT_EQ(xtxpool_error_tx_nonce_too_old, queue->push_send_tx(tx));
    }
    {
        xcons_transaction_ptr_t tx = queue->get_tx();
        ASSERT_EQ(tx->get_transaction()->get_tx_subtype(), enum_transaction_subtype_send);
        ASSERT_EQ(tx->get_transaction()->get_last_nonce(), 10);
        queue->pop_tx();
    }
    {
        xcons_transaction_ptr_t tx = queue->get_tx();
        ASSERT_EQ(tx->get_transaction()->get_tx_subtype(), enum_transaction_subtype_send);
        ASSERT_EQ(tx->get_transaction()->get_last_nonce(), 11);
        queue->pop_tx();
    }
    queue->close();
    queue->release_ref();
}

TEST_F(test_tx_queue, tx_order_2_send) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t                     now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 0, {}, &table);

    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 0, now + 1);
    ASSERT_EQ(0, queue->push_send_tx(tx1));

    xcons_transaction_ptr_t tx2 = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, tx1->get_transaction()->get_tx_nonce(), now + 1, tx1->get_transaction()->digest());
    ASSERT_EQ(0, queue->push_send_tx(tx2));

    xcons_transaction_ptr_t tx3 = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, tx2->get_transaction()->get_tx_nonce(), now + 1, tx2->get_transaction()->digest());
    ASSERT_EQ(0, queue->push_send_tx(tx3));

    std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(0, {});
    ASSERT_EQ(queue->get_queue_size(), 3);
    ASSERT_EQ(txs.size(), 3);
    ASSERT_EQ(txs[0]->get_transaction()->get_tx_nonce(), 1);
    ASSERT_EQ(txs[1]->get_transaction()->get_tx_nonce(), 2);
    ASSERT_EQ(txs[2]->get_transaction()->get_tx_nonce(), 3);
}

// when tx nonce conflict, should not get next tx
TEST_F(test_tx_queue, tx_order_3_send) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t                     now = xverifier::xtx_utl::get_gmttime_s();
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t("aaaa", 0, {}, &table);

    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, 0, now + 1);
    ASSERT_EQ(0, queue->push_send_tx(tx1));

    xcons_transaction_ptr_t tx2 = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, tx1->get_transaction()->get_tx_nonce(), now + 1, tx1->get_transaction()->digest());
    ASSERT_EQ(0, queue->push_send_tx(tx2));

    xcons_transaction_ptr_t tx3 =
        test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, tx1->get_transaction()->get_tx_nonce(), now + 1, tx1->get_transaction()->digest(), 0, 300);
    ASSERT_EQ(0, queue->push_send_tx(tx3));

    xcons_transaction_ptr_t tx4 = test_xtxpool_util_t::create_cons_transfer_tx(0 , 1, tx3->get_transaction()->get_tx_nonce(), now + 1, tx3->get_transaction()->digest());
    ASSERT_EQ(0, queue->push_send_tx(tx4));

    std::vector<xcons_transaction_ptr_t> txs = queue->get_cons_txs(0, {});
    ASSERT_EQ(txs.size(), 3);
    ASSERT_EQ(txs[0]->get_transaction()->get_tx_nonce(), 1);
    ASSERT_EQ(txs[1]->get_transaction()->get_tx_nonce(), 2);
    ASSERT_EQ(txs[2]->get_transaction()->get_tx_nonce(), 3);
}

TEST_F(test_tx_queue, pop_event) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    uint64_t        now = xverifier::xtx_utl::get_gmttime_s();
    std::string     source_address = sys_contract_rec_tcc_addr;
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t(source_address, 0, {}, &table);
    ASSERT_EQ(true, data::is_sys_contract_address(common::xaccount_address_t{source_address}));

    uint256_t hash1;

    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 16);
    for (auto iter : txs) {
        ASSERT_EQ(0, queue->push_send_tx(iter));
    }

    for (auto iter = txs.rbegin(); iter != txs.rend(); iter++) {
        xcons_transaction_ptr_t tx = queue->pop_tx_by_hash(iter->get()->get_transaction()->digest(), enum_transaction_subtype_send, 0);
        ASSERT_NE(tx, nullptr);
    }

    {
        xcons_transaction_ptr_t tx = queue->get_tx();
        ASSERT_EQ(tx, nullptr);
    }

    queue->close();
    queue->release_ref();
}

TEST_F(test_tx_queue, pop_recv_tx) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);

    std::string                          sender = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde1", 1);
    std::string                          receiver = base::xvaccount_t::make_account_address(base::enum_vaccount_addr_type_secp256k1_user_account, 0, "1234567890abcde2", 1);
    std::vector<xcons_transaction_ptr_t> raw_txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 11);
    xtxpool_table_t table(0, 0, para);
    xaccountobj_t * queue = new xaccountobj_t(receiver, 0, {}, &table);

    base::xvblock_t * genesis_block = data::xblocktool_t::create_genesis_empty_unit(sender);
    store_ptr->set_vblock(genesis_block);

    xlightunit_block_para_t block_para;
    block_para.set_input_txs(raw_txs);
    block_para.set_balance_change(100);

    // create receipt by block derictly
    base::xvblock_t *                    proposal_block = test_blocktuil::create_next_lightunit_with_consensus(block_para, genesis_block);
    auto                                 block = dynamic_cast<data::xblock_t *>(proposal_block);
    data::xlightunit_block_t *           lightunit = dynamic_cast<data::xlightunit_block_t *>(block);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    lightunit->create_txreceipts(sendtx_receipts, recvtx_receipts);

    uint256_t hash1 = sendtx_receipts[0]->get_transaction()->digest();
    ASSERT_EQ(0, queue->push_recv_tx(sendtx_receipts[0]));

//    {
//        xcons_transaction_ptr_t tx = queue->pop_tx_by_hash(hash1, enum_transaction_subtype_recv, 0);
//        ASSERT_NE(tx, nullptr);
//    }
    queue->close();
    queue->release_ref();
}