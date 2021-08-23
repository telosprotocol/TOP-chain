#include "gtest/gtest.h"

#include "test_common.hpp"

#include "xblockmaker/xtable_maker.h"
#include "tests/mock/xvchain_creator.hpp"
#include "tests/mock/xdatamock_table.hpp"
#include "tests/mock/xdatamock_address.hpp"

using namespace top;
using namespace top::base;
using namespace top::mbus;
using namespace top::store;
using namespace top::data;
using namespace top::mock;
using namespace top::blockmaker;

class test_tablemaker : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_tablemaker, make_proposal_1) {
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    std::vector<xblock_ptr_t> all_gene_units = mocktable.get_all_genesis_units();
    for (auto & v : all_gene_units) {
        resources->get_blockstore()->store_block(base::xvaccount_t(v->get_account()), v.get());
    }

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        xtablemaker_para_t table_para2(mocktable.get_table_state());
        table_para2.set_origin_txs(send_txs);
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 2);

        xtablemaker_para_t table_para2(mocktable.get_table_state());
        table_para2.set_other_accounts(table_para.get_proposal()->get_other_accounts());
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 3);

        xtablemaker_para_t table_para2(mocktable.get_table_state());
        table_para2.set_other_accounts(table_para.get_proposal()->get_other_accounts());
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block == nullptr);        
    }    
}

TEST_F(test_tablemaker, receipt_id_check_1) {
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 2);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    std::vector<xblock_ptr_t> all_gene_units = mocktable.get_all_genesis_units();
    for (auto & v : all_gene_units) {
        resources->get_blockstore()->store_block(base::xvaccount_t(v->get_account()), v.get());
    }
    
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 2);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 3);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        auto tableblocks = mocktable.get_history_tables();
        std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(tableblocks[1]);
        xassert(recv_txs.size() == 2);
        std::cout << "recv_txs[0]->get_last_action_receipt_id() " << recv_txs[0]->get_last_action_receipt_id() << std::endl;
        std::cout << "recv_txs[0]->get_last_action_sender_confirmed_receipt_id() " << recv_txs[0]->get_last_action_sender_confirmed_receipt_id() << std::endl;
        std::cout << "recv_txs[1]->get_last_action_receipt_id() " << recv_txs[1]->get_last_action_receipt_id() << std::endl;
        std::cout << "recv_txs[1]->get_last_action_sender_confirmed_receipt_id() " << recv_txs[1]->get_last_action_sender_confirmed_receipt_id() << std::endl;    
        xassert(recv_txs[0]->get_last_action_receipt_id() == 1);
        xassert(recv_txs[1]->get_last_action_receipt_id() == 2);

        table_para.set_origin_txs(recv_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 4);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 5);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 6);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        auto tableblocks = mocktable.get_history_tables();
        std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(tableblocks[4]);
        xassert(confirm_txs.size() == 2);
        xassert(confirm_txs[0]->get_last_action_receipt_id() == 1);
        xassert(confirm_txs[1]->get_last_action_receipt_id() == 2);
        std::cout << "confirm_txs[0]->get_last_action_receipt_id() " << confirm_txs[0]->get_last_action_receipt_id() << std::endl;
        std::cout << "confirm_txs[0]->get_last_action_sender_confirmed_receipt_id() " << confirm_txs[0]->get_last_action_sender_confirmed_receipt_id() << std::endl;
        std::cout << "confirm_txs[1]->get_last_action_receipt_id() " << confirm_txs[1]->get_last_action_receipt_id() << std::endl;
        std::cout << "confirm_txs[1]->get_last_action_sender_confirmed_receipt_id() " << confirm_txs[1]->get_last_action_sender_confirmed_receipt_id() << std::endl;    

        table_para.set_origin_txs(confirm_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 7);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 8);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 9);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    {
        xassert(mocktable.get_table_state()->get_block_height() == 9);
        xtablemaker_para_t table_para(mocktable.get_table_state());
        std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 2);
        table_para.set_origin_txs(send_txs);

        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 10);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 11);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 12);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        auto tableblocks = mocktable.get_history_tables();
        std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(tableblocks[10]);
        xassert(recv_txs.size() == 2);
        std::cout << "recv_txs[0]->get_last_action_receipt_id() " << recv_txs[0]->get_last_action_receipt_id() << std::endl;
        std::cout << "recv_txs[0]->get_last_action_sender_confirmed_receipt_id() " << recv_txs[0]->get_last_action_sender_confirmed_receipt_id() << std::endl;
        std::cout << "recv_txs[1]->get_last_action_receipt_id() " << recv_txs[1]->get_last_action_receipt_id() << std::endl;
        std::cout << "recv_txs[1]->get_last_action_sender_confirmed_receipt_id() " << recv_txs[1]->get_last_action_sender_confirmed_receipt_id() << std::endl;
        xassert(recv_txs[0]->get_last_action_receipt_id() == 3);
        xassert(recv_txs[1]->get_last_action_receipt_id() == 4);
        xassert(recv_txs[0]->get_last_action_sender_confirmed_receipt_id() == 2);
        xassert(recv_txs[1]->get_last_action_sender_confirmed_receipt_id() == 2);

        table_para.set_origin_txs(recv_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 13);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 14);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 15);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    {
        xtablemaker_para_t table_para(mocktable.get_table_state());
        auto tableblocks = mocktable.get_history_tables();
        std::vector<xcons_transaction_ptr_t> confirm_txs = mocktable.create_receipts(tableblocks[13]);
        xassert(confirm_txs.size() == 2);
        std::cout << "confirm_txs[0]->get_last_action_receipt_id() " << confirm_txs[0]->get_last_action_receipt_id() << std::endl;
        std::cout << "confirm_txs[0]->get_last_action_sender_confirmed_receipt_id() " << confirm_txs[0]->get_last_action_sender_confirmed_receipt_id() << std::endl;
        std::cout << "confirm_txs[1]->get_last_action_receipt_id() " << confirm_txs[1]->get_last_action_receipt_id() << std::endl;
        std::cout << "confirm_txs[1]->get_last_action_sender_confirmed_receipt_id() " << confirm_txs[1]->get_last_action_sender_confirmed_receipt_id() << std::endl;    
        xassert(confirm_txs[0]->get_last_action_receipt_id() == 3);
        xassert(confirm_txs[1]->get_last_action_receipt_id() == 4);

        table_para.set_origin_txs(confirm_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 16);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    // {
    //     xtablemaker_para_t table_para(mocktable.get_table_state());
    //     xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

    //     xtablemaker_result_t table_result;
    //     xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
    //     xassert(proposal_block != nullptr);
    //     xassert(proposal_block->get_height() == 17);

    //     mocktable.do_multi_sign(proposal_block);
    //     mocktable.on_table_finish(proposal_block);
    //     resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    // }
    // {
    //     xtablemaker_para_t table_para(mocktable.get_table_state());
    //     xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

    //     xtablemaker_result_t table_result;
    //     xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
    //     xassert(proposal_block != nullptr);
    //     xassert(proposal_block->get_height() == 18);

    //     mocktable.do_multi_sign(proposal_block);
    //     mocktable.on_table_finish(proposal_block);
    //     resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    // }    
}
