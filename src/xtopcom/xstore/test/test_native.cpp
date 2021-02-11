#include <vector>
#include <iostream>

#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "gtest/gtest.h"
#include "xbase/xcontext.h"
#include "xbasic/xobject_ptr.h"
#include "xdata/xnative_property.h"
#include "xdb/xdb_mem.h"
#include "xdb/xdb_factory.h"
#include "xstore/xstore.h"
#include "xstore/xaccount_context.h"

using namespace std;
using namespace top;
using namespace top::store;

class test_native : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};


TEST_F(test_native, string_0) {
    std::string value = "aaa";
    base::xstream_t stream(base::xcontext_t::instance());
    stream << value;
    std::cout << "size:" << stream.size() << std::endl;
    ASSERT_EQ(stream.size(), 7);
}

TEST_F(test_native, string_01) {
    auto xxx = make_object_ptr<base::xstring_t>();
    xdataobj_ptr_t data = xxx;
    xxx->set("aaa");
    base::xstream_t stream(base::xcontext_t::instance());
    data->do_write(stream);
    std::cout << "size:" << stream.size() << std::endl;
    ASSERT_EQ(stream.size(), 7);
}

TEST_F(test_native, string_1) {
    xnative_property_t native;
    std::string value;
    ASSERT_NE(0, native.native_string_get("aaa", value));
    value = "111";
    ASSERT_EQ(0, native.native_string_set("aaa", value));
    ASSERT_EQ(0, native.native_string_get("aaa", value));
    ASSERT_EQ(value, "111");

    base::xstream_t stream(base::xcontext_t::instance());
    native.serialize_to(stream);
    std::cout << "size:" << stream.size() << std::endl;
    // ASSERT_EQ(stream.size(), 20);

    xnative_property_t native2;
    native2.serialize_from(stream);
    ASSERT_EQ(0, native2.native_string_get("aaa", value));
    ASSERT_EQ(value, "111");
}

TEST_F(test_native, string_2) {
    xnative_property_t native;

    ASSERT_EQ(0, native.native_string_set("aaa", "111"));
    ASSERT_EQ(0, native.native_string_set("bbb", "222"));
    ASSERT_EQ(0, native.native_string_set("ccc", "333"));

    std::string value;
    ASSERT_EQ(0, native.native_string_get("ccc", value));
    ASSERT_EQ(value, "333");

    ASSERT_EQ(native.is_dirty(), true);
    ASSERT_EQ(native.is_empty(), false);

    base::xstream_t stream(base::xcontext_t::instance());
    native.serialize_to(stream);

    xnative_property_t native2;
    native2.serialize_from(stream);
    ASSERT_EQ(0, native2.native_string_get("bbb", value));
    ASSERT_EQ(value, "222");
    ASSERT_EQ(native2.is_dirty(), false);
    ASSERT_EQ(native2.is_empty(), false);
}

TEST_F(test_native, map) {
    xnative_property_t native;
    std::string key1 = "key1";
    std::string key2 = "key2";
    std::string value;
    ASSERT_NE(0, native.native_map_get("aaa", key1, value));
    ASSERT_NE(0, native.native_map_get("aaa", key2, value));

    ASSERT_EQ(0, native.native_map_set("aaa", key1, "111"));
    ASSERT_EQ(0, native.native_map_set("aaa", key2, "222"));
    int32_t size;
    ASSERT_EQ(0, native.native_map_size("aaa", size));
    ASSERT_EQ(size, 2);
    ASSERT_EQ(0, native.native_map_get("aaa", key1, value));
    ASSERT_EQ(value, "111");
    ASSERT_EQ(0, native.native_map_get("aaa", key2, value));
    ASSERT_EQ(value, "222");
}

TEST_F(test_native, deque) {
    xnative_property_t native;
    std::string value;
    ASSERT_EQ(0, native.native_deque_push_back("aaa", "111"));
    ASSERT_EQ(0, native.native_deque_push_back("aaa", "222"));
    ASSERT_EQ(0, native.native_deque_push_back("aaa", "333"));
    ASSERT_EQ(0, native.native_deque_push_back("aaa", "444"));
    ASSERT_EQ(0, native.native_deque_push_back("aaa", "555"));
    int32_t size;
    ASSERT_EQ(0, native.native_deque_size("aaa", size));
    ASSERT_EQ(size, 5);
    ASSERT_EQ(true, native.native_deque_exist("aaa", "111"));
    ASSERT_EQ(false, native.native_deque_exist("aaa", "666"));

    ASSERT_EQ(0, native.native_deque_erase("aaa", "333"));
    ASSERT_EQ(0, native.native_deque_size("aaa", size));
    ASSERT_EQ(size, 4);

    ASSERT_EQ(0, native.native_deque_pop_back("aaa", value));
    ASSERT_EQ(value, "555");
    ASSERT_EQ(0, native.native_deque_pop_back("aaa", value));
    ASSERT_EQ(value, "444");
    ASSERT_EQ(0, native.native_deque_pop_back("aaa", value));
    ASSERT_EQ(value, "222");
    ASSERT_EQ(0, native.native_deque_pop_back("aaa", value));
    ASSERT_EQ(value, "111");
}
