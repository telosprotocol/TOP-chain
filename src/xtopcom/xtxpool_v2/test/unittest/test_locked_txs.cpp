#include "gtest/gtest.h"
#include "test_xtxpool_util.h"
#include "xverifier/xverifier_utl.h"
#include "xtxpool_v2/xlocked_txs.h"
#include "xtxpool_v2/xtxpool_error.h"

using namespace top::xtxpool_v2;
using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;

class test_locked_txs : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_locked_txs, locked_txs_basic) {
    std::string table_addr = "table_test";
    xtxpool_shard_info_t shard(0, 0, 0);
    xtxpool_table_info_t table_para(table_addr, &shard);
    xlocked_txs_t locked_txs(&table_para);
    uint256_t last_tx_hash = {};
    uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    xtx_para_t para;

    uint32_t txs_num = 20;
    std::vector<xcons_transaction_ptr_t> txs = test_xtxpool_util_t::create_cons_transfer_txs(0, 1, txs_num);

    int32_t ret;
    locked_tx_map_t new_locked_tx_map1;
    for (uint32_t i = 0; i < 3; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        std::shared_ptr<locked_tx_t> locked_tx = std::make_shared<locked_tx_t>(txs[i]->get_account_addr(), txs[i]->get_tx_subtype(), tx_ent_tmp);
        new_locked_tx_map1[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
    }

    std::vector<std::shared_ptr<xtx_entry>> unlocked_txs1;
    locked_txs.update(new_locked_tx_map1, unlocked_txs1);
    ASSERT_EQ(unlocked_txs1.size(), 0);
    
    locked_tx_map_t new_locked_tx_map2;
    for (uint32_t i = 3; i < 6; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        std::shared_ptr<locked_tx_t> locked_tx = std::make_shared<locked_tx_t>(txs[i]->get_account_addr(), txs[i]->get_tx_subtype(), tx_ent_tmp);
        new_locked_tx_map1[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
        new_locked_tx_map2[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
    }

    std::vector<std::shared_ptr<xtx_entry>> unlocked_txs2;
    locked_txs.update(new_locked_tx_map1, unlocked_txs2);
    ASSERT_EQ(unlocked_txs2.size(), 0);

    locked_tx_map_t new_locked_tx_map3;
    for (uint32_t i = 6; i < 9; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        std::shared_ptr<locked_tx_t> locked_tx = std::make_shared<locked_tx_t>(txs[i]->get_account_addr(), txs[i]->get_tx_subtype(), tx_ent_tmp);
        new_locked_tx_map2[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
        new_locked_tx_map3[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
    }

    std::vector<std::shared_ptr<xtx_entry>> unlocked_txs3;
    locked_txs.update(new_locked_tx_map2, unlocked_txs3);
    ASSERT_EQ(unlocked_txs3.size(), 3);
    ASSERT_EQ(unlocked_txs3[0]->get_tx()->get_transaction()->get_tx_nonce() <= txs[2]->get_transaction()->get_tx_nonce(), true);
    ASSERT_EQ(unlocked_txs3[1]->get_tx()->get_transaction()->get_tx_nonce() <= txs[2]->get_transaction()->get_tx_nonce(), true);
    ASSERT_EQ(unlocked_txs3[2]->get_tx()->get_transaction()->get_tx_nonce() <= txs[2]->get_transaction()->get_tx_nonce(), true);

    for (uint32_t i = 3; i < 6; i++) {
        tx_info_t txinfo(txs[i]);
        bool result = false;
        auto tx_ent = locked_txs.pop_tx(txinfo, result);
        ASSERT_NE(tx_ent, nullptr);
    }

    locked_tx_map_t new_locked_tx_map4;
    for (uint32_t i = 9; i < 12; i++) {
        std::shared_ptr<xtx_entry> tx_ent_tmp = std::make_shared<xtx_entry>(txs[i], para);
        std::shared_ptr<locked_tx_t> locked_tx = std::make_shared<locked_tx_t>(txs[i]->get_account_addr(), txs[i]->get_tx_subtype(), tx_ent_tmp);
        new_locked_tx_map3[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
        new_locked_tx_map4[txs[i]->get_transaction()->get_digest_str()] = locked_tx;
    }

    std::vector<std::shared_ptr<xtx_entry>> unlocked_txs4;
    locked_txs.update(new_locked_tx_map3, unlocked_txs4);
    ASSERT_EQ(unlocked_txs4.size(), 0);

    std::shared_ptr<xtx_entry> tx_ent1 = std::make_shared<xtx_entry>(txs[0], para);
    bool result = locked_txs.try_push_tx(tx_ent1);
    ASSERT_EQ(result, false);

    std::shared_ptr<xtx_entry> tx_ent9 = std::make_shared<xtx_entry>(txs[9], para);
    result = locked_txs.try_push_tx(tx_ent9);
    ASSERT_EQ(result, true);

    std::shared_ptr<xtx_entry> tx_ent12 = std::make_shared<xtx_entry>(txs[12], para);
    std::shared_ptr<locked_tx_t> locked_tx12 = std::make_shared<locked_tx_t>(txs[12]->get_account_addr(), txs[12]->get_tx_subtype(), nullptr);
    new_locked_tx_map4[txs[12]->get_transaction()->get_digest_str()] = locked_tx12;

    std::vector<std::shared_ptr<xtx_entry>> unlocked_txs5;
    locked_txs.update(new_locked_tx_map4, unlocked_txs5);
    ASSERT_EQ(unlocked_txs5.size(), 3);

    result = locked_txs.try_push_tx(tx_ent12);
    ASSERT_EQ(result, true);
}
