#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xtxpool_v2/xunconfirmed_tx_queue.h"
#include "xverifier/xverifier_utl.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_unconfirmed_tx_queue : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_unconfirmed_tx_queue, unconfirmed_tx_queue_basic) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_shard_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    xtx_para_t para;

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    store::xstore_face_t * xstore = creator.get_xstore();

    xtxpool_resources resources(make_observer(xstore), make_observer(blockstore), nullptr, nullptr);

    xunconfirmed_tx_queue_t unconfirmed_tx_queue(&resources, &table_para);

    // construct account
    std::vector<std::string> unit_addrs = xdatamock_address::make_multi_user_address_in_table(table_addr, 2);
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];
    mock::xdatamock_table mocktable(table_addr, unit_addrs);

    // insert committed txs to blockstore
    uint32_t tx_num = 1;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xtablestate_ptr_t table_state = mocktable.get_table_state();
    xtable_state_cache_t table_state_cache(&resources, table_addr);
    table_state_cache.update(table_state);

    base::xvaccount_t _vaccount(sender);
    base::xauto_ptr<base::xvblock_t> unitblock = blockstore->get_latest_committed_block(_vaccount);

    blockstore->load_block_input(_vaccount, unitblock.get());
    xblock_t * block = dynamic_cast<xblock_t *>(unitblock.get());
    unconfirmed_tx_queue.udpate_latest_confirmed_block(block, table_state_cache);

    auto tx_find = unconfirmed_tx_queue.find(send_txs[0]->get_source_addr(), send_txs[0]->get_transaction()->digest());
    ASSERT_NE(tx_find, nullptr);

    auto resend_txs1 = unconfirmed_tx_queue.get_resend_txs(_tableblock1.get()->get_timestamp() + 50);
    ASSERT_EQ(resend_txs1.size(), 0);

    auto resend_txs2 = unconfirmed_tx_queue.get_resend_txs(_tableblock1.get()->get_timestamp() + 301);
    ASSERT_EQ(resend_txs2.size(), 1);
}

TEST_F(test_unconfirmed_tx_queue, recover) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_shard_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    xtx_para_t para;

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    store::xstore_face_t * xstore = creator.get_xstore();

    auto mbus = std::make_shared<top::mbus::xmessage_bus_t>();
    xtxpool_resources resources(make_observer(xstore), make_observer(blockstore), nullptr, make_observer(mbus.get()));

    xunconfirmed_tx_queue_t unconfirmed_tx_queue(&resources, &table_para);

    // construct account
    std::vector<std::string> unit_addrs = xdatamock_address::make_multi_user_address_in_table(table_addr, 2);
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];
    mock::xdatamock_table mocktable(table_addr, unit_addrs);

    // insert committed txs to blockstore
    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    const std::vector<xblock_ptr_t> & tableblocks = mocktable.get_history_tables();
    for (auto & block : tableblocks) {
        ASSERT_TRUE(blockstore->store_block(mocktable, block.get()));
    }

    xtablestate_ptr_t table_state = mocktable.get_table_state();
    xtable_state_cache_t table_state_cache(&resources, table_addr);
    table_state_cache.update(table_state);
    unconfirmed_tx_queue.recover(table_state_cache, table_state);
    for (uint32_t i = 0; i < tx_num; i++) {
        // std::cout << "i:" << i << std::endl;
        auto tx_find = unconfirmed_tx_queue.find(send_txs[i]->get_source_addr(), send_txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_find, nullptr);
    }

    auto resend_txs1 = unconfirmed_tx_queue.get_resend_txs(_tableblock1->get_timestamp() + 301);
    ASSERT_EQ(resend_txs1.size(), 1);

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());

    std::vector<xcons_transaction_ptr_t> recv_txs012(recv_txs.begin(), recv_txs.begin() + 3);
    mocktable.push_txs(recv_txs012);
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock2n1 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock2n2 = mocktable.generate_one_table();
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock2.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock2n1.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock2n2.get()));

    std::vector<xcons_transaction_ptr_t> confirm_txs012 = mocktable.create_receipts(_tableblock2);
    xassert(confirm_txs012.size() == recv_txs012.size());

    mocktable.push_txs(confirm_txs012);
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock3n1 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock3n2 = mocktable.generate_one_table();
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock3.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock3n1.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock3n2.get()));

    xtablestate_ptr_t table_state2 = mocktable.get_table_state();
    xtable_state_cache_t table_state_cache2(&resources, table_addr);
    table_state_cache2.update(table_state2);
    unconfirmed_tx_queue.recover(table_state_cache2, table_state2);
    for (uint32_t i = 0; i < 3; i++) {
        // std::cout << "i:" << i << std::endl;
        auto tx_find = unconfirmed_tx_queue.find(send_txs[i]->get_source_addr(), send_txs[i]->get_transaction()->digest());
        ASSERT_EQ(tx_find, nullptr);
    }

    for (uint32_t i = 3; i < tx_num; i++) {
        // std::cout << "i:" << i << std::endl;
        auto tx_find = unconfirmed_tx_queue.find(send_txs[i]->get_source_addr(), send_txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_find, nullptr);
    }

    auto resend_txs2 = unconfirmed_tx_queue.get_resend_txs(_tableblock2->get_timestamp() + 301);
    ASSERT_EQ(resend_txs2.size(), 1);

    std::vector<xcons_transaction_ptr_t> recv_txs34(recv_txs.begin() + 3, recv_txs.end());
    mocktable.push_txs(recv_txs34);
    xblock_ptr_t _tableblock4 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock4n1 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock4n2 = mocktable.generate_one_table();
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock4.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock4n1.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock4n2.get()));

    std::vector<xcons_transaction_ptr_t> confirm_txs34 = mocktable.create_receipts(_tableblock4);
    xassert(confirm_txs34.size() == recv_txs34.size());

    mocktable.push_txs(confirm_txs34);
    xblock_ptr_t _tableblock5 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock5n1 = mocktable.generate_one_table();
    xblock_ptr_t _tableblock5n2 = mocktable.generate_one_table();
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock5.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock5n1.get()));
    ASSERT_TRUE(blockstore->store_block(mocktable, _tableblock5n2.get()));

    xtablestate_ptr_t table_state3 = mocktable.get_table_state();
    xtable_state_cache_t table_state_cache3(&resources, table_addr);
    table_state_cache3.update(table_state3);
    unconfirmed_tx_queue.recover(table_state_cache3, table_state3);
    for (uint32_t i = 0; i < tx_num; i++) {
        // std::cout << "i:" << i << std::endl;
        auto tx_find = unconfirmed_tx_queue.find(send_txs[i]->get_source_addr(), send_txs[i]->get_transaction()->digest());
        ASSERT_EQ(tx_find, nullptr);
    }

    auto resend_txs3 = unconfirmed_tx_queue.get_resend_txs(_tableblock2->get_timestamp() + 301);
    ASSERT_EQ(resend_txs3.size(), 0);
}

