#include <vector>
#include <sys/time.h>

#include "gtest/gtest.h"

#include "xstore/xstore.h"
#include "xstore/xaccount_cmd.h"
#include "xdata/xblockchain.h"
#include "xdata/xpropertylog.h"
#include "xdata/tests/test_blockutl.hpp"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"

#include "xstore/xaccount_context.h"
#include "xstore/test/test_datamock.hpp"

// #include "generator.h"

using namespace top::base;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace std;

class test_discrete_block : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xblock_t* set_unit_with_property(xstore_face_t* store, const std::string & address, const std::string & prop_name, const std::string & value) {
    auto account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        assert(store->set_vblock(genesis_block));
        assert(store->execute_block(genesis_block));
        account = store->clone_account(address);
    }

    xaccount_cmd accountcmd1(account, store);
    int32_t error_code;
    auto prop_ptr = accountcmd1.get_property(prop_name, error_code);
    if (prop_ptr == nullptr) {
        auto ret = accountcmd1.list_create(prop_name, true);
        assert(ret == 0);
        ret = accountcmd1.list_push_back(prop_name, value, true);
        assert(ret == 0);
    } else {
        auto ret = accountcmd1.list_push_back(prop_name, value, true);
        assert(ret == 0);
    }

    std::map<std::string, std::string> prop_hashs;
    xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
    assert(binlog != nullptr);
    accountcmd1.get_change_property();

    prop_hashs = accountcmd1.get_property_hash();
    assert(prop_hashs.size() == 1);

    xaccount_context_t context(address, store);

    uint64_t amount = 100;
    std::string to_account("T-to-xxxxxxxxxxxxxxxxxx");

    xtransaction_ptr_t tx = account->make_transfer_tx(to_account, -amount, 0, 0, 0);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(amount);
    para.set_property_log(accountcmd1.get_property_log());
    para.set_propertys_change(accountcmd1.get_property_hash());

    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit(para, account);

    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);
    assert(store->set_vblock(proposal_block));
    assert(store->execute_block(proposal_block));

    return dynamic_cast<data::xblock_t*>(proposal_block);
}

TEST_F(test_discrete_block, property_log_sync_with_block_all_property) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string prop_name = "aaa";
    uint64_t count = 50;

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    std::vector<data::xblock_t*> transfer_blocks;

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = set_unit_with_property(store1.get(), address, prop_name, std::to_string(i));
        ASSERT_EQ(unit->get_height(), i);
        uint64_t chainheight = store1->get_blockchain_height(address);
        ASSERT_EQ(chainheight, unit->get_height());
        transfer_blocks.push_back(unit);
    }
    account = store1->clone_account(address);
    ASSERT_TRUE(account != nullptr);
    std::cout << "origin account balance: " << account->balance() << std::endl;

    for (size_t i = 0; i < transfer_blocks.size(); ++i) {
        ASSERT_TRUE(store->set_vblock(transfer_blocks[i]));
        ASSERT_TRUE(store->execute_block(transfer_blocks[i]));
    }

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = store->get_block_by_height(address, i);
        ASSERT_NE(unit, nullptr);

        ASSERT_EQ(unit->get_balance_change(), 100);
    }

    account = store->clone_account(address);
    ASSERT_TRUE(account != nullptr);
    std::cout << "synced account balance: " << account->balance() << std::endl;

    std::vector<std::string> values;
    store->list_get_all(address, prop_name, values);
    ASSERT_EQ(values.size(), count);
    std::cout << "=====list all=====" << std::endl;
    for (auto & v : values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

TEST_F(test_discrete_block, property_log_not_sync_with_block) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string prop_name = "aaa";
    uint64_t count = 50;

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }

    std::vector<data::xblock_t*> transfer_blocks;

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = set_unit_with_property(store1.get(), address, prop_name, std::to_string(i));
        ASSERT_EQ(unit->get_height(), i);
        uint64_t chainheight = store1->get_blockchain_height(address);
        ASSERT_EQ(chainheight, unit->get_height());

        transfer_blocks.push_back(unit);
    }

    for (size_t i = 0; i < transfer_blocks.size(); ++i) {
        if ((i + 1) % 5 == 0 && i > 0) {
            std::cout << "transfer block, missing the " << (i + 1) << "rd height block" << std::endl;
            continue;
        } else {
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i]));
        }
    }

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = store->get_block_by_height(address, i);
        if (i % 5 == 0) {
            ASSERT_EQ(unit, nullptr);
            continue;
        } else {
            ASSERT_NE(unit, nullptr);

        }
        ASSERT_EQ(unit->get_balance_change(), 100);

        std::vector<std::string> values;
        store->list_get_all(address, prop_name, values);
        ASSERT_EQ(values.size(), 0);
    }

    for (size_t i = 0; i < transfer_blocks.size(); ++i) {
        if ((i + 1) % 5 == 0 && i > 0) {
            std::cout << "resyncing the missing " << (i + 1) << "rd block" << std::endl;
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i]));
            ASSERT_TRUE(store->execute_block(transfer_blocks[i]));
        } else {
            ASSERT_TRUE(store->execute_block(transfer_blocks[i]));
        }
        std::vector<std::string> values;
        store->list_get_all(address, prop_name, values);

        ASSERT_EQ(values.size(), i + 1);
    }

    std::vector<std::string> final_values;
    store->list_get_all(address, prop_name, final_values);
    ASSERT_EQ(final_values.size(), count);
    std::cout << "=====list all=====" << std::endl;
    for (auto & v : final_values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

xblock_t* set_unit_with_random_property(xstore_face_t* store, const std::string & address, const std::string & prop_name, int value) {
    auto account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        assert(store->set_vblock(genesis_block));
        assert(store->execute_block(genesis_block));
        account = store->clone_account(address);
    }

    bool need_create_property = value % 7;

    xaccount_cmd accountcmd1(account, store);
    int32_t error_code;
    auto prop_ptr = accountcmd1.get_property(prop_name, error_code);
    if (prop_ptr == nullptr) {
        auto ret = accountcmd1.list_create(prop_name, true);
        assert(ret == 0);
    }

    if (need_create_property) {
        auto ret = accountcmd1.list_push_back(prop_name, std::to_string(value), true);
        assert(ret == 0);

        std::map<std::string, std::string> prop_hashs;
        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        assert(binlog != nullptr);
        accountcmd1.get_change_property();

        prop_hashs = accountcmd1.get_property_hash();
        assert(prop_hashs.size() == 1);
    }

    uint64_t amount = 100;
    std::string to_account("T-to-xxxxxxxxxxxxxxxxxx");

    xtransaction_ptr_t tx = account->make_transfer_tx(to_account, amount, 0, 0, 0);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-amount);
    para.set_property_log(accountcmd1.get_property_log());
    para.set_propertys_change(accountcmd1.get_property_hash());

    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit(para, account);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);

    assert(store->set_vblock(proposal_block));
    assert(store->execute_block(proposal_block));
    return dynamic_cast<data::xblock_t*>(proposal_block);
}

xblock_t* create_unit_with_diff_random_property(xstore_face_t* store, const std::string & address) {
    static size_t height = 0;
    auto account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        assert(store->set_vblock(genesis_block));
        assert(store->execute_block(genesis_block));
        account = store->clone_account(address);
    }

    ++height;

    bool need_create_property = height % 7 == 0 ? true : false;

    xaccount_cmd accountcmd1(account, store);
    int32_t error_code;
    if (need_create_property) {
        std::string prop_name = "random_prop_" + std::to_string(height);
        std::cout << "property: " << prop_name << ", height: " << height << std::endl;
        auto ret = accountcmd1.string_create(prop_name, true);
        assert(ret == 0);
        ret = accountcmd1.string_set(prop_name, prop_name);
        assert(ret == 0);
    }

    uint64_t amount = 100;
    std::string to_account("T-to-xxxxxxxxxxxxxxxxxx");

    xtransaction_ptr_t tx = account->make_transfer_tx(to_account, amount, 0, 0, 0);

    xlightunit_block_para_t para;
    para.set_one_input_tx(tx);
    para.set_balance_change(-amount);
    if (need_create_property) {
        para.set_property_log(accountcmd1.get_property_log());
        para.set_propertys_change(accountcmd1.get_property_hash());
    }

    base::xvblock_t* proposal_block = test_blocktuil::create_next_lightunit(para, account);
    proposal_block->set_block_flag(base::enum_xvblock_flag_connected);

    assert(store->set_vblock(proposal_block));
    assert(store->execute_block(proposal_block));
    return dynamic_cast<data::xblock_t*>(proposal_block);
}

TEST_F(test_discrete_block, property_log_sync_with_block_random_property) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string prop_name = "aaa";
    uint64_t count = 50;

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }

    std::vector<data::xblock_t*> transfer_blocks;

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = set_unit_with_random_property(store1.get(), address, prop_name, i);
        ASSERT_EQ(unit->get_height(), i);
        uint64_t chainheight = store1->get_blockchain_height(address);
        ASSERT_EQ(chainheight, unit->get_height());

        transfer_blocks.push_back(unit);
    }

    std::vector<std::string> values1;
    store1->list_get_all(address, prop_name, values1);
    std::cout << "=====list all in store1 with random properties=====" << std::endl;
    for (auto & v : values1) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    for (size_t i = 0; i < transfer_blocks.size(); ++i) {
        ASSERT_TRUE(store->set_vblock(transfer_blocks[i]));
        ASSERT_TRUE(store->execute_block(transfer_blocks[i]));
    }

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = store->get_block_by_height(address, i);
        ASSERT_NE(unit, nullptr);
        ASSERT_EQ(unit->get_balance_change(), -100);
    }

    std::vector<std::string> values2;
    store->list_get_all(address, prop_name, values2);
    ASSERT_EQ(values1.size(), values2.size());
    std::cout << "=====list all=====" << std::endl;
    for (auto & v : values2) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

TEST_F(test_discrete_block, property_log_not_sync_with_block_random) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string prop_name = "aaa";
    uint64_t count = 50;

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
    }

    std::vector<data::xblock_t*> transfer_blocks;

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = set_unit_with_random_property(store1.get(), address, prop_name, i);
        ASSERT_EQ(unit->get_height(), i);
        uint64_t chainheight = store1->get_blockchain_height(address);
        ASSERT_EQ(chainheight, unit->get_height());
        transfer_blocks.push_back(unit);
    }

    for (size_t i = 0; i < transfer_blocks.size(); ++i) {
        if ((i + 1) % 5 == 0 && i > 0) {
            std::cout << "transfer block, missing the " << (i + 1) << "rd height block" << std::endl;
            continue;
        } else {
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i]));
        }
    }

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = store->get_block_by_height(address, i);
        if (i % 5 == 0) {
            ASSERT_EQ(unit, nullptr);
            continue;
        } else {
            ASSERT_NE(unit, nullptr);
        }
        ASSERT_EQ(unit->get_balance_change(), -100);

        std::vector<std::string> values;
        store->list_get_all(address, prop_name, values);

        ASSERT_EQ(values.size(), 0);
    }

    for (size_t i = 0; i < transfer_blocks.size(); ++i) {
        if ((i + 1) % 5 == 0 && i > 0) {
            std::cout << "resyncing the missing " << (i + 1) << "rd block" << std::endl;
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i]));
            ASSERT_TRUE(store->execute_block(transfer_blocks[i]));
        } else {
            ASSERT_TRUE(store->execute_block(transfer_blocks[i]));
        }
        std::vector<std::string> values;
        store->list_get_all(address, prop_name, values);

        std::cout << "=====list all not sync with block random property =====" << std::endl;
        for (auto & v : values) {
            std::cout << v << " ";
        }
        std::cout << std::endl;
    }

    std::vector<std::string> final_values;
    store->list_get_all(address, prop_name, final_values);
    // ASSERT_EQ(final_values.size(), count);
    std::cout << "=====list all=====" << std::endl;
    for (auto & v : final_values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

TEST_F(test_discrete_block, block_with_different_random_property) {
    xobject_ptr_t<xstore_face_t> store1 = xstore_factory::create_store_with_memdb();
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store1.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    uint64_t count = 50;

    auto account = store1->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store1->set_vblock(genesis_block));
        ASSERT_TRUE(store1->execute_block(genesis_block));
    }

    account = store->clone_account(address);
    if (account == nullptr) {
        base::xvblock_t* genesis_block = test_blocktuil::create_genesis_empty_unit(address);
        ASSERT_TRUE(store->set_vblock(genesis_block));
        ASSERT_TRUE(store->execute_block(genesis_block));
    }

    std::vector<data::xblock_t*> transfer_blocks;

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = create_unit_with_diff_random_property(store1.get(), address);
        ASSERT_EQ(unit->get_height(), i);
        uint64_t chainheight = store1->get_blockchain_height(address);
        ASSERT_EQ(chainheight, unit->get_height());
        transfer_blocks.push_back(unit);
    }

    for (size_t i = 1; i <= transfer_blocks.size(); ++i) {
        if (i % 5 == 0) {
            std::cout << "transfer block, missing the " << i << "rd height block" << std::endl;
            continue;
        } else {
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i-1]));
        }
    }

    for (uint64_t i = 1; i <= count; i++) {
        auto unit = store->get_block_by_height(address, i);
        if (i % 5 == 0) {
            ASSERT_EQ(unit, nullptr);
            continue;
        } else {
            ASSERT_NE(unit, nullptr);
        }
        ASSERT_EQ(unit->get_balance_change(), -100);
        auto account = store->clone_account(address);
        ASSERT_NE(account, nullptr);
        ASSERT_EQ(account->get_last_height(), 0);
    }

    for (size_t i = 1; i <= transfer_blocks.size(); ++i) {
        if (i % 5 == 0) {
            std::cout << "resyncing the missing " << i << "rd block" << std::endl;
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i-1]));
            ASSERT_TRUE(store->execute_block(transfer_blocks[i-1]));
        } else if (i % 7 == 0) {
            ASSERT_TRUE(store->set_vblock(transfer_blocks[i-1]));
            ASSERT_TRUE(store->execute_block(transfer_blocks[i-1]));
        } else {
            ASSERT_TRUE(store->execute_block(transfer_blocks[i-1]));
        }
        auto account = store->clone_account(address);
        ASSERT_NE(account, nullptr);
        std::cout << "after resyncing block(height:" << i << "), property confirm height: " << account->get_last_height() << std::endl;
    }
}
