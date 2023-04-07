#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xblocktool.h"
#include "xdata/xlightunit.h"
#include "xdata/xtx_factory.h"
#include "xtxpool_v2/xtx_queue.h"
#include "xtxpool_v2/xtxpool_para.h"
#include "xtxpool_v2/xtxpool_error.h"
#include "xdata/xverifier/xtx_verifier.h"
#include "xdata/xverifier/xverifier_utl.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;

class test_send_tx_queue : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

#if 0
TEST_F(test_send_tx_queue, continuous_txs_basic) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xsend_tx_queue_internal_t send_tx_queue_internal(&table_para);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    xcontinuous_txs_t continuous_txs(&send_tx_queue_internal, 0);
    ASSERT_EQ(0, continuous_txs.get_back_nonce());

    ASSERT_EQ(true, continuous_txs.empty());

    uint32_t txs_num = 17;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    std::shared_ptr<xtx_entry> tx_ent16 = std::make_shared<xtx_entry>(txs[16], para);
    int32_t ret = continuous_txs.insert(tx_ent16);
    ASSERT_EQ(ret, xtxpool_error_tx_nonce_out_of_scope);
    ASSERT_EQ(true, continuous_txs.empty());

    continuous_txs.update_latest_nonce(txs[0]->get_transaction()->get_tx_nonce());

    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(txs[0], para);
    ret = continuous_txs.insert(tx_ent0);
    ASSERT_EQ(ret, xtxpool_error_tx_nonce_expired);

    std::shared_ptr<xtx_entry> tx_ent2 = std::make_shared<xtx_entry>(txs[2], para);
    ret = continuous_txs.insert(tx_ent2);
    ASSERT_EQ(ret, xtxpool_error_tx_nonce_uncontinuous);

    ret = continuous_txs.insert(tx_ent16);
    ASSERT_EQ(ret, xtxpool_error_tx_nonce_uncontinuous);

    auto txs_get = continuous_txs.get_txs(4, 3);
    ASSERT_EQ(txs_get.size(), 0);

    for (uint32_t i = 1; i <= 6; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = continuous_txs.insert(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }
    ASSERT_EQ(false, continuous_txs.empty());
    ASSERT_EQ(7, continuous_txs.get_back_nonce());

    ret = continuous_txs.insert(tx_ent2);
    ASSERT_EQ(ret, xtxpool_error_tx_nonce_duplicate);

    ASSERT_EQ(continuous_txs.get_back_nonce(), txs[6]->get_transaction()->get_tx_nonce());

    txs_get = continuous_txs.get_txs(2, 3);
    ASSERT_EQ(txs_get.size(), 1);
    ASSERT_EQ(txs_get[0]->get_tx()->get_transaction()->get_tx_nonce(), 2);

    txs_get = continuous_txs.get_txs(3, 3);
    ASSERT_EQ(txs_get.size(), 2);
    ASSERT_EQ(txs_get[0]->get_tx()->get_transaction()->get_tx_nonce(), 2);
    ASSERT_EQ(txs_get[1]->get_tx()->get_transaction()->get_tx_nonce(), 3);

    txs_get = continuous_txs.get_txs(4, 3);
    ASSERT_EQ(txs_get.size(), 3);
    ASSERT_EQ(txs_get[0]->get_tx()->get_transaction()->get_tx_nonce(), 2);
    ASSERT_EQ(txs_get[1]->get_tx()->get_transaction()->get_tx_nonce(), 3);
    ASSERT_EQ(txs_get[2]->get_tx()->get_transaction()->get_tx_nonce(), 4);

    txs_get = continuous_txs.get_txs(5, 3);
    ASSERT_EQ(txs_get.size(), 0);

    txs_get = continuous_txs.get_txs(6, 3);
    ASSERT_EQ(txs_get.size(), 0);

    continuous_txs.erase(6, true);
    ASSERT_EQ(continuous_txs.get_back_nonce(), txs[4]->get_transaction()->get_tx_nonce());

    auto pop_txs = continuous_txs.pop_uncontinuous_txs();
    ASSERT_EQ(pop_txs.size(), 0);

    continuous_txs.erase(3, false);
    pop_txs = continuous_txs.pop_uncontinuous_txs();
    ASSERT_EQ(pop_txs.size(), 2);
    ASSERT_EQ(pop_txs[0]->get_tx()->get_transaction()->get_tx_nonce(), 4);
    ASSERT_EQ(pop_txs[1]->get_tx()->get_transaction()->get_tx_nonce(), 5);
    ASSERT_EQ(true, continuous_txs.empty());
}

TEST_F(test_send_tx_queue, continuous_txs_replace) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xsend_tx_queue_internal_t send_tx_queue_internal(&table_para);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    xcontinuous_txs_t continuous_txs(&send_tx_queue_internal, 0);
    ASSERT_EQ(0, continuous_txs.get_back_nonce());

    ASSERT_EQ(true, continuous_txs.empty());

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    int32_t ret;
    for (uint32_t i = 0; i < txs_num; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = continuous_txs.insert(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }

    ASSERT_EQ(txs[txs_num - 1]->get_transaction()->get_tx_nonce(), continuous_txs.get_back_nonce());

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 1, now + 100, txs[0]->get_transaction()->digest());
    std::shared_ptr<xtx_entry> tx_ent1b = std::make_shared<xtx_entry>(tx1, para);
    ret = continuous_txs.insert(tx_ent1b);
    ASSERT_EQ(ret, xsuccess);

    ASSERT_EQ(txs[txs_num - 1]->get_transaction()->get_tx_nonce(), continuous_txs.get_back_nonce());
}

TEST_F(test_send_tx_queue, uncontinuous_txs_basic) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xsend_tx_queue_internal_t send_tx_queue_internal(&table_para);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    xuncontinuous_txs_t uncontinuous_txs(&send_tx_queue_internal);

    ASSERT_EQ(uncontinuous_txs.empty(), true);

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    int32_t ret;
    for (uint32_t i = 1; i < txs_num; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = uncontinuous_txs.insert(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }

    auto pop_tx = uncontinuous_txs.pop_by_last_nonce(txs[0]->get_transaction()->get_tx_nonce());
    ASSERT_EQ(pop_tx->get_tx()->get_transaction()->get_tx_nonce(), txs[1]->get_transaction()->get_tx_nonce());

    pop_tx = uncontinuous_txs.pop_by_last_nonce(txs[3]->get_transaction()->get_tx_nonce());
    ASSERT_EQ(pop_tx->get_tx()->get_transaction()->get_tx_nonce(), txs[4]->get_transaction()->get_tx_nonce());

    pop_tx = uncontinuous_txs.pop_by_last_nonce(txs[2]->get_transaction()->get_tx_nonce());
    ASSERT_TRUE(pop_tx == nullptr);

    uncontinuous_txs.erase(txs[7]->get_transaction()->get_tx_nonce());
}

TEST_F(test_send_tx_queue, uncontinuous_txs_replace) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xsend_tx_queue_internal_t send_tx_queue_internal(&table_para);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    xuncontinuous_txs_t uncontinuous_txs(&send_tx_queue_internal);

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    int32_t ret;
    for (uint32_t i = 1; i < txs_num; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = uncontinuous_txs.insert(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 5, now + 100, txs[4]->get_transaction()->digest());
    std::shared_ptr<xtx_entry> tx_ent1b = std::make_shared<xtx_entry>(tx1, para);
    ret = uncontinuous_txs.insert(tx_ent1b);
    ASSERT_EQ(ret, xsuccess);

    auto pop_tx1 = uncontinuous_txs.pop_by_last_nonce(txs[4]->get_transaction()->get_tx_nonce());
    ASSERT_EQ(pop_tx1->get_tx()->get_transaction()->get_tx_nonce(), tx1->get_transaction()->get_tx_nonce());
}
#endif

TEST_F(test_send_tx_queue, send_tx_account_basic) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    xsend_tx_queue_internal_t send_tx_queue_internal(&table_para);
    uint256_t last_tx_hash = {};
    xtx_para_t para;

    xsend_tx_account_t send_tx_account(&send_tx_queue_internal, 0);

    uint32_t txs_num = 20;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    ASSERT_EQ(send_tx_account.empty(), true);

    int32_t ret;
    for (uint32_t i = 0; i < 5; i++) {
        if (i == 1) {
            continue;
        }
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = send_tx_account.push_tx(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }

    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    auto get_txs = send_tx_account.get_continuous_txs(3, 3, 0, now);
    ASSERT_EQ(get_txs.size(), 1);

    get_txs = send_tx_account.get_continuous_txs(3, 1, 0, now);
    ASSERT_EQ(get_txs.size(), 1);

    std::shared_ptr<xtx_entry> tx_ent_tmp1 = std::make_shared<xtx_entry>(txs[1], para);
    ret = send_tx_account.push_tx(tx_ent_tmp1);
    ASSERT_EQ(ret, xsuccess);

    get_txs = send_tx_account.get_continuous_txs(3, 3, 0, now);
    ASSERT_EQ(get_txs.size(), 3);

    get_txs = send_tx_account.get_continuous_txs(3, 4, 0, now);
    ASSERT_EQ(get_txs.size(), 3);

    for (uint32_t i = 7; i < 15; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        ret = send_tx_account.push_tx(tx_ent_tmp);
        ASSERT_EQ(ret, xsuccess);
    }

    send_tx_account.update_latest_nonce(txs[4]->get_transaction()->get_tx_nonce());

    get_txs = send_tx_account.get_continuous_txs(3, 8, txs[4]->get_transaction()->get_tx_nonce(), now);
    ASSERT_EQ(get_txs.size(), 0);

    tx_ent_tmp1 = std::make_shared<xtx_entry>(txs[6], para);
    ret = send_tx_account.push_tx(tx_ent_tmp1);
    ASSERT_EQ(ret, xsuccess);

    get_txs = send_tx_account.get_continuous_txs(3, 8, txs[4]->get_transaction()->get_tx_nonce(), now);
    ASSERT_EQ(get_txs.size(), 0);

    tx_ent_tmp1 = std::make_shared<xtx_entry>(txs[5], para);
    ret = send_tx_account.push_tx(tx_ent_tmp1);
    ASSERT_EQ(ret, xsuccess);

    get_txs = send_tx_account.get_continuous_txs(3, 8, txs[4]->get_transaction()->get_tx_nonce(), now);
    ASSERT_EQ(get_txs.size(), 3);

    send_tx_account.erase(8, false);

    get_txs = send_tx_account.get_continuous_txs(3, 8, txs[4]->get_transaction()->get_tx_nonce(), now);
    ASSERT_EQ(get_txs.size(), 2);
    get_txs = send_tx_account.get_continuous_txs(3, 7, txs[4]->get_transaction()->get_tx_nonce(), now);
    ASSERT_EQ(get_txs.size(), 2);

    send_tx_account.update_latest_nonce(txs[7]->get_transaction()->get_tx_nonce());

    get_txs = send_tx_account.get_continuous_txs(3, 11, txs[7]->get_transaction()->get_tx_nonce(), now);
    ASSERT_EQ(get_txs.size(), 3);

    get_txs = send_tx_account.get_continuous_txs(3, 11, txs[7]->get_transaction()->get_tx_nonce(), now + 500);
    ASSERT_EQ(get_txs.size(), 0);
}

TEST_F(test_send_tx_queue, send_tx_queue_sigle_tx) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now, last_tx_hash);
    tx->set_push_pool_timestamp(now);

    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx, para);

    // push first time
    int32_t ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    auto tx_tmp = send_tx_queue.find(tx->get_tx_hash());
    ASSERT_NE(tx_tmp.get(), nullptr);

    // duplicate push
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(xtxpool_error_tx_nonce_duplicate, ret);

    // pop out
    auto tx_ent_tmp = send_tx_queue.pop_tx(tx->get_tx_hash(), false);
    ASSERT_NE(tx_ent_tmp.get(), nullptr);
    tx_tmp = send_tx_queue.find(tx->get_tx_hash());
    ASSERT_EQ(tx_tmp.get(), nullptr);

    // push again
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);
    tx_tmp = send_tx_queue.find(tx->get_tx_hash());
    ASSERT_NE(tx_tmp.get(), nullptr);

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    uint32_t expired_num;
    uint32_t unconituous_num;
    auto get_txs = send_tx_queue.get_txs(100, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(get_txs.size(), 1);

    // ASSERT_EQ(send_tx_queue.is_account_need_update(tx->get_transaction()->get_source_addr()), false);

    send_tx_queue.updata_latest_nonce(tx->get_transaction()->source_address().to_string(), tx->get_transaction()->get_tx_nonce());

    get_txs = send_tx_queue.get_txs(100, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(get_txs.size(), 0);

    // ASSERT_EQ(send_tx_queue.is_account_need_update(tx->get_transaction()->get_source_addr()), false);
}

TEST_F(test_send_tx_queue, send_tx_queue_continuous_txs) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    // push txs by inverted order, high nonce with high charge score
    for (uint32_t i = 0; i < txs.size(); i++) {
        xtx_para_t para;
        para.set_charge_score(i);
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[txs.size() - i - 1], para);
        int32_t ret = send_tx_queue.push_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());

    uint32_t expired_num;
    uint32_t unconituous_num;
    auto tx_ents = send_tx_queue.get_txs(txs_num, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents.size(), txs_num);
    for (uint32_t i = 0; i < tx_ents.size(); i++) {
        ASSERT_EQ(tx_ents[i]->get_transaction()->get_last_nonce(), i);
    }

    // push again
    for (uint32_t i = 0; i < tx_ents.size(); i++) {
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(tx_ents[tx_ents.size() - i - 1], para);
        int32_t ret = send_tx_queue.push_tx(tx_ent, 0);
        ASSERT_EQ(xtxpool_error_tx_nonce_duplicate, ret);
    }

    // pop one tx, that will pop txs those nonce are less than the poped tx, and no continuos tx
    auto tx_tmp = send_tx_queue.pop_tx(txs[3]->get_tx_hash(), false);
    auto tx_ents2 = send_tx_queue.get_txs(txs_num, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents2.size(), 3);

    send_tx_queue.updata_latest_nonce(txs[3]->get_transaction()->source_address().to_string(), txs[3]->get_transaction()->get_tx_nonce());
    tx_ents2 = send_tx_queue.get_txs(txs_num, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents2.size(), txs_num - 4);
}

TEST_F(test_send_tx_queue, send_tx_queue_uncontinuous_send_txs) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[3], para);
    int32_t ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    tx_ent = std::make_shared<xtx_entry>(txs[4], para);
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    tx_ent = std::make_shared<xtx_entry>(txs[6], para);
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    tx_ent = std::make_shared<xtx_entry>(txs[5], para);
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    tx_ent = std::make_shared<xtx_entry>(txs[2], para);
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    for (uint32_t i = 2; i < 5; i++) {
        auto tx_tmp = send_tx_queue.find(txs[i]->get_tx_hash());
        ASSERT_NE(tx_tmp.get(), nullptr);
    }

    tx_ent = std::make_shared<xtx_entry>(txs[0], para);
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());
    uint32_t expired_num;
    uint32_t unconituous_num;
    auto tx_ents = send_tx_queue.get_txs(10, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents.size(), 1);

    tx_ent = std::make_shared<xtx_entry>(txs[1], para);
    ret = send_tx_queue.push_tx(tx_ent, 0);
    ASSERT_EQ(0, ret);

    tx_ents = send_tx_queue.get_txs(10, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents.size(), 7);
}

TEST_F(test_send_tx_queue, 2_nonce_duplicate_send_tx) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    uint256_t last_tx_hash1 = {};
    uint256_t last_tx_hash2 = {};
    std::vector<xcons_transaction_ptr_t> txs1;
    std::vector<xcons_transaction_ptr_t> txs2;

    xcons_transaction_ptr_t tx = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, 0, now + 1, {});
    tx->set_push_pool_timestamp(now + 1);
    last_tx_hash1 = tx->get_transaction()->digest();
    last_tx_hash2 = tx->get_transaction()->digest();
    txs1.push_back(tx);
    txs2.push_back(tx);

    for (uint64_t i = 1; i <= 2; i++) {
        xcons_transaction_ptr_t tx1 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, i, now + i, last_tx_hash1);
        tx1->set_push_pool_timestamp(now + i);
        last_tx_hash1 = tx1->get_transaction()->digest();
        txs1.push_back(tx1);

        xcons_transaction_ptr_t tx2 = test_xtxpool_util_t::create_cons_transfer_tx(0, 1, i, now + i + 10, last_tx_hash2);
        tx2->set_push_pool_timestamp(now + i + 10);
        last_tx_hash2 = tx2->get_transaction()->digest();
        txs2.push_back(tx2);
    }

    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(txs1[0], para);
    ASSERT_EQ(0, send_tx_queue.push_tx(tx_ent0, 0));
    std::shared_ptr<xtx_entry> tx_ent11 = std::make_shared<xtx_entry>(txs1[1], para);
    ASSERT_EQ(0, send_tx_queue.push_tx(tx_ent11, 0));
    std::shared_ptr<xtx_entry> tx_ent21 = std::make_shared<xtx_entry>(txs2[1], para);
    ASSERT_EQ(0, send_tx_queue.push_tx(tx_ent21, 0));
    std::shared_ptr<xtx_entry> tx_ent22 = std::make_shared<xtx_entry>(txs2[2], para);
    ASSERT_EQ(0, send_tx_queue.push_tx(tx_ent22, 0));
    std::shared_ptr<xtx_entry> tx_ent12 = std::make_shared<xtx_entry>(txs1[2], para);
    ASSERT_EQ(xtxpool_error_tx_nonce_duplicate, send_tx_queue.push_tx(tx_ent12, 0));
}

#if 0
// hash continuous constraint was canceled
TEST_F(test_send_tx_queue, update_latest_nonce_hash_not_match) {
    std::string table_addr = "table_test";
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);
    std::string sender = test_xtxpool_util_t::get_account(0);

    // push txs by inverted order, high nonce with high charge score
    for (uint32_t i = 0; i < txs.size(); i++) {
        if (i == 5) {
            continue;
        }
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs[i], para);
        int32_t ret = send_tx_queue.push_tx(tx_ent, 0);
        ASSERT_EQ(0, ret);
    }

    send_tx_queue.updata_latest_nonce(sender, 3);

    for (uint32_t i = 0; i <= 5; i++) {
        auto tx_ent = send_tx_queue.find(txs[i]->get_transaction()->digest());
        ASSERT_EQ(tx_ent, nullptr);
    }

    for (uint32_t i = 6; i < txs.size(); i++) {
        auto tx_ent = send_tx_queue.find(txs[i]->get_transaction()->digest());
        ASSERT_NE(tx_ent, nullptr);
    }
}
#endif

TEST_F(test_send_tx_queue, reached_upper_limit_basic) {
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 15, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    uint32_t txs_num = 10;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    table_para.send_tx_inc(table_send_tx_queue_size_max);

    std::shared_ptr<xtx_entry> tx_ent0 = std::make_shared<xtx_entry>(txs[0], para);
    int32_t ret = send_tx_queue.push_tx(tx_ent0, 0);
    ASSERT_EQ(xtxpool_error_table_reached_upper_limit, ret);

    table_para.send_tx_dec(1);

    ret = send_tx_queue.push_tx(tx_ent0, 0);
    ASSERT_EQ(0, ret);

    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(txs[1], para);
    ret = send_tx_queue.push_tx(tx_ent1, 0);
    ASSERT_EQ(xtxpool_error_table_reached_upper_limit, ret);

    std::shared_ptr<xtx_entry> tx_ent2 = std::make_shared<xtx_entry>(txs[2], para);
    ret = send_tx_queue.push_tx(tx_ent2, 0);
    ASSERT_EQ(xtxpool_error_table_reached_upper_limit, ret);

    table_para.send_tx_dec(1);

    ret = send_tx_queue.push_tx(tx_ent2, 0);
    ASSERT_EQ(0, ret);

    ret = send_tx_queue.push_tx(tx_ent1, 0);
    ASSERT_EQ(0, ret);

    auto find_tx = send_tx_queue.find(txs[2]->get_tx_hash());
    ASSERT_EQ(find_tx.get(), nullptr);

    std::shared_ptr<xtx_entry> tx_ent3 = std::make_shared<xtx_entry>(txs[3], para);
    ret = send_tx_queue.push_tx(tx_ent3, 0);
    ASSERT_EQ(xtxpool_error_table_reached_upper_limit, ret);
}

TEST_F(test_send_tx_queue, send_tx_queue_too_many_uncontinuous_send_txs) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();
    std::string table_addr = xdatamock_address::make_consensus_table_address(1);
    xtxpool_role_info_t shard(0, 0, 0, common::xnode_type_t::consensus_auditor);
    xtxpool_statistic_t statistic;
    xtable_state_cache_t table_state_cache(nullptr, table_addr);
    xtxpool_table_info_t table_para(table_addr, &shard, &statistic, &table_state_cache);
    uint256_t last_tx_hash = {};
    xtx_para_t para;
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();

    xtxpool_resources resource(nullptr, nullptr, nullptr);
    xsend_tx_queue_t send_tx_queue(&table_para, &resource);

    uint32_t txs_num_0_1 = 11;
    std::vector<xcons_transaction_ptr_t> txs_0_1 = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num_0_1);

    uint32_t txs_num_1_0 = 10;
    std::vector<xcons_transaction_ptr_t> txs_1_0 = test_xtxpool_util_t::create_cons_transfer_txs(1, 0, txs_num_1_0);

    for (uint32_t i = 1; i < txs_num_0_1; i++) {
        std::shared_ptr<xtx_entry> tx_ent = std::make_shared<xtx_entry>(txs_0_1[i], para);
        send_tx_queue.push_tx(tx_ent, 0);
    }

    base::xauto_ptr<base::xvblock_t> table_genesis_block = xblocktool_t::create_genesis_empty_table(table_addr);
    blockstore->store_block(base::xvaccount_t(table_addr), table_genesis_block.get());
    uint32_t expired_num;
    uint32_t unconituous_num;
    auto tx_ents = send_tx_queue.get_txs(10, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents.size(), 0);

    std::shared_ptr<xtx_entry> tx_ent1_0 = std::make_shared<xtx_entry>(txs_1_0[0], para);
    send_tx_queue.push_tx(tx_ent1_0, 0);

    tx_ents = send_tx_queue.get_txs(10, table_genesis_block.get(), expired_num, unconituous_num);
    ASSERT_EQ(tx_ents.size(), 1);
}

TEST_F(test_send_tx_queue, v3_tx_expire) {
    std::string rawtx_bin = "0x02f8708203ff80822710822710827b0c94b7762d8dbd7e5c023ff99402b78af7c13b01eec1881bc16d674ec8000080c001a0d336694faa98f9cd69792ee30f9979f906f997d7562a0cb86d109f3476643a65a0679a7f510b12d48f9d1637884245229b7037b4f4094d1fb30e0f4f5cc77388ec";
    data::eth_error ec;
    data::xeth_transaction_t ethtx = data::xeth_transaction_t::build_from(rawtx_bin,ec);
    if (ec.error_code) {xassert(false);}

    xtransaction_ptr_t v3tx = xtx_factory::create_v3_tx(ethtx);

    uint64_t fire_timestamp = 10000;
    v3tx->set_fire_timestamp_ext(fire_timestamp);

    uint32_t trx_fire_tolerance_time = XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_send_timestamp_tolerance);

    auto expire_duration = v3tx->get_expire_duration();
    EXPECT_EQ(43200, expire_duration);
    
    auto ret = xverifier::xtx_verifier::verify_tx_fire_expiration(v3tx.get(), fire_timestamp + expire_duration + trx_fire_tolerance_time, false);
    EXPECT_EQ(ret , 0);
    ret = xverifier::xtx_verifier::verify_tx_fire_expiration(v3tx.get(), fire_timestamp + expire_duration + trx_fire_tolerance_time + 1, false);
    EXPECT_NE(ret , 0);
}
