#include "xblock_common.hpp"
namespace top {
namespace test {
class xclock_block_test : public testing::Test {
protected:
    void SetUp() override {
        //block.init();
        // block.m_local_time = make_object_ptr<xlocal_time>(block.m_dispatchers,
        //                             top::base::xcontext_t::instance(),
        //                             block.global_worker_pool->get_thread_ids()[0]);
    }

    void TearDown() override {
        // delete m_engines;
    }

protected:
    xblock_common block;
};

TEST_F(xclock_block_test, run) {
    block.test_xblockstore_mock_enable = true;
    block.test_xstore_mock_enable = true;
    block.test_xtxpool_block_maker_mock_enable = true;

    block.init();
    block.m_local_time = make_object_ptr<xlocal_time>(block.m_dispatcher_builders,
                                top::base::xcontext_t::instance(),
                                block.global_worker_pool->get_thread_ids()[0]);

    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("***************create finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("***************start finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(200));
    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    cons_proxies.clear();
    xinfo("****************old fade!***************");
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(block.address_set[i]);
        EXPECT_TRUE(block.m_cons_mgrs[i]->destroy(xip));
    }
    xinfo("***************time out destroy!***************");
}

TEST_F(xclock_block_test, run_xblockstore) {
    block.test_xblockstore_mock_enable = false;
    block.test_xstore_mock_enable = true;
    block.init();
    block.m_local_time = make_object_ptr<xlocal_time>(block.m_dispatcher_builders,
                                top::base::xcontext_t::instance(),
                                block.global_worker_pool->get_thread_ids()[0]);

    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("***************create finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("***************start finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(200));
    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    cons_proxies.clear();
    xinfo("****************old fade!***************");
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(block.address_set[i]);
        EXPECT_TRUE(block.m_cons_mgrs[i]->destroy(xip));
    }
    xinfo("***************time out destroy!***************");
}

TEST_F(xclock_block_test, run_emptyblock_xblockstore_xstore) {
    block.test_xblockstore_mock_enable = false;
    block.test_xstore_mock_enable = false;
    block.test_xtxpool_block_maker_mock_enable = true;
    block.init();
    block.m_local_time = make_object_ptr<xlocal_time>(block.m_dispatcher_builders,
                                top::base::xcontext_t::instance(),
                                block.global_worker_pool->get_thread_ids()[0]);

    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("***************create finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("***************start finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(200));
    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    cons_proxies.clear();
    xinfo("****************old fade!***************");
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(block.address_set[i]);
        EXPECT_TRUE(block.m_cons_mgrs[i]->destroy(xip));
    }
    xinfo("***************time out destroy!***************");
}

uint64_t get_gmttime_s() {
    struct timeval val;
    base::xtime_utl::gettimeofday(&val);
    return (uint64_t)val.tv_sec;
}

void timer_thread_create_transaction(std::string tableaddr, xblock_common* env, uint32_t txcount) {
    auto xid = base::xvaccount_t::get_xid_from_account(tableaddr);
    uint8_t zone = get_vledger_zone_index(xid);
    uint16_t subaddr = get_vledger_subaddr(xid);
    std::string account_1 = data::xblocktool_t::make_address_shard_sys_account("11111111111111111111", subaddr);
    std::string account_2 = data::xblocktool_t::make_address_shard_sys_account("11111111111111111112", subaddr);
#if 1
    auto xid1 = base::xvaccount_t::get_xid_from_account(account_1);
    uint8_t zone1 = get_vledger_zone_index(xid1);
    uint16_t subaddr1 = get_vledger_subaddr(xid1);
    xassert(zone == zone1);
    xassert(subaddr == subaddr1);
#endif
    for (size_t i = 0; i < env->m_cons_mgrs.size(); i++) {
        auto genesis_block = xblocktool_t::create_genesis_lightunit(account_1, 1000000000);
        xassert(genesis_block != nullptr);
        env->store_set[i]->set_vblock(genesis_block);
    }

    auto blockchain = env->store_set[0]->clone_account(account_1);
    xassert(blockchain != nullptr);
    for (uint32_t i = 0; i < txcount; i++) {
        //std::this_thread::sleep_for(std::chrono::seconds(1));
        auto tx = blockchain->make_transfer_tx(account_2, 10, get_gmttime_s(), 1000, 0);

        // TODO(jimmy) need case: push transaction to one node only
        for (size_t j = 0; j < env->m_cons_mgrs.size(); j++) {
            xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
            xtx_para_t para;
            std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(cons_tx, para);
            auto ret = env->txpool_set[j]->push_send_tx(tx_ent);
            //xassert(ret == 0);
        }
    }
}

TEST_F(xclock_block_test, run_txpool_maker_1) {
    block.test_xblockstore_mock_enable = false;
    block.test_xstore_mock_enable = false;
    block.test_xtxpool_block_maker_mock_enable = false;
    block.init();
    block.m_local_time = make_object_ptr<xlocal_time>(block.m_dispatcher_builders,
                                top::base::xcontext_t::instance(),
                                block.global_worker_pool->get_thread_ids()[0]);

    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("***************create finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::thread t1 = std::thread(timer_thread_create_transaction, block.table_address, &block, 50);
    t1.join();

    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("***************start finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(1000));

    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    cons_proxies.clear();
    xinfo("****************old fade!***************");
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(block.address_set[i]);
        EXPECT_TRUE(block.m_cons_mgrs[i]->destroy(xip));
    }
    xinfo("***************time out destroy!***************");
}

void timer_thread_multi_account_create_transaction(std::string tableaddr, xblock_common* env, uint32_t txcount) {
    auto xid = base::xvaccount_t::get_xid_from_account(tableaddr);
    uint8_t zone = get_vledger_zone_index(xid);
    uint16_t subaddr = get_vledger_subaddr(xid);
    const uint32_t test_account_count = 5;
    std::string account_send[test_account_count];
    std::string account_recv[test_account_count];

    for (uint32_t i = 0; i < test_account_count; i++) {
        account_send[i] = data::xblocktool_t::make_address_shard_sys_account("11111111111111111111" + std::to_string(i), subaddr);
        account_recv[i] = data::xblocktool_t::make_address_shard_sys_account("11111111111111111112" + std::to_string(i), subaddr);
    }


#if 1
    auto xid1 = base::xvaccount_t::get_xid_from_account(account_send[0]);
    uint8_t zone1 = get_vledger_zone_index(xid1);
    uint16_t subaddr1 = get_vledger_subaddr(xid1);
    xassert(zone == zone1);
    xassert(subaddr == subaddr1);
#endif
    for (size_t i = 0; i < env->m_cons_mgrs.size(); i++) {
        for (uint32_t j = 0; j < test_account_count; j++) {
            auto genesis_block = xblocktool_t::create_genesis_lightunit(account_send[j], 1000000000);
            xassert(genesis_block != nullptr);
            env->store_set[i]->set_vblock(genesis_block);
        }
    }

    for (uint32_t k = 0; k < txcount; k++) {
        for (size_t i = 0; i < env->m_cons_mgrs.size(); i++) {
            for (uint32_t j = 0; j < test_account_count; j++) {
                auto blockchain = env->store_set[i]->clone_account(account_send[j]);
                auto tx = blockchain->make_transfer_tx(account_recv[j], 1, get_gmttime_s(), 1000, 0);

                xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
                xtx_para_t para;
                std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(cons_tx, para);
                auto ret = env->txpool_set[j]->push_send_tx(tx_ent);
            }
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

TEST_F(xclock_block_test, run_txpool_maker_2) {
    block.test_xblockstore_mock_enable = false;
    block.test_xstore_mock_enable = false;
    block.test_xtxpool_block_maker_mock_enable = false;
    block.init();
    block.m_local_time = make_object_ptr<xlocal_time>(block.m_dispatcher_builders,
                                top::base::xcontext_t::instance(),
                                block.global_worker_pool->get_thread_ids()[0]);

    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("***************create finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::thread t1 = std::thread(timer_thread_multi_account_create_transaction, block.table_address, &block, 100);

    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("***************start finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(1000));
    t1.join();

    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    cons_proxies.clear();
    xinfo("****************old fade!***************");
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(block.address_set[i]);
        EXPECT_TRUE(block.m_cons_mgrs[i]->destroy(xip));
    }
    xinfo("***************time out destroy!***************");
}

TEST_F(xclock_block_test, roundinandout) {
    std::vector<xcons_proxy_face_ptr> cons_proxies;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies.push_back(cons_proxy);
    }
    xinfo("***************create finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies[i]->start(10));
    }
    xinfo("***************start finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(15));
    block.create_address("node1234567890abcdef", 2);
    std::vector<xcons_proxy_face_ptr> cons_proxies_new;
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        std::shared_ptr<vnetwork::xvnetwork_driver_face_t> pnet = std::make_shared<network_mock>(block.address_set[i]);
        auto cons_proxy = block.m_cons_mgrs[i]->create(pnet);
        EXPECT_NE(cons_proxy, nullptr);
        cons_proxies_new.push_back(cons_proxy);
    }
    xinfo("***************create new round finish!***************");
    for (size_t i = 0; i < cons_proxies.size(); i++) {
        auto cons_proxy = cons_proxies[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    cons_proxies.clear();
    xinfo("****************old fade!***************");
    std::this_thread::sleep_for(std::chrono::seconds(3));
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        EXPECT_TRUE(cons_proxies_new[i]->start(10));
    }
    xinfo("****************start new round finish!***************");
    std::this_thread::sleep_for(std::chrono::seconds(30));
    for (size_t i = 0; i < cons_proxies_new.size(); i++) {
        auto cons_proxy = cons_proxies_new[i];
        EXPECT_TRUE(cons_proxy->fade());
    }
    xinfo("**************new fade!***************");
    for (size_t i = 0; i < block.m_cons_mgrs.size(); i++) {
        auto xip = xcons_utl::to_xip2(block.address_set[i]);
        EXPECT_TRUE(block.m_cons_mgrs[i]->destroy(xip));
    }
    xinfo("***************time out destroy!***************");
}
}  // namespace test
}  // namespace top
