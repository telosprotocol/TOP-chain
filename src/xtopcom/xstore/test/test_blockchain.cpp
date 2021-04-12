#include <vector>

#include "gtest/gtest.h"
#include "xstore/xstore.h"
#include "xbase/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xdata/xblockchain.h"
#include "xdata/xblocktool.h"
#include "xdata/tests/test_blockutl.hpp"
#include "test_datamock.hpp"

using namespace top;
using namespace top::store;
using namespace top::data;

class test_blockchain : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_blockchain, create_unit) {
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::map<std::string, std::string> prop_list;

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);

    base::xvblock_t* proposal_block = test_blocktuil::create_next_emptyblock(genesis_block);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    auto block = dynamic_cast<data::xblock_t*>(proposal_block);
    ASSERT_TRUE(store->set_vblock(block));
    ASSERT_TRUE(store->execute_block(block));

    auto account2 = store->clone_account(address);
    ASSERT_EQ(account2->get_chain_height(), 1);

    base::xvblock_t* proposal_block2 = test_blocktuil::create_next_emptyblock(proposal_block);
    proposal_block2->set_block_flag(base::enum_xvblock_flag_connected);
    block = dynamic_cast<data::xblock_t*>(proposal_block2);
    ASSERT_TRUE(store->set_vblock(block));
    ASSERT_TRUE(store->execute_block(block));

    account2 = store->clone_account(address);
    ASSERT_EQ(account2->get_chain_height(), 2);
}

xtransaction_ptr_t create_sample_transfer_tx(const std::string & from, const std::string & to, uint64_t amount = 100) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from, to);
    tx->set_digest();
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

TEST_F(test_blockchain, create_units_and_tableblock) {
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();

    // create unit1
    std::string account1 = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("11111111111111122222");
    base::xvblock_t* account1_genesis_block = test_blocktuil::create_genesis_empty_unit(account1);
    xcons_transaction_ptr_t account1_tx1 = create_cons_transfer_tx(account1, to_account);
    xcons_transaction_ptr_t account1_tx2 = create_cons_transfer_tx(account1, to_account);
    xlightunit_block_para_t para1;
    para1.set_one_input_tx(account1_tx1);
    para1.set_one_input_tx(account1_tx2);
    xblock_t* lightunit1 = (xblock_t*)test_blocktuil::create_next_lightunit(para1, account1_genesis_block);

    // create unit2
    std::string account2 = xblocktool_t::make_address_user_account("11111111111111111112");
    base::xvblock_t* account2_genesis_block = test_blocktuil::create_genesis_empty_unit(account2);
    xcons_transaction_ptr_t account2_tx1 = create_cons_transfer_tx(account2, to_account);
    xcons_transaction_ptr_t account2_tx2 = create_cons_transfer_tx(account2, to_account);
    xlightunit_block_para_t para2;
    para2.set_one_input_tx(account2_tx1);
    para2.set_one_input_tx(account2_tx2);
    auto lightunit2 = test_blocktuil::create_next_lightunit(para2, account2_genesis_block);

    // create table-block
    xtable_block_para_t table_para;
    table_para.add_unit(lightunit1);
    table_para.add_unit(lightunit2);

    std::string taccount1 = data::xblocktool_t::make_address_table_account(base::enum_chain_zone_beacon_index, 0);
    base::xvblock_t* taccount1_genesis_block = test_blocktuil::create_genesis_empty_table(taccount1);
    auto taccount1_proposal_block = test_blocktuil::create_next_tableblock(table_para, taccount1_genesis_block);
    taccount1_proposal_block->set_block_flag(base::enum_xvblock_flag_connected);

    // then make units from tableblock
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(taccount1_proposal_block);
#if 0
    auto & cache_units = tableblock->get_consensused_units();
    for (auto & cache_unit : cache_units) {

        base::xstream_t stream(base::xcontext_t::instance());
        auto ret = cache_unit->full_serialize_to(stream);
        ASSERT_TRUE(0 != ret);
        ASSERT_FALSE(cache_unit->get_block_hash().empty());
    }
#endif
    ASSERT_FALSE(taccount1_proposal_block->get_block_hash().empty());

    ASSERT_TRUE(store->set_vblock(taccount1_proposal_block));
    ASSERT_TRUE(store->execute_block(taccount1_proposal_block));
    ASSERT_NE(taccount1_proposal_block, nullptr);
    ASSERT_EQ(tableblock->get_txs_count(), 4);

    auto temp_block = store->get_block_by_height(taccount1, 1);
    ASSERT_NE(temp_block, nullptr);
}
