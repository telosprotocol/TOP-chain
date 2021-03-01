#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_base.h"
#include "xtxpool_v2/xtxpool_error.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_txmgr_table : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_txmgr_table, sigle_send_tx) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now, last_tx_hash);
    tx->get_transaction()->set_push_pool_timestamp(now);

    xtx_para_t para;

    // push first time
    int32_t ret = txmgr_table.push_tx(tx, para);
    ASSERT_EQ(0, ret);
    auto tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);

    // duplicate push
    ret = txmgr_table.push_tx(tx, para);
    ASSERT_EQ(xtxpool_error_tx_duplicate, ret);

    // pop out
    auto tx_ent_tmp = txmgr_table.pop_tx_by_hash(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest(), tx->get_tx_subtype(), 0);
    ASSERT_NE(tx_ent_tmp, nullptr);
    tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_EQ(tx_tmp, nullptr);

    // push again
    ret = txmgr_table.push_tx(tx, para);
    ASSERT_EQ(0, ret);
    tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);
}

TEST_F(test_txmgr_table, sigle_account_multi_send_tx) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    // push txs by inverted order, high nonce with high charge score
    for (uint32_t i = txs.size(); i > 0; i--) {
        xtx_para_t para;
        para.set_charge_score(i);
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[i - 1], para);
        int32_t ret = txmgr_table.push_tx(txs[i - 1], para);
        ASSERT_EQ(0, ret);
    }

    auto accounts = txmgr_table.get_accounts_txs(10);
    ASSERT_EQ(accounts.size(), 1);
    auto txs_ents = accounts[0]->get_txs(enum_transaction_subtype_send);
    ASSERT_EQ(txs_ents.size(), 3);

    accounts = txmgr_table.get_accounts_txs(10);
    ASSERT_EQ(accounts.size(), 1);
    txs_ents = accounts[0]->get_txs(enum_transaction_subtype_send);
    ASSERT_EQ(txs_ents.size(), 3);

    accounts = txmgr_table.get_accounts_txs(10);
    ASSERT_EQ(accounts.size(), 1);
    txs_ents = accounts[0]->get_txs(enum_transaction_subtype_send);
    ASSERT_EQ(txs_ents.size(), 3);

    accounts = txmgr_table.get_accounts_txs(10);
    ASSERT_EQ(accounts.size(), 1);
    txs_ents = accounts[0]->get_txs(enum_transaction_subtype_send);
    ASSERT_EQ(txs_ents.size(), 1);
}

#if 0
TEST_F(test_txmgr_table, sigle_account_uncontinuous_send_txs) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[3], para);
        int32_t ret = txmgr_table.push_tx(tx_ent);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[4], para);
        int32_t ret = txmgr_table.push_tx(tx_ent);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[6], para);
        int32_t ret = txmgr_table.push_tx(tx_ent);
        ASSERT_EQ(xtxpool_error_tx_nonce_incontinuity, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[5], para);
        int32_t ret = txmgr_table.push_tx(tx_ent);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[2], para);
        int32_t ret = txmgr_table.push_tx(tx_ent);
        ASSERT_EQ(0, ret);
    }

    for (uint32_t i = 2; i < 5; i++) {
        auto tx_tmp = txmgr_table.query_tx(txs[i]->get_transaction()->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_tmp, nullptr);
    }

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[0], para);
        int32_t ret = txmgr_table.push_tx(tx_ent);
        ASSERT_EQ(0, ret);
    }

    auto tx_ents = txmgr_table.pop_send_txs(10);
    ASSERT_EQ(tx_ents.size(), 1);
}
#endif
