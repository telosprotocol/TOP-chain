#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xunconfirm_raw_txs.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_unconfirm_raw_txs : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_unconfirm_raw_txs, unconfirm_raw_txs) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    auto table_sid = mocktable.get_short_table_id();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts(); 
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    auto tx_actions = data::xblockextract_t::unpack_txactions(_tableblock1.get());
    std::vector<xraw_tx_info> raw_txs;

    for (auto & txaction : tx_actions) {
        xtransaction_ptr_t _rawtx = _tableblock1->query_raw_transaction(txaction.get_tx_hash());
        raw_txs.push_back(xraw_tx_info(txaction.get_receipt_id_peer_tableid(), txaction.get_receipt_id(), _rawtx));
    }

    xunconfirm_raw_txs unconfirm_raw_txs(table_sid);
    unconfirm_raw_txs.add_raw_txs(raw_txs);
    for (auto & tx_tmp : raw_txs) {
        auto raw_tx = unconfirm_raw_txs.get_raw_tx(table_sid, tx_tmp.m_receipt_id);
        ASSERT_NE(raw_tx.get(), nullptr);
    }

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());

    mocktable.push_txs(recv_txs);
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(_tableblock2);
    xassert(confirm_txs.size() == send_txs.size());
    for (uint32_t i = 0; i < confirm_txs.size(); i++) {
        confirm_txs[i]->set_raw_tx(send_txs[i]->get_transaction());
    }

    mocktable.push_txs(confirm_txs);
    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    unconfirm_raw_txs.refresh(mocktable.get_table_state()->get_receiptid_state());
    for (auto & tx_tmp : raw_txs) {
        auto raw_tx = unconfirm_raw_txs.get_raw_tx(table_sid, tx_tmp.m_receipt_id);
        ASSERT_EQ(raw_tx.get(), nullptr);
    }
}
