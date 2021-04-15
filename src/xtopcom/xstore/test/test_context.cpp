#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xbase/xcontext.h"
#include "xbase/xobject_ptr.h"
#include "xdata/tests/test_blockutl.hpp"
#include "xdb/xdb_mem.h"
#include "xdb/xdb_factory.h"
#include "xstore/xstore.h"
#include "xstore/xaccount_context.h"
#include "xstore/xstore_error.h"

using namespace top;

static const int config_property_submap_capacity = 256;
static const int config_property_subdeque_capacity = 256;


class test_xstore_context : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_xstore_context, property_string_not_exist)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    store::xtransaction_result_t result;
    account_context->get_transaction_result(result);
    std::string hash = result.m_props["prop1"];
    ASSERT_EQ(hash.empty(), true);

    ret = account_context->string_get("prop1", value);
    ASSERT_NE(ret, 0);

    ret = account_context->string_empty("prop1", empty);
    ASSERT_NE(ret, 0);

    ret = account_context->string_size("prop1", size);
    ASSERT_NE(ret, 0);
}

TEST_F(test_xstore_context, property_string_create)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    ret = account_context->string_create("prop1");
    ASSERT_EQ(ret, 0);
    ret = account_context->string_create("prop1");
    ASSERT_NE(ret, 0);

    store::xtransaction_result_t result;
    account_context->get_transaction_result(result);
    std::string hash = result.m_props["prop1"];
    std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    //ASSERT_EQ(data::to_hex_str(hash), "0000000000000000000000000000000000000000000000000000000000000000");

    ret = account_context->string_get("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value.empty(), true);

    ret = account_context->string_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, true);

    ret = account_context->string_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}


TEST_F(test_xstore_context, property_string_set_get)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    // create property
    ret = account_context->string_create("prop1");
    ASSERT_EQ(ret, 0);

    ret = account_context->string_set("prop1", "bbb");
    ASSERT_EQ(ret, 0);

    ret = account_context->string_get("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "bbb");
    ret = account_context->string_size("prop1", size);
    std::string temp = "bbb";
    ASSERT_EQ(size, temp.size());
    ret = account_context->string_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, false);

    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        std::string hash = result.m_props["prop1"];
        ASSERT_EQ(hash.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    }
}

TEST_F(test_xstore_context, property_string_2)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    ret = account_context->do_prop_set(xproperty_cmd_type_string_create, "prop1");
    ASSERT_EQ(ret, 0);
    ret = account_context->do_prop_set(xproperty_cmd_type_string_set, "prop1", "a");
    ASSERT_EQ(ret, 0);
    ret = account_context->string_get("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "a");
}

TEST_F(test_xstore_context, property_deque_not_exist)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    ret = account_context->list_get_back("prop1", value);
    ASSERT_NE(ret, 0);

    ret = account_context->list_empty("prop1", empty);
    ASSERT_NE(ret, 0);

    ret = account_context->list_size("prop1", size);
    ASSERT_NE(ret, 0);
}

TEST_F(test_xstore_context, property_deque_create)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    ret = account_context->list_create("prop1");
    ASSERT_EQ(ret, 0);
    ret = account_context->list_create("prop1");
    ASSERT_NE(ret, 0);

    store::xtransaction_result_t result;
    account_context->get_transaction_result(result);
    std::string hash = result.m_props["prop1"];
    std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    //ASSERT_EQ(data::to_hex_str(hash), "0000000000000000000000000000000000000000000000000000000000000000");

    ret = account_context->list_get_back("prop1", value);
    ASSERT_NE(ret, 0);

    ret = account_context->list_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, true);

    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}

TEST_F(test_xstore_context, property_deque_push_pop_back)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret = 0;
    std::string value;
    int32_t size;
    bool empty = false;

    ret = account_context->list_create("prop1");
    ASSERT_EQ(ret, 0);
    ret = account_context->list_push_back("prop1", "aa");
    ASSERT_EQ(ret, 0);
    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        std::string hash = result.m_props["prop1"];
        ASSERT_EQ(hash.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    }
    ret = account_context->list_push_back("prop1", "bb");
    ASSERT_EQ(ret, 0);
    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        std::string hash = result.m_props["prop1"];
        ASSERT_EQ(hash.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    }
    ret = account_context->list_get_back("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "bb");
    ret = account_context->list_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, false);
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 2);

    ret = account_context->list_pop_back("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "bb");
    ret = account_context->list_pop_back("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "aa");
    ret = account_context->list_pop_back("prop1", value);
    ASSERT_NE(ret, 0);
    ret = account_context->list_get_back("prop1", value);
    ASSERT_NE(ret, 0);
    ret = account_context->list_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, true);
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}


TEST_F(test_xstore_context, property_deque_push_pop_front)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    int32_t size;
    bool empty = false;
    ret = account_context->list_create("prop1");
    ASSERT_EQ(ret, 0);
    ret = account_context->list_push_front("prop1", "aa");
    ASSERT_EQ(ret, 0);
    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        std::string hash = result.m_props["prop1"];
        ASSERT_EQ(hash.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    }
    ret = account_context->list_push_front("prop1", "bb");
    ASSERT_EQ(ret, 0);
    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        std::string hash = result.m_props["prop1"];
        ASSERT_EQ(hash.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    }
    ret = account_context->list_get_front("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "bb");
    ret = account_context->list_get_back("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "aa");
    ret = account_context->list_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, false);
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 2);

    std::deque<std::string> deque;
    ret = account_context->list_copy_get("prop1", deque);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(deque.size(), 2);
    for (auto & v : deque) {
        std::cout << "deque.value " << v << std::endl;
    }

    ret = account_context->list_pop_front("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "bb");
    ret = account_context->list_pop_front("prop1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "aa");
    ret = account_context->list_pop_front("prop1", value);
    ASSERT_NE(ret, 0);
    ret = account_context->list_get_front("prop1", value);
    ASSERT_NE(ret, 0);
    ret = account_context->list_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, true);
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}

TEST_F(test_xstore_context, property_deque_push_loop_1)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    int32_t size;

    int loopcount = 3;

    ret = account_context->list_create("prop1");
    ASSERT_EQ(ret, 0);

    //base test
    for (int i = 0; i < loopcount; i++)
    {
        for (int j = 0; j < config_property_subdeque_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->list_push_back("prop1", tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->list_get_back("prop1", value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, loopcount*config_property_subdeque_capacity);

    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_subdeque_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->list_pop_front("prop1", value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);

    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_subdeque_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->list_push_front("prop1", tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->list_get_front("prop1", value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, loopcount*config_property_subdeque_capacity);

    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_subdeque_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->list_pop_back("prop1", value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}

TEST_F(test_xstore_context, property_deque_push_loop_2)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    int32_t size;

    ret = account_context->list_create("prop1");
    ASSERT_EQ(ret, 0);

    //one bucket push back and push front
    for (int i=(int)(config_property_subdeque_capacity/2); i<(int)config_property_subdeque_capacity; i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_push_back("prop1", tmp);
        ASSERT_EQ(ret, 0);
    }
    for (int i=(int)config_property_subdeque_capacity/2-1; i>=0; i--)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_push_front("prop1", tmp);
        ASSERT_EQ(ret, 0);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, config_property_subdeque_capacity);
    for (int i=0; i<(int)config_property_subdeque_capacity; i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_get("prop1", i, value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    for (int i=0; i<(int)config_property_subdeque_capacity; i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_pop_front("prop1", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);

    //one bucket push front and push back
    for (int i=(int)config_property_subdeque_capacity/2-1; i>=0; i--)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_push_front("prop1", tmp);
        ASSERT_EQ(ret, 0);
    }
    for (int i=(int)(config_property_subdeque_capacity/2); i<(int)config_property_subdeque_capacity; i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_push_back("prop1", tmp);
        ASSERT_EQ(ret, 0);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, config_property_subdeque_capacity);
    for (int i=0; i<(int)config_property_subdeque_capacity; i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_get("prop1", i, value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    for (int i=0; i<(int)config_property_subdeque_capacity; i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_pop_front("prop1", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}

TEST_F(test_xstore_context, property_deque_push_loop_3)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    int32_t size;

    ret = account_context->list_create("prop1");
    ASSERT_EQ(ret, 0);

    //two bucket push back and pop front
    for (int i=0; i<(int)(2*config_property_subdeque_capacity); i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_push_back("prop1", tmp);
        ASSERT_EQ(ret, 0);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 2*config_property_subdeque_capacity);
    for (int i=0; i<(int)(2*config_property_subdeque_capacity); i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_get("prop1", i, value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    for (int i=0; i<(int)(2*config_property_subdeque_capacity); i++)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_pop_front("prop1", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);

    //two bucket push front and pop back
    for (int i=(int)2*config_property_subdeque_capacity-1; i>=0; i--)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_push_front("prop1", tmp);
        ASSERT_EQ(ret, 0);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 2*config_property_subdeque_capacity);
    for (int i=(int)2*config_property_subdeque_capacity-1; i>=0; i--)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_get("prop1", i, value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    for (int i=(int)2*config_property_subdeque_capacity-1; i>=0; i--)
    {
        char tmp[256] = {0};
        sprintf(tmp, "%d", i);
        ret = account_context->list_pop_back("prop1", value);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(value, tmp);
    }
    ret = account_context->list_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}


TEST_F(test_xstore_context, property_map_not_exist)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    ret = account_context->map_get("prop1", "1", value);
    ASSERT_NE(ret, 0);

    ret = account_context->map_empty("prop1", empty);
    ASSERT_NE(ret, 0);

    ret = account_context->map_size("prop1", size);
    ASSERT_NE(ret, 0);
}

TEST_F(test_xstore_context, property_map_create)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t size;
    int32_t ret;
    std::string value;
    bool empty;

    ret = account_context->map_create("prop1");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_create("prop1");
    ASSERT_NE(ret, 0);

    store::xtransaction_result_t result;
    account_context->get_transaction_result(result);
    std::string hash = result.m_props["prop1"];
    std::cout << "prop1 hash " << data::to_hex_str(hash) << std::endl;
    //ASSERT_EQ(data::to_hex_str(hash), "0000000000000000000000000000000000000000000000000000000000000000");

    ret = account_context->map_get("prop1", "1", value);
    ASSERT_NE(ret, 0);

    ret = account_context->map_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, true);

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}

TEST_F(test_xstore_context, property_map_set_get)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    bool empty = false;
    int32_t size = 0;

    ret = account_context->map_create("prop1");
    ASSERT_EQ(ret, 0);

    ret = account_context->map_set("prop1", "1", "aa");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_get("prop1", "1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "aa");

    ret = account_context->map_set("prop1", "1", "aaa");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_set("prop1", "2", "bbb");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_set("prop1", "3", "ccc");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_get("prop1", "1", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "aaa");
    ret = account_context->map_get("prop1", "2", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "bbb");
    ret = account_context->map_get("prop1", "3", value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "ccc");

    std::map<std::string, std::string> copy_map;
    ret = account_context->map_copy_get("prop1", copy_map);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(copy_map.size(), 3);
    for (auto & v : copy_map) {
        std::cout << " key " << v.first << " value " << v.second << std::endl;
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 3);
    ret = account_context->map_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, false);

    ret = account_context->map_remove("prop1", "1");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 2);
    ret = account_context->map_remove("prop1", "2");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_remove("prop1", "3");
    ASSERT_EQ(ret, 0);
    ret = account_context->map_empty("prop1", empty);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(empty, true);
    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);
}

TEST_F(test_xstore_context, property_map_set_get_loop)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;
    int32_t size = 0;
    int loopcount = 3;
    ret = account_context->map_create("prop1");
    ASSERT_EQ(ret, 0);
    //insert
    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_submap_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->map_set("prop1", tmp, tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->map_size("prop1", size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(size, i*config_property_submap_capacity + j + 1);
            ret = account_context->map_get("prop1", tmp, value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, loopcount*config_property_submap_capacity);

    //remove
    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_submap_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->map_remove("prop1", tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->map_size("prop1", size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(size, loopcount*config_property_submap_capacity - (i*config_property_submap_capacity + j + 1));
        }
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);

    //insert
    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_submap_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->map_set("prop1", tmp, tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->map_size("prop1", size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(size, i*config_property_submap_capacity + j + 1);
            ret = account_context->map_get("prop1", tmp, value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, loopcount*config_property_submap_capacity);

    //remove
    for (int i=0; i<loopcount; i++)
    {
        for (int j=0; j<(int)config_property_submap_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->map_remove("prop1", tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->map_size("prop1", size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(size, loopcount*config_property_submap_capacity - (i*config_property_submap_capacity + j + 1));
        }
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);

    //reversed insert
    for (int i=loopcount-1; i>=0; i--)
    {
        for (int j=0; j<(int)config_property_submap_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->map_set("prop1", tmp, tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->map_size("prop1", size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(size, (loopcount-i-1)*config_property_submap_capacity + j + 1);
            ret = account_context->map_get("prop1", tmp, value);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(value, tmp);
        }
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, loopcount*config_property_submap_capacity);

    //remove
    for (int i=loopcount-1; i>=0; i--)
    {
        for (int j=0; j<(int)config_property_submap_capacity; j++)
        {
            char tmp[256] = {0};
            sprintf(tmp, "%d%d", i, j);
            ret = account_context->map_remove("prop1", tmp);
            ASSERT_EQ(ret, 0);
            ret = account_context->map_size("prop1", size);
            ASSERT_EQ(ret, 0);
            ASSERT_EQ(size, loopcount*config_property_submap_capacity - ((loopcount-i-1)*config_property_submap_capacity + j + 1));
        }
    }

    ret = account_context->map_size("prop1", size);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(size, 0);

}

TEST_F(test_xstore_context, property_map_hash)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    bool bret;
    std::string hash1;
    std::string hash2;
    std::string hash3;
    ret = account_context->map_create("prop1");
    ASSERT_EQ(ret, 0);

    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        hash1 = result.m_props["prop1"];
        ASSERT_EQ(hash1.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash1) << std::endl;
    }

    ret = account_context->map_set("prop1", "1", "aa");
    ASSERT_EQ(ret, 0);
    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        hash2 = result.m_props["prop1"];
        ASSERT_EQ(hash2.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash2) << std::endl;
    }

    ret = account_context->map_set("prop1", "1", "bb");
    ASSERT_EQ(ret, 0);
    {
        store::xtransaction_result_t result;
        account_context->get_transaction_result(result);
        hash3 = result.m_props["prop1"];
        ASSERT_EQ(hash3.empty(), false);
        std::cout << "prop1 hash " << data::to_hex_str(hash3) << std::endl;
    }

    ASSERT_NE(hash1, hash2);
    ASSERT_NE(hash1, hash3);
    ASSERT_NE(hash2, hash3);
}

TEST_F(test_xstore_context, property_code)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    int32_t ret;
    std::string value;

    ret = account_context->set_contract_code("code");
    ASSERT_EQ(ret, 0);

    ret = account_context->get_contract_code(value);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(value, "code");
}

TEST_F(test_xstore_context, DISABLED_too_long_action)
{
    xobject_ptr_t<store::xstore_face_t> store = store::xstore_factory::create_store_with_memdb();
    std::string address = xblocktool_t::make_address_user_account("11111111111111111111");
    std::string tgtaddress = xblocktool_t::make_address_user_account("11111111111111111112");
    auto account_context = std::make_shared<store::xaccount_context_t>(address, store.get());

    char ss[65500];
    for (int32_t i = 0; i < 65499; i++) {
        ss[i] = 'a';
    }
    std::string func_param(ss);

    int32_t ret = account_context->generate_tx(tgtaddress, "test_func_name", func_param);
    ASSERT_EQ(ret, store::xtransaction_param_invalid);
}
