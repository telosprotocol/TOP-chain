#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xverifier/xverifier_utl.h"
#include "xtxpool_v2/xpending_account.h"
#include "xtxpool_v2/xtxpool_error.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_pending_account : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_pending_account, sigle_send_tx) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xpending_accounts_t pending_accounts(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now, last_tx_hash);
    tx->set_push_pool_timestamp(now);

    xtx_para_t para;
    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);

    // push first time
    int32_t ret = pending_accounts.push_tx(tx_ent);
    ASSERT_EQ(0, ret);
    auto tx_tmp = pending_accounts.find(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);

    // duplicate push
    ret = pending_accounts.push_tx(tx_ent);
    ASSERT_EQ(xtxpool_error_tx_nonce_expired, ret);

    // pop out
    tx_info_t txinfo(tx);
    auto tx_ent_tmp = pending_accounts.pop_tx(txinfo, false);
    ASSERT_NE(tx_ent_tmp, nullptr);
    tx_tmp = pending_accounts.find(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_EQ(tx_tmp, nullptr);

    // push again
    ret = pending_accounts.push_tx(tx_ent);
    ASSERT_EQ(0, ret);
    tx_tmp = pending_accounts.find(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);
}

TEST_F(test_pending_account, sigle_account_multi_send_tx) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xpending_accounts_t pending_accounts(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    uint32_t txs_num = 3;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    // push txs by order
    for (uint32_t i = 0; i < txs.size(); i++) {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[i], para);
        int32_t ret = pending_accounts.push_tx(tx_ent);
        ASSERT_EQ(0, ret);
    }

    // pop one of the txs, index is around middle
    tx_info_t txinfo(txs[1]);
    auto tx_ent_tmp = pending_accounts.pop_tx(txinfo, false);
    ASSERT_NE(tx_ent_tmp, nullptr);

    std::shared_ptr<xtx_entry> tx_tmp;
    uint32_t i = 0;
    for (; i <= 1; i++) {
        tx_tmp = pending_accounts.find(txs[i]->get_transaction()->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_EQ(tx_tmp, nullptr);
    }

    for (; i < txs.size(); i++) {
        tx_tmp = pending_accounts.find(txs[i]->get_transaction()->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_tmp, nullptr);
    }
}
