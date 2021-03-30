#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xverifier/xverifier_utl.h"

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
    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
    ASSERT_EQ(0, ret);
    auto tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);

    // duplicate push
    ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
    ASSERT_EQ(xtxpool_error_request_tx_repeat, ret);

    // pop out
    auto tx_ent_tmp = txmgr_table.pop_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest(), tx->get_tx_subtype(), false);
    ASSERT_NE(tx_ent_tmp, nullptr);
    tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_EQ(tx_tmp, nullptr);

    // push again
    ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }

    auto accounts1 = txmgr_table.pop_ready_accounts(10);
    ASSERT_EQ(accounts1.size(), 1);
    auto txs_ents = accounts1[0]->get_txs();
    ASSERT_EQ(txs_ents.size(), 3);

    auto accounts2 = txmgr_table.pop_ready_accounts(10);
    ASSERT_EQ(accounts2.size(), 0);
}

TEST_F(test_txmgr_table, duplicate_send_tx_to_pending) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtx_para_t para;

    xcons_transaction_ptr_t tx0 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now + 1, last_tx_hash);
    last_tx_hash = tx0->get_transaction()->digest();

    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(tx0, para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent0, 0, {});
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now + 2, last_tx_hash);
    last_tx_hash = tx1->get_transaction()->digest();
    uint256_t last_tx_hash_1 = last_tx_hash;

    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(tx1, para);
    ret = txmgr_table.push_send_tx(tx_ent1, 0, {});
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx2a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 3, last_tx_hash);
    last_tx_hash = tx2a->get_transaction()->digest();

    std::shared_ptr<xtx_entry> tx_ent2a = std::make_shared<xtx_entry>(tx2a, para);
    ret = txmgr_table.push_send_tx(tx_ent2a, 0, {});
    ASSERT_EQ(0, ret);

    ready_accounts_t ready_accounts = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts.size());
    auto txs = ready_accounts[0]->get_txs();
    ASSERT_EQ(3, txs.size());
    ASSERT_EQ(tx0->get_digest_hex_str(), txs[0]->get_digest_hex_str());
    ASSERT_EQ(tx1->get_digest_hex_str(), txs[1]->get_digest_hex_str());
    ASSERT_EQ(tx2a->get_digest_hex_str(), txs[2]->get_digest_hex_str());

    xcons_transaction_ptr_t tx2b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 4, last_tx_hash_1);
    last_tx_hash = tx2b->get_transaction()->digest();

    // ret = txmgr_table.push_send_tx(tx_ent0, 0, {});
    // ASSERT_EQ(0, ret);
    // ret = txmgr_table.push_send_tx(tx_ent1, 0, {});
    // ASSERT_EQ(0, ret);
    std::shared_ptr<xtx_entry> tx_ent2b = std::make_shared<xtx_entry>(tx2b, para);
    ret = txmgr_table.push_send_tx(tx_ent2b, 0, {});
    ASSERT_EQ(0, ret);

    auto tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_account_addr(), tx_ent2b->get_tx()->get_transaction()->digest());
    ASSERT_EQ(tx_q->get_tx()->get_digest_hex_str(), tx2b->get_digest_hex_str());

    ready_accounts_t ready_accounts2 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts2.size());
    auto txs2 = ready_accounts2[0]->get_txs();
    ASSERT_EQ(3, txs2.size());
    ASSERT_EQ(tx0->get_digest_hex_str(), txs2[0]->get_digest_hex_str());
    ASSERT_EQ(tx1->get_digest_hex_str(), txs2[1]->get_digest_hex_str());
    ASSERT_EQ(tx2a->get_digest_hex_str(), txs2[2]->get_digest_hex_str());

    txmgr_table.updata_latest_nonce(tx1->get_account_addr(), tx1->get_transaction()->get_tx_nonce(), tx1->get_transaction()->digest());

    ready_accounts_t ready_accounts3 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts3.size());
    auto txs3 = ready_accounts3[0]->get_txs();
    ASSERT_EQ(1, txs3.size());
    ASSERT_EQ(tx2a->get_digest_hex_str(), txs3[0]->get_digest_hex_str());

    tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_account_addr(), tx_ent2b->get_tx()->get_transaction()->digest());
    ASSERT_NE(tx_q, nullptr);

    txmgr_table.updata_latest_nonce(tx2a->get_account_addr(), tx2a->get_transaction()->get_tx_nonce(), tx2a->get_transaction()->digest());
    ready_accounts_t ready_accounts4 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(0, ready_accounts4.size());

    tx_q = txmgr_table.query_tx(tx2b->get_account_addr(), tx2b->get_transaction()->digest());
    ASSERT_EQ(tx_q, nullptr);

    tx_q = txmgr_table.query_tx(tx2a->get_account_addr(), tx2a->get_transaction()->digest());
    ASSERT_EQ(tx_q, nullptr);
}

TEST_F(test_txmgr_table, duplicate_send_tx_to_pending_2) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtx_para_t para;

    xcons_transaction_ptr_t tx0 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now + 1, last_tx_hash);
    last_tx_hash = tx0->get_transaction()->digest();
    uint256_t last_tx_hash_0 = last_tx_hash;
    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(tx0, para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent0, 0, {});
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx1a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now + 2, last_tx_hash);
    last_tx_hash = tx1a->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(tx1a, para);
    ret = txmgr_table.push_send_tx(tx_ent1, 1, last_tx_hash_0);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx2a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 3, last_tx_hash);
    last_tx_hash = tx2a->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent2a = std::make_shared<xtx_entry>(tx2a, para);
    ret = txmgr_table.push_send_tx(tx_ent2a, 1, last_tx_hash_0);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx3a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 3, now + 4, last_tx_hash);
    last_tx_hash = tx3a->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent3a = std::make_shared<xtx_entry>(tx3a, para);

    xcons_transaction_ptr_t tx1b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now + 5, last_tx_hash_0);
    last_tx_hash = tx1b->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent1b = std::make_shared<xtx_entry>(tx1b, para);
    ret = txmgr_table.push_send_tx(tx_ent1b, 1, last_tx_hash_0);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx2b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 6, last_tx_hash);
    last_tx_hash = tx2b->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent2b = std::make_shared<xtx_entry>(tx2b, para);

    ret = txmgr_table.push_send_tx(tx_ent3a, 1, last_tx_hash_0);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx3b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 3, now + 7, last_tx_hash);
    last_tx_hash = tx3b->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent3b = std::make_shared<xtx_entry>(tx3b, para);
    ret = txmgr_table.push_send_tx(tx_ent3b, 1, last_tx_hash_0);
    ASSERT_EQ(0, ret);

    ret = txmgr_table.push_send_tx(tx_ent2b, 1, last_tx_hash_0);
    ASSERT_EQ(0, ret);

    ready_accounts_t ready_accounts = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts.size());
    auto txs = ready_accounts[0]->get_txs();
    ASSERT_EQ(3, txs.size());
    ASSERT_EQ(tx1b->get_digest_hex_str(), txs[0]->get_digest_hex_str());
    ASSERT_EQ(tx2b->get_digest_hex_str(), txs[1]->get_digest_hex_str());
    ASSERT_EQ(tx3b->get_digest_hex_str(), txs[2]->get_digest_hex_str());
}

TEST_F(test_txmgr_table, send_tx_clear_follower) {
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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }

    auto accounts1 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(accounts1.size(), 1);
    auto txs_ents1 = accounts1[0]->get_txs();
    ASSERT_EQ(txs_ents1.size(), 3);

    txmgr_table.pop_tx(txs_ents1[0]->get_transaction()->get_source_addr(), txs_ents1[0]->get_transaction()->digest(), txs_ents1[0]->get_tx_subtype(), false);

    auto accounts2 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(accounts2.size(), 1);
    auto txs_ents2 = accounts2[0]->get_txs();
    ASSERT_EQ(txs_ents2.size(), 2);

    txmgr_table.pop_tx(txs_ents2[0]->get_transaction()->get_source_addr(), txs_ents2[0]->get_transaction()->digest(), txs_ents2[0]->get_tx_subtype(), true);

    auto accounts3 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(accounts3.size(), 0);
}

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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[4], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[6], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[5], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[2], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }

    for (uint32_t i = 2; i < 5; i++) {
        auto tx_tmp = txmgr_table.query_tx(txs[i]->get_transaction()->get_source_addr(), txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_tmp, nullptr);
    }

    ready_accounts_t ready_accounts = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(0, ready_accounts.size());

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[0], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }

    ready_accounts_t ready_accounts2 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts2.size());
    auto txs2 = ready_accounts2[0]->get_txs();
    ASSERT_EQ(1, txs2.size());
    ASSERT_EQ(txs[0]->get_digest_hex_str(), txs2[0]->get_digest_hex_str());

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[1], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0, last_tx_hash);
        ASSERT_EQ(0, ret);
    }

    {
        ready_accounts_t ready_accounts3 = txmgr_table.get_ready_accounts(10);
        ASSERT_EQ(1, ready_accounts3.size());
        auto txs3 = ready_accounts3[0]->get_txs();
        ASSERT_EQ(1, txs3.size());
    }

    txmgr_table.updata_latest_nonce(txs[0]->get_account_addr(), txs[0]->get_transaction()->get_tx_nonce(), txs[0]->get_transaction()->digest());

    {
        ready_accounts_t ready_accounts3 = txmgr_table.get_ready_accounts(10);
        ASSERT_EQ(1, ready_accounts3.size());
        auto txs3 = ready_accounts3[0]->get_txs();
        ASSERT_EQ(3, txs3.size());
        ASSERT_EQ(txs[1]->get_digest_hex_str(), txs3[0]->get_digest_hex_str());
        ASSERT_EQ(txs[2]->get_digest_hex_str(), txs3[1]->get_digest_hex_str());
        ASSERT_EQ(txs[3]->get_digest_hex_str(), txs3[2]->get_digest_hex_str());
    }

    txmgr_table.updata_latest_nonce(txs[3]->get_account_addr(), txs[3]->get_transaction()->get_tx_nonce(), txs[3]->get_transaction()->digest());

    ready_accounts_t ready_accounts4 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts4.size());
    auto txs3 = ready_accounts4[0]->get_txs();
    ASSERT_EQ(3, txs3.size());
    ASSERT_EQ(txs[4]->get_digest_hex_str(), txs3[0]->get_digest_hex_str());
    ASSERT_EQ(txs[5]->get_digest_hex_str(), txs3[1]->get_digest_hex_str());
    ASSERT_EQ(txs[6]->get_digest_hex_str(), txs3[2]->get_digest_hex_str());

    txmgr_table.updata_latest_nonce(txs[5]->get_account_addr(), txs[5]->get_transaction()->get_tx_nonce(), txs[5]->get_transaction()->digest());

    ready_accounts_t ready_accounts5 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts5.size());
    auto txs5 = ready_accounts5[0]->get_txs();
    ASSERT_EQ(1, txs5.size());
    ASSERT_EQ(txs[6]->get_digest_hex_str(), txs5[0]->get_digest_hex_str());
}

TEST_F(test_txmgr_table, expired_tx) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xtx_para_t para;

    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now - 300, last_tx_hash, 0);
    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(tx1, para);

    xcons_transaction_ptr_t tx2 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now - 300, tx1->get_transaction()->digest(), 0);
    std::shared_ptr<xtx_entry> tx_ent2 = std::make_shared<xtx_entry>(tx2, para);

    int32_t ret = txmgr_table.push_send_tx(tx_ent2, 0, last_tx_hash);
    ASSERT_EQ(0, ret);
    sleep(1);

    ret = txmgr_table.push_send_tx(tx_ent1, 0, last_tx_hash);
    ASSERT_EQ(0, ret);

    auto q_tx = txmgr_table.query_tx(tx2->get_account_addr(), tx2->get_transaction()->digest());
    ASSERT_EQ(q_tx, nullptr);

    sleep(1);
    auto accounts = txmgr_table.get_ready_accounts(100);
    ASSERT_EQ(accounts.size(), 0);
}
