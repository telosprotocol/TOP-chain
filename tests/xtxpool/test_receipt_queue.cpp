#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xtx_receipt_queue.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xverifier/xverifier_utl.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_new_receipt_queue : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_new_receipt_queue, receipt_queue_basic) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xreceipt_queue_new_t receipt_queue(&table_para, &resource);

    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());

    for (uint32_t i = 0; i < tx_num; i++) {
        xassert(recv_txs[i]->is_recv_tx());
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(recv_txs[i], para);
        int32_t ret = receipt_queue.push_tx(tx_ent);
        ASSERT_EQ(ret, 0);
        auto find_receipt = receipt_queue.find(receiver, recv_txs[i]->get_transaction()->digest());
        ASSERT_NE(find_receipt, nullptr);
    }

    uint32_t confirm_txs_num;
    base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();
    auto receipts1 = receipt_queue.get_txs(10, 10, receiptid_state, confirm_txs_num);
    ASSERT_EQ(receipts1.size(), tx_num);

    xreceiptid_pair_t receiptid_pair(1, 0, 1);
    receiptid_state->add_pair(1, receiptid_pair);
    tx_info_t txinfo(sender, send_txs[0]->get_tx_hash_256(), enum_transaction_subtype_recv);
    receipt_queue.update_receipt_id_by_confirmed_tx(txinfo, 1, 1);

    auto receipts2 = receipt_queue.get_txs(10, 10, receiptid_state, confirm_txs_num);

    ASSERT_EQ(receipts2.size(), tx_num - 1);

    auto find_receipt0 = receipt_queue.find(receiver, recv_txs[0]->get_transaction()->digest());
    ASSERT_EQ(find_receipt0, nullptr);

    tx_info_t txinfo1(recv_txs[1]);
    auto pop_receipt = receipt_queue.pop_tx(txinfo1);
    ASSERT_NE(pop_receipt, nullptr);

    auto find_receipt = receipt_queue.find(receiver, recv_txs[1]->get_transaction()->digest());
    ASSERT_EQ(find_receipt, nullptr);
}
