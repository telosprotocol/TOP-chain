#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xnon_ready_account.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xdata/xverifier/xverifier_utl.h"
#include "xtxpool_v2/xreceipt_state_cache.h"
#include "tests/mock/xdatamock_table.hpp"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_non_ready_account : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_non_ready_account, non_ready_account_basic) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xnon_ready_accounts_t non_ready_accounts(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xtx_para_t para;

    uint32_t txs_num = 20;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    int32_t ret;
    for (uint32_t i = 0; i < 5; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = non_ready_accounts.push_tx(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }

    for (uint32_t i = 0; i < 5; i++) {
        auto find_tx = non_ready_accounts.find_tx(txs[i]->get_account_addr(), txs[i]->get_transaction()->digest());
        ASSERT_NE(find_tx.get(), nullptr);
    }

    auto accounts = non_ready_accounts.get_accounts();
    ASSERT_EQ(accounts.size(), 1);

    tx_info_t txinfo(txs[1]);
    auto poped_tx1 = non_ready_accounts.pop_tx(txinfo);
    ASSERT_NE(poped_tx1.get(), nullptr);

    auto find_tx1 = non_ready_accounts.find_tx(txs[1]->get_account_addr(), txs[1]->get_transaction()->digest());
    ASSERT_EQ(find_tx1.get(), nullptr);

    auto poped_txs = non_ready_accounts.pop_account_txs(txs[1]->get_account_addr());
    ASSERT_EQ(poped_txs.size(), 4);

    for (uint32_t i = 0; i < 5; i++) {
        auto find_tx = non_ready_accounts.find_tx(txs[i]->get_account_addr(), txs[i]->get_transaction()->digest());
        ASSERT_EQ(find_tx.get(), nullptr);
    }

    auto accounts2 = non_ready_accounts.get_accounts();
    ASSERT_EQ(accounts2.size(), 0);
}
