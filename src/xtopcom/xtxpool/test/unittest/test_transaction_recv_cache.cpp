#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xbase/xvledger.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xtransaction_maker.hpp"
#include "xloader/xconfig_onchain_loader.h"
#include "xstore/test/test_datamock.hpp"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_face.h"
#include "xtxpool/xtransaction_recv_cache.h"
#include "xtxpool/xtxpool_error.h"
#include "xtxpool/xtxpool_para.h"
#include "xtxpool/xtxpool_table.h"
#include "xblockstore/xblockstore_face.h"
#include "xtestca.hpp"
#include "xcertauth/xcertauth_face.h"
#include "xelection/xvnode_house.h"
#include "xbasic/xasio_io_context_wrapper.h"
#include "xbasic/xtimer_driver.h"

using namespace top::xtxpool;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::test;

class xchain_timer_mock final : public time::xchain_time_face_t {
public:
    void update_time(common::xlogic_time_t, time::xlogic_timer_update_strategy_t) override {
        return;
    }

    //void restore_last_db_time() override {}

    // time::xchain_time_st const & get_local_time() const noexcept override {
    //     static time::xchain_time_st st;
    //     st.xtime_round = 1;
    //     return st;
    // }

    void start() override {
    }

    void stop() override {
    }

    common::xlogic_time_t logic_time() const noexcept override { return timer_height; }

    bool watch(const std::string &, uint64_t, time::xchain_time_watcher) override { return true; }

    bool watch_one(uint64_t, time::xchain_time_watcher) override { return true; }

    bool unwatch(const std::string &) override { return true; }

    void init() override {}

    void                close() override {}
    base::xiothread_t * get_iothread() const noexcept override { return nullptr; }

    uint64_t timer_height{0};
};

class test_transaction_recv_cache : public testing::Test {
protected:
    void SetUp() override {
        m_chain_timer = std::make_shared<xchain_timer_mock>();
    }

    void TearDown() override {}
public:
    std::shared_ptr<xchain_timer_mock> m_chain_timer;
};

// void create_block(xobject_ptr_t<xstore_face_t> &store_ptr,
//                   std::string &receiver,
//                   xtableblock_make_result_t &tableblock_result,
//                   std::vector<xtransaction_ptr_t> &txs,
//                   uint64_t height,
//                   uint64_t timer_height) {
//     std::string identity = "YYYYYYY";
//     xtableblock_consensus_target_ptr_t _targets = make_object_ptr<xtableblock_consensus_target_t>();
//     _targets->m_object = "tableblock-0001";
//     _targets->leader_add_consensus_txs(identity, 0, receiver, height, txs, nullptr);

//     xtableblock_make_para_t tableblock_para(_targets, 0, timer_height);
//     xtransactions_packer_t packer(store_ptr, identity);
//     int32_t ret = packer.make_tableblock(tableblock_para, tableblock_result);

//     tableblock_result.m_block->calc_block_hash();
//     ASSERT_EQ(ret, 0);
//     ASSERT_NE(tableblock_result.m_block, nullptr);
// }

// void set_block_to_db(xobject_ptr_t<xstore_face_t> &store_ptr,
//                      std::string &receiver,
//                      xtableblock_make_result_t &tableblock_result,
//                      std::vector<xtransaction_ptr_t> &txs,
//                      uint64_t height,
//                      uint64_t timer_height,
//                      bool sync) {
//     create_block(store_ptr, receiver, tableblock_result, txs, height, timer_height);

//     if (sync) {
//         store_ptr->set_sync_block(tableblock_result.m_block);
//     } else {
//         store_ptr->set_block(tableblock_result.m_block);
//     }
// }

TEST_F(test_transaction_recv_cache, one_duplicate_tx) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    std::shared_ptr<xtransaction_recv_cache_t> cache = std::make_shared<xtransaction_recv_cache_t>(para, "T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0");
    para->set_receipt_valid_window_day(10);

    std::string sender = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string receiver = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xvblock_t *                    sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);

    xblock_t * table_block = test_xtxpool_util_t::create_tableblock_with_cons_txs_with_next_two_emptyblock(store_ptr.get(), receiver, sender_genesis_block, txs, 1, 3);

    cache->update_tx_cache(table_block);
    std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
    int32_t ret = cache->is_receipt_duplicated(1, txs[0]->get_transaction(), committed_recv_txs);
    ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_duplicate);
}

TEST_F(test_transaction_recv_cache, ten_duplicate_tx) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    std::shared_ptr<xtransaction_recv_cache_t> cache = std::make_shared<xtransaction_recv_cache_t>(para, "T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0");
    para->set_receipt_valid_window_day(10);

    std::string sender = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string receiver = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xvblock_t *                    sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 10);

    xblock_t * table_block = test_xtxpool_util_t::create_tableblock_with_cons_txs_with_next_two_emptyblock(store_ptr.get(), receiver, sender_genesis_block, txs, 1, 3);

    cache->update_tx_cache(table_block);

    for (int32_t i = 0; i < 10; i++) {
        std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
        int32_t ret = cache->is_receipt_duplicated(1, txs[i]->get_transaction(), committed_recv_txs);
        ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_duplicate);
    }
}

TEST_F(test_transaction_recv_cache, one_expired_tx) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    std::shared_ptr<xtransaction_recv_cache_t> cache = std::make_shared<xtransaction_recv_cache_t>(para, "T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0");
    para->set_receipt_valid_window_day(1);

    std::string sender = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string receiver = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xvblock_t *                    sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);

    xblock_t * table_block = test_xtxpool_util_t::create_tableblock_with_cons_txs_with_next_two_emptyblock(store_ptr.get(), receiver, sender_genesis_block, txs, 1, 3);

    cache->update_tx_cache(table_block);
    m_chain_timer->timer_height = (24 * 360 + 1000);
    std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
    int32_t ret = cache->is_receipt_duplicated(1, txs[0]->get_transaction(), committed_recv_txs);
    ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_expired);
}

TEST_F(test_transaction_recv_cache, one_pass_tx) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    std::shared_ptr<xtransaction_recv_cache_t> cache = std::make_shared<xtransaction_recv_cache_t>(para, "T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0");
    para->set_receipt_valid_window_day(10);

    std::string sender = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string receiver = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xvblock_t *                    sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 2);

    std::vector<xcons_transaction_ptr_t> txs1;
    txs1.push_back(txs[0]);

    xblock_t * table_block = test_xtxpool_util_t::create_tableblock_with_cons_txs_with_next_two_emptyblock(store_ptr.get(), receiver, sender_genesis_block, txs1, 1, 3);

    cache->update_tx_cache(table_block);
    std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
    int32_t ret = cache->is_receipt_duplicated(1, txs[1]->get_transaction(), committed_recv_txs);
    ASSERT_EQ(ret, xsuccess);
}

TEST_F(test_transaction_recv_cache, db_empty) {
    std::shared_ptr<top::mbus::xmessage_bus_t> mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto                                       store_ptr = store::xstore_factory::create_store_with_memdb(make_observer(mbus));
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    auto para = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, nullptr, nullptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    std::shared_ptr<xtransaction_recv_cache_t> cache = std::make_shared<xtransaction_recv_cache_t>(para, "T-a1-3T7BKn5pP8Zi3K5z2Z5BQuSXTf5u37Se79x@0");
    para->set_receipt_valid_window_day(10);

    std::string sender = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string receiver = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xvblock_t *                    sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 2);

    xblock_t * table_block = test_xtxpool_util_t::create_tableblock_with_cons_txs_with_next_two_emptyblock(store_ptr.get(), receiver, sender_genesis_block, txs, 1, 3);

    std::vector<std::pair<std::string, uint256_t>> committed_recv_txs;
    int32_t ret = cache->is_receipt_duplicated(1, txs[0]->get_transaction(), committed_recv_txs);
    ASSERT_EQ(ret, xsuccess);
}

TEST_F(test_transaction_recv_cache, push_recv_tx_1) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr, make_observer(m_chain_timer));

    std::string table_account = xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0);
    xblock_t* genesis_block = (xblock_t*)test_blocktuil::create_genesis_empty_table(table_account);
    xassert(true == blockstore->store_block(genesis_block));

    std::vector<base::xvblock_t*> blocks;

    base::xvblock_t* prev_block = genesis_block;
    uint64_t clock = 10;
    for (uint64_t i = 1; i < 80; i++) {
        xblock_t* empty_block = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block, clock++);
        blocks.push_back(empty_block);
        prev_block = empty_block;
    }

    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);
    xtable_block_t * tx_tableblock1;
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(txs);
        base::xvblock_t * sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, sender_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock1 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock1;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock1, clock++);
        prev_block = empty_block2;

        blocks.push_back(tx_tableblock1);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);

        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        tx_tableblock1->create_txreceipts(sendtx_receipts, recvtx_receipts);
        assert(!sendtx_receipts.empty());
        for (auto iter : sendtx_receipts) {
            iter->set_commit_prove_with_parent_cert(empty_block2->get_cert());
        }
    }

    xtable_block_t * tx_tableblock2;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(sendtx_receipts);
        base::xvblock_t * recv_genesis_block = xblocktool_t::create_genesis_empty_unit(receiver);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, recv_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock2 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock2;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock2, clock++);
        prev_block = empty_block2;
        blocks.push_back(tx_tableblock2);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);
    }


    blockstore->store_block(*blocks.rbegin());
    xtxpool->on_block_confirmed((xblock_t*)*blocks.rbegin());
    // chain_timer->update_time(prev_block->get_clock() + 1);
    usleep(10000);

    {
        int32_t ret = xtxpool->push_recv_tx(sendtx_receipts[0]);
        ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_data_not_synced);
        auto account_tx_map = xtxpool->get_txs(prev_block->get_account(), prev_block->get_height());
        ASSERT_EQ(0, account_tx_map.size());
    }

    for (auto iter = blocks.rbegin(); iter != blocks.rend(); iter++) {
        blockstore->store_block(*iter);
        xtxpool->on_block_confirmed((xblock_t*)*iter);
    }
    usleep(100000);

    auto account_tx_map = xtxpool->get_txs(prev_block->get_account(), prev_block->get_height());
    ASSERT_EQ(0, account_tx_map.size());
}

TEST_F(test_transaction_recv_cache, push_recv_tx_2) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr, make_observer(m_chain_timer));

    std::string table_account = xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0);
    xblock_t* genesis_block = (xblock_t*)test_blocktuil::create_genesis_empty_table(table_account);
    xassert(true == blockstore->store_block(genesis_block));

    std::vector<base::xvblock_t*> blocks;

    base::xvblock_t* prev_block = genesis_block;
    uint64_t clock = 10;
    for (uint64_t i = 1; i < 80; i++) {
        xblock_t* empty_block = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block, clock++);
        blocks.push_back(empty_block);
        prev_block = empty_block;
    }

    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 2);
    xtable_block_t * tx_tableblock1;
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(txs);
        base::xvblock_t * sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, sender_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock1 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock1;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock1, clock++);
        prev_block = empty_block2;

        blocks.push_back(tx_tableblock1);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);

        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        tx_tableblock1->create_txreceipts(sendtx_receipts, recvtx_receipts);
        assert(!sendtx_receipts.empty());
        for (auto iter : sendtx_receipts) {
            iter->set_commit_prove_with_parent_cert(empty_block2->get_cert());
        }
    }

    xtable_block_t * tx_tableblock2;
    {
        xlightunit_block_para_t para;
        std::vector<xcons_transaction_ptr_t> sendtx_receipts_0;
        sendtx_receipts_0.push_back(sendtx_receipts[0]);
        para.set_input_txs(sendtx_receipts_0);
        base::xvblock_t * recv_genesis_block = xblocktool_t::create_genesis_empty_unit(receiver);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, recv_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock2 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock2;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock2, clock++);
        prev_block = empty_block2;
        blocks.push_back(tx_tableblock2);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);
    }


    blockstore->store_block(*blocks.rbegin());
    xtxpool->on_block_confirmed((xblock_t*)*blocks.rbegin());
    // chain_timer->update_time(prev_block->get_clock() + 1);
    usleep(10000);

    {
        int32_t ret = xtxpool->push_recv_tx(sendtx_receipts[0]);
        ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_data_not_synced);
        ret = xtxpool->push_recv_tx(sendtx_receipts[1]);
        ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_data_not_synced);
        auto account_tx_map = xtxpool->get_txs(prev_block->get_account(), prev_block->get_height());
        ASSERT_EQ(0, account_tx_map.size());
    }

    for (auto iter = blocks.rbegin(); iter != blocks.rend(); iter++) {
        blockstore->store_block(*iter);
        xtxpool->on_block_confirmed((xblock_t*)*iter);
    }
    usleep(100000);

    {
        int32_t ret = xtxpool->push_recv_tx(sendtx_receipts[0]);
        ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_duplicate);
        ret = xtxpool->push_recv_tx(sendtx_receipts[1]);
        ASSERT_EQ(ret, 0);
        auto account_tx_map = xtxpool->get_txs(prev_block->get_account(), prev_block->get_height());
        ASSERT_EQ(1, account_tx_map.size());
    }
}

TEST_F(test_transaction_recv_cache, push_recv_tx_3) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
// #ifdef MOCK_CA
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);
// #else
//     common::xnode_id_t node_id { "T00000LgGPqEpiK6XLCKRj9gVPN8Ej1aMbyAb3Hu" };
//     std::string sign_key = "ONhWC2LJtgi9vLUyoa48MF3tiXxqWf7jmT9KtOg/Lwo=";
//     xobject_ptr_t<base::xvnodesrv_t> nodesvr_ptr = make_object_ptr<top::election::xvnode_house_t>(node_id, sign_key, blockstore, make_observer(mbus));
//     cert_ptr.attach(&auth::xauthcontext_t::instance(*nodesvr_ptr.get()));
// #endif
    auto     xtxpool = xtxpool_instance::create_xtxpool_inst(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr, make_observer(m_chain_timer));

    std::string table_account = xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0);
    xblock_t* genesis_block = (xblock_t*)test_blocktuil::create_genesis_empty_table(table_account);
    xassert(true == blockstore->store_block(genesis_block));

    std::vector<base::xvblock_t*> blocks;

    base::xvblock_t* prev_block = genesis_block;
    uint64_t clock = 10;
    for (uint64_t i = 1; i < 80; i++) {
        xblock_t* empty_block = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block, clock++);
        blocks.push_back(empty_block);
        prev_block = empty_block;
    }

    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 2);
    xtable_block_t * tx_tableblock1;
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(txs);
        base::xvblock_t * sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, sender_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock1 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock1;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock1, clock++);
        prev_block = empty_block2;

        blocks.push_back(tx_tableblock1);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);

        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        tx_tableblock1->create_txreceipts(sendtx_receipts, recvtx_receipts);
        assert(!sendtx_receipts.empty());
        for (auto iter : sendtx_receipts) {
            iter->set_commit_prove_with_parent_cert(empty_block2->get_cert());
        }
    }

    xschnorrcert_t * schnorrcert = dynamic_cast<xschnorrcert_t *>(cert_ptr.get());
    schnorrcert->set_verify_muti_sign_ret(base::enum_vcert_auth_result::enum_verify_fail);
    int32_t ret = xtxpool->push_recv_tx(sendtx_receipts[0]);
    ASSERT_EQ(ret, xtxpool_error_tx_multi_sign_error);
}

TEST_F(test_transaction_recv_cache, update_committed_block) {
    auto     mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    auto     store_ptr = xstore_factory::create_store_with_memdb(nullptr);
    xobject_ptr_t<base::xvblockstore_t> blockstore;
    blockstore.attach(store::xblockstorehub_t::instance().create_block_store(*store_ptr, ""));
    xobject_ptr_t<base::xvcertauth_t> cert_ptr;
    cert_ptr = make_object_ptr<xschnorrcert_t>((uint32_t)1);

    auto txpoolpara = std::make_shared<xtxpool_resources>(make_observer(store_ptr), blockstore, make_observer(mbus.get()), cert_ptr, make_observer(m_chain_timer), enum_xtxpool_order_strategy_default);
    xtxpool_table_t* txpool_table = new xtxpool_table_t(base::enum_chain_zone_consensus_index, 0, txpoolpara);

    std::string table_account = xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0);
    xblock_t* genesis_block = (xblock_t*)test_blocktuil::create_genesis_empty_table(table_account);
    xassert(true == blockstore->store_block(genesis_block));

    std::vector<base::xvblock_t*> blocks;

    base::xvblock_t* prev_block = genesis_block;
    uint64_t clock = 10;

    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0 , 1, 1);
    xtable_block_t * tx_tableblock1;
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(txs);
        base::xvblock_t * sender_genesis_block = xblocktool_t::create_genesis_empty_unit(sender);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, sender_genesis_block);
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock1 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock1;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock1, clock++);
        prev_block = empty_block2;

        blocks.push_back(tx_tableblock1);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);

        std::vector<xcons_transaction_ptr_t> recvtx_receipts;
        tx_tableblock1->create_txreceipts(sendtx_receipts, recvtx_receipts);
        assert(!sendtx_receipts.empty());
        for (auto iter : sendtx_receipts) {
            iter->set_commit_prove_with_parent_cert(empty_block2->get_cert());
        }
        int32_t ret = txpool_table->push_recv_tx(sendtx_receipts[0]);
        ASSERT_EQ(ret, 0);
        auto accounts = txpool_table->get_accounts();
        ASSERT_EQ(1, accounts.size());
    }

    xtable_block_t * tx_tableblock2;
    uint64_t unit_height;
    {
        xlightunit_block_para_t para;
        para.set_input_txs(sendtx_receipts);
        base::xvblock_t * recv_genesis_block = xblocktool_t::create_genesis_empty_unit(receiver);
        data::xlightunit_block_t*  lightunit = (xlightunit_block_t *)test_blocktuil::create_next_lightunit(para, recv_genesis_block);
        unit_height = lightunit->get_height();
        xtable_block_para_t table_para;
        table_para.add_unit(lightunit);
        tx_tableblock2 = (xtable_block_t *)test_blocktuil::create_next_tableblock(table_para, prev_block, clock++);
        prev_block = tx_tableblock2;
        xblock_t* empty_block1 = (xblock_t*)test_blocktuil::create_next_emptyblock(prev_block);
        prev_block = empty_block1;
        xblock_t* empty_block2 = (xblock_t*)test_blocktuil::create_next_emptyblock_with_justify(prev_block, tx_tableblock2, clock++);
        prev_block = empty_block2;
        blocks.push_back(tx_tableblock2);
        blocks.push_back(empty_block1);
        blocks.push_back(empty_block2);
        txpool_table->update_committed_unit_block((xblock_t*)lightunit, 0, {});
        auto accounts = txpool_table->get_accounts();
        ASSERT_EQ(0, accounts.size());
        int32_t ret = txpool_table->push_recv_tx(sendtx_receipts[0]);
        ASSERT_EQ(ret, 0);
        accounts = txpool_table->get_accounts();
        ASSERT_EQ(1, accounts.size());
        xblock_t * blocktmp = (xblock_t*)*blocks.rbegin();
        auto account_txs = txpool_table->get_account_txs(accounts[0], blocktmp->get_height(), unit_height, 0, {}, xverifier::xtx_utl::get_gmttime_s());
        ASSERT_EQ(0, account_txs.size());
        auto tx_tmp = txpool_table->query_tx(receiver, sendtx_receipts[0]->get_transaction()->digest());
        ASSERT_EQ(tx_tmp, sendtx_receipts[0]);
    }

    {
        blockstore->store_block(*blocks.rbegin());
        xblock_t * blocktmp = (xblock_t*)*blocks.rbegin();
        txpool_table->update_committed_table_block(tx_tableblock2);
        usleep(10000);
        auto accounts = txpool_table->get_accounts();
        ASSERT_EQ(0, accounts.size());
    }

    {
        int32_t ret = txpool_table->push_recv_tx(sendtx_receipts[0]);
        ASSERT_EQ(ret, xtxpool_error_sendtx_receipt_duplicate);
    }

    for (auto iter = blocks.rbegin(); iter != blocks.rend(); iter++) {
        blockstore->store_block(*iter);
        txpool_table->on_block_confirmed((xblock_t*)*iter);
    }
    usleep(100000);

    auto accounts = txpool_table->get_accounts();
    ASSERT_EQ(0, accounts.size());
}
