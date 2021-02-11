#include <vector>

#include "gtest/gtest.h"

#include "xstore/xstore.h"
#include "xstore/xstore_face.h"
#include "xstore/xaccount_cmd.h"
#include "xdb/xdb.h"
#include "xdata/xblockchain.h"
#include "xdata/xpropertylog.h"
#include "xdata/xblock.h"
#include "xdata/xlightunit.h"
#include "xdata/xblocktool.h"

#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"

#include "test_datamock.hpp"

using namespace top::base;
using namespace top::store;
using namespace top::data;
using namespace top;
using namespace std;

class test_accountcmd : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_accountcmd, make_log_and_exec_string_1) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.string_create("aaa", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::string value;
        ret = accountcmd2.string_get("aaa", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, "");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_string_2) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.string_create("aaa", true);
        ASSERT_EQ(ret, 0);

        ret = accountcmd1.string_set("aaa", "1111", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::string value;
        ret = accountcmd2.string_get("aaa", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, "1111");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_string_3) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.string_create("aaa", true);
        ASSERT_EQ(ret, 0);

        ret = accountcmd1.string_set("aaa", "1111", true);
        ASSERT_EQ(ret, 0);

        ret = accountcmd1.string_set("aaa", "2222", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 2);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::string value;
        ret = accountcmd2.string_get("aaa", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, "2222");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_list_1) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.list_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "1111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "2222", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "3333", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 4);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::vector<std::string> values;
        ret = accountcmd2.list_get_all("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values.size(), 3);
        ASSERT_EQ(values[0], "1111");
        ASSERT_EQ(values[1], "2222");
        ASSERT_EQ(values[2], "3333");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_list_2) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.list_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "1111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "2222", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "3333", true);
        ASSERT_EQ(ret, 0);
        std::string temp;
        ret = accountcmd1.list_pop_back("aaa", temp, true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 3);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::vector<std::string> values;
        ret = accountcmd2.list_get_all("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values.size(), 2);
        ASSERT_EQ(values[0], "1111");
        ASSERT_EQ(values[1], "2222");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_list_3) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.list_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "1111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "2222", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_push_back("aaa", "3333", true);
        ASSERT_EQ(ret, 0);
        std::string temp;
        ret = accountcmd1.list_pop_back("aaa", temp, true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.list_clear("aaa", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 2);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::vector<std::string> values;
        ret = accountcmd2.list_get_all("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values.size(), 0);
    }
}

TEST_F(test_accountcmd, make_log_and_exec_map_1) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.map_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "1", "1111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "2", "2222", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "3", "3333", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 4);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::map<std::string, std::string> values;
        ret = accountcmd2.map_copy_get("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values.size(), 3);
        ASSERT_EQ(values["1"], "1111");
        ASSERT_EQ(values["2"], "2222");
        ASSERT_EQ(values["3"], "3333");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_map_2) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.map_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "1", "1111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "2", "2222", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "3", "3333", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_remove("aaa", "2", true);
        ASSERT_EQ(ret, 0);

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 3);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::map<std::string, std::string> values;
        ret = accountcmd2.map_copy_get("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values.size(), 2);
        ASSERT_EQ(values["1"], "1111");
        ASSERT_EQ(values["3"], "3333");
    }
}

TEST_F(test_accountcmd, make_log_and_exec_map_3) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    {
        auto account = new xblockchain2_t(address);
        xaccount_cmd accountcmd1(account, store_face.get());
        auto ret = accountcmd1.map_create("aaa", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "1", "1111", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "2", "2222", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_set("aaa", "3", "3333", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_remove("aaa", "2", true);
        ASSERT_EQ(ret, 0);
        ret = accountcmd1.map_clear("aaa", true);
        ASSERT_EQ(ret, 0);
        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_NE(binlog, nullptr);
        ASSERT_EQ(binlog->get_instruction_size(), 2);
        accountcmd1.get_change_property();

        xaccount_cmd accountcmd2(account, store_face.get());
        ret = accountcmd2.do_property_log(binlog);
        ASSERT_EQ(ret, 0);
        std::map<std::string, std::string> values;
        ret = accountcmd2.map_copy_get("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values.size(), 0);
    }
}

TEST_F(test_accountcmd, property_log_serialize) {
    xobject_ptr_t<xstore_face_t> store_face = xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string to_account = "T-to";
    {
        auto account = store_face->clone_account(address);
        if (account == nullptr) {
            base::xvblock_t* genesis_block = data::xblocktool_t::create_genesis_empty_unit(address);
            ASSERT_TRUE(store_face->set_vblock(genesis_block));
            ASSERT_TRUE(store_face->execute_block(genesis_block));
            account = store_face->clone_account(address);
            ASSERT_TRUE(account != nullptr);
        }

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
        test_datamock_t mock(store_face.get());
        auto block = mock.create_sample_block(&accountcmd1, address);
        block->set_block_flag(base::enum_xvblock_flag_connected);

        data::xlightunit_block_t* b = dynamic_cast<data::xlightunit_block_t*>(block);
        std::cout << "light unit property size: " << b->get_property_hash_map().size() << std::endl;

        ASSERT_TRUE(store_face->set_vblock(block));
        ASSERT_TRUE(store_face->execute_block(block));

        ASSERT_EQ(ret, 0);

        std::vector<std::string> values;
        ret = store_face->list_get_all(address, "aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values[0], "111");
        ASSERT_EQ(values[1], "222");
    }

    {
        auto account = store_face->clone_account(address);
        ASSERT_NE(account, nullptr);
        xaccount_cmd accountcmd1(account, store_face.get());
        std::vector<std::string> values;
        auto ret = accountcmd1.list_get_all("aaa", values);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(values[0], "111");
        ASSERT_EQ(values[1], "222");

        xproperty_log_ptr_t binlog = accountcmd1.get_property_log();
        ASSERT_EQ(binlog, nullptr);

        auto change_property_obj = accountcmd1.get_change_property();
        ASSERT_EQ(change_property_obj.size(), 0);

        auto prop_hashs = accountcmd1.get_property_hash();
        ASSERT_EQ(prop_hashs.size(), 0);
    }
}

TEST_F(test_accountcmd, property_copy) {
    xstrdeque_ptr_t from = make_object_ptr<base::xstrdeque_t>();
    xstrdeque_ptr_t to = make_object_ptr<base::xstrdeque_t>();

    from->push_back("aaa");
    from->push_back("bbb");
    to->push_back("ccc");

    xstream_t stream(xcontext_t::instance());
    auto ret = from->serialize_to(stream);
    assert(0 != ret);

    to->clear();
    ret = to->serialize_from(stream);
    assert(0 != ret);

    std::vector<std::string> values;
    int32_t size = to->size();
    for (int32_t i = 0; i < size; i++) {
        std::string value;
        to->get(i, value);
        std::cout << value << std::endl;
        values.push_back(value);
    }

    ASSERT_EQ(size, 2);
    ASSERT_EQ(values[0], "aaa");
    ASSERT_EQ(values[1], "bbb");
}
