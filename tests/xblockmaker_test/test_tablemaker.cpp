#include "gtest/gtest.h"

#include "test_common.hpp"
#include "xblockmaker/xtable_maker.h"
#include "xchain_fork/xchain_upgrade_center.h"
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
        chain_fork::xtop_chain_fork_config_center::init();
        base::xvblock_fork_t::instance().init(chain_fork::xtop_chain_fork_config_center::is_block_forked);
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        xtablemaker_para_t table_para2(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para2.set_origin_txs(send_txs);
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 2);

        xtablemaker_para_t table_para2(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para2.set_other_accounts(table_para.get_proposal()->get_other_accounts());
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 3);

        xtablemaker_para_t table_para2(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para2.set_other_accounts(table_para.get_proposal()->get_other_accounts());
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block == nullptr);        
    }    
}


TEST_F(test_tablemaker, make_proposal_block_build_hash_count) {
#ifdef ENABLE_METRICS
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 100);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    std::vector<xblock_ptr_t> all_gene_units = mocktable.get_all_genesis_units();
    for (auto & v : all_gene_units) {
        resources->get_blockstore()->store_block(base::xvaccount_t(v->get_account()), v.get());
    }

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 100);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        int64_t cur_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "before cur_hash_count " << cur_hash_count << std::endl;

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        int64_t last_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after last_hash_count, " << last_hash_count << " count " << (last_hash_count - cur_hash_count)<< std::endl;

    }
#endif
}


TEST_F(test_tablemaker, make_proposal_verify_build_hash_count) {
#ifdef ENABLE_METRICS
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 100);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();
    std::string from_addr = unit_addrs[0];
    std::string to_addr = unit_addrs[1];

    std::vector<xblock_ptr_t> all_gene_units = mocktable.get_all_genesis_units();
    for (auto & v : all_gene_units) {
        resources->get_blockstore()->store_block(base::xvaccount_t(v->get_account()), v.get());
    }

    int64_t cur_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
    std::cout << "before cur_hash_count " << cur_hash_count << std::endl;

    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, 4);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);
        
        int64_t table_make_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after table make_proposal table_make_hash_count, " << table_make_hash_count << " count " \
         << (table_make_hash_count - cur_hash_count)<< std::endl;
      
        xtablemaker_para_t table_para2(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para2.set_origin_txs(send_txs);
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        int64_t verify_make_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after table verify_proposal verify_make_hash_count, " << verify_make_hash_count << " count " \
        << (verify_make_hash_count - table_make_hash_count)<< std::endl;
          
       int64_t before_unpack_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
     
        int64_t last_unpack_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after last_unpack_count, " << last_unpack_count << " count " << (last_unpack_count - before_unpack_count)<< std::endl;


    }
#endif
}


TEST_F(test_tablemaker, make_unpack_units_hash_8_4_count) {
#ifdef ENABLE_METRICS
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 100);
    std::string table_addr = mocktable.get_account();
    std::vector<std::string> unit_addrs = mocktable.get_unit_accounts();

    std::vector<xblock_ptr_t> all_gene_units = mocktable.get_all_genesis_units();
    for (auto & v : all_gene_units) {
        resources->get_blockstore()->store_block(base::xvaccount_t(v->get_account()), v.get());
    }

    int64_t cur_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
    std::cout << "before cur_hash_count " << cur_hash_count << std::endl;

    int account_count = 4;
    int transaction_count = 100;
    std::vector<xcons_transaction_ptr_t> all_txs;
    std::string to_addr = unit_addrs[0];
    for (int i = 1; i < account_count; i++)
    {
       std::string from_addr = unit_addrs[i];
       std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, transaction_count);
       all_txs.insert(all_txs.end(),send_txs.begin(),send_txs.end());
    }
    std::cout << "account count "<< account_count << ", and every send count " << transaction_count << std::endl;
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(all_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);
        
        int64_t table_make_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after table make_proposal table_make_hash_count, " << table_make_hash_count << " count " \
         << (table_make_hash_count - cur_hash_count)<< std::endl;
      
        xtablemaker_para_t table_para2(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para2.set_origin_txs(all_txs);
        int32_t ret = tablemaker->verify_proposal(proposal_block.get(), table_para2, proposal_para);
        xassert(ret == 0);

        int64_t verify_make_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after table verify_proposal verify_make_hash_count, " << verify_make_hash_count << " count " \
        << (verify_make_hash_count - table_make_hash_count)<< std::endl;
          
        int64_t before_unpack_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
     
        int64_t last_unpack_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after last_unpack_count, " << last_unpack_count << " count " << (last_unpack_count - before_unpack_count)<< std::endl;
    }
#endif
}


TEST_F(test_tablemaker, make_receipt_hash_count) {
#ifdef ENABLE_METRICS
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 100);
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
         int account_count = 64;
        int transaction_count = 4;
        std::vector<xcons_transaction_ptr_t> all_txs;
        std::string to_addr = unit_addrs[0];
        for (int i = 1; i <= account_count; i++)
        {
            std::string from_addr = unit_addrs[i];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, transaction_count);
            all_txs.insert(all_txs.end(),send_txs.begin(),send_txs.end());
        }

         std::cout << "account count "<< account_count << ", and every send count " << transaction_count << std::endl;
          xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(all_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    for(int block_index = 2 ; block_index < 4; block_index++){

            xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
            xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

            xtablemaker_result_t table_result;
            xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
            xassert(proposal_block != nullptr);
           // xassert(proposal_block->get_height() == block_index);

            mocktable.do_multi_sign(proposal_block);
            mocktable.on_table_finish(proposal_block);
            resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        auto tableblocks = mocktable.get_history_tables();

        int64_t before_receipt_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "before_receipt_hash_count " << before_receipt_hash_count << std::endl;
        xinfo("before_receipt_hash_count %d ",before_receipt_hash_count);
        std::vector<xcons_transaction_ptr_t> recv_txs = mocktable.create_receipts(tableblocks[1]);
        int64_t last_receipt_hash_count =  XMETRICS_GAUGE_GET_VALUE(metrics::cpu_hash_256_calc);
        std::cout << "after last_receipt_hash_count  " << last_receipt_hash_count << ", count "\
         << (last_receipt_hash_count - before_receipt_hash_count) << std::endl;
 
      //  xassert(recv_txs.size() == 2);
        std::cout << "recv_txs[0]->get_last_action_receipt_id() " << recv_txs[0]->get_last_action_receipt_id() << std::endl;
        std::cout << "recv_txs[0]->get_last_action_sender_confirmed_receipt_id() " << recv_txs[0]->get_last_action_sender_confirmed_receipt_id() << std::endl;
        std::cout << "recv_txs[1]->get_last_action_receipt_id() " << recv_txs[1]->get_last_action_receipt_id() << std::endl;
        std::cout << "recv_txs[1]->get_last_action_sender_confirmed_receipt_id() " << recv_txs[1]->get_last_action_sender_confirmed_receipt_id() << std::endl;    
       // xassert(recv_txs[0]->get_last_action_receipt_id() == 1);
       // xassert(recv_txs[1]->get_last_action_receipt_id() == 2);

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
#endif
}


TEST_F(test_tablemaker, make_receipt_hash_new_count) {
#ifdef ENABLE_METRICS  
    xblockmaker_resources_ptr_t resources = std::make_shared<test_xblockmaker_resources_t>();

    mock::xdatamock_table mocktable(1, 100);
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
         int account_count = 64;
        int transaction_count = 4;
        std::vector<xcons_transaction_ptr_t> all_txs;
        std::string to_addr = unit_addrs[0];
        for (int i = 1; i <= account_count; i++)
        {
            std::string from_addr = unit_addrs[i];
            std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, transaction_count);
            all_txs.insert(all_txs.end(),send_txs.begin(),send_txs.end());
        }

         std::cout << "account count "<< account_count << ", and every send count " << transaction_count << std::endl;
          xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(all_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    for(int block_index = 2 ; block_index < 4; block_index++){

            xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
            xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

            xtablemaker_result_t table_result;
            xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
            xassert(proposal_block != nullptr);
         
            mocktable.do_multi_sign(proposal_block);
            mocktable.on_table_finish(proposal_block);
            resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
#endif
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
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
        xassert(proposal_block->get_height() == 17);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 18);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }
    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para();

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 19);

        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());
    }

    {
    // full-table 16 height
    auto tableblocks = mocktable.get_history_tables();
    auto tablestate = resources->get_xblkstatestore()->get_block_state(tableblocks[16].get());
    xvproperty_prove_ptr_t propreceipt = xblocktool_t::create_receiptid_property_prove(tableblocks[16].get(), tableblocks[18].get(), tablestate.get());
    xassert(propreceipt != nullptr);
    xassert(propreceipt->is_valid());

    std::string propreceipt_bin;
    propreceipt->serialize_to_string(propreceipt_bin);
    std::cout << "propreceipt_bin size=" << propreceipt_bin.size() << std::endl;

    base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)propreceipt_bin.data(), (uint32_t)propreceipt_bin.size());
    xdataunit_t* _dataunit = xdataunit_t::read_from(_stream);
    xassert(_dataunit != nullptr);
    xvproperty_prove_t* _propreceipt = dynamic_cast<xvproperty_prove_t*>(_dataunit);
    xassert(_propreceipt != nullptr);
    xvproperty_prove_ptr_t propreceipt2;
    propreceipt2.attach(_propreceipt);
    xassert(propreceipt2->is_valid());

    base::xreceiptid_state_ptr_t receiptid_state = xblocktool_t::get_receiptid_from_property_prove(propreceipt2);
    xassert(receiptid_state->get_self_tableid() == mocktable.get_short_table_id());
    xassert(receiptid_state->get_block_height() == 16);
    auto all_pairs = receiptid_state->get_all_receiptid_pairs();
    std::cout << "all_pairs=" << all_pairs->dump() << std::endl;
    }

}

TEST_F(test_tablemaker, version_1) {
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

    const uint32_t tx_cnt = 2;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, tx_cnt);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para(100);

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        auto txs = proposal_block->get_txs();
        EXPECT_EQ(txs.size(), tx_cnt);
        EXPECT_EQ(proposal_block->get_block_version(), base::enum_xvblock_fork_version_table_prop_prove);

        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);
        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());

        {
            xJson::Value jv1;
            proposal_block->parse_to_json(jv1, RPC_VERSION_V1);
            auto j_txs = jv1["tableblock"]["units"][from_addr]["lightunit_input"];
            auto tx_hashes = j_txs.getMemberNames();
            for(auto hash : tx_hashes) {
                auto sender = j_txs[hash]["sender"].asString();
                EXPECT_EQ(sender, from_addr);
                auto receiver = j_txs[hash]["receiver"].asString();
                EXPECT_EQ(receiver, to_addr);
                auto tx_consensus_phase = j_txs[hash]["tx_consensus_phase"].asString();
                EXPECT_EQ(tx_consensus_phase, "send");
            }
            auto unit_height = jv1["tableblock"]["units"][from_addr]["unit_height"].asUInt64();
            EXPECT_EQ(unit_height, 1);
        } 
        
        {
            xJson::Value jv2;
            proposal_block->parse_to_json(jv2, RPC_VERSION_V2);
            auto j_txs = jv2["tableblock"]["txs"];
            for(auto tx : j_txs) {
                auto tx_consensus_phase = tx["tx_consensus_phase"].asString();
                EXPECT_EQ(tx_consensus_phase, "send");
            }
            auto units = jv2["tableblock"]["units"];
            for (auto & unit : units) {
                auto unit_height = unit["unit_height"].asUInt64();
                EXPECT_EQ(unit_height, 1);
                auto account = unit["account"].asString();
                EXPECT_EQ(account, from_addr);
            }
        }

        std::vector<xobject_ptr_t<base::xvblock_t>> units;
        proposal_block->extract_sub_blocks(units);        
        EXPECT_EQ(units.size(), 1);
        for (auto & v : units) {
            xobject_ptr_t<data::xblock_t> unit = dynamic_xobject_ptr_cast<data::xblock_t>(v);
            auto txs = unit->get_txs();
            EXPECT_EQ(txs.size(), tx_cnt);

            {
                xJson::Value jv1;
                unit->parse_to_json(jv1, RPC_VERSION_V1);
                auto j_txs = jv1["lightunit"]["lightunit_input"]["txs"];
                for(auto tx : j_txs) {
                    auto hashes = tx.getMemberNames();
                    for (auto & hash : hashes) {
                        auto tx_consensus_phase = tx[hash]["tx_consensus_phase"].asString();
                        EXPECT_EQ(tx_consensus_phase, "send");
                        auto tx_exec_status = tx[hash]["tx_exec_status"].asString();
                        EXPECT_EQ(tx_exec_status, "success");
                    }
                }
            }

            {
                xJson::Value jv2;
                unit->parse_to_json(jv2, RPC_VERSION_V2);
                auto txs = jv2["lightunit"]["lightunit_input"];
                for (auto & tx : txs) {
                    auto tx_consensus_phase = tx["tx_consensus_phase"].asString();
                    EXPECT_EQ(tx_consensus_phase, "send");
                }
            }
         }

        auto headers = proposal_block->get_sub_block_headers();
        EXPECT_EQ(headers.size(), 1);
        for (auto & header : headers) {
            EXPECT_EQ(header->get_extra_data().empty(), true);
            EXPECT_EQ(header->get_block_version(), base::enum_xvblock_fork_version_table_prop_prove);
        }
    }
}

TEST_F(test_tablemaker, version_2) {
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

    const uint32_t tx_cnt = 2;
    std::vector<xcons_transaction_ptr_t> send_txs = mocktable.create_send_txs(from_addr, to_addr, tx_cnt);
    EXPECT_EQ(send_txs.size(), tx_cnt);
    xtable_maker_ptr_t tablemaker = make_object_ptr<xtable_maker_t>(table_addr, resources);

    {
        xtablemaker_para_t table_para(mocktable.get_table_state(), mocktable.get_commit_table_state());
        table_para.set_origin_txs(send_txs);
        xblock_consensus_para_t proposal_para = mocktable.init_consensus_para(10000000);

        xtablemaker_result_t table_result;
        xblock_ptr_t proposal_block = tablemaker->make_proposal(table_para, proposal_para, table_result);
        auto txs = proposal_block->get_txs();
        EXPECT_EQ(txs.size(), tx_cnt);
        EXPECT_EQ(proposal_block->get_block_version(), base::enum_xvblock_fork_version_unit_opt);

        xassert(proposal_block != nullptr);
        xassert(proposal_block->get_height() == 1);
        mocktable.do_multi_sign(proposal_block);
        mocktable.on_table_finish(proposal_block);
        resources->get_blockstore()->store_block(mocktable, proposal_block.get());

        {
            xJson::Value jv1;
            proposal_block->parse_to_json(jv1, RPC_VERSION_V1);
            auto j_txs = jv1["tableblock"]["units"][from_addr]["lightunit_input"];
            auto tx_hashes = j_txs.getMemberNames();
            EXPECT_EQ(tx_hashes.empty(), true);
            auto unit_height = jv1["tableblock"]["units"][from_addr]["unit_height"].asUInt64();
            EXPECT_EQ(unit_height, 1);
        }
        
        {
            xJson::Value jv2;
            proposal_block->parse_to_json(jv2, RPC_VERSION_V2);
            auto j_txs = jv2["tableblock"]["txs"];
            for(auto tx : j_txs) {
                auto tx_consensus_phase = tx["tx_consensus_phase"].asString();
                EXPECT_EQ(tx_consensus_phase, "send");
            }
            auto units = jv2["tableblock"]["units"];
            for (auto & unit : units) {
                auto unit_height = unit["unit_height"].asUInt64();
                EXPECT_EQ(unit_height, 1);
                auto account = unit["account"].asString();
                EXPECT_EQ(account, from_addr);
            }
        }

        std::vector<xobject_ptr_t<base::xvblock_t>> units;
        proposal_block->extract_sub_blocks(units);
        EXPECT_EQ(units.size(), 1);
        for (auto & v : units) {
            xobject_ptr_t<data::xblock_t> unit = dynamic_xobject_ptr_cast<data::xblock_t>(v);
            auto txs = unit->get_txs();
            EXPECT_EQ(txs.size(), 0);
            
            {
                xJson::Value jv1;
                unit->parse_to_json(jv1, RPC_VERSION_V1);
                auto j_txs = jv1["lightunit"]["lightunit_input"]["txs"];
                for(auto tx : j_txs) {
                    auto hashes = tx.getMemberNames();
                    for (auto & hash : hashes) {
                        auto tx_consensus_phase = tx[hash]["tx_consensus_phase"].asString();
                        EXPECT_EQ(tx_consensus_phase, "send");
                    }
                }
            }

            {
                xJson::Value jv2;
                unit->parse_to_json(jv2, RPC_VERSION_V2);
                auto txs = jv2["lightunit"]["lightunit_input"];
                for (auto & tx : txs) {
                    auto tx_consensus_phase = tx["tx_consensus_phase"].asString();
                    EXPECT_EQ(tx_consensus_phase, "send");
                }
            }
        }

        auto headers = proposal_block->get_sub_block_headers();
        EXPECT_EQ(headers.size(), 1);
        for (auto & header : headers) {
            EXPECT_EQ(header->get_extra_data().empty(), false);
            EXPECT_EQ(header->get_block_version(), base::enum_xvblock_fork_version_unit_opt);
        }
    }
}

TEST_F(test_tablemaker, fullunit) {
    uint64_t count = 25;
    mock::xdatamock_table mocktable;
    mocktable.genrate_table_chain(count);

    auto & tables = mocktable.get_history_tables();
    auto & fullunit_table = tables[23];
    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
    fullunit_table->extract_sub_blocks(sub_blocks);
    for(auto & unit : sub_blocks) {
        EXPECT_EQ(unit->get_block_class(), enum_xvblock_class_full);
        auto fullunit = dynamic_cast<xfullunit_block_t*>(unit.get());
        if (fullunit != nullptr) {
            xJson::Value jv;
            fullunit->parse_to_json(jv, RPC_VERSION_V2);
            auto txs = jv["fullunit"]["txs"];
            for (auto tx : txs) {
                auto tx_hash = tx["tx_hash"].asString();
                std::cout << tx_hash << std::endl;
                EXPECT_EQ(tx_hash.empty(), false);
            }
        }
    }
}
