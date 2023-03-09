#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xdata/xlightunit.h"
#include "xtxpool_v2/xtxmgr_table.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xdata/xverifier/xverifier_utl.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xstatistic/xstatistic.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;
using namespace top::xstatistic;

class test_txmgr_table : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_txmgr_table, sigle_send_tx) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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
    auto tx_tmp = txmgr_table.query_tx(txs[0]->get_tx_hash());
    ASSERT_NE(tx_tmp.get(), nullptr);

    // duplicate push
    ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(xtxpool_error_request_tx_repeat, ret);

    // pop out
    auto tx_ent_tmp = txmgr_table.pop_tx(txs[0]->get_tx_hash(), txs[0]->get_tx_subtype(), false);
    ASSERT_NE(tx_ent_tmp.get(), nullptr);
    tx_tmp = txmgr_table.query_tx(txs[0]->get_tx_hash());
    ASSERT_EQ(tx_tmp.get(), nullptr);

    // push again
    ret = txmgr_table.push_send_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    tx_tmp = txmgr_table.query_tx(txs[0]->get_tx_hash());
    ASSERT_NE(tx_tmp.get(), nullptr);

    table_para.send_tx_inc(1025);
    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(txs[1], para);
    ret = txmgr_table.push_send_tx(tx_ent1, 0);
    ASSERT_NE(0, ret);
}

TEST_F(test_txmgr_table, sigle_account_multi_send_tx) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, table_genesis_block.get(), 40, 35, 30, peer_sids_for_confirm_id);

    xunconfirm_id_height id_height_map(1);
    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(3, ready_txs.size());
    ASSERT_EQ(tx0->get_digest_hex_str(), ready_txs[0]->get_digest_hex_str());
    ASSERT_EQ(tx1->get_digest_hex_str(), ready_txs[1]->get_digest_hex_str());
    ASSERT_EQ(tx2a->get_digest_hex_str(), ready_txs[2]->get_digest_hex_str());

    xcons_transaction_ptr_t tx2b = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 2, now + 4, last_tx_hash_1);
    last_tx_hash = tx2b->get_transaction()->digest();

    std::shared_ptr<xtx_entry> tx_ent2b = std::make_shared<xtx_entry>(tx2b, para);
    ret = txmgr_table.push_send_tx(tx_ent2b, 0);
    ASSERT_EQ(0, ret);

    auto tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_tx_hash());
    ASSERT_EQ(tx_q->get_digest_hex_str(), tx2b->get_digest_hex_str());

    auto ready_txs2 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(3, ready_txs2.size());
    ASSERT_EQ(tx0->get_digest_hex_str(), ready_txs2[0]->get_digest_hex_str());
    ASSERT_EQ(tx1->get_digest_hex_str(), ready_txs2[1]->get_digest_hex_str());
    ASSERT_EQ(tx2b->get_digest_hex_str(), ready_txs2[2]->get_digest_hex_str());

    txmgr_table.updata_latest_nonce(tx1->get_account_addr(), tx1->get_transaction()->get_tx_nonce());

    auto ready_txs3 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(1, ready_txs3.size());
    ASSERT_EQ(tx2b->get_digest_hex_str(), ready_txs3[0]->get_digest_hex_str());

    tx_q = txmgr_table.query_tx(tx_ent2b->get_tx()->get_tx_hash());
    ASSERT_NE(tx_q.get(), nullptr);

    txmgr_table.updata_latest_nonce(tx2b->get_account_addr(), tx2b->get_transaction()->get_tx_nonce());
    auto ready_txs4 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(0, ready_txs4.size());

    tx_q = txmgr_table.query_tx(tx2b->get_tx_hash());
    ASSERT_EQ(tx_q.get(), nullptr);

    tx_q = txmgr_table.query_tx(tx2a->get_tx_hash());
    ASSERT_EQ(tx_q.get(), nullptr);
}

TEST_F(test_txmgr_table, duplicate_send_tx_to_pending_2) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, table_genesis_block.get(), 40, 35, 30, peer_sids_for_confirm_id);
    xunconfirm_id_height id_height_map(1);
    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(3, ready_txs.size());
    ASSERT_EQ(tx1b->get_digest_hex_str(), ready_txs[0]->get_digest_hex_str());
    ASSERT_EQ(tx2b->get_digest_hex_str(), ready_txs[1]->get_digest_hex_str());
    ASSERT_EQ(tx3b->get_digest_hex_str(), ready_txs[2]->get_digest_hex_str());
}

TEST_F(test_txmgr_table, send_tx_clear_follower) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, table_genesis_block.get(), 40, 35, 30, peer_sids_for_confirm_id);
    xunconfirm_id_height id_height_map(1);
    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(10, ready_txs.size());

    txmgr_table.pop_tx(ready_txs[0]->get_tx_hash(), ready_txs[0]->get_tx_subtype(), false);

    auto ready_txs2 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(0, ready_txs2.size());
}
TEST_F(test_txmgr_table, sigle_account_uncontinuous_send_txs) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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
        auto tx_tmp = txmgr_table.query_tx(txs[i]->get_tx_hash());
        ASSERT_NE(tx_tmp.get(), nullptr);
    }

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, table_genesis_block.get(), 40, 35, 30, peer_sids_for_confirm_id);
    xunconfirm_id_height id_height_map(1);
    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(0, ready_txs.size());

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[0], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    auto ready_txs2 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(1, ready_txs2.size());
    ASSERT_EQ(txs[0]->get_digest_hex_str(), ready_txs2[0]->get_digest_hex_str());

    {
        xtx_para_t para;
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[1], para);
        int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    {
        auto ready_txs3 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
        ASSERT_EQ(7, ready_txs3.size());
    }

    txmgr_table.updata_latest_nonce(txs[0]->get_account_addr(), txs[0]->get_transaction()->get_tx_nonce());

    {
        auto ready_txs4 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
        ASSERT_EQ(6, ready_txs4.size());
        ASSERT_EQ(txs[1]->get_digest_hex_str(), ready_txs4[0]->get_digest_hex_str());
        ASSERT_EQ(txs[2]->get_digest_hex_str(), ready_txs4[1]->get_digest_hex_str());
        ASSERT_EQ(txs[3]->get_digest_hex_str(), ready_txs4[2]->get_digest_hex_str());
    }

    txmgr_table.updata_latest_nonce(txs[3]->get_account_addr(), txs[3]->get_transaction()->get_tx_nonce());

    auto ready_txs5 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(3, ready_txs5.size());
    ASSERT_EQ(txs[4]->get_digest_hex_str(), ready_txs5[0]->get_digest_hex_str());
    ASSERT_EQ(txs[5]->get_digest_hex_str(), ready_txs5[1]->get_digest_hex_str());
    ASSERT_EQ(txs[6]->get_digest_hex_str(), ready_txs5[2]->get_digest_hex_str());

    txmgr_table.updata_latest_nonce(txs[5]->get_account_addr(), txs[5]->get_transaction()->get_tx_nonce());

    auto ready_txs6 = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(1, ready_txs6.size());
    ASSERT_EQ(txs[6]->get_digest_hex_str(), ready_txs6[0]->get_digest_hex_str());
}

TEST_F(test_txmgr_table, expired_tx) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
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

    auto q_tx = txmgr_table.query_tx(tx2->get_tx_hash());
    ASSERT_EQ(q_tx.get(), nullptr);

    sleep(1);

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    top::xobject_ptr_t<xvbstate_t> vbstate;
    vbstate.attach(new xvbstate_t{table_addr, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(vbstate.get());
    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(table_addr, tablestate, table_genesis_block.get(), 40, 35, 30, peer_sids_for_confirm_id);
    xunconfirm_id_height id_height_map(1);
    auto ready_txs = txmgr_table.get_ready_txs(txpool_pack_para, id_height_map);
    ASSERT_EQ(1, ready_txs.size());
}

TEST_F(test_txmgr_table, repeat_receipt) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    xtx_para_t para;

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

#if defined(CACHE_SIZE_STATISTIC) || defined(CACHE_SIZE_STATISTIC_MORE_DETAIL)
TEST_F(test_txmgr_table, sendtx_mem_loss) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    xtx_para_t para;

    uint32_t tx_num = 1;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);

    // deliberately memory loss
    xcons_transaction_t * tx = send_txs[0].get();
    tx->add_ref();

    usleep(2000);
    xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
    ASSERT_EQ(obj_num, 1);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
    std::cout << "statistic_tx_v2_size : " << obj_size << std::endl;
#endif
}

TEST_F(test_txmgr_table, receipt_mem_loss) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xtxmgr_table_t txmgr_table(&table_para, &resource);
    xtx_para_t para;

    uint32_t tx_num = 1;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    mocktable.generate_one_table();
    mocktable.generate_one_table();

    xdbg("receipt_mem_loss send_tx:%p", send_txs[0].get());

    std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(_tableblock1);
    xassert(recv_txs.size() == send_txs.size());

    // deliberately memory loss
    xcons_transaction_t * tx = recv_txs[0].get();
    tx->add_ref();

    xdbg("receipt_mem_loss recv_tx:%p", recv_txs[0].get());

    usleep(1000);
    xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
    auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_receipt_num);
    ASSERT_EQ(obj_num, 1);
    auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_receipt_size);
    std::cout << "statistic_receipt_size : " << obj_size << std::endl;
#endif
}

TEST_F(test_txmgr_table, xvaction_mem_loss) {
    xvaction_t * action = new xvaction_t("1", "12", "123", "1234");
}

TEST_F(test_txmgr_table, large_number_of_send_tx) {
    uint32_t tx_num = 16;
    uint32_t loop_num = 1024/tx_num;
    xtxpool_statistic_t statistic;
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xtx_para_t para;

    mock::xdatamock_table mocktable(1, loop_num + 1);
    std::string table_addr = mocktable.get_account();
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xtxmgr_table_t txmgr_table(&table_para, &resource);

    mock::xdatamock_table mocktable2(2, loop_num + 1);
    std::string table_addr2 = mocktable2.get_account();
    xtable_state_cache_t table_state_cache2(nullptr, table_addr2);
    xtxpool_table_info_t table_para2(table_addr2, &shard, &statistic, &table_state_cache2);
    xtxmgr_table_t txmgr_table2(&table_para2, &resource);
    
    mock::xdatamock_table mocktable3(3, loop_num + 1);
    std::string table_addr3 = mocktable3.get_account();
    xtable_state_cache_t table_state_cache3(nullptr, table_addr3);
    xtxpool_table_info_t table_para3(table_addr3, &shard, &statistic, &table_state_cache3);
    xtxmgr_table_t txmgr_table3(&table_para3, &resource);

    mock::xdatamock_table mocktable4(4, loop_num + 1);
    std::string table_addr4 = mocktable4.get_account();
    xtable_state_cache_t table_state_cache4(nullptr, table_addr4);
    xtxpool_table_info_t table_para4(table_addr4, &shard, &statistic, &table_state_cache4);
    xtxmgr_table_t txmgr_table4(&table_para4, &resource);

    mock::xdatamock_table mocktable5(5, loop_num + 1);
    std::string table_addr5 = mocktable5.get_account();
    xtable_state_cache_t table_state_cache5(nullptr, table_addr5);
    xtxpool_table_info_t table_para5(table_addr5, &shard, &statistic, &table_state_cache5);
    xtxmgr_table_t txmgr_table5(&table_para5, &resource);

    {
        std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
        for (uint32_t i = 0; i < loop_num; i++) {
            std::string sender = unit_addrs[i];
            std::string receiver = unit_addrs[loop_num];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
            // push txs by inverted order, high nonce with high charge score
            for (auto & tx : send_txs) {
                xtx_para_t para;
                std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
                int32_t ret = txmgr_table.push_send_tx(tx_ent, 0);
                ASSERT_EQ(0, ret);
            }
        }

        usleep(1000);
        xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
        auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
        ASSERT_EQ(obj_num, 1024);
        auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
        std::cout << "statistic_tx_v2_size : " << obj_size << std::endl;
#endif
    }
    {
        std::vector<std::string> unit_addrs = mocktable2.get_unit_accounts();
        for (uint32_t i = 0; i < loop_num; i++) {
            std::string sender = unit_addrs[i];
            std::string receiver = unit_addrs[loop_num];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable2.create_send_txs(sender, receiver, tx_num);
            // push txs by inverted order, high nonce with high charge score
            for (auto & tx : send_txs) {
                xtx_para_t para;
                std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
                int32_t ret = txmgr_table2.push_send_tx(tx_ent, 0);
                ASSERT_EQ(0, ret);
            }
        }
        usleep(1000);
        xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
        auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
        ASSERT_EQ(obj_num, 1024*2);
        auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
        std::cout << "statistic_tx_v2_size : " << obj_size << std::endl;
#endif
    }

    {
        std::vector<std::string> unit_addrs = mocktable3.get_unit_accounts();
        for (uint32_t i = 0; i < loop_num; i++) {
            std::string sender = unit_addrs[i];
            std::string receiver = unit_addrs[loop_num];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable3.create_send_txs(sender, receiver, tx_num);
            // push txs by inverted order, high nonce with high charge score
            for (auto & tx : send_txs) {
                xtx_para_t para;
                std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
                int32_t ret = txmgr_table3.push_send_tx(tx_ent, 0);
                ASSERT_EQ(0, ret);
            }
        }
        usleep(1000);
        xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
        auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
        ASSERT_EQ(obj_num, 1024*3);
        auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
        std::cout << "statistic_tx_v2_size : " << obj_size << std::endl;
#endif
    }

    {
        std::vector<std::string> unit_addrs = mocktable4.get_unit_accounts();
        for (uint32_t i = 0; i < loop_num; i++) {
            std::string sender = unit_addrs[i];
            std::string receiver = unit_addrs[loop_num];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable4.create_send_txs(sender, receiver, tx_num);
            // push txs by inverted order, high nonce with high charge score
            for (auto & tx : send_txs) {
                xtx_para_t para;
                std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
                int32_t ret = txmgr_table4.push_send_tx(tx_ent, 0);
                ASSERT_EQ(0, ret);
            }
        }
        usleep(1000);
        xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
        auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
        ASSERT_EQ(obj_num, 1024*4);
        auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
        std::cout << "statistic_tx_v2_size : " << obj_size << std::endl;
#endif
    }

    {
        std::vector<std::string> unit_addrs = mocktable5.get_unit_accounts();
        for (uint32_t i = 0; i < loop_num; i++) {
            std::string sender = unit_addrs[i];
            std::string receiver = unit_addrs[loop_num];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable5.create_send_txs(sender, receiver, tx_num);
            // push txs by inverted order, high nonce with high charge score
            for (auto & tx : send_txs) {
                xtx_para_t para;
                std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);
                int32_t ret = txmgr_table5.push_send_tx(tx_ent, 0);
                ASSERT_EQ(0, ret);
            }
        }
        usleep(1000);
        xstatistic_t::instance().refresh();
#ifdef ENABLE_METRICS
        auto obj_num = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_num);
        ASSERT_EQ(obj_num, 1024*5);
        auto obj_size = XMETRICS_GAUGE_GET_VALUE(metrics::statistic_tx_v2_size);
        std::cout << "statistic_tx_v2_size : " << obj_size << std::endl;
#endif
    }

    // uint32_t ddd = 0;
    // while(1) {
    //     ddd ++;
    // }
}
#endif
