#include <vector>

#include "gtest/gtest.h"


#include "xbase/xcontext.h"
#include "xbase/xdata.h"
#include "xbase/xobject.h"
#include "xbase/xmem.h"

#include "test_xstore_data1.h"
#include "xstore/xstore.h"
#include "xdb/xdb.h"

using namespace top;
using namespace top::db;
using namespace top::base;
using namespace top::store;
using namespace std;

std::string DB_NAME  = "./db_xstore_test";

bool g_is_register = false;
class test_xdataobj : public testing::Test {
protected:
    void SetUp() override {
        if(!g_is_register)
        {
            g_is_register = true;
            xcontext_t::register_xobject((enum_xobject_type)xstore_data1::enum_obj_type, xstore_data1::create_object);
        }
    }

    void TearDown() override {
    }
};

xobject_t* create_xstore_data1(int type)
{
    (void)type;
    return new xstore_data1;
}

void test_xstore_data1_set(xstore_data1 & d1)
{
    d1.m_account = "abcdefg";
    d1.m_data_1 = 0x11223344;
    d1.m_data_2 = 0x55667788;
    d1.m_data_3 = 0xaabb;
    d1.m_data_4 = "123456";
    d1.add_modified_count();
}

void test_xstore_data1_is_valid(xstore_data1 & d1)
{
    ASSERT_EQ(d1.m_account, "abcdefg");
    ASSERT_EQ(d1.m_data_1, 0x11223344);
    ASSERT_EQ(d1.m_data_2, 0x55667788);
    ASSERT_EQ(d1.m_data_3, 0xaabb);
    ASSERT_EQ(d1.m_data_4, "123456");
    ASSERT_EQ(d1.get_last_error(), 0);
}

void test_xstore_data1_set_2(xstore_data1 & d1)
{
    d1.m_account = "abcdefg";
    d1.m_data_1 = 0x1122;
    d1.m_data_2 = 0x5566;
    d1.m_data_3 = 0xaa;
    d1.m_data_4 = "123456";
    d1.add_modified_count();
}

void test_xstore_data1_is_valid_2(xstore_data1 & d1)
{
    ASSERT_EQ(d1.m_account, "abcdefg");
    ASSERT_EQ(d1.m_data_1, 0x1122);
    ASSERT_EQ(d1.m_data_2, 0x5566);
    ASSERT_EQ(d1.m_data_3, 0xaa);
    ASSERT_EQ(d1.m_data_4, "123456");
    ASSERT_EQ(d1.get_last_error(), 0);
}

TEST_F(test_xdataobj, serialize_to_and_from) {
    xstream_t stream(xcontext_t::instance());
    //serialize to
    {
        xstore_data1 d1;
        test_xstore_data1_set(d1);
        auto ret = d1.serialize_to(stream);
        ASSERT_NE(ret, 0);
    }
    //serialize from
    {
        xstore_data1 d1;
        auto ret = d1.serialize_from(stream);
        ASSERT_NE(ret, 0);
        test_xstore_data1_is_valid(d1);
    }
}

TEST_F(test_xdataobj, object_serialize_with_stream) {
    xstream_t stream(xcontext_t::instance());
    //serialize to
    {
        std::shared_ptr<xstore_data1> d1 = std::make_shared<xstore_data1>();
        test_xstore_data1_set(*d1);
        auto ret = d1->serialize_to(stream);
        ASSERT_NE(ret, 0);
    }
    //serialize from
    {
        xdataobj_t* obj = xdataobj_t::read_from(stream);
        ASSERT_NE(obj, nullptr);
        xstore_data1* obj2 = (xstore_data1*)obj;
        ASSERT_NE(obj2, nullptr);
        ASSERT_EQ(obj->get_obj_type(), xstore_data1::enum_obj_type);
        xstore_data1* obj3 = (xstore_data1*)obj->query_interface(obj->get_obj_type());
        ASSERT_NE(obj3, nullptr);

        test_xstore_data1_is_valid(*obj2);
        ASSERT_EQ(1, obj->get_refcount());
        obj->release_ref();
    }
}


TEST_F(test_xdataobj, string_object_serialize_with_stream) {
    xstream_t stream(xcontext_t::instance());
    //serialize to
    {
        xstring_t * obj = new xstring_t;
        obj->set("11111111");
        obj->serialize_to(stream);
    }
    //serialize from
    {
        xdataobj_t* obj = xdataobj_t::read_from(stream);
        ASSERT_NE(obj, nullptr);
        xstring_t* obj2 = (xstring_t*)obj;
        ASSERT_NE(obj2, nullptr);
        xstring_t* obj3 = (xstring_t*)obj->query_interface(obj->get_obj_type());
        ASSERT_NE(obj3, nullptr);

        string value;
        ASSERT_TRUE(obj2->get(value));
        ASSERT_EQ(value, "11111111");
        ASSERT_EQ(1, obj->get_refcount());
        obj->release_ref();
    }
}

TEST_F(test_xdataobj, object_serialize_with_string) {
    string value;
    //serialize to
    {
        xstream_t stream(xcontext_t::instance());
        std::shared_ptr<xstore_data1> d1 = std::make_shared<xstore_data1>();
        test_xstore_data1_set(*d1);

        auto ret = d1->serialize_to(stream);
        ASSERT_NE(ret, 0);
        value = string((char*)stream.data(), stream.size());
    }
    //serialize from
    {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)value.data(), value.size());
        xdataobj_t* obj = xdataobj_t::read_from(stream);
        ASSERT_NE(obj, nullptr);

        xstore_data1* obj2 = (xstore_data1*)obj->query_interface(obj->get_obj_type());
        ASSERT_NE(obj2, nullptr);

        test_xstore_data1_is_valid(*obj2);
        ASSERT_EQ(1, obj->get_refcount());
        obj->release_ref();
    }
}

TEST_F(test_xdataobj, object_serialize_with_db) {
    xdb db(DB_NAME);
    std::shared_ptr<xstore_data1> d1 = std::make_shared<xstore_data1>();
    test_xstore_data1_set(*d1);
    //serialize to
    {
        xstream_t stream(xcontext_t::instance());
        auto ret = d1->serialize_to(stream);
        ASSERT_NE(ret, 0);
        db.write(d1->m_account, (char*)stream.data(), stream.size());
    }
    //serialize from
    {
        string value;
        auto ret1 = db.read(d1->m_account, value);
        ASSERT_TRUE(ret1);
        xstream_t stream(xcontext_t::instance(), (uint8_t*)value.data(), value.size());

        xdataobj_t* obj = xdataobj_t::read_from(stream);
        ASSERT_NE(obj, nullptr);
        xstore_data1* obj2 = (xstore_data1*)obj->query_interface(obj->get_obj_type());
        ASSERT_NE(obj2, nullptr);
        test_xstore_data1_is_valid(*obj2);
        ASSERT_EQ(1, obj->get_refcount());
        obj->release_ref();
    }
    xdb::destroy(DB_NAME);
    std::string cleanup = "rm -rf " + DB_NAME;
    system(cleanup.c_str());
}
