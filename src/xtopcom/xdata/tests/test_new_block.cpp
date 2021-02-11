#include <map>
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xfullunit.h"
#include "xdata/xemptyblock.h"
#include "xdata/xblockchain.h"
#include "xdata/xtableblock.h"
#include "xdata/xblocktool.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xbase/xhash.h"

using namespace top;
using namespace top::data;

class test_new_block : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xtransaction_ptr_t create_transfer_tx(const std::string & from, const std::string & to) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(100);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from, to);
    tx->set_digest();
    tx->set_len();
    tx->set_tx_subtype(enum_transaction_subtype_send);
    return tx;
}

xcons_transaction_ptr_t create_cons_transfer_tx(const std::string & from, const std::string & to, uint64_t amount = 100) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from, to);
    tx->set_digest();
    tx->set_tx_subtype(enum_transaction_subtype_send);

    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    return cons_tx;
}

xblock_t* create_transfer_lightunit(base::xvblock_t* prev_block, const std::string & to) {
    // step1: get tx from txpool
    xtransaction_ptr_t tx = create_transfer_tx(prev_block->get_account(), to);
    // step2: exec tx and get tx result
    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(100);
    // step3: use lightunit unit block para to create lightunit unit
    return (xblock_t*)xlightunit_block_t::create_next_lightunit(para, prev_block);;
}

TEST_F(test_new_block, emptyblock_1) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");;
    base::xauto_ptr<base::xvblock_t> ptr_block(xblocktool_t::create_genesis_empty_unit(account));
    xassert(ptr_block->get_refcount() == 1);
    ASSERT_EQ(ptr_block->get_height(), 0);
    ASSERT_EQ(ptr_block->get_account(), account);
    ASSERT_FALSE(ptr_block->get_block_hash().empty());
}

TEST_F(test_new_block, emptyblock_2) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");;
    base::xauto_ptr<base::xvblock_t> genesis_block(xblocktool_t::create_genesis_empty_unit(account));
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    genesis_block->serialize_to(stream);
    size_t block_size = stream.size();
    std::cout << "block size " << block_size << std::endl;
    xblock_t* block2 = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(block2, nullptr);
    block2->release_ref();
}

TEST_F(test_new_block, emptyblock_3) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");;
    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    genesis_block->serialize_to(stream);
    xblock_t* block2 = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(block2, nullptr);
}

TEST_F(test_new_block, emptyblock_4) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");;
    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    base::xvblock_t* proposal_block = xblocktool_t::create_next_emptyblock(genesis_block);

    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    proposal_block->serialize_to(stream);
    size_t block_size = stream.size();
    std::cout << "block size " << block_size << std::endl;
    xblock_t* block2 = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(block2, nullptr);
}

TEST_F(test_new_block, cons_transaction_1) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, account2);

    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    account1_tx1->serialize_to(stream);

    xcons_transaction_ptr_t account1_tx2 = make_object_ptr<xcons_transaction_t>();
    account1_tx2->serialize_from(stream);
}

TEST_F(test_new_block, lightunit_serialize) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    // create a transaction
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(100);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(account, account2);
    tx->set_digest();

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-100);

    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    base::xvblock_t* proposal_block = xlightunit_block_t::create_next_lightunit(para, genesis_block);

    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    proposal_block->serialize_to(stream);
    size_t block_size = stream.size();
    std::cout << "block size " << block_size << std::endl;

    xblock_t* block2 = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(block2, nullptr);
}

TEST_F(test_new_block, lightunit_1) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    // create a transaction
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(100);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(account, to_account);
    tx->set_digest();
    tx->set_tx_subtype(enum_transaction_subtype_send);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-100);

    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    xassert(genesis_block->get_refcount() == 1);
    base::xvblock_t* proposal_block = xlightunit_block_t::create_next_lightunit(para, genesis_block);
    xlightunit_block_t* proposal_lightunit = (xlightunit_block_t*)proposal_block;
    ASSERT_EQ(para.get_balance_change(), proposal_lightunit->get_balance_change());
    ASSERT_EQ(1, proposal_lightunit->get_height());
    ASSERT_FALSE(proposal_block->is_deliver());

    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    proposal_block->serialize_to(stream);
    size_t block_size = stream.size();
    std::cout << "block size " << block_size << std::endl;

    xblock_t* block2 = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(block2, nullptr);
}

TEST_F(test_new_block, lightunit_2) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    xblock_t* lightunit1 = (xblock_t*)xblocktool_t::create_next_lightunit(para1, account1_genesis_block);
    ASSERT_EQ(lightunit1->get_txs().size(), 1);
    ASSERT_EQ(lightunit1->get_txs_count(), 1);

    std::string block_object_bin;
    lightunit1->serialize_to_string(block_object_bin);
    base::xvblock_t* new_block = base::xvblockstore_t::create_block_object(block_object_bin);
    xlightunit_block_t* lightunit1_recv = (xlightunit_block_t*)new_block;
    xassert(lightunit1_recv != NULL);
    lightunit1_recv->set_input_resources(lightunit1->get_input()->get_resources_data());
    lightunit1_recv->set_output_resources(lightunit1->get_output()->get_resources_data());
    const std::vector<xlightunit_tx_info_ptr_t> & txs = lightunit1_recv->get_txs();
    ASSERT_EQ(txs[0]->get_raw_tx()->digest(), account1_tx1->get_transaction()->digest());
}

TEST_F(test_new_block, unit_unconfirm_send_tx) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    xlightunit_block_t* proposal_lightunit_1;
    xlightunit_block_t* proposal_lightunit_2;

    {
    // create a transaction
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(100);
    tx->make_tx_transfer(asset);
    tx->set_same_source_target_address(account);
    tx->set_digest();

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-100);

    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    xassert(genesis_block->get_refcount() == 1);
    xlightunit_block_t* lightunit = (xlightunit_block_t*)xlightunit_block_t::create_next_lightunit(para, genesis_block);
    ASSERT_EQ(lightunit->get_unconfirm_sendtx_num(), 0);
    ASSERT_EQ(lightunit->is_prev_sendtx_confirmed(), true);
    }

    {
    // create a transaction
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(100);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(account, to_account);
    tx->set_digest();

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-100);

    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    xassert(genesis_block->get_refcount() == 1);
    proposal_lightunit_1 = (xlightunit_block_t*)test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
    ASSERT_EQ(proposal_lightunit_1->get_unconfirm_sendtx_num(), 1);
    ASSERT_EQ(proposal_lightunit_1->is_prev_sendtx_confirmed(), true);
    }

    {
    // create a transaction
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(200);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(account, to_account);
    tx->set_digest();

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-200);
    para.set_account_unconfirm_sendtx_num(proposal_lightunit_1->get_unconfirm_sendtx_num());

    ASSERT_NE(proposal_lightunit_1->get_block_hash(), std::string());
    proposal_lightunit_2 = (xlightunit_block_t*)xlightunit_block_t::create_next_lightunit(para, proposal_lightunit_1);
    ASSERT_EQ(proposal_lightunit_2->get_unconfirm_sendtx_num(), 2);
    ASSERT_EQ(proposal_lightunit_2->is_prev_sendtx_confirmed(), false);
    }
}


TEST_F(test_new_block, check_block_hash_calc_1) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);

    std::cout << "=======" << std::endl;
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    genesis_block->serialize_to(stream);

    xblock_t* read_block = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(read_block, nullptr);
}

TEST_F(test_new_block, check_block_hash_calc_2) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    std::cout << "=======" << std::endl;
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    genesis_block->serialize_to(stream);

    xblock_t* read_block = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(read_block, nullptr);
}

TEST_F(test_new_block, check_block_hash_calc_3) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    std::cout << "=======" << std::endl;
    xblock_t* lightunit1 = create_transfer_lightunit(genesis_block, to_account);
    std::cout << "=======" << std::endl;
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    lightunit1->serialize_to(stream);
    xblock_t* read_block = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(read_block, nullptr);
}

TEST_F(test_new_block, check_block_hash_calc_4) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(account);
    xblock_t* lightunit1 = create_transfer_lightunit(genesis_block, to_account);
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    lightunit1->serialize_to(stream);
    xblock_t* read_block = (xblock_t*)base::xdataobj_t::read_from(stream);
    ASSERT_NE(read_block, nullptr);
}

TEST_F(test_new_block, get_block_hash_1) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    base::xvblock_t* next_block = test_blocktuil::create_next_emptyblock(genesis_block);
    ASSERT_FALSE(genesis_block->get_block_hash().empty());
    ASSERT_FALSE(next_block->get_block_hash().empty());
}

TEST_F(test_new_block, get_block_hash_2) {
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("11111111111111111112");
    {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        uint64_t amount = 100;
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_different_source_target_address(address, to_account);
        tx->set_digest();

        xlightunit_block_para_t para;
        para.set_one_input_tx(tx);
        para.set_balance_change(-amount);

        base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
        ASSERT_FALSE(proposal_block->get_block_hash().empty());
    }
}


TEST_F(test_new_block, create_units_and_tableblock) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);
    ASSERT_TRUE(lightunit1->get_block_hash().empty());

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = xblocktool_t::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account);
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, to_account);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    xblock_t* lightunit2 = (xblock_t*)test_blocktuil::create_next_lightunit(para2, account2_genesis_block);
    ASSERT_TRUE(lightunit2->get_block_hash().empty());

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    xblock_t* taccount1_proposal_block = (xblock_t*)test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());

    // then make units from tableblock
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    auto & units = tableblock->get_tableblock_units(true);
    for (auto & cache_unit : units) {
        ASSERT_TRUE(cache_unit->check_block_flag(base::enum_xvblock_flag_committed));
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }
}

TEST_F(test_new_block, merge_multi_unit_to_tableblock_1) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = xblocktool_t::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account);
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, to_account);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    xblock_t* lightunit2 = (xblock_t*)test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    xblock_t* taccount1_proposal_block = (xblock_t*)test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    ASSERT_NE(taccount1_proposal_block, nullptr);
    ASSERT_EQ(taccount1_proposal_block->get_txs_count(), 4);
    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());

    {
        bool ret = taccount1_proposal_block->check_block_hash();
        ASSERT_TRUE(ret);
        base::xautostream_t<4096> stream(base::xcontext_t::instance());
        taccount1_proposal_block->serialize_to(stream);
        xblock_t* read_block = (xblock_t*)base::xdataobj_t::read_from(stream);
        ASSERT_NE(read_block, nullptr);
        read_block->set_input_resources(taccount1_proposal_block->get_input()->get_resources_data());
        read_block->set_output_resources(taccount1_proposal_block->get_output()->get_resources_data());
        ret = read_block->check_block_hash();
        ASSERT_TRUE(ret);
    }

    // xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    // auto & cache_units = tableblock->get_consensused_units();
    // for (auto & cache_unit : cache_units) {
    //     bool ret = cache_unit->check_block_hash();
    //     ASSERT_TRUE(ret);
    //     base::xautostream_t<4096> stream(base::xcontext_t::instance());
    //     cache_unit->serialize_to(stream);
    //     xblock_t* read_block = (xblock_t*)base::xdataobj_t::read_from(stream);
    //     ASSERT_NE(read_block, nullptr);
    //     ret = read_block->check_block_hash();
    //     ASSERT_TRUE(ret);
    //     read_block->set_input(cache_unit->get_input());
    //     read_block->set_output(cache_unit->get_output());
    //     ret = read_block->check_block_hash();
    //     ASSERT_TRUE(ret);
    // }
}


TEST_F(test_new_block, merge_multi_unit_to_tableblock_2) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account, 100);
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account, 200);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = xblocktool_t::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account, 100);
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, to_account, 200);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    xblock_t* lightunit2 = (xblock_t*)test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    xblock_t* taccount1_proposal_block = (xblock_t*)test_blocktuil::create_next_tableblock_with_next_two_emptyblock(table_para, taccount1_genesis_block);
    ASSERT_NE(taccount1_proposal_block, nullptr);

    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    std::vector<xcons_transaction_ptr_t> sendtx_receipts;
    std::vector<xcons_transaction_ptr_t> recvtx_receipts;
    tableblock->create_txreceipts(sendtx_receipts, recvtx_receipts);
    ASSERT_EQ(sendtx_receipts.size(), 4);
    ASSERT_EQ(recvtx_receipts.size(), 0);
    for (auto & v : sendtx_receipts) {
        //ASSERT_TRUE(v->verify_cons_transaction());
        ASSERT_TRUE(v->is_recv_tx());
    }
}

TEST_F(test_new_block, tableblock_get_units_1) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account, 100);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    xtable_block_t* tableblock1 = (xtable_block_t*)test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    ASSERT_NE(tableblock1, nullptr);

    auto & units_ptr = tableblock1->get_tableblock_units(true);
    auto unit1 = units_ptr[0];

    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    xassert(tableblock1->full_block_serialize_to(stream) > 0);
    std::string message = std::string((const char*)stream.data(), stream.size());

    // backup verify table-block
    base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());
    xtable_block_t* tableblock2 = dynamic_cast<xtable_block_t*>(xblock_t::full_block_read_from(stream2));
    xassert(tableblock2->get_block_flags() == tableblock1->get_block_flags());
    tableblock2->set_block_flag(base::enum_xvblock_flag_executed);

    ASSERT_NE(tableblock1->get_block_flags(), tableblock2->get_block_flags());

    std::string tableblock1_cert_bin;
    std::string tableblock2_cert_bin;
    tableblock1->get_cert()->serialize_to_string(tableblock1_cert_bin);
    tableblock2->get_cert()->serialize_to_string(tableblock2_cert_bin);
    ASSERT_EQ(tableblock1_cert_bin, tableblock2_cert_bin);  // block flags should not influence qcert serialize string

    auto units_ptr2 = tableblock2->get_tableblock_units(true);
    auto unit2 = units_ptr2[0];

    if (unit1->get_block_hash() != unit2->get_block_hash()) {
        std::cout << "tableblock1:" << tableblock1->dump_cert() << std::endl;
        std::cout << "tableblock2:" << tableblock2->dump_cert() << std::endl;
        std::cout << "unit1:" << unit1->dump_cert() << std::endl;
        std::cout << "unit2:" << unit2->dump_cert() << std::endl;
        ASSERT_EQ(unit1->get_block_hash(), unit2->get_block_hash());
    }
}

TEST_F(test_new_block, backup_verify_and_create_tableblock) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account, 100);
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account, 200);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = xblocktool_t::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account, 100);
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, to_account, 200);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    xblock_t* lightunit2 = (xblock_t*)test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    auto leader_tableblock = xtable_block_t::create_next_tableblock(table_para, taccount1_genesis_block);
    ASSERT_NE(leader_tableblock, nullptr);

    // block send to backup
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    leader_tableblock->serialize_to(stream);
    stream << leader_tableblock->get_input()->get_resources_data();
    std::string message = std::string((const char*)stream.data(), stream.size());

    // backup verify table-block
    base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());
    xblock_t* backup_block = dynamic_cast<xblock_t*>(base::xdataobj_t::read_from(stream2));
    ASSERT_NE(backup_block, nullptr);
    std::string input_str;
    stream2 >> input_str;
    backup_block->set_input_resources(input_str);
    ASSERT_TRUE(backup_block->check_block_hash());

    xtable_block_t* backup_tableblock = dynamic_cast<xtable_block_t*>(backup_block);
    backup_tableblock->set_output_resources(leader_tableblock->get_output()->get_resources_data());
}

TEST_F(test_new_block, sync_tableblock_rebuild_units) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account, 100);
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account, 200);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = xblocktool_t::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account, 100);
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, to_account, 200);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    xblock_t* lightunit2 = (xblock_t*)test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    xblock_t* leader_tableblock = (xblock_t*)test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    ASSERT_NE(leader_tableblock, nullptr);

    // block send to backup
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    leader_tableblock->full_block_serialize_to(stream);
    std::string message = std::string((const char*)stream.data(), stream.size());

    // backup verify table-block
    base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());
    xblock_t* backup_block = (xblock_t*)xblock_t::full_block_read_from(stream2);
    ASSERT_NE(backup_block, nullptr);
    ASSERT_TRUE(backup_block->check_block_hash());

    xtable_block_t* backup_tableblock = dynamic_cast<xtable_block_t*>(backup_block);
    auto & cache_units = backup_tableblock->get_tableblock_units(true);
    for (auto & cache_unit : cache_units) {
        bool ret = cache_unit->check_block_hash();
        ASSERT_TRUE(ret);
    }
}

TEST_F(test_new_block, create_block_and_do_consensus_1) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    auto next_block = test_blocktuil::create_next_emptyblock(genesis_block);
    ASSERT_FALSE(next_block->get_block_hash().empty());
}
TEST_F(test_new_block, create_block_and_do_consensus_2) {
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    base::xvblock_t* lightunit1 = test_blocktuil::create_next_lightunit_with_consensus(para1, account1_genesis_block);
    ASSERT_FALSE(lightunit1->get_block_hash().empty());
}
TEST_F(test_new_block, create_block_and_do_consensus_3) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    base::xvblock_t* lightunit1 = test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = test_blocktuil::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    base::xvblock_t* lightunit2 = test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    base::xvblock_t* taccount1_proposal_block = test_blocktuil::create_next_tableblock_with_next_two_emptyblock(table_para, taccount1_genesis_block);

    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());
    // then make units from tableblock
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    auto & units_ptr = tableblock->get_tableblock_units(true);
    xassert(units_ptr.size() != 0);
    for (auto & cache_unit : units_ptr) {
        const int      this_block_flags  = cache_unit->get_block_flags();
        printf("this_block_flags 0x%x", this_block_flags);
        ASSERT_EQ(this_block_flags & base::enum_xvblock_flag_stored, 0);
        ASSERT_FALSE(cache_unit->check_block_flag(base::enum_xvblock_flag_stored));
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }


    // block send to backup
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    tableblock->full_block_serialize_to(stream);
    std::string message = std::string((const char*)stream.data(), stream.size());

    // backup verify table-block
    base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());
    xblock_t* backup_block = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream2));
    ASSERT_NE(backup_block, nullptr);
    ASSERT_FALSE(backup_block->get_block_hash().empty());
    // then make units from tableblock
    xtable_block_t* tableblock2 = dynamic_cast<xtable_block_t*>(backup_block);
    auto & units_ptr2 = tableblock2->get_tableblock_units(true);
    xassert(!units_ptr2.empty());
    for (auto & cache_unit : units_ptr2) {
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }

}

TEST_F(test_new_block, create_block_and_do_consensus_4) {
    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("111111111111111111112");
    base::xvblock_t* account1_genesis_block = xblocktool_t::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    base::xvblock_t* lightunit1 = xblocktool_t::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = xblocktool_t::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    base::xvblock_t* lightunit2 = xblocktool_t::create_next_lightunit(para2, account2_genesis_block);

    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    // create table-block
    std::string taccount1 = xblocktool_t::make_address_shard_table_account(100);
    base::xvblock_t* taccount1_genesis_block = xblocktool_t::create_genesis_empty_table(taccount1);
    auto table_header = xblockheader_t::create_next_blockheader(taccount1_genesis_block, base::enum_xvblock_class_light);
    xblockcert_t* table_cert = new xblockcert_t(taccount1, taccount1_genesis_block->get_height() + 1);

    for (auto & unit : table_para.get_account_units()) {
        unit->get_cert()->set_viewtoken(table_cert->get_viewtoken());
    }




    base::xvblock_t* taccount1_proposal_block = test_blocktuil::create_next_tableblock_with_next_two_emptyblock(table_para, taccount1_genesis_block);

    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());
    // then make units from tableblock
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
    auto & units_ptr = tableblock->get_tableblock_units(true);
    for (auto & cache_unit : units_ptr) {
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }


    // block send to backup
    base::xautostream_t<4096> stream(base::xcontext_t::instance());
    tableblock->full_block_serialize_to(stream);
    std::string message = std::string((const char*)stream.data(), stream.size());

    // backup verify table-block
    base::xstream_t stream2(base::xcontext_t::instance(), (uint8_t*)message.data(), message.size());
    xblock_t* backup_block = dynamic_cast<xblock_t*>(xblock_t::full_block_read_from(stream2));
    ASSERT_NE(backup_block, nullptr);
    ASSERT_FALSE(backup_block->get_block_hash().empty());
    // then make units from tableblock
    xtable_block_t* tableblock2 = dynamic_cast<xtable_block_t*>(backup_block);
    auto & units_ptr2 = tableblock2->get_tableblock_units(true);
    for (auto & cache_unit : units_ptr2) {
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }

}

TEST_F(test_new_block, genesis_block_1) {
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");

    base::xauto_ptr<base::xvblock_t> genesis_1(test_blocktuil::create_genesis_empty_unit(address));
    ASSERT_FALSE(genesis_1->get_block_hash().empty());

    for(size_t i = 0; i < 10000; i++)
    {
        base::xauto_ptr<base::xvblock_t> genesis_2(test_blocktuil::create_genesis_empty_unit(address));
        ASSERT_EQ(genesis_1->get_block_hash(), genesis_2->get_block_hash());
    }
}

TEST_F(test_new_block, genesis_block_2) {
    std::string address = xblocktool_t::make_address_user_account("11111111111111111112");
    uint64_t init_balance = 100000000000;
    base::xauto_ptr<base::xvblock_t> genesis_1(test_blocktuil::create_genesis_lightunit(address, init_balance));
    ASSERT_FALSE(genesis_1->get_block_hash().empty());

    {
        std::string entity_str;
        genesis_1->get_output()->get_entitys()[0]->serialize_to_string(entity_str);
        std::string merkle_str = genesis_1->get_output()->get_entitys()[0]->query_value("");
        xinfo(" genesis_1 output account=%s tx=%s value=%ld %ld entityindex=%d", genesis_1->get_account().c_str(),
            genesis_1->get_output()->get_entitys()[0]->dump().c_str(),
            base::xhash64_t::digest(entity_str), base::xhash64_t::digest(merkle_str),
            genesis_1->get_output()->get_entitys()[0]->get_entity_index());
    }

    for(size_t i = 0; i < 10000; i++)
    {
        base::xauto_ptr<base::xvblock_t> genesis_2(test_blocktuil::create_genesis_lightunit(address, init_balance));
        if (genesis_1->get_block_hash() != genesis_2->get_block_hash()) {
            std::string entity_str;
            genesis_2->get_output()->get_entitys()[0]->serialize_to_string(entity_str);
            std::string merkle_str = genesis_2->get_output()->get_entitys()[0]->query_value("");
            xinfo("genesis_2 output account=%s tx=%s value=%ld %ld entityindex=%d", genesis_2->get_account().c_str(),
                genesis_2->get_output()->get_entitys()[0]->dump().c_str(),
                base::xhash64_t::digest(entity_str), base::xhash64_t::digest(merkle_str),
                genesis_2->get_output()->get_entitys()[0]->get_entity_index());
        }
        ASSERT_EQ(genesis_1->get_block_hash(), genesis_2->get_block_hash());
    }
}

// TEST_F(test_new_block, genesis_block_2) {
//     std::string address = xblocktool_t::make_address_user_account("11111111111111111112");

//     base::xauto_ptr<base::xvblock_t> genesis_1(test_blocktuil::create_genesis_empty_unit(address));

//     base::xauto_ptr<base::xvblock_t> genesis_2(test_blocktuil::create_genesis_empty_unit(address));


//     uint64_t init_balance = 100000000000;
//     base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_lightunit(address, init_balance);
//     base::xauto_ptr<base::xvblock_t> auto_genesis_block(genesis_block);
//     xassert(genesis_block);

//     xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();
//     auto store = store_face.get();
//     xvblockstore_t* blockstore = xblockstorehub_t::instance().create_block_store(*store, address);

//     auto ret = blockstore->store_block(genesis_block);
//     // TODO(jimmy) ASSERT_TRUE(ret);
// }

TEST_F(test_new_block, block_serialize_1) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xauto_ptr<base::xvblock_t> genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    base::xauto_ptr<base::xvblock_t> next_block = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_FALSE(genesis_block->get_block_hash().empty());
    ASSERT_FALSE(next_block->get_block_hash().empty());

    std::string block_object_bin;
    next_block->serialize_to_string(block_object_bin);
    std::string input_bin = next_block->get_input()->get_resources_data();
    std::string output_bin = next_block->get_output()->get_resources_data();
    ASSERT_TRUE(input_bin.empty());
    ASSERT_TRUE(output_bin.empty());
    ASSERT_TRUE(next_block->get_input()->get_resources_hash().empty());
    ASSERT_TRUE(next_block->get_output()->get_resources_hash().empty());

    ASSERT_TRUE(next_block->is_input_ready());
    ASSERT_TRUE(next_block->is_output_ready());
    ASSERT_TRUE(next_block->is_input_ready(true));
    ASSERT_TRUE(next_block->is_output_ready(true));
    ASSERT_TRUE(next_block->get_input()->get_resources_data().empty() == next_block->get_input()->get_resources_hash().empty());
    ASSERT_TRUE(next_block->get_output()->get_resources_data().empty() == next_block->get_output()->get_resources_hash().empty());

    base::xauto_ptr<base::xvblock_t> _sync_block(base::xvblockstore_t::create_block_object(block_object_bin));
    ASSERT_NE(_sync_block, nullptr);
    ASSERT_TRUE(_sync_block->is_valid(false));
    ASSERT_TRUE(_sync_block->set_input_resources(input_bin));
    ASSERT_TRUE(_sync_block->set_output_resources(output_bin));

    ASSERT_TRUE(next_block->is_input_ready());
    ASSERT_TRUE(next_block->is_output_ready());
    ASSERT_TRUE(next_block->is_input_ready(true));
    ASSERT_TRUE(next_block->is_output_ready(true));
    ASSERT_TRUE(next_block->get_input()->get_resources_data().empty() == next_block->get_input()->get_resources_hash().empty());
    ASSERT_TRUE(next_block->get_output()->get_resources_data().empty() == next_block->get_output()->get_resources_hash().empty());

}

TEST_F(test_new_block, block_serialize_2) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xauto_ptr<base::xvblock_t> genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    base::xauto_ptr<base::xvblock_t> next_block = test_blocktuil::create_next_emptyblock(genesis_block.get());
    ASSERT_FALSE(genesis_block->get_block_hash().empty());
    ASSERT_FALSE(next_block->get_block_hash().empty());

    base::xstream_t stream(base::xcontext_t::instance());
    xblock_t* block_1 = (xblock_t*)next_block.get();
    xassert(block_1->full_block_serialize_to(stream) > 0);
    base::xauto_ptr<base::xvblock_t> sync_block = xblock_t::full_block_read_from(stream);
    xassert(sync_block != nullptr);
    std::string input_bin = sync_block->get_input()->get_resources_data();
    std::string output_bin = sync_block->get_output()->get_resources_data();
    ASSERT_TRUE(input_bin.empty());
    ASSERT_TRUE(output_bin.empty());
}

TEST_F(test_new_block, block_serialize_3) {
    std::string account = xblocktool_t::make_address_user_account("11111111111111111111");

    // now fullunit has no resource
    base::xauto_ptr<base::xvblock_t> genesis_block = xblocktool_t::create_genesis_empty_unit(account);
    xfullunit_block_para_t para;
    base::xauto_ptr<base::xvblock_t> next_block = test_blocktuil::create_next_fullunit_with_consensus(para, genesis_block.get());
    xassert(next_block != nullptr);

{
    std::string block_object_bin;
    next_block->serialize_to_string(block_object_bin);
    std::string input_bin = next_block->get_input()->get_resources_data();
    std::string output_bin = next_block->get_output()->get_resources_data();
    ASSERT_TRUE(input_bin.empty());
    ASSERT_TRUE(output_bin.empty());
    ASSERT_TRUE(next_block->get_input()->get_resources_hash().empty());
    ASSERT_TRUE(next_block->get_output()->get_resources_hash().empty());

    base::xauto_ptr<base::xvblock_t> _sync_block(base::xvblockstore_t::create_block_object(block_object_bin));
    ASSERT_NE(_sync_block, nullptr);
    ASSERT_TRUE(_sync_block->is_valid(false));
    ASSERT_TRUE(_sync_block->set_input_resources(input_bin));
    ASSERT_TRUE(_sync_block->set_output_resources(output_bin));
}

}
