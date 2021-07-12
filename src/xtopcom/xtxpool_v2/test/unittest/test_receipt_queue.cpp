#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xtx_receipt_queue.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xverifier/xverifier_utl.h"
#include "tests/mock/xvchain_creator.hpp"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_new_receipt_queue : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
    std::vector<xcons_transaction_ptr_t> get_tx(base::xvblockstore_t * blockstore,
                                                xstore_face_t * store,
                                                std::string sender,
                                                std::string receiver,
                                                std::vector<xcons_transaction_ptr_t> txs,
                                                xblock_t ** block) {
        *block = test_xtxpool_util_t::create_unit_with_cons_txs(blockstore, store, sender, txs);
        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(*block);
        std::vector<xcons_transaction_ptr_t> receipts1;
        std::vector<xcons_transaction_ptr_t> receipts2;
        lightunit->create_txreceipts(receipts1, receipts2);

        return receipts1;
    }
};

TEST_F(test_new_receipt_queue, receipt_queue_basic) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xreceipt_queue_new_t receipt_queue(&table_para);

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    // construct account
    std::string sender = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(0));
    std::string receiver = xblocktool_t::make_address_user_account(test_xtxpool_util_t::get_account(1));

    uint32_t tx_num = 5;
    // insert committed txs to blockstore
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, tx_num);
    uint64_t receipt_id = 1;
    for (auto & tx : txs) {
        tx->set_current_receipt_id(0, receipt_id);
        receipt_id++;
    }
    
    xblock_t * block;
    std::vector<xcons_transaction_ptr_t> recvtxs = get_tx(blockstore, xstore, sender, receiver, txs, &block);


    for (uint32_t i = 0; i < tx_num; i++) {
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(recvtxs[i], para);
        int32_t ret = receipt_queue.push_tx(tx_ent);
        ASSERT_EQ(ret, 0);
        auto find_receipt = receipt_queue.find(receiver, recvtxs[i]->get_transaction()->digest());
        ASSERT_NE(find_receipt, nullptr);
    }

    base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();
    auto receipts1 = receipt_queue.get_txs(10, 10, receiptid_state);
    ASSERT_EQ(receipts1.size(), tx_num);

    xreceiptid_pair_t receiptid_pair(1, 0, 1);
    receiptid_state->add_pair(0, receiptid_pair);
    receipt_queue.update_receiptid_state(receiptid_state);

    auto receipts2 = receipt_queue.get_txs(10, 10, receiptid_state);
    ASSERT_EQ(receipts2.size(), tx_num - 1);
    
    auto find_receipt0 = receipt_queue.find(receiver, recvtxs[0]->get_transaction()->digest());
    ASSERT_EQ(find_receipt0, nullptr);

    tx_info_t txinfo(recvtxs[1]);
    auto pop_receipt = receipt_queue.pop_tx(txinfo);
    ASSERT_NE(pop_receipt, nullptr);

    auto find_receipt = receipt_queue.find(receiver, recvtxs[1]->get_transaction()->digest());
    ASSERT_EQ(find_receipt, nullptr);
}