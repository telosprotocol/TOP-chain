#include "gtest/gtest.h"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xmock_auth.hpp"
#include "tests/mock/xvchain_creator.hpp"
#include "xblockstore/src/xuncommitted_subblock_cache.h"

#include <stdio.h>
#include <string>

using namespace top::data;
using namespace top;
using namespace top::base;
using namespace std;
using namespace top::utl;
using namespace top::mock;
using namespace top::store;

class test_uncommitted_subblock_cache : public testing::Test {
protected:
    void SetUp() override {
    }
    void TearDown() override {
    }
};

TEST_F(test_uncommitted_subblock_cache, store_and_load) {
    mock::xvchain_creator creator;
    base::xvblockstore_t * blockstore = creator.get_blockstore();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string sender = unit_addrs[0];
    std::string receiver = unit_addrs[1];

    uint32_t tx_num = 5;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs);
    xblock_ptr_t _tableblock1 = mocktable.generate_one_table();
    std::vector<xcons_transaction_ptr_t> send_txs2 = mocktable.create_send_txs(sender, receiver, tx_num);
    mocktable.push_txs(send_txs2);
    xblock_ptr_t _tableblock2 = mocktable.generate_one_table();

    xuncommitted_subblock_cache_t unconfirm_cache;

    std::vector<base::xvblock_ptr_t> lock_blocks;
    std::vector<base::xvblock_ptr_t> cert_blocks;
    lock_blocks.push_back(_tableblock1);
    cert_blocks.push_back(_tableblock2);

    unconfirm_cache.update_height(2);
    unconfirm_cache.add_blocks(cert_blocks, lock_blocks);

    ASSERT_EQ(unconfirm_cache.get_lock_cache().size(), 1);
    ASSERT_EQ(unconfirm_cache.get_cert_cache().size(), 1);

    std::vector<base::xvblock_ptr_t> sub_blocks1;
    _tableblock1->extract_sub_blocks(sub_blocks1);
    std::vector<base::xvblock_ptr_t> sub_blocks2;
    _tableblock2->extract_sub_blocks(sub_blocks2);

    xvaccount_t account_sender(sender);

    {
        auto block1 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_t(1));
        ASSERT_EQ(block1->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto block2 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_t(2));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());
    }

    {
        auto block1 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_viewid_t(1, _tableblock1->get_viewid()));
        ASSERT_EQ(block1->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto block2 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_viewid_t(2, _tableblock2->get_viewid()));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());    
    }

    {
        auto block1 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_hash_t(1, sub_blocks1[0]->get_block_hash()));
        ASSERT_EQ(block1->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto block2 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_hash_t(2, sub_blocks2[0]->get_block_hash()));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());    
    }

    {
        auto blocks1 = unconfirm_cache.load_blocks_object(account_sender, 1);
        ASSERT_EQ(blocks1.get_vector().size(), 1);
        ASSERT_EQ(blocks1.get_vector().at(0)->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto blocks2 = unconfirm_cache.load_blocks_object(account_sender, 2);
        ASSERT_EQ(blocks2.get_vector().size(), 1);
        ASSERT_EQ(blocks2.get_vector().at(0)->get_block_hash(), sub_blocks2[0]->get_block_hash());
    }

    {
        auto block1 = unconfirm_cache.load_block_index(account_sender, xblock_match_by_height_t(1));
        ASSERT_EQ(block1->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto block2 = unconfirm_cache.load_block_index(account_sender, xblock_match_by_height_t(2));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());
    }

    {
        auto block1 = unconfirm_cache.load_block_index(account_sender, xblock_match_by_height_viewid_t(1, _tableblock1->get_viewid()));
        ASSERT_EQ(block1->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto block2 = unconfirm_cache.load_block_index(account_sender, xblock_match_by_height_viewid_t(2, _tableblock2->get_viewid()));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());    
    }

    {
        auto block1 = unconfirm_cache.load_block_index(account_sender, xblock_match_by_height_hash_t(1, sub_blocks1[0]->get_block_hash()));
        ASSERT_EQ(block1->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto block2 = unconfirm_cache.load_block_index(account_sender, xblock_match_by_height_hash_t(2, sub_blocks2[0]->get_block_hash()));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());    
    }

    {
        auto blocks1 = unconfirm_cache.load_blocks_index(account_sender, 1);
        ASSERT_EQ(blocks1.get_vector().size(), 1);
        ASSERT_EQ(blocks1.get_vector().at(0)->get_block_hash(), sub_blocks1[0]->get_block_hash());
        auto blocks2 = unconfirm_cache.load_blocks_index(account_sender, 2);
        ASSERT_EQ(blocks2.get_vector().size(), 1);
        ASSERT_EQ(blocks2.get_vector().at(0)->get_block_hash(), sub_blocks2[0]->get_block_hash());
    }

    xblock_ptr_t _tableblock3 = mocktable.generate_one_table();

    lock_blocks.clear();
    cert_blocks.clear();
    cert_blocks.push_back(_tableblock3);

    unconfirm_cache.update_height(3);
    unconfirm_cache.add_blocks(cert_blocks, lock_blocks);
    {
        auto block1 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_t(1));
        ASSERT_EQ(block1, nullptr);
        auto block2 = unconfirm_cache.load_block_object(account_sender, xblock_match_by_height_t(2));
        ASSERT_EQ(block2->get_block_hash(), sub_blocks2[0]->get_block_hash());
    }
}
