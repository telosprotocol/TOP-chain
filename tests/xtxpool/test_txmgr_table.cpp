#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xverifier/xverifier_utl.h"
#include "xtxpool_v2/xtxpool_para.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_txmgr_table : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_txmgr_table, sigle_send_tx) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, 2);
    txs[0]->set_push_pool_timestamp(now);

    xtx_para_t para;

    // push first time
    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[0], para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    auto tx_tmp = txmgr_table.query_tx(txs[0]->get_transaction()->get_source_addr(), txs[0]->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);

    // duplicate push
    ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(xtxpool_error_request_tx_repeat, ret);

    // pop out
    tx_info_t txinfo(txs[0]);
    auto tx_ent_tmp = txmgr_table.pop_tx(txinfo, false);
    ASSERT_NE(tx_ent_tmp, nullptr);
    tx_tmp = txmgr_table.query_tx(txs[0]->get_transaction()->get_source_addr(), txs[0]->get_transaction()->digest());
    ASSERT_EQ(tx_tmp, nullptr);

    // push again
    ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    tx_tmp = txmgr_table.query_tx(txs[0]->get_transaction()->get_source_addr(), txs[0]->get_transaction()->digest());
    ASSERT_NE(tx_tmp, nullptr);

    table_para.send_tx_inc(1025);
    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(txs[1], para);
    ret = txmgr_table.push_send_tx(tx_ent1, 0);
    ASSERT_NE(0, ret);
}

TEST_F(test_txmgr_table, sigle_account_multi_send_tx) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
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
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtx_para_t para;

    xcons_transaction_ptr_t tx0 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now + 1, last_tx_hash);
    last_tx_hash = tx0->get_transaction()->digest();

    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(tx0, para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent0, 0);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now + 2, last_tx_hash);
    last_tx_hash = tx1->get_transaction()->digest();
    uint256_t last_tx_hash_1 = last_tx_hash;

    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(tx1, para);
    ret = txmgr_table.push_send_tx(tx_ent1, 0);
    ASSERT_EQ(0, ret);

    xcons_transaction_ptr_t tx2a = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 3, last_tx_hash);
    last_tx_hash = tx2a->get_transaction()->digest();

    std::shared_ptr<xtx_entry> tx_ent2a = std::make_shared<xtx_entry>(tx2a, para);
    ret = txmgr_table.push_send_tx(tx_ent2a, 0);
    ASSERT_EQ(0, ret);

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, 40, 35, 30);

    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(3, ready_txs.size());
    ASSERT_EQ(tx0->get_digest_hex_str(), ready_txs[0]->get_digest_hex_str());
    ASSERT_EQ(tx1->get_digest_hex_str(), ready_txs[1]->get_digest_hex_str());
    ASSERT_EQ(tx2a->get_digest_hex_str(), ready_txs[2]->get_digest_hex_str());

    xcons_transaction_ptr_t tx2b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 4, last_tx_hash_1);
    last_tx_hash = tx2b->get_transaction()->digest();

    std::shared_ptr<xtx_entry> tx_ent2b = std::make_shared<xtx_entry>(tx2b, para);
    ret = txmgr_table.push_send_tx(tx_ent2b, 0);
    ASSERT_EQ(0, ret);

    auto tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_account_addr(), tx_ent2b->get_tx()->get_transaction()->digest());
    ASSERT_EQ(tx_q->get_tx()->get_digest_hex_str(), tx2b->get_digest_hex_str());

    auto ready_txs2 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(3, ready_txs2.size());
    ASSERT_EQ(tx0->get_digest_hex_str(), ready_txs2[0]->get_digest_hex_str());
    ASSERT_EQ(tx1->get_digest_hex_str(), ready_txs2[1]->get_digest_hex_str());
    ASSERT_EQ(tx2a->get_digest_hex_str(), ready_txs2[2]->get_digest_hex_str());

    txmgr_table.updata_latest_nonce(tx1->get_account_addr(), tx1->get_transaction()->get_tx_nonce());

    auto ready_txs3 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(1, ready_txs3.size());
    ASSERT_EQ(tx2a->get_digest_hex_str(), ready_txs3[0]->get_digest_hex_str());

    tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_account_addr(), tx_ent2b->get_tx()->get_transaction()->digest());
    ASSERT_NE(tx_q, nullptr);

    txmgr_table.updata_latest_nonce(tx2a->get_account_addr(), tx2a->get_transaction()->get_tx_nonce());
    auto ready_txs4 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(0, ready_txs4.size());

    tx_q = txmgr_table.query_tx(tx2b->get_account_addr(), tx2b->get_transaction()->digest());
    ASSERT_EQ(tx_q, nullptr);

    tx_q = txmgr_table.query_tx(tx2a->get_account_addr(), tx2a->get_transaction()->digest());
    ASSERT_EQ(tx_q, nullptr);
}

TEST_F(test_txmgr_table, duplicate_send_tx_to_pending_2) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtx_para_t para;

    xcons_transaction_ptr_t tx0 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now + 1, last_tx_hash);
    last_tx_hash = tx0->get_transaction()->digest();
    uint256_t last_tx_hash_0 = last_tx_hash;
    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(tx0, para);
    int32_t ret = txmgr_table.push_send_tx(tx_ent0, 0);
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

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, 40, 35, 30);
    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(3, ready_txs.size());
    ASSERT_EQ(tx1b->get_digest_hex_str(), ready_txs[0]->get_digest_hex_str());
    ASSERT_EQ(tx2b->get_digest_hex_str(), ready_txs[1]->get_digest_hex_str());
    ASSERT_EQ(tx3b->get_digest_hex_str(), ready_txs[2]->get_digest_hex_str());
}

TEST_F(test_txmgr_table, send_tx_clear_follower) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
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

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, 40, 35, 30);

    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(3, ready_txs.size());

    tx_info_t txinfo1(ready_txs[0]);
    txmgr_table.pop_tx(txinfo1, false);

    auto ready_txs2 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(2, ready_txs2.size());

    tx_info_t txinfo2(ready_txs2[0]);
    txmgr_table.pop_tx(txinfo2, true);

    auto ready_txs3 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(0, ready_txs3.size());
}
TEST_F(test_txmgr_table, sigle_account_uncontinuous_send_txs) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
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

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, 40, 35, 30);

    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(0, ready_txs.size());

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[0], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    auto ready_txs2 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(1, ready_txs2.size());
    ASSERT_EQ(txs[0]->get_digest_hex_str(), ready_txs2[0]->get_digest_hex_str());

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[1], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    {
        auto ready_txs3 = txmgr_table.get_ready_txs(txpool_pack_para);
        ASSERT_EQ(1, ready_txs3.size());
    }

    txmgr_table.updata_latest_nonce(txs[0]->get_account_addr(), txs[0]->get_transaction()->get_tx_nonce());

    {
        auto ready_txs4 = txmgr_table.get_ready_txs(txpool_pack_para);
        ASSERT_EQ(3, ready_txs4.size());
        ASSERT_EQ(txs[1]->get_digest_hex_str(), ready_txs4[0]->get_digest_hex_str());
        ASSERT_EQ(txs[2]->get_digest_hex_str(), ready_txs4[1]->get_digest_hex_str());
        ASSERT_EQ(txs[3]->get_digest_hex_str(), ready_txs4[2]->get_digest_hex_str());
    }

    txmgr_table.updata_latest_nonce(txs[3]->get_account_addr(), txs[3]->get_transaction()->get_tx_nonce());

    auto ready_txs5 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(3, ready_txs5.size());
    ASSERT_EQ(txs[4]->get_digest_hex_str(), ready_txs5[0]->get_digest_hex_str());
    ASSERT_EQ(txs[5]->get_digest_hex_str(), ready_txs5[1]->get_digest_hex_str());
    ASSERT_EQ(txs[6]->get_digest_hex_str(), ready_txs5[2]->get_digest_hex_str());

    txmgr_table.updata_latest_nonce(txs[5]->get_account_addr(), txs[5]->get_transaction()->get_tx_nonce());

    auto ready_txs6 = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(1, ready_txs6.size());
    ASSERT_EQ(txs[6]->get_digest_hex_str(), ready_txs6[0]->get_digest_hex_str());
}

TEST_F(test_txmgr_table, expired_tx) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
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
    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, 40, 35, 30);

    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para);
    ASSERT_EQ(0, ready_txs.size());
}

TEST_F(test_txmgr_table, repeat_receipt) {
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
    xtxpool_resources resource(nullptr, nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    xtx_para_t para;

    xreceipt_queue_new_t receipt_queue(&table_para, &resource);

    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());

    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(recv_txs[0], para);
    int32_t ret = txmgr_table.push_receipt(tx_ent);
    ASSERT_EQ(ret, 0);
    bool is_repeat = txmgr_table.is_repeat_tx(tx_ent);
    ASSERT_EQ(is_repeat, true);
}
