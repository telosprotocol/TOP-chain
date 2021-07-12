#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xverifier/xverifier_utl.h"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xlightunit.h"

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

TEST_F(test_txmgr_table, sigle_send_tx) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now, last_tx_hash);
    tx->set_push_pool_timestamp(now);

    xtx_para_t para;

    // push first time
    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    auto tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);

    // duplicate push
    ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(xtxpool_error_request_tx_repeat, ret);

    // pop out
    tx_info_t txinfo(tx);
    auto tx_ent_tmp = txmgr_table.pop_tx(txinfo, false);
    ASSERT_NE(tx_ent_tmp, nullptr);
    tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_EQ(tx_tmp, nullptr);

    // push again
    ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    tx_tmp = txmgr_table.query_tx(tx->get_transaction()->get_source_addr(), tx->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);
}

TEST_F(test_txmgr_table, sigle_account_multi_send_tx) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }
}

TEST_F(test_txmgr_table, duplicate_send_tx_to_pending) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
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

    txmgr_table.updata_latest_nonce(tx1->get_account_addr(), tx1->get_transaction()->get_tx_nonce());

    ready_accounts_t ready_accounts3 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(1, ready_accounts3.size());
    auto txs3 = ready_accounts3[0]->get_txs();
    ASSERT_EQ(1, txs3.size());
    ASSERT_EQ(tx2a->get_digest_hex_str(), txs3[0]->get_digest_hex_str());

    tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_account_addr(), tx_ent2b->get_tx()->get_transaction()->digest());
    ASSERT_NE(tx_q, nullptr);

    txmgr_table.updata_latest_nonce(tx2a->get_account_addr(), tx2a->get_transaction()->get_tx_nonce());
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
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
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
    ret = txmgr_table.push_send_tx(tx_ent1, 1);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx2a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 3, last_tx_hash);
    last_tx_hash = tx2a->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent2a = std::make_shared<xtx_entry>(tx2a, para);
    ret = txmgr_table.push_send_tx(tx_ent2a, 1);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx3a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 3, now + 4, last_tx_hash);
    last_tx_hash = tx3a->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent3a = std::make_shared<xtx_entry>(tx3a, para);

    xcons_transaction_ptr_t tx1b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now + 5, last_tx_hash_0);
    last_tx_hash = tx1b->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent1b = std::make_shared<xtx_entry>(tx1b, para);
    ret = txmgr_table.push_send_tx(tx_ent1b, 1);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx2b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 6, last_tx_hash);
    last_tx_hash = tx2b->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent2b = std::make_shared<xtx_entry>(tx2b, para);

    ret = txmgr_table.push_send_tx(tx_ent3a, 1);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx3b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 3, now + 7, last_tx_hash);
    last_tx_hash = tx3b->get_transaction()->digest();
    std::shared_ptr<xtx_entry> tx_ent3b = std::make_shared<xtx_entry>(tx3b, para);
    ret = txmgr_table.push_send_tx(tx_ent3b, 1);
    ASSERT_EQ(0, ret);

    ret = txmgr_table.push_send_tx(tx_ent2b, 1);
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
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    auto accounts1 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(accounts1.size(), 1);
    auto txs_ents1 = accounts1[0]->get_txs();
    ASSERT_EQ(txs_ents1.size(), 3);

    tx_info_t txinfo1(txs_ents1[0]);
    txmgr_table.pop_tx(txinfo1, false);

    auto accounts2 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(accounts2.size(), 1);
    auto txs_ents2 = accounts2[0]->get_txs();
    ASSERT_EQ(txs_ents2.size(), 2);

    tx_info_t txinfo2(txs_ents2[0]);
    txmgr_table.pop_tx(txinfo2, true);

    auto accounts3 = txmgr_table.get_ready_accounts(10);
    ASSERT_EQ(accounts3.size(), 0);
}

TEST_F(test_txmgr_table, sigle_account_uncontinuous_send_txs) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[3], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[4], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[6], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[5], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }
    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[2], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
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
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
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
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xtx_para_t para;

    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now - 300, last_tx_hash, 0);
    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(tx1, para);

    xcons_transaction_ptr_t tx2 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now - 300, tx1->get_transaction()->digest(), 0);
    std::shared_ptr<xtx_entry> tx_ent2 = std::make_shared<xtx_entry>(tx2, para);

    int32_t ret = txmgr_table.push_send_tx(tx_ent2, 0);
    ASSERT_EQ(0, ret);
    sleep(1);

    ret = txmgr_table.push_send_tx(tx_ent1, 0);
    ASSERT_EQ(0, ret);

    auto q_tx = txmgr_table.query_tx(tx2->get_account_addr(), tx2->get_transaction()->digest());
    ASSERT_EQ(q_tx, nullptr);

    sleep(1);
    auto accounts = txmgr_table.get_ready_accounts(100);
    ASSERT_EQ(accounts.size(), 0);
}

xtxpool_v2::ready_accounts_t table_rules_filter(const std::set<std::string> & locked_account_set,
                                                const base::xreceiptid_state_ptr_t & receiptid_state_highqc,
                                                const std::vector<xcons_transaction_ptr_t> & ready_txs) {
    std::map<std::string, std::shared_ptr<xtxpool_v2::xready_account_t>> ready_account_map;

    enum_transaction_subtype last_tx_subtype = enum_transaction_subtype::enum_transaction_subtype_all;
    base::xtable_shortid_t last_peer_table_shortid;
    uint64_t last_receipt_id;

    for (auto & tx : ready_txs) {
        auto & account_addr = tx->get_account_addr();

        // remove locked accounts' txs
        auto it_locked_account_set = locked_account_set.find(account_addr);
        if (it_locked_account_set != locked_account_set.end()) {
            continue;
        }

        enum_transaction_subtype cur_tx_subtype;
        base::xtable_shortid_t cur_peer_table_sid;
        uint64_t cur_receipt_id;

        // make sure receipt id is continuous
        if (tx->is_recv_tx() || tx->is_confirm_tx()) {
            cur_tx_subtype = tx->get_tx_subtype();
            auto & account_addr = (tx->is_recv_tx()) ? tx->get_source_addr() : tx->get_target_addr();
            base::xvaccount_t vaccount(account_addr);
            cur_peer_table_sid = vaccount.get_short_table_id();
            cur_receipt_id = tx->get_last_action_receipt_id();
            if (cur_tx_subtype != last_tx_subtype || last_peer_table_shortid != cur_peer_table_sid) {
                base::xreceiptid_pair_t receiptid_pair;
                receiptid_state_highqc->find_pair(cur_peer_table_sid, receiptid_pair);
                last_receipt_id = tx->is_recv_tx() ? receiptid_pair.get_recvid_max() : receiptid_pair.get_confirmid_max();
            }
            if (cur_receipt_id != last_receipt_id + 1) {
                continue;
            }
        }

        // push to account level tx set, there is a filter rule for account tx set, push can be fail.
        bool ret = true;
        auto it_ready_account = ready_account_map.find(account_addr);
        if (it_ready_account == ready_account_map.end()) {
            auto ready_account = std::make_shared<xtxpool_v2::xready_account_t>(account_addr);
            ret = ready_account->put_tx(tx);
            ready_account_map[account_addr] = ready_account;
        } else {
            auto & ready_account = it_ready_account->second;
            ret = ready_account->put_tx(tx);
        }
        if (ret && (tx->is_recv_tx() || tx->is_confirm_tx())) {
            last_tx_subtype = cur_tx_subtype;
            last_peer_table_shortid = cur_peer_table_sid;
            last_receipt_id = cur_receipt_id;
        }
    }

    xtxpool_v2::ready_accounts_t ready_accounts;
    for (auto & ready_account_pair : ready_account_map) {
        ready_accounts.push_back(ready_account_pair.second);
    }
    return ready_accounts;
}

TEST_F(test_txmgr_table, table_rules_filter_basic) {
    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    std::string account0 = test_xtxpool_util_t::get_account(0);
    std::string account1 = test_xtxpool_util_t::get_account(1);
    std::string account2 = test_xtxpool_util_t::get_account(2);

    std::cout << "account0:" << account0 << std::endl;
    std::cout << "account1:" << account1 << std::endl;
    std::cout << "account2:" << account2 << std::endl;

    uint32_t txs_num0 = 10;
    uint32_t txs_num1 = 6;
    uint32_t txs_num2 = 8;
    std::vector<xcons_transaction_ptr_t> txs0 = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num0);
    std::vector<xcons_transaction_ptr_t> txs1 = test_xtxpool_util_t::create_cons_transfer_txs(1, 2, txs_num1);
    std::vector<xcons_transaction_ptr_t> txs2 = test_xtxpool_util_t::create_cons_transfer_txs(2, 0, txs_num2);

    txs0[0]->set_current_receipt_id(0, 1);
    txs0[1]->set_current_receipt_id(0, 2);
    txs1[0]->set_current_receipt_id(0, 3);
    txs1[1]->set_current_receipt_id(0, 4);
    txs0[2]->set_current_receipt_id(0, 5);
    txs1[2]->set_current_receipt_id(0, 6);
    txs1[3]->set_current_receipt_id(0, 7);
    txs1[4]->set_current_receipt_id(0, 8);
    txs0[3]->set_current_receipt_id(0, 9);
    txs0[4]->set_current_receipt_id(0, 10);
    txs0[5]->set_current_receipt_id(0, 11);
    txs1[5]->set_current_receipt_id(0, 12);

    xblock_t * block0;
    std::vector<xcons_transaction_ptr_t> recvtxs1 = get_tx(blockstore, xstore, account0, account1, txs0, &block0);
    
    xblock_t * block1;
    std::vector<xcons_transaction_ptr_t> recvtxs2 = get_tx(blockstore, xstore, account1, account2, txs1, &block1);

    std::vector<xcons_transaction_ptr_t> ready_txs;
    ready_txs.push_back(recvtxs1[0]);
    ready_txs.push_back(recvtxs1[1]);
    ready_txs.push_back(recvtxs2[0]);
    ready_txs.push_back(recvtxs2[1]);
    ready_txs.push_back(recvtxs1[2]);
    ready_txs.push_back(recvtxs2[2]);
    ready_txs.push_back(recvtxs2[3]);
    ready_txs.push_back(recvtxs2[4]);
    ready_txs.push_back(recvtxs1[3]);
    ready_txs.push_back(recvtxs1[4]);
    ready_txs.push_back(recvtxs1[5]);
    ready_txs.push_back(recvtxs2[5]);
    ready_txs.insert(ready_txs.end(), txs2.begin(), txs2.end());
    ready_txs.insert(ready_txs.end(), txs0.begin() + 6, txs0.end());

    {
        std::set<std::string> locked_account_set;
        locked_account_set.insert(account2);

        base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();

        auto ready_accounts = table_rules_filter(locked_account_set, receiptid_state, ready_txs);
        ASSERT_EQ(ready_accounts.size(), 2);
        ASSERT_EQ(ready_accounts[0]->get_addr(), account0);
        ASSERT_EQ(ready_accounts[0]->get_txs().size(), 4);
        ASSERT_EQ(ready_accounts[1]->get_addr(), account1);
        ASSERT_EQ(ready_accounts[1]->get_txs().size(), 2);
    }

    {
        std::set<std::string> locked_account_set;
        locked_account_set.insert(account0);

        base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();

        auto ready_accounts = table_rules_filter(locked_account_set, receiptid_state, ready_txs);
        ASSERT_EQ(ready_accounts.size(), 2);
        ASSERT_EQ(ready_accounts[0]->get_addr(), account2);
        ASSERT_EQ(ready_accounts[0]->get_txs().size(), 6);
        ASSERT_EQ(ready_accounts[1]->get_addr(), account1);
        ASSERT_EQ(ready_accounts[1]->get_txs().size(), 6);
    }

    {
        std::set<std::string> locked_account_set;
        locked_account_set.insert(account1);

        base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();

        auto ready_accounts = table_rules_filter(locked_account_set, receiptid_state, ready_txs);
        ASSERT_EQ(ready_accounts.size(), 2);
        ASSERT_EQ(ready_accounts[0]->get_addr(), account2);
        ASSERT_EQ(ready_accounts[0]->get_txs().size(), 8);
        ASSERT_EQ(ready_accounts[1]->get_addr(), account0);
        ASSERT_EQ(ready_accounts[1]->get_txs().size(), 4);
    }

    {
        std::set<std::string> locked_account_set;

        base::xreceiptid_state_ptr_t receiptid_state = std::make_shared<base::xreceiptid_state_t>();

        auto ready_accounts = table_rules_filter(locked_account_set, receiptid_state, ready_txs);
        ASSERT_EQ(ready_accounts.size(), 3);
        ASSERT_EQ(ready_accounts[0]->get_addr(), account2);
        ASSERT_EQ(ready_accounts[0]->get_txs().size(), 6);
        ASSERT_EQ(ready_accounts[1]->get_addr(), account0);
        ASSERT_EQ(ready_accounts[1]->get_txs().size(), 4);
        ASSERT_EQ(ready_accounts[2]->get_addr(), account1);
        ASSERT_EQ(ready_accounts[2]->get_txs().size(), 6);
    }
}

TEST_F(test_txmgr_table, repeat_receipt) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_statistic_t statistic;
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic);
    xtxmgr_table_t txmgr_table(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xtx_para_t para;

    mock::xvchain_creator creator;
    creator.create_blockstore_with_xstore();
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    store::xstore_face_t* xstore = creator.get_xstore();

    // construct account
    std::string sender = test_xtxpool_util_t::get_account(0);
    std::string receiver = test_xtxpool_util_t::get_account(1);

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

    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(recvtxs[0], para);
    int32_t ret = txmgr_table.push_receipt(tx_ent);
    ASSERT_EQ(ret, 0);
    bool is_repeat = txmgr_table.is_repeat_tx(tx_ent);
    ASSERT_EQ(is_repeat, true);
}
