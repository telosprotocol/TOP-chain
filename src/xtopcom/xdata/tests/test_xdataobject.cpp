#include <vector>
#include <stdio.h>

#include "gtest/gtest.h"
#include "xbase/xdata.h"
#include "xbase/xutl.h"
#include "xdata/xhash_base.hpp"
#include "xdata/xdata_common.h"

using namespace std;
using namespace top;
using namespace top::base;
using namespace top::data;

class test_xdataobject : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xdataobject, string_1) {
    xstring_ptr_t mystr1 = make_object_ptr<base::xstring_t>();
    string hash = xhash_base_t::calc_dataunit_hash(mystr1.get());
    std::cout << "hash1 " << xstring_utl::to_hex(hash) << std::endl;

    auto hash1 = xhash_base_t::calc_dataunit_hash(mystr1.get());
    ASSERT_EQ(mystr1->get_obj_version(), 0);
    mystr1->set("1111");
    auto hash2 = xhash_base_t::calc_dataunit_hash(mystr1.get());
    ASSERT_EQ(mystr1->get_obj_version(), 0);
    mystr1->set("2222");
    ASSERT_EQ(mystr1->get_obj_version(), 0);
    auto hash3 = xhash_base_t::calc_dataunit_hash(mystr1.get());
    mystr1->set("2222");
    ASSERT_EQ(mystr1->get_obj_version(), 0);
    auto hash4 = xhash_base_t::calc_dataunit_hash(mystr1.get());
    ASSERT_NE(hash1, hash2);
    ASSERT_NE(hash2, hash3);
    ASSERT_EQ(hash3, hash4);
}

TEST_F(test_xdataobject, string_2) {
    xstring_ptr_t mystr1 = make_object_ptr<base::xstring_t>();
    mystr1->set("3333");

    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, mystr1->serialize_to(stream));
    ASSERT_EQ(mystr1->get_obj_version(), 0);

    xstring_ptr_t mystr2 = make_object_ptr<base::xstring_t>();
    ASSERT_NE(0, mystr2->serialize_from(stream));

    ASSERT_EQ(mystr2->get(), "3333");
    ASSERT_EQ(mystr2->get(), mystr1->get());
    ASSERT_EQ(xhash_base_t::calc_dataunit_hash(mystr2.get()), xhash_base_t::calc_dataunit_hash(mystr1.get()));
    ASSERT_EQ(mystr2->get_obj_version(), mystr1->get_obj_version());

    mystr1->clear();
    ASSERT_EQ(mystr1->get(), "");
}


// test verion change
TEST_F(test_xdataobject, deque_1) {

    xstrdeque_ptr_t deque1 = make_object_ptr<base::xstrdeque_t>();

    ASSERT_EQ(deque1->get_obj_version(), 0);
    ASSERT_EQ(true, deque1->push_back("111"));
    ASSERT_EQ(deque1->get_obj_version(), 0);

    string value;
    ASSERT_EQ(true, deque1->pop_front(value));
    ASSERT_EQ(value, "111");
    ASSERT_EQ(deque1->get_obj_version(), 0);
}

TEST_F(test_xdataobject, deque_2) {

    xstrdeque_ptr_t deque1 = make_object_ptr<base::xstrdeque_t>();
    size_t count = 48;
    string hash = xhash_base_t::calc_dataunit_hash(deque1.get());

    for(size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(true, deque1->push_back(to_string(i)));
        ASSERT_NE(hash, xhash_base_t::calc_dataunit_hash(deque1.get()));
        hash = xhash_base_t::calc_dataunit_hash(deque1.get());
    }

    string value;
    for(size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(true, deque1->pop_front(value));
        ASSERT_EQ(value, to_string(i));
        ASSERT_NE(hash, xhash_base_t::calc_dataunit_hash(deque1.get()));
        hash = xhash_base_t::calc_dataunit_hash(deque1.get());
    }
}

TEST_F(test_xdataobject, deque_3) {

    xstrdeque_ptr_t deque1 = make_object_ptr<base::xstrdeque_t>();
    size_t count = 48;
    for(size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(true, deque1->push_back(to_string(i)));
    }

    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, deque1->serialize_to(stream));
    ASSERT_EQ(deque1->get_obj_version(), 0);

    xstrdeque_ptr_t deque2 = make_object_ptr<base::xstrdeque_t>();
    ASSERT_NE(0, deque2->serialize_from(stream));

    ASSERT_EQ(deque2->get_obj_version(), deque1->get_obj_version());
    ASSERT_EQ(xhash_base_t::calc_dataunit_hash(deque2.get()), xhash_base_t::calc_dataunit_hash(deque1.get()));

    string value;
    for(size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(true, deque2->pop_front(value));
        ASSERT_EQ(value, to_string(i));
    }
}

TEST_F(test_xdataobject, zero_test_xstrdeque_t_1) {
    xobject_ptr_t<xstrdeque_t> obj1 = make_object_ptr<xstrdeque_t>();
    string key = "name";
    string test_para = std::string(10, 0) + std::string(10, 1);
    ASSERT_EQ(test_para.size(), 20);
    {
        string value = test_para;
        bool ret = obj1->push_back(value);
        ASSERT_EQ(ret, true);
    }
    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, obj1->serialize_to(stream));
    std::cout << "serialize size " << stream.size() << std::endl;
    xobject_ptr_t<xstrdeque_t> obj2 = make_object_ptr<xstrdeque_t>();
    ASSERT_NE(0, obj2->serialize_from(stream));
    {
        string value;
        bool ret = obj2->get(0, value);
        ASSERT_EQ(ret, true);
        ASSERT_EQ(value, test_para);
    }
}

TEST_F(test_xdataobject, zero_test_xstring_t_1) {
    xobject_ptr_t<xstring_t> obj1 = make_object_ptr<xstring_t>();
    string key = "name";
    string test_para = std::string(10, 0) + std::string(10, 1);
    ASSERT_EQ(test_para.size(), 20);
    {
        string value = test_para;
        obj1->set(value);
    }
    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, obj1->serialize_to(stream));
    std::cout << "serialize size " << stream.size() << std::endl;
    xobject_ptr_t<xstring_t> obj2 = make_object_ptr<xstring_t>();
    ASSERT_NE(0, obj2->serialize_from(stream));
    {
        string value;
        bool ret = obj2->get(value);
        ASSERT_EQ(ret, true);
        ASSERT_EQ(value, test_para);
    }
}

TEST_F(test_xdataobject, zero_test_xstrmap_t_1) {
    xobject_ptr_t<xstrmap_t> obj1 = make_object_ptr<xstrmap_t>();
    string key = "name";
    string test_para = std::string(10, 0) + std::string(10, 1);
    ASSERT_EQ(test_para.size(), 20);
    {
        string value = test_para;
        obj1->set(key, value);
    }
    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, obj1->serialize_to(stream));
    std::cout << "serialize size " << stream.size() << std::endl;
    xobject_ptr_t<xstrmap_t> obj2 = make_object_ptr<xstrmap_t>();
    ASSERT_NE(0, obj2->serialize_from(stream));
    {
        string value;
        bool ret = obj2->get(key, value);
        ASSERT_EQ(ret, true);
        ASSERT_EQ(value, test_para);
    }
}


TEST_F(test_xdataobject, map_1) {

    xstrmap_ptr_t map1 = make_object_ptr<base::xstrmap_t>();
    ASSERT_EQ(map1->get_obj_version(), 0);
    map1->set("1", "111");
    ASSERT_EQ(map1->get_obj_version(), 0);

    string value;
    ASSERT_EQ(true, map1->get("1", value));
    ASSERT_EQ(value, "111");
    ASSERT_EQ(map1->get_obj_version(), 0);
}

TEST_F(test_xdataobject, map_2) {

    xstrmap_ptr_t map1 = make_object_ptr<base::xstrmap_t>();
    string hash = xhash_base_t::calc_dataunit_hash(map1.get());

    size_t count = 48;
    for(size_t i = 0; i < count; i++)
    {
        map1->set(to_string(i), to_string(i));
        ASSERT_NE(hash, xhash_base_t::calc_dataunit_hash(map1.get()));
        hash = xhash_base_t::calc_dataunit_hash(map1.get());
    }
    ASSERT_EQ(map1->get_obj_version(), 0);

    string value;
    for(size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(true, map1->get(to_string(i), value));
        ASSERT_EQ(value, to_string(i));
    }
    ASSERT_EQ(map1->get_obj_version(), 0);
}

TEST_F(test_xdataobject, map_3) {

    xstrmap_ptr_t map1 = make_object_ptr<base::xstrmap_t>();

    size_t count = 48;
    for(size_t i = 0; i < count; i++)
    {
        map1->set(to_string(i), to_string(i));
    }

    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, map1->serialize_to(stream));
    ASSERT_EQ(map1->get_obj_version(), 0);

    xstrmap_ptr_t map2 = make_object_ptr<base::xstrmap_t>();
    ASSERT_NE(0, map2->serialize_from(stream));

    ASSERT_EQ(map2->get_obj_version(), map1->get_obj_version());
    ASSERT_EQ(xhash_base_t::calc_dataunit_hash(map2.get()), xhash_base_t::calc_dataunit_hash(map1.get()));

    string value;
    for(size_t i = 0; i < count; i++)
    {
        ASSERT_EQ(true, map2->get(to_string(i), value));
        ASSERT_EQ(value, to_string(i));
    }
}

TEST_F(test_xdataobject, xdataobject_map_iter) {
    xstrmap_ptr_t map1 = make_object_ptr<base::xstrmap_t>();
    map1->set("111", "aaa");
    map1->set("222", "bbb");
    map1->set("333", "ccc");
    map1->set("444", "ddd");
    std::string value;
    map1->get("111", value);
    ASSERT_EQ(value, "aaa");

    xstream_t stream(xcontext_t::instance());
    ASSERT_NE(0, map1->serialize_to(stream));
    std::cout << "serialize size " << stream.size() << std::endl;

    xstrmap_ptr_t map2 = make_object_ptr<base::xstrmap_t>();
    ASSERT_NE(0, map2->serialize_from(stream));
    map2->get("111", value);
    ASSERT_EQ(value, "aaa");
}
