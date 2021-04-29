#include "gtest/gtest.h"
#include "xblockmaker/xtable_maker.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"
#include "xblockstore/xblockstore_face.h"
#include "xdata/xtransaction_maker.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_tx.hpp"
#include "tests/mock/xcertauth_util.hpp"
#include "tests/mock/xdatamock_address.hpp"
#include "test_common.hpp"

using namespace top::blockmaker;
using namespace top::store;
using namespace top::base;
using namespace top::mock;

class test_table_maker : public testing::Test {
 protected:
    void SetUp() override {

    }

    void TearDown() override {
    }
 public:

};

class test_table_maker_base {
 public:
    explicit test_table_maker_base(uint32_t user_count) {
        m_resouces = test_xblockmaker_resources_t::create();
        m_table_account = xblocktool_t::make_address_shard_table_account(1);
        for (uint32_t i = 0; i < user_count; i++) {
            uint8_t addr_type = base::enum_vaccount_addr_type_secp256k1_user_account;
            uint16_t ledger_id = base::xvaccount_t::make_ledger_id(base::enum_main_chain_id, base::enum_chain_zone_consensus_index);
            xecprikey_t pri_key_obj;
            xecpubkey_t pub_key_obj = pri_key_obj.get_public_key();
            std::string user_account = pub_key_obj.to_address(addr_type, ledger_id);
            xdatamock_tx datamock_tx(m_resouces, user_account, pri_key_obj);
            m_user_accounts.push_back(datamock_tx);
        }
        m_dst_account = xblocktool_t::make_address_user_account("222222222222233333");
        m_table_maker = make_object_ptr<xtable_maker_t>(m_table_account, m_resouces);
        xassert(!m_user_accounts.empty());
        xassert(xsuccess == m_table_maker->default_check_latest_state());
    }
    const std::string &     get_table_account() const {return m_table_account;}
    const xblockmaker_resources_ptr_t & get_resources() const {return m_resouces;}
    uint32_t                get_max_batch_txs_account_num() const {return m_non_empty_unit_max;}
    const std::vector<xdatamock_tx> &   get_unit_accounts() const {return m_user_accounts;}

    std::vector<xcons_transaction_ptr_t> generate_batch_txs(uint32_t user_count, uint32_t tx_count) {
        std::vector<xcons_transaction_ptr_t> batch_txs;
        xassert(user_count <= m_user_accounts.size());
        xassert(user_count <= m_non_empty_unit_max);

        uint32_t max_count = user_count;
        uint32_t index = m_last_generate_txs_account_index;
        for (uint32_t i = 0; i < max_count; i++) {
            if (index >= m_user_accounts.size()) {
                index = 0;
            }
            auto txs = m_user_accounts[index].generate_transfer_tx_with_db_state(m_dst_account, tx_count);
            for (auto & tx : txs) {
                batch_txs.push_back(tx);
            }
            index++;
        }
        m_last_generate_txs_account_index += max_count;
        return batch_txs;
    }

    xblock_ptr_t    make_proposal(uint32_t user_count, uint32_t tx_count) {
        base::xblock_mptrs latest_blocks = m_resouces->get_blockstore()->get_latest_blocks(m_table_account);

        xblock_consensus_para_t cs_para({10, 10}, latest_blocks.get_latest_cert_block());
        cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
        cs_para.set_latest_blocks(latest_blocks);

        xtablemaker_result_t result;

        create_txreceipts_to_pool(cs_para.get_latest_committed_block());
        if (is_txpool_empty()) {
            create_sendtxs_to_pool(user_count, tx_count);
        }

        std::vector<xcons_transaction_ptr_t> origin_txs = get_txs_from_txpool();
        xtablemaker_para_t table_para(origin_txs);
        xblock_ptr_t proposal_block = m_table_maker->make_proposal(table_para, cs_para, result);
        m_proposal_txs = table_para.get_proposal()->get_input_txs();
        pop_txs_from_txpool(proposal_block);
        return proposal_block;
    }

    int32_t  verify_proposal(const xblock_ptr_t & proposal_block) {
        base::xblock_mptrs latest_blocks = m_resouces->get_blockstore()->get_latest_blocks(m_table_account);
        xblock_consensus_para_t cs_para({10, 10}, latest_blocks.get_latest_cert_block());
        cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
        cs_para.set_latest_blocks(latest_blocks);
        xtablemaker_para_t table_para(m_proposal_txs);
        return m_table_maker->verify_proposal(proposal_block.get(), table_para, cs_para);
    }

    bool is_txpool_empty() {
        return m_pool_txs.empty();
    }

    std::vector<xcons_transaction_ptr_t>    get_txs_from_txpool() {
        // pop all txs from txpool
        std::vector<xcons_transaction_ptr_t> pool_txs = m_pool_txs;
        m_pool_txs.clear();
        return pool_txs;
    }

    void    pop_tx(const base::xvtxkey_t & txkey) {
        for (auto iter = m_pool_txs.begin(); iter != m_pool_txs.end(); iter++) {
            auto & tx = *iter;
            if (tx->get_transaction()->get_digest_str() == txkey.get_tx_hash() && tx->get_tx_subtype() == txkey.get_tx_subtype()) {
                m_pool_txs.erase(iter);
                break;
            }
        }
    }

    void    pop_txs_from_txpool(const xblock_ptr_t & commit_block) {
        if (commit_block->get_block_class() == base::enum_xvblock_class_light) {
            const auto & units = commit_block->get_tableblock_units(false);
            for (auto & unit : units) {
                if (unit->is_lightunit()) {
                    const std::vector<xlightunit_tx_info_ptr_t> & txs_info = unit->get_txs();
                    for (auto & tx : txs_info) {
                        pop_tx(tx->get_tx_key());
                    }
                }
            }
        }
    }

    void    create_sendtxs_to_pool(uint32_t user_count, uint32_t tx_count) {
        if (user_count != 0 && tx_count != 0) {
            std::vector<xcons_transaction_ptr_t> origin_txs = generate_batch_txs(user_count, tx_count);
            for (auto & tx : origin_txs) {
                m_pool_txs.push_back(tx);
            }
        }
    }

    void    create_txreceipts_to_pool(const xblock_ptr_t & proposal_block) {
        if (proposal_block->get_block_class() == base::enum_xvblock_class_light) {
            const auto & units = proposal_block->get_tableblock_units(false);
            for (auto & unit : units) {
                if (unit->is_lightunit()) {
                    std::vector<xcons_transaction_ptr_t> recvtxs;
                    std::vector<xcons_transaction_ptr_t> confirmtxs;
                    (dynamic_cast<xlightunit_block_t*>(unit.get()))->create_txreceipts(recvtxs, confirmtxs);
                    for (auto & tx : recvtxs) {
                        m_pool_txs.push_back(tx);
                    }
                    for (auto & tx : confirmtxs) {
                        m_pool_txs.push_back(tx);
                    }
                }
            }
        }
    }

 protected:
    xblockmaker_resources_ptr_t m_resouces;
    std::string                 m_table_account;
    std::vector<xdatamock_tx>   m_user_accounts;
    std::string                 m_dst_account;
    xtable_maker_ptr_t          m_table_maker;
    uint32_t                    m_non_empty_unit_max{32};  // TODO(jimmy)
    uint32_t                    m_last_generate_txs_account_index{0};
    std::vector<xcons_transaction_ptr_t> m_proposal_txs;

    std::vector<xcons_transaction_ptr_t> m_pool_txs;
};

TEST_F(test_table_maker, table_maker_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
}

TEST_F(test_table_maker, table_maker_update_latest_blocks_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

    xdatamock_table mocktable(1, 1, resouces->get_store());
    base::xvaccount_t _vaddr(mocktable.get_account());
    mocktable.genrate_table_chain(3);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    for (auto & table : tables) {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(_vaddr, table.get()));
    }
    auto latest_cert_block = resouces->get_blockstore()->get_latest_cert_block(_vaddr);
    ASSERT_EQ(latest_cert_block->get_height(), 3);

    std::string taccount = mocktable.get_account();
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
}

TEST_F(test_table_maker, table_maker_update_latest_blocks_2) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

    xdatamock_table mocktable(1, 1, resouces->get_store());
    base::xvaccount_t _vaddr(mocktable.get_account());
    mocktable.genrate_table_chain(100);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    for (auto & table : tables) {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(_vaddr, table.get()));
    }
    auto latest_cert_block = resouces->get_blockstore()->get_latest_cert_block(_vaddr);
    ASSERT_EQ(latest_cert_block->get_height(), 100);

    std::string taccount = mocktable.get_account();
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
}

TEST_F(test_table_maker, table_maker_update_latest_blocks_3) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();

    xdatamock_table mocktable(1, 1, resouces->get_store());
    base::xvaccount_t _vaddr(mocktable.get_account());
    mocktable.genrate_table_chain(1000);
    const std::vector<xblock_ptr_t> & tables = mocktable.get_history_tables();
    for (auto & table : tables) {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(_vaddr, table.get()));
    }
    auto latest_cert_block = resouces->get_blockstore()->get_latest_cert_block(_vaddr);
    ASSERT_EQ(latest_cert_block->get_height(), 1000);

    std::string taccount = mocktable.get_account();
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
}

std::vector<xcons_transaction_ptr_t>    generate_transfer_txs(uint16_t subaddr, const xblockmaker_resources_ptr_t & resouces, uint32_t account_count, uint32_t tx_count_per_account) {
    std::vector<xcons_transaction_ptr_t> origin_txs;
    for (uint32_t i = 0; i < account_count; i++) {
        std::string source_address = xdatamock_address::make_user_address_random(subaddr);
        xdatamock_tx datamock_account1(resouces, source_address);
        std::string target_address = xdatamock_address::make_user_address_random(subaddr);
        auto txs = datamock_account1.generate_transfer_tx(target_address, tx_count_per_account);
        for (auto & tx : txs) {
            origin_txs.push_back(tx);
        }
    }
    return origin_txs;
}

TEST_F(test_table_maker, table_maker_roll_back_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    uint16_t subaddr = 1;
    std::string taccount = xblocktool_t::make_address_shard_table_account(subaddr);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);

    auto txs1 = generate_transfer_txs(subaddr, resouces, 1, 2);

    xblock_ptr_t first_table;
    xblock_ptr_t second_table;
    {
        base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
        xblock_consensus_para_t cs_para(taccount, 10, 10, 10, latest_blocks.get_latest_cert_block()->get_height() + 1);
        cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
        cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
        cs_para.set_latest_blocks(latest_blocks);

        xtablemaker_para_t table_para(txs1);

        xtablemaker_result_t result;
        first_table = table_maker->make_proposal(table_para, cs_para, result);
        ASSERT_NE(first_table, nullptr);
        xdatamock_tx::do_mock_signature(first_table.get());
    }
    {
        base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
        xblock_consensus_para_t cs_para(taccount, 20, 20, 20, latest_blocks.get_latest_cert_block()->get_height() + 1);
        cs_para.set_common_consensus_para(20, {-1, -1}, {0, 0}, 20, 20, 20);
        cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
        cs_para.set_latest_blocks(latest_blocks);

        xtablemaker_para_t table_para;
        table_para.set_origin_txs(txs1);

        xtablemaker_result_t result;
        second_table = table_maker->make_proposal(table_para, cs_para, result);
        ASSERT_NE(second_table, nullptr);
        xdatamock_tx::do_mock_signature(second_table.get());
    }

    {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), first_table.get()));
        ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
        ASSERT_EQ(table_maker->get_highest_height_block()->get_block_hash(), first_table->get_block_hash());
    }
    {
        ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), second_table.get()));
        ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);
        ASSERT_EQ(table_maker->get_highest_height_block()->get_block_hash(), second_table->get_block_hash());
    }
}

TEST_F(test_table_maker, table_maker_make_proposal_0) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);

    std::vector<xcons_transaction_ptr_t> origin_txs = generate_transfer_txs(1, resouces, 2, 2);

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
    cs_para.set_latest_blocks(latest_blocks);

    xtablemaker_para_t table_para(origin_txs);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 4);
}

TEST_F(test_table_maker, table_maker_make_proposal_1) {
    xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
    std::string taccount = xblocktool_t::make_address_shard_table_account(1);
    std::vector<xcons_transaction_ptr_t> origin_txs = generate_transfer_txs(1, resouces, 2, 2);

    xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);
    ASSERT_EQ(table_maker->default_check_latest_state(), xsuccess);

    base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
    xblock_consensus_para_t cs_para;
    cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
    cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
    cs_para.set_latest_blocks(latest_blocks);

    xtablemaker_para_t table_para(origin_txs);

    xtablemaker_result_t result;
    xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
    ASSERT_NE(table1, nullptr);
    xdatamock_tx::do_mock_signature(table1.get());
    ASSERT_EQ(table1->get_height(), 1);
    ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
    ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
    ASSERT_EQ(table1->get_txs_count(), 4);

    xtablemaker_para_t table_para2(origin_txs);
    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para2, cs_para));

    ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));

    xtablemaker_para_t table_para3(origin_txs);
    ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para3, cs_para));
}

// TEST_F(test_table_maker, table_maker_make_proposal_2) {
//     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
//     std::string taccount = xblocktool_t::make_address_shard_table_account(1);
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
//     std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

//     xdatamock_tx datamock_account1(resouces, account1);
//     xdatamock_tx datamock_account2(resouces, account2);

//     xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);

//     {
//     auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
//     auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);

//     base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(taccount);
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
//     cs_para.set_latest_blocks(latest_blocks);

//     xtablemaker_para_t table_para;
//     table_para.set_unitmaker_txs(account1, txs1);
//     table_para.set_unitmaker_txs(account2, txs2);

//     xtablemaker_result_t result;
//     xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
//     ASSERT_NE(table1, nullptr);
//     xdatamock_tx::do_mock_signature(table1.get());
//     ASSERT_EQ(table1->get_height(), 1);
//     ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
//     ASSERT_EQ(table1->get_txs_count(), 4);

//     ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));
//     }

//     {
//     base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
//     cs_para.set_latest_blocks(latest_blocks);

//     xtablemaker_para_t table_para;

//     xtablemaker_result_t result;
//     table_maker->default_check_latest_state();
//     xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
//     ASSERT_NE(table1, nullptr);
//     xdatamock_tx::do_mock_signature(table1.get());
//     ASSERT_EQ(table1->get_height(), 2);
//     ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
//     ASSERT_EQ(table1->get_txs_count(), 0);

//     ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));
//     }

//     {
//     base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
//     cs_para.set_latest_blocks(latest_blocks);

//     xtablemaker_para_t table_para;

//     xtablemaker_result_t result;
//     table_maker->default_check_latest_state();
//     xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
//     ASSERT_NE(table1, nullptr);
//     xdatamock_tx::do_mock_signature(table1.get());
//     ASSERT_EQ(table1->get_height(), 3);
//     ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
//     ASSERT_EQ(table1->get_txs_count(), 0);

//     ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));
//     }

//     {
//     auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 3);
//     auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 3);
//     base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
//     cs_para.set_latest_blocks(latest_blocks);

//     xtablemaker_para_t table_para;
//     table_para.set_unitmaker_txs(account1, txs1);
//     table_para.set_unitmaker_txs(account2, txs2);

//     xtablemaker_result_t result;
//     table_maker->default_check_latest_state();
//     xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
//     ASSERT_NE(table1, nullptr);
//     xdatamock_tx::do_mock_signature(table1.get());
//     ASSERT_EQ(table1->get_height(), 4);
//     ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
//     ASSERT_EQ(table1->get_txs_count(), 6);

//     ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_make_proposal_3) {
//     xblockmaker_resources_ptr_t resouces = test_xblockmaker_resources_t::create();
//     std::string taccount = xblocktool_t::make_address_shard_table_account(1);
//     std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
//     std::string account2 = xblocktool_t::make_address_user_account("22222222222222222222");
//     std::string dstaccount = xblocktool_t::make_address_user_account("222222222222233333");

//     xdatamock_tx datamock_account1(resouces, account1);
//     xdatamock_tx datamock_account2(resouces, account2);

//     xtable_maker_ptr_t table_maker = make_object_ptr<xtable_maker_t>(taccount, resouces);

//     {
//     auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
//     auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);
//     base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
//     cs_para.set_latest_blocks(latest_blocks);

//     xtablemaker_para_t table_para;
//     table_para.set_unitmaker_txs(account1, txs1);
//     table_para.set_unitmaker_txs(account2, txs2);

//     xtablemaker_result_t result;
//     xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
//     ASSERT_NE(table1, nullptr);
//     xdatamock_tx::do_mock_signature(table1.get());
//     ASSERT_EQ(table1->get_height(), 1);
//     ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
//     ASSERT_EQ(table1->get_txs_count(), 4);

//     ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));
//     }

//     {
//     auto txs1 = datamock_account1.generate_transfer_tx(dstaccount, 2);
//     auto txs2 = datamock_account2.generate_transfer_tx(dstaccount, 2);
//     base::xblock_mptrs latest_blocks = resouces->get_blockstore()->get_latest_blocks(base::xvaccount_t(taccount));
//     xblock_consensus_para_t cs_para;
//     cs_para.set_common_consensus_para(10, {-1, -1}, {0, 0}, 10, 10, 10);
//     cs_para.set_tableblock_consensus_para(10, "random", 10, "extra");
//     cs_para.set_latest_blocks(latest_blocks);

//     xtablemaker_para_t table_para;
//     table_para.set_unitmaker_txs(account1, txs1);
//     table_para.set_unitmaker_txs(account2, txs2);

//     xtablemaker_result_t result;
//     table_maker->default_check_latest_state();
//     xblock_ptr_t table1 = table_maker->make_proposal(table_para, cs_para, result);
//     ASSERT_NE(table1, nullptr);
//     xdatamock_tx::do_mock_signature(table1.get());
//     ASSERT_EQ(table1->get_height(), 2);
//     ASSERT_EQ(table1->get_block_class(), base::enum_xvblock_class_light);
//     ASSERT_EQ(table1->get_tableblock_units(false).size(), 2);
//     ASSERT_EQ(table1->get_txs_count(), 0);

//     auto & units = table1->get_tableblock_units(true);
//     ASSERT_EQ(units.size(), 2);
//     ASSERT_EQ(units[0]->get_block_class(), base::enum_xvblock_class_nil);
//     ASSERT_EQ(units[1]->get_block_class(), base::enum_xvblock_class_nil);

//     ASSERT_EQ(xsuccess, table_maker->verify_proposal(table1.get(), table_para, cs_para));
//     ASSERT_TRUE(resouces->get_blockstore()->store_block(base::xvaccount_t(taccount), table1.get()));
//     }

// }

// TEST_F(test_table_maker, table_maker_one_account_0) {
//     test_table_maker_base   table_maker(1);
//     for (uint64_t height = 1; height < 50; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 1);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         // ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//         const std::vector<xblock_ptr_t> & units = block->get_tableblock_units(true);
//         for (auto & unit : units) {
//             std::cout << "unit account=" << unit->get_account() << " height=" << unit->get_height() << " class=" << unit->get_block_class() << std::endl;
//         }
//     }
// }

// TEST_F(test_table_maker, table_maker_one_account_1) {
//     test_table_maker_base   table_maker(1);
//     for (uint64_t height = 1; height < 1000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 1);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_one_account_2) {
//     test_table_maker_base   table_maker(1);
//     for (uint64_t height = 1; height < 2; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 1);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }


// TEST_F(test_table_maker, table_maker_little_account_1) {
//     test_table_maker_base   table_maker(3);
//     for (uint64_t height = 1; height < 1000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 2);
//         xassert(block != nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         xassert(xsuccess == table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_little_account_2) {
//     test_table_maker_base   table_maker(3);
//     for (uint64_t height = 1; height < 1000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(3, 2);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_little_account_3) {
//     test_table_maker_base   table_maker(20);
//     for (uint64_t height = 1; height < 1000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 2);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_little_account_4) {
//     test_table_maker_base   table_maker(20);
//     for (uint64_t height = 1; height < 1000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(20, 2);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_multi_account_1) {
//     test_table_maker_base   table_maker(500);
//     for (uint64_t height = 1; height < 1000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 2);
//         xassert(block != nullptr);
//         xassert(block->get_height() == height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_multi_account_2) {
//     test_table_maker_base   table_maker(500);
//     for (uint64_t height = 1; height < 3000; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(3, 2);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         if (block->get_block_class() != base::enum_xvblock_class_light) {
//             std::cout << "full-table height=" << block->get_height() << std::endl;
//         }
//         ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//     }
// }

// TEST_F(test_table_maker, table_maker_multi_account_3) {
//     {
//         int64_t start_ms = base::xtime_utl::gettimeofday_ms();
//         test_table_maker_base   table_maker(1000);
//         for (uint64_t height = 1; height < 1000; height++) {
//             xblock_ptr_t block = table_maker.make_proposal(table_maker.get_max_batch_txs_account_num(), 2);
//             ASSERT_NE(block, nullptr);
//             ASSERT_EQ(block->get_height(), height);
//             xdatamock_tx::do_mock_signature(block.get());
//             if (block->get_block_class() != base::enum_xvblock_class_light) {
//                 std::cout << "full-table height=" << block->get_height() << std::endl;
//             }
//             ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//             ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//         }
//         int64_t end_ms = base::xtime_utl::gettimeofday_ms();
//         std::cout << "non empty txs timer = " << end_ms - start_ms << std::endl;
//     }

//     {
//         int64_t start_ms = base::xtime_utl::gettimeofday_ms();
//         test_table_maker_base   table_maker(1000);
//         for (uint64_t height = 1000; height < 2000; height++) {
//             xblock_ptr_t block = table_maker.make_proposal(0, 2);
//             if (block != nullptr) {
//                 ASSERT_NE(block, nullptr);
//                 ASSERT_EQ(block->get_height(), height);
//                 xdatamock_tx::do_mock_signature(block.get());
//                 std::cout << "empty block = " << block->get_height() << std::endl;
//                 ASSERT_EQ(xsuccess, table_maker.verify_proposal(block));
//                 ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));
//             }
//         }
//         int64_t end_ms = base::xtime_utl::gettimeofday_ms();
//         std::cout << "non empty txs timer = " << end_ms - start_ms << std::endl;
//     }
// }



// TEST_F(test_table_maker, uint265_compare) {
//     std::string cccc = "111111111111111111111111111111111";
//     uint256_t cccc_digest = utl::xsha2_256_t::digest(cccc.data(), cccc.size());

//     for (uint32_t i = 0; i < 100000; i++) {
//         std::string aaaa = "111111111111111111111111111111111" + std::to_string(i);
//         uint256_t aaaa_digest = utl::xsha2_256_t::digest(aaaa.data(), aaaa.size());
//         std::string bbbb = "111111111111111111111111111111111" + std::to_string(i+1);
//         uint256_t bbbb_digest = utl::xsha2_256_t::digest(bbbb.data(), bbbb.size());
//         ASSERT_NE(aaaa, bbbb);
//         ASSERT_NE(aaaa_digest, bbbb_digest);

//         std::string cccc = aaaa;
//         uint256_t cccc_digest = aaaa_digest;
//         ASSERT_EQ(aaaa, cccc);
//         ASSERT_EQ(aaaa_digest, cccc_digest);
//     }
// }

// TEST_F(test_table_maker, test_111) {
//     std::string cccc = "111111111111111111111111111111111";
//     uint256_t cccc_digest = utl::xsha2_256_t::digest(cccc.data(), cccc.size());

//     for (uint32_t i = 0; i < 2; i++) {
//         std::string aaaa = "111111111111111111111111111111111" + std::to_string(i);
//         uint256_t aaaa_digest = utl::xsha2_256_t::digest(aaaa.data(), aaaa.size());
//         std::string bbbb = "111111111111111111111111111111111" + std::to_string(i+1);
//         uint256_t bbbb_digest = utl::xsha2_256_t::digest(bbbb.data(), bbbb.size());
//         ASSERT_NE(aaaa, bbbb);
//         ASSERT_NE(aaaa_digest, bbbb_digest);

//         std::string cccc = aaaa;
//         uint256_t cccc_digest = aaaa_digest;
//         ASSERT_EQ(aaaa, cccc);
//         ASSERT_EQ(aaaa_digest, cccc_digest);
//     }
//     std::cout << "hello" << std::endl;
// }

// TEST_F(test_table_maker, get_unit_block) {
//     test_table_maker_base   table_maker(1);
//     auto & unit_accounts = table_maker.get_unit_accounts();
//     std::string unit_account = unit_accounts[0].get_account();

//     for (uint64_t height = 1; height < 200; height++) {
//         xblock_ptr_t block = table_maker.make_proposal(1, 1);
//         ASSERT_NE(block, nullptr);
//         ASSERT_EQ(block->get_height(), height);
//         xdatamock_tx::do_mock_signature(block.get());
//         ASSERT_TRUE(table_maker.get_resources()->get_blockstore()->store_block(base::xvaccount_t(block->get_account()), block.get()));

//         base::xauto_ptr<base::xvblock_t> lightunit = xblocktool_t::get_latest_committed_lightunit(table_maker.get_resources()->get_blockstore(), unit_account);
//         xassert(lightunit->get_block_class() == base::enum_xvblock_class_light);
//         uint64_t prev_height = lightunit->get_height() > 0 ? lightunit->get_height() - 1 : 0;
//         base::xauto_ptr<base::xvblock_t> prev_lightunit = xblocktool_t::get_committed_lightunit(table_maker.get_resources()->get_blockstore(), unit_account, prev_height);
//         xassert(prev_lightunit->get_block_class() == base::enum_xvblock_class_light);
//         if (lightunit->get_height() > 0) {
//             xassert(lightunit->get_height() > prev_lightunit->get_height());
//         } else {
//             xassert(lightunit->get_height() == prev_lightunit->get_height());
//         }
//     }

//     {
//         base::xauto_ptr<base::xvblock_t> lightunit1 = table_maker.get_resources()->get_blockstore()->get_latest_cert_block(base::xvaccount_t(unit_account));
//         base::xauto_ptr<base::xvblock_t> lightunit2 = table_maker.get_resources()->get_blockstore()->get_latest_locked_block(base::xvaccount_t(unit_account));
//         base::xauto_ptr<base::xvblock_t> lightunit3 = table_maker.get_resources()->get_blockstore()->get_latest_committed_block(base::xvaccount_t(unit_account));
//         xassert(lightunit1->get_height() == lightunit2->get_height());
//         xassert(lightunit1->get_height() == lightunit3->get_height());
//         base::xauto_ptr<base::xvblock_t> lightunit = xblocktool_t::get_latest_committed_lightunit(table_maker.get_resources()->get_blockstore(), unit_account);
//         base::xauto_ptr<base::xvblock_t> prev_lightunit = xblocktool_t::get_committed_lightunit(table_maker.get_resources()->get_blockstore(), unit_account, lightunit->get_height() - 1);
//         std::cout << "lightunit prev height=" << prev_lightunit->get_height() << std::endl;
//         std::cout << "lightunit height=" << lightunit->get_height() << std::endl;
//         std::cout << "lightunit3 height=" << lightunit3->get_height() << std::endl;
//         xassert(lightunit->get_height() > prev_lightunit->get_height());
//     }
// }


