#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xtxpool_v2/xunconfirmed_tx_queue.h"
#include "xverifier/xverifier_utl.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_unconfirmed_tx_queue : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_unconfirmed_tx_queue, unconfirmed_tx_queue_basic) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    xtxpool_resources resources(make_observer(xstore), make_observer(blockstore), nullptr, nullptr, nullptr);

    xunconfirmed_tx_queue_t unconfirmed_tx_queue(&resources, &table_para);

    // construct account
    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);

    // insert committed txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 1);
    xblock_t * block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, xstore, sender, txs);

    base::xreceiptid_state_ptr_t receiptid_state = make_object_ptr<base::xreceiptid_state_t>();
    xreceipt_state_cache_t receipt_state_cache;
    receipt_state_cache.update(receiptid_state);
    unconfirmed_tx_queue.udpate_latest_confirmed_block(block, receipt_state_cache);

    auto tx_find = unconfirmed_tx_queue.find(txs[0]->get_source_addr(), txs[0]->get_transaction()->digest());
    ASSERT_NE(tx_find, nullptr);

    auto resend_txs1 = unconfirmed_tx_queue.get_resend_txs(block->get_timestamp() + 50);
    ASSERT_EQ(resend_txs1.size(), 0);

    auto resend_txs2 = unconfirmed_tx_queue.get_resend_txs(block->get_timestamp() + 61);
    ASSERT_EQ(resend_txs2.size(), 1);
}

TEST_F(test_unconfirmed_tx_queue, recover) {
    std::string table_addr = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_consensus_index, 0);
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    xtxpool_resources resources(make_observer(xstore), make_observer(blockstore), nullptr, nullptr, nullptr);

    xunconfirmed_tx_queue_t unconfirmed_tx_queue(&resources, &table_para);

    // construct account
    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);

    // insert committed txs to blockstore
    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, tx_num);
    xblock_t * block = test_xtxpool_util_t::create_tableblock_with_send_txs_with_next_two_emptyblock(blockstore, xstore, sender, table_addr, txs, 100);

    base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();
    xreceiptid_pair_t receiptid_pair(0, 5, 0);
    receiptid_state->add_pair(0, receiptid_pair);

    xreceipt_state_cache_t receipt_state_cache;
    receipt_state_cache.update(receiptid_state);
    unconfirmed_tx_queue.recover(receipt_state_cache);

    for (uint32_t i = 0; i < tx_num; i++) {
        std::cout << "i:" << i << std::endl;
        auto tx_find = unconfirmed_tx_queue.find(txs[i]->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_find, nullptr);
    }

    auto resend_txs1 = unconfirmed_tx_queue.get_resend_txs(block->get_timestamp() + 61);
    ASSERT_EQ(resend_txs1.size(), tx_num);

    xreceiptid_pair_t receiptid_pair2(2, 3, 0);
    receiptid_state->add_pair(0, receiptid_pair2);
    receipt_state_cache.update(receiptid_state);
    unconfirmed_tx_queue.recover(receipt_state_cache);

    for (uint32_t i = 0; i < 2; i++) {
        auto tx_find = unconfirmed_tx_queue.find(txs[i]->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_EQ(tx_find, nullptr);
    }

    for (uint32_t i = 2; i < tx_num; i++) {
        auto tx_find = unconfirmed_tx_queue.find(txs[i]->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_find, nullptr);
    }

    auto resend_txs2 = unconfirmed_tx_queue.get_resend_txs(block->get_timestamp() + 61);
    ASSERT_EQ(resend_txs2.size(), tx_num - 2);

    xreceiptid_pair_t receiptid_pair3(5, 0, 0);
    receiptid_state->add_pair(0, receiptid_pair3);
    receipt_state_cache.update(receiptid_state);
    unconfirmed_tx_queue.recover(receipt_state_cache);

    auto resend_txs3 = unconfirmed_tx_queue.get_resend_txs(block->get_timestamp() + 61);
    ASSERT_EQ(resend_txs3.size(), 0);
}
