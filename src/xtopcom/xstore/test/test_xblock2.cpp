#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xbasic/xobject_ptr.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"

#include "xdata/xemptyblock.h"
#include "xdata/xlightunit.h"
#include "xdata/tests/test_blockutl.hpp"

#include "xstore/xstore.h"
#include "xstore/xstore_face.h"

#include "test_datamock.hpp"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace top::data;

class test_xblock2 : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xtransaction_ptr_t create_sample_transaction(const std::string& from_account, const std::string& to_account, uint64_t amount) {
    xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
    data::xproperty_asset asset(amount);
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address(from_account, to_account);
    tx->set_digest();

    return tx;
}

TEST_F(test_xblock2, lightunit_basic_test) {
    xobject_ptr_t<store::xstore_face_t> store_face = store::xstore_factory::create_store_with_memdb();

    uint32_t chainid = 0;
    std::string from_account = xblocktool_t::make_address_user_account("11111111111111111111");
    const std::string to_account = xblocktool_t::make_address_user_account("23111111111111111111");
    uint64_t amount = 100;

    auto tx = create_sample_transaction(from_account, to_account, amount);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-amount);

    base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(from_account);

    ASSERT_TRUE(store_face->set_vblock(genesis_block));
    ASSERT_TRUE(store_face->execute_block(genesis_block));
    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);

    ASSERT_TRUE(store_face->set_vblock(proposal_block));
    ASSERT_TRUE(store_face->execute_block(proposal_block));
    std::cout << "genesis block height: " << genesis_block->get_height() << std::endl;
    std::cout << "proposal block height: " << proposal_block->get_height() << std::endl;

    xblock_t* block2 = store_face->get_block_by_height(from_account, 1);
    ASSERT_NE(block2, nullptr);
    ASSERT_EQ(block2->get_height(), 1);
}

TEST_F(test_xblock2, set_get_check_block) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = xblocktool_t::make_address_user_account("55111111111111111111");
    {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_face->set_vblock(genesis_block));
        ASSERT_TRUE(store_face->execute_block(genesis_block));
        auto account = store_face->clone_account(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.list_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "222", true);
        ASSERT_EQ(ret, 0);
        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        accountcmd1.get_change_property();
        auto prop_hashs = accountcmd1.get_property_hash();
        ASSERT_EQ(prop_hashs.size(), 1);

        uint64_t amount = 100;
        xtransaction_ptr_t tx = make_object_ptr<xtransaction_t>();
        data::xproperty_asset asset(amount);
        tx->make_tx_transfer(asset);
        tx->set_different_source_target_address(address, to_account);
        tx->set_digest();

        ASSERT_TRUE(accountcmd1.get_property_log() != nullptr);

        xlightunit_block_para_t para;
        para.set_one_input_tx(tx);
        para.set_balance_change(-amount);
        para.set_property_log(accountcmd1.get_property_log());
        para.set_propertys_change(accountcmd1.get_property_hash());

        base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit_with_consensus(para, genesis_block);
        proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
        data::xlightunit_block_t* b = dynamic_cast<data::xlightunit_block_t*>(proposal_block);
        std::cout << "light unit property size: " << b->get_property_hash_map().size() << std::endl;

        ASSERT_TRUE(store_face->set_vblock(proposal_block));
        ASSERT_TRUE(store_face->execute_block(proposal_block));

        xblock_t* block2 = store_face->get_block_by_height(address, 1);
        ASSERT_NE(block2, nullptr);
        ASSERT_EQ(block2->get_height(), 1);
        ASSERT_TRUE(block2->get_property_log() != nullptr);
    }
}

TEST_F(test_xblock2, cache_time_1) {
    std::map<std::string, std::string> prop_list;
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    uint64_t count = 1;
    for (uint64_t i = 0; i < count; i++) {
        datamock.create_unit(address, prop_list);
    }

    {
        ASSERT_EQ(count, store->get_blockchain_height(address));
        for (uint64_t i = 0; i < count; i++) {
            uint64_t block_height = i + 1;
            auto block = store->get_block_by_height(address, block_height);
            ASSERT_EQ(block->get_height(), block_height);
        }
    }

    // std::this_thread::sleep_for(std::chrono::seconds(4));

    {
        ASSERT_EQ(count, store->get_blockchain_height(address));
        for (uint64_t i = 0; i < count; i++) {
            uint64_t block_height = i + 1;
            auto block = store->get_block_by_height(address, block_height);
            ASSERT_EQ(block->get_height(), block_height);
        }
    }
}
