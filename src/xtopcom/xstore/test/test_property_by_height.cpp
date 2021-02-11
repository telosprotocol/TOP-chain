#include <vector>

#include "gtest/gtest.h"
#include "xbasic/xobject_ptr.h"
#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xstore/test/xstore_face_mock.h"
#include "xstore/test/test_datamock.hpp"

#include "xstore/xstore.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_context.h"

using namespace top;
using namespace top::store;
using namespace top::data;

class test_property_by_height : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

xaccount_cmd_ptr_t make_string_cmd(const observer_ptr<xstore_face_t> & store, const std::string & address, const std::string & property_name, const std::string& value) {
    auto account = store->clone_account(address);
    xaccount_cmd_ptr_t cmd;

    int32_t ret;
    if (account == nullptr) {
        account = new xblockchain2_t(address);
        cmd = std::make_shared<xaccount_cmd>(account, store.get());
        ret = cmd->string_create(property_name, true);
    } else {
        cmd = std::make_shared<xaccount_cmd>(account, store.get());
        ret = cmd->string_set(property_name, value, true);
    }
    xassert(ret == 0);
    account->release_ref();
    return cmd;
}

xaccount_cmd_ptr_t create_or_modify_property(const observer_ptr<xstore_face_t> & store, const std::string &address, const std::string& list_name, const std::string& item_value) {
    auto account = store->clone_account(address);
    xaccount_cmd_ptr_t cmd;

    int32_t ret;
    if (account == nullptr) {
        account = new xblockchain2_t(address);
        cmd = std::make_shared<xaccount_cmd>(account, store.get());
        auto ret = cmd->list_create(list_name, true);
        assert(ret == 0);
    } else {
        int32_t error_code;
        cmd = std::make_shared<xaccount_cmd>(account, store.get());
        auto prop_ptr = cmd->get_property(list_name, error_code);

        if (prop_ptr == nullptr) {
            assert(0);
        }
        auto ret = cmd->list_push_back(list_name, item_value, true);
        assert(ret == 0);
    }

    return cmd;
}

TEST_F(test_property_by_height, property_by_height_1) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");

    std::string prop_name = "aaa";
    uint64_t count = 100;

    xblock_t *prev_block = nullptr;
    xblock_t *curr_block = nullptr;
    for (uint64_t i = 0; i <= count; i++) {

        std::string value = std::to_string(i);

        auto cmd = create_or_modify_property(make_observer(store), address, prop_name, value);
        curr_block = datamock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(store->set_vblock(curr_block));
        ASSERT_TRUE(store->execute_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
    }

    std::vector<std::string> values;

    uint64_t height = 8;
    int32_t ret = store->get_list_property(address, height, prop_name, values);
    ASSERT_EQ(ret, xsuccess);

    std::cout << "=====list all @" << height << "=====" << std::endl;
    for (auto & v : values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

TEST_F(test_property_by_height, property_by_height_2) {
    xobject_ptr_t<xstore_face_t> store = xstore_factory::create_store_with_memdb();
    test_datamock_t datamock(store.get());
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string prop_name = "aaa";
    uint64_t count = 100;

    xblock_t *prev_block = nullptr;
    xblock_t *curr_block = nullptr;
    for (uint64_t i = 0; i <= count; i++) {

        std::string value = std::to_string(i);
        xaccount_cmd_ptr_t cmd = nullptr;
        if (i % 3 == 0) {
            cmd = create_or_modify_property(make_observer(store), address, prop_name, value);
        }
        curr_block = datamock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(store->set_vblock(curr_block));
        ASSERT_TRUE(store->execute_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
    }

    std::vector<std::string> values;
    uint64_t height = 16;
    int32_t ret = store->get_list_property(address, height, prop_name, values);
    ASSERT_EQ(ret, xsuccess);
    ASSERT_NE(values.size(), 0);

    std::cout << "=====list all @" << height << "=====" << std::endl;
    for (auto & v : values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;

    height = 86;
    ret = store->get_list_property(address, height, prop_name, values);
    ASSERT_EQ(ret, xsuccess);
    ASSERT_NE(values.size(), 0);

    std::cout << "=====list all @" << height << "=====" << std::endl;
    for (auto & v : values) {
        std::cout << v << " ";
    }
    std::cout << std::endl;
}

TEST_F(test_property_by_height, property_by_height_3) {
    auto store_ptr = xstore_factory::create_store_with_memdb();

    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    test_datamock_t datamock(store_ptr.get());

    uint64_t count = 90;
    std::string name("prop");

    xblock_t *prev_block = nullptr;
    xblock_t *curr_block = nullptr;
    for (uint64_t i = 0; i <= count; i++) {
        std::string value = name + std::to_string(i);
        auto cmd = make_string_cmd(make_observer(store_ptr), address, name, value);
        curr_block = datamock.create_sample_block(prev_block, cmd.get(), address);

        ASSERT_TRUE(store_ptr->set_vblock(curr_block));
        ASSERT_TRUE(store_ptr->execute_block(curr_block));
        ASSERT_EQ(curr_block->get_height(), i);
        uint64_t chainheight = store_ptr->get_blockchain_height(address);
        ASSERT_EQ(chainheight, curr_block->get_height());
        prev_block = curr_block;
    }

    std::string value;

    int32_t ret = store_ptr->get_string_property(address, 3, "non_exist", value);
    ASSERT_NE(ret, xsuccess);
    ASSERT_TRUE(value.empty());

    ret = store_ptr->get_string_property(address, 0, name, value);
    ASSERT_EQ(ret, xsuccess);
    ASSERT_TRUE(value.empty());

    auto& config_register = top::config::xconfig_register_t::get_instance();

    uint32_t fullunit_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);

    uint64_t heights[] = {3, 4, 10, 21, 23, 45, 63, 75};
    for (uint64_t height: heights) {
        ret = store_ptr->get_string_property(address, height, name, value);
        ASSERT_EQ(ret, xsuccess);
        std::string original_value;
        if (height % fullunit_num == 0) {
            original_value = name + std::to_string(height - 1);
        } else {
            original_value = name + std::to_string(height);
        }
        ASSERT_EQ(value, original_value);
    }
}
