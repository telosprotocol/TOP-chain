#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xstore/xstore.h"
#include "xstore/xstore_face.h"
#include "xbasic/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xstore/test/test_datamock.hpp"
#include "xmbus/xevent_store.h"
#include "xstore/xaccount_context.h"

using namespace top;
using namespace top::base;
using namespace top::store;
using namespace top::data;

class test_property : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xaccount_cmd_ptr_t make_account_cmd(const xstore_face_ptr_t & store, const std::string & address, const std::string & property_name = {}) {
    xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
    xaccount_cmd_ptr_t accountcmd1 = std::make_shared<xaccount_cmd>(account.get(), store.get());
    std::string name;
    if (property_name.empty()) {
        name = "aaa";
    } else {
        name = property_name;
    }

    auto ret = accountcmd1->list_create(name, true);
    xassert(ret == 0);
    ret = accountcmd1->list_push_back(name, "111", true);
    xassert(ret == 0);
    ret = accountcmd1->list_push_back(name, "222", true);
    xassert(ret == 0);
    return accountcmd1;
}

xaccount_cmd_ptr_t make_account_cmd_map(xstore_face_t* store, const std::string & address, const std::string & property_name = {}) {
    xaccount_ptr_t account = make_object_ptr<xaccount_t>(address);
    xaccount_cmd_ptr_t accountcmd1 = std::make_shared<xaccount_cmd>(account.get(), store);
    std::string name;
    if (property_name.empty()) {
        name = "aaa";
    } else {
        name = property_name;
    }

    auto ret = accountcmd1->map_create(name, true);
    xassert(ret == 0);
    ret = accountcmd1->map_set(name, "111", "value111");
    xassert(ret == 0);
    ret = accountcmd1->map_set(name, "222", "value222");
    xassert(ret == 0);
    return accountcmd1;
}

xvblock_t* create_sample_block(xstore_face_t* store, const std::string& address) {
    assert(store != nullptr);
    auto account = store->clone_account(address);
    base::xvblock_t* genesis_block = nullptr;
    if (account == nullptr) {
        genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        account = new xblockchain2_t(address);
    }
    uint64_t amount = 100;
    std::string to_account = "T-to-xxxxxxxxxxxxxxxxxxxxx";
    xtransaction_ptr_t tx = account->make_transfer_tx(to_account, -amount, 0, 0, 0);

    // create property
    auto accountcmd = make_account_cmd_map(store, address);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(amount);
    para.set_property_log(accountcmd->get_property_log());
    para.set_propertys_change(accountcmd->get_property_hash());
    base::xvblock_t* proposal_block = nullptr;
    if (genesis_block != nullptr) {
        proposal_block = test_blocktuil::create_next_lightunit(para, genesis_block);
    } else {
        proposal_block = test_blocktuil::create_next_lightunit(para, account);
    }

    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    return proposal_block;
}

xblock_t* create_sample_block(xaccount_cmd* cmd, xstore_face_t* store, const std::string& address) {
    assert(store != nullptr);
    auto account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        assert(store->set_vblock(genesis_block));
        account = store->clone_account(address);
    }
    uint64_t amount = 100;
    std::string to_account = "T-to-xxxxxxxxxxxxxxxxxxxxx";
    xtransaction_ptr_t tx = account->make_transfer_tx(to_account, -amount, 0, 0, 0);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(amount);
    para.set_property_log(cmd->get_property_log());
    para.set_propertys_change(cmd->get_property_hash());
    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit(para, account);
    assert(proposal_block);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    auto block = dynamic_cast<data::xblock_t*>(proposal_block);

    return block;
}

xblock_t* create_sample_block1(xaccount_context_t* context) {
    assert(context != nullptr);

    uint64_t amount = 100;
    std::string to_account = "T-to-xxxxxxxxxxxxxxxxxxxxx";
    xtransaction_ptr_t tx = context->get_blockchain()->make_transfer_tx(to_account, -amount, 0, 0, 0);

    xtransaction_result_t result;
    assert(context->get_transaction_result(result));

    xlightunit_block_para_t para;
    para.set_native_property(result.m_native_property);
    para.set_one_input_tx(tx);
    para.set_balance_change(amount);
    para.set_property_log(result.m_prop_log);
    para.set_propertys_change(result.m_props);
    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit(para, context->get_blockchain());
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    auto block = dynamic_cast<data::xblock_t*>(proposal_block);
    return block;
}

base::xvblock_t* create_sample_fullblock(xvblock_t* prev_block, xstore_face_t* store, const std::string& address) {
    assert(store != nullptr);

    auto account = store->clone_account(address);

    std::map<std::string, std::string> properties;
    const auto & property_map = account->get_property_hash_map();
    for (auto & v : property_map) {
        xdataobj_ptr_t db_prop = store->clone_property(address, v.first);
        if (db_prop == nullptr) {
            xerror("test::create_sample_block account:%s property(%s) not exist.",
                address.c_str(), v.first.c_str());
            return nullptr;
        }

        std::string db_prop_hash = xhash_base_t::calc_dataunit_hash(db_prop.get());
        if (db_prop_hash != v.second) {
            xerror("test::create_sample_block account:%s property(%s) hash not match fullunit.",
                address.c_str(), v.first.c_str());
            return nullptr;
        }
        base::xstream_t _stream(base::xcontext_t::instance());
        db_prop->serialize_to(_stream);
        std::string prop_str((const char *)_stream.data(), _stream.size());
        properties[v.first] = prop_str;
    }

    xfullunit_block_para_t block_para;
    block_para.m_account_state = account->get_account_mstate();
    block_para.m_first_unit_hash = account->get_last_full_unit_hash();
    block_para.m_first_unit_height = account->get_last_full_unit_height();
    block_para.m_account_propertys = properties;

    base::xvblock_t* proposal_block = test_blocktuil::create_next_fullunit_with_consensus(block_para, prev_block);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    account->release_ref();
    return proposal_block;
}

TEST_F(test_property, property_confirm_set_vblock_1) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }

    auto block = create_sample_block(store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block));
    ASSERT_TRUE(store_ptr->execute_block(block));
    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 1);
}

TEST_F(test_property, property_confirm_set_vblock_2) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string property_name1 = "#p1";
    std::string property_name2 = "#p2";

    {
        auto account = store_ptr->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
            ASSERT_TRUE(store_ptr->execute_block(genesis_block));
            account = store_ptr->clone_account(address);
        }
        xaccount_cmd_ptr_t accountcmd1 = std::make_shared<xaccount_cmd>(account, store_ptr.get());
        ASSERT_EQ(accountcmd1->map_create(property_name1), 0);
        ASSERT_EQ(accountcmd1->map_create(property_name2), 0);
        uint64_t amount = 100;
        std::string to_account = "T-to-xxxxxxxxxxxxxxxxxxxxx";
        xtransaction_ptr_t tx = account->make_transfer_tx(to_account, -amount, 0, 0, 0);

        xlightunit_block_para_t para;
        para.set_one_input_tx(tx);
        para.set_balance_change(amount);
        para.set_property_log(accountcmd1->get_property_log());
        para.set_propertys_change(accountcmd1->get_property_hash());
        auto block = test_blocktuil::create_next_lightunit(para, account);

        ASSERT_TRUE(store_ptr->set_vblock(block));
        block->set_block_flag(base::enum_xvblock_flag_connected);
        ASSERT_TRUE(store_ptr->execute_block(block));
        account = store_ptr->clone_account(address);
        ASSERT_EQ(account->get_last_height(), 1);
    }
    {
        auto account = store_ptr->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
            ASSERT_TRUE(store_ptr->execute_block(genesis_block));
            account = store_ptr->clone_account(address);
        }
        xaccount_cmd_ptr_t accountcmd1 = std::make_shared<xaccount_cmd>(account, store_ptr.get());
        ASSERT_EQ(accountcmd1->map_set(property_name1, "111", "value111"), 0);
        ASSERT_EQ(accountcmd1->map_set(property_name2, "111", "value111"), 0);

        auto block = create_sample_block(accountcmd1.get(), store_ptr.get(), address);

        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
        account = store_ptr->clone_account(address);
        ASSERT_EQ(account->get_last_height(), 2);
    }
    {
        auto account = store_ptr->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
            ASSERT_TRUE(store_ptr->execute_block(genesis_block));
            account = store_ptr->clone_account(address);
        }
        xaccount_cmd_ptr_t accountcmd1 = std::make_shared<xaccount_cmd>(account, store_ptr.get());
        ASSERT_EQ(accountcmd1->map_set(property_name1, "1111", "value1111"), 0);
        ASSERT_EQ(accountcmd1->map_set(property_name2, "1111", "value1111"), 0);

        auto block = create_sample_block(accountcmd1.get(), store_ptr.get(), address);

        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
        account = store_ptr->clone_account(address);
        ASSERT_EQ(account->get_last_height(), 3);
    }
}

TEST_F(test_property, property_confirm_set_sync_unit) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }

    auto block = create_sample_block(store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(block));
    ASSERT_TRUE(store_ptr->execute_block(block));
    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 1);
}

TEST_F(test_property, property_confirm_with_empty_unit) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }

    size_t count = 50;

    for (size_t i = 1; i <= count; ++i) {
        base::xvblock_t* block = nullptr;
        account = store_ptr->clone_account(address);
        block = test_blocktuil::create_next_emptyblock(account);
        block->set_block_flag(base::enum_xvblock_flag_connected);

        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
    }

    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), count);
}

TEST_F(test_property, property_confirm_set) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    auto store2 = xstore_factory::create_store_with_memdb();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }

    auto cmd = make_account_cmd(store_ptr, address, "prop1");
    std::map<std::string, xdataobj_ptr_t> prop1 = cmd->get_change_property();
    ASSERT_EQ(prop1.size(), 1);

    auto block = create_sample_block(cmd.get(), store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(block));
    ASSERT_TRUE(store_ptr->execute_block(block));

    auto cmd2 = make_account_cmd(store_ptr, address, "prop2");
    std::map<std::string, xdataobj_ptr_t> prop2 = cmd2->get_change_property();
    ASSERT_EQ(prop2.size(), 1);

    auto block2 = create_sample_block(cmd2.get(), store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(block2));
    ASSERT_TRUE(store_ptr->execute_block(block2));

    auto fullunit = create_sample_fullblock(block2, store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(fullunit));
    ASSERT_TRUE(store_ptr->execute_block(fullunit));

    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 3);

    auto account2 = store2->clone_account(address);
    ASSERT_EQ(account2, nullptr);
    ASSERT_TRUE(store2->set_vblock(fullunit));
    ASSERT_TRUE(store2->execute_block(fullunit));

    account = store2->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 3);
    {
        xdataobj_ptr_t prop1 = store2->clone_property(address, "prop1");
        xstrdeque_ptr_t deque = dynamic_xobject_ptr_cast<base::xstrdeque_t>(prop1);
        std::string value;
        ASSERT_TRUE(deque->pop_back(value));
        ASSERT_EQ(value, "222");
        ASSERT_TRUE(deque->pop_back(value));
        ASSERT_EQ(value, "111");
        ASSERT_FALSE(deque->pop_back(value));
    }
    {
        xdataobj_ptr_t prop2 = store2->clone_property(address, "prop2");
        xstrdeque_ptr_t deque = dynamic_xobject_ptr_cast<base::xstrdeque_t>(prop2);
        std::string value;
        ASSERT_TRUE(deque->pop_back(value));
        ASSERT_EQ(value, "222");
        ASSERT_TRUE(deque->pop_back(value));
        ASSERT_EQ(value, "111");
        ASSERT_FALSE(deque->pop_back(value));
    }
}

TEST_F(test_property, property_confirm_get_1) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }
    auto cmd = make_account_cmd(store_ptr, address, "prop1");
    auto block = create_sample_block(cmd.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block));
    ASSERT_TRUE(store_ptr->execute_block(block));

    auto cmd2 = make_account_cmd(store_ptr, address, "prop2");
    std::map<std::string, xdataobj_ptr_t> prop2 = cmd2->get_change_property();
    ASSERT_EQ(prop2.size(), 1);

    auto block2 = create_sample_block(cmd.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block2));
    ASSERT_TRUE(store_ptr->execute_block(block2));

    auto fullunit = create_sample_fullblock(block2, store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(fullunit));
    ASSERT_TRUE(store_ptr->execute_block(fullunit));

    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 3);
}

TEST_F(test_property, property_confirm_get_2) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    auto store_ptr2 = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }

    auto cmd = make_account_cmd(store_ptr, address, "prop1");
    std::map<std::string, xdataobj_ptr_t> prop1 = cmd->get_change_property();
    ASSERT_EQ(prop1.size(), 1);

    auto block = create_sample_block(cmd.get(), store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(block));
    ASSERT_TRUE(store_ptr->execute_block(block));

    auto cmd2 = make_account_cmd(store_ptr, address, "prop2");
    std::map<std::string, xdataobj_ptr_t> prop2 = cmd2->get_change_property();
    ASSERT_EQ(prop2.size(), 1);

    xaccount_context_t ac2(address, store_ptr.get());
    auto block2 = create_sample_block(cmd.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block2));
    ASSERT_TRUE(store_ptr->execute_block(block2));

    auto fullunit = create_sample_fullblock(block2, store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(fullunit));
    ASSERT_TRUE(store_ptr->execute_block(fullunit));

    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 3);

    ASSERT_TRUE(store_ptr2->set_vblock(fullunit));
    ASSERT_TRUE(store_ptr2->execute_block(fullunit));

    ASSERT_TRUE(store_ptr2->set_vblock(block2));
    ASSERT_TRUE(store_ptr2->execute_block(block2));

    ASSERT_TRUE(store_ptr2->set_vblock(block));
    ASSERT_TRUE(store_ptr2->execute_block(block));
    auto account2 = store_ptr2->clone_account(address);
    ASSERT_EQ(account2->get_last_height(), 3);
}

TEST_F(test_property, property_confirm_get_3) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    auto store_ptr2 = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account = store_ptr->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
        ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        account = store_ptr->clone_account(address);
    }

    auto cmd = make_account_cmd(store_ptr, address, "prop1");
    std::map<std::string, xdataobj_ptr_t> prop1 = cmd->get_change_property();
    ASSERT_EQ(prop1.size(), 1);

    auto block = create_sample_block(cmd.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block));
    ASSERT_TRUE(store_ptr->execute_block(block));

    auto cmd2 = make_account_cmd(store_ptr, address, "prop2");

    std::map<std::string, xdataobj_ptr_t> prop2 = cmd2->get_change_property();
    ASSERT_EQ(prop2.size(), 1);
    auto block2 = create_sample_block(cmd2.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block2));
    ASSERT_TRUE(store_ptr->execute_block(block2));

    auto cmd3 = make_account_cmd(store_ptr, address, "prop3");
    auto block3 = create_sample_block(cmd3.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block3));
    ASSERT_TRUE(store_ptr->execute_block(block3));

    auto fullunit = create_sample_fullblock(block3, store_ptr.get(), address);

    ASSERT_TRUE(store_ptr->set_vblock(fullunit));
    ASSERT_TRUE(store_ptr->execute_block(fullunit));

    auto cmd5 = make_account_cmd(store_ptr, address, "prop5");
    auto block5 = create_sample_block(cmd5.get(), store_ptr.get(), address);
    ASSERT_TRUE(store_ptr->set_vblock(block5));
    ASSERT_TRUE(store_ptr->execute_block(block5));

    account = store_ptr->clone_account(address);
    ASSERT_EQ(account->get_last_height(), 5);

    auto account2 = store_ptr2->clone_account(address);
    if (account2 == nullptr) {
        base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
        ASSERT_TRUE(store_ptr2->set_vblock(genesis_block));
        account2 = store_ptr2->clone_account(address);
    }
    ASSERT_TRUE(store_ptr2->set_vblock(block5));

    ASSERT_TRUE(store_ptr2->set_vblock(fullunit));
    ASSERT_TRUE(store_ptr2->execute_block(fullunit));

    ASSERT_TRUE(store_ptr2->execute_block(block5));

    ASSERT_TRUE(store_ptr2->set_vblock(block2));
    ASSERT_TRUE(store_ptr2->execute_block(block2));

    ASSERT_TRUE(store_ptr2->set_vblock(block));
    ASSERT_TRUE(store_ptr2->execute_block(block));

    ASSERT_TRUE(store_ptr2->set_vblock(block3));
    ASSERT_TRUE(store_ptr2->execute_block(block3));

    account2 = store_ptr2->clone_account(address);
    ASSERT_EQ(account2->get_last_height(), 5);
}

TEST_F(test_property, native_property_change) {
    auto store_ptr = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = store_ptr->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            ASSERT_TRUE(store_ptr->set_vblock(genesis_block));
            ASSERT_TRUE(store_ptr->execute_block(genesis_block));
        }
    }
    {
        xtransaction_result_t result;
        xaccount_context_t ac(address, store_ptr.get());
        ASSERT_EQ(0, ac.string_set("@1", "1111", true));
        ASSERT_EQ(0, ac.string_set("@2", "2222", true));

        auto block = create_sample_block1(&ac);
        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
        auto objs = dynamic_cast<xblock_t*>(block)->get_native_property().get_properties();
        ASSERT_EQ(objs.size(), 2);

        xaccount_ptr_t account = store_ptr->query_account(address);
        auto & native_prop = account->get_native_property();
        auto objs1 = native_prop.get_properties();
        ASSERT_EQ(objs1.size(), 2);
        std::string value;
        native_prop.native_string_get("@1", value);
        ASSERT_EQ(value, "1111");
        native_prop.native_string_get("@2", value);
        ASSERT_EQ(value, "2222");
    }

    {
        xaccount_context_t ac(address, store_ptr.get());
        auto block = create_sample_block1(&ac);
        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
        auto objs = dynamic_cast<xblock_t*>(block)->get_native_property().get_properties();
        ASSERT_EQ(objs.size(), 0);

        xaccount_ptr_t account = store_ptr->query_account(address);
        auto & native_prop = account->get_native_property();
        auto objs1 = native_prop.get_properties();
        ASSERT_EQ(objs1.size(), 2);
        std::string value;
        native_prop.native_string_get("@1", value);
        ASSERT_EQ(value, "1111");
        native_prop.native_string_get("@2", value);
        ASSERT_EQ(value, "2222");
    }

    {
        xaccount_context_t ac(address, store_ptr.get());
        ASSERT_EQ(0, ac.string_set("@1", "11111", true));

        auto block = create_sample_block1(&ac);
        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
        auto objs = dynamic_cast<xblock_t*>(block)->get_native_property().get_properties();
        ASSERT_EQ(objs.size(), 1);

        xaccount_ptr_t account = store_ptr->query_account(address);
        auto & native_prop = account->get_native_property();
        auto objs1 = native_prop.get_properties();
        ASSERT_EQ(objs1.size(), 2);
        std::string value;
        native_prop.native_string_get("@1", value);
        ASSERT_EQ(value, "11111");
        native_prop.native_string_get("@2", value);
        ASSERT_EQ(value, "2222");
    }

    {
        xaccount_context_t ac(address, store_ptr.get());
        ASSERT_EQ(0, ac.string_set("@3", "3333", true));

        auto block = create_sample_block1(&ac);
        ASSERT_TRUE(store_ptr->set_vblock(block));
        ASSERT_TRUE(store_ptr->execute_block(block));
        auto objs = dynamic_cast<xblock_t*>(block)->get_native_property().get_properties();
        ASSERT_EQ(objs.size(), 1);

        auto account = store_ptr->clone_account(address);
        auto & native_prop = account->get_native_property();
        auto objs1 = native_prop.get_properties();
        ASSERT_EQ(objs1.size(), 3);
        std::string value;
        native_prop.native_string_get("@1", value);
        ASSERT_EQ(value, "11111");
        native_prop.native_string_get("@2", value);
        ASSERT_EQ(value, "2222");
        native_prop.native_string_get("@3", value);
        ASSERT_EQ(value, "3333");
    }
}
