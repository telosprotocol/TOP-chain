#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xstore/xstore.h"

#include "test_xstore_data1.h"

using namespace top::store;
using namespace std;

class test_serialize : public testing::Test {
protected:
    void SetUp() override {

    }

    void TearDown() override {

    }
};

TEST_F(test_serialize, msgpack_serialize) {
    string serialize_str;
    {
        xstore_data1 d1;
        test_xstore_data1_set(d1);
        serialize_str = d1.pack();
        cout << "msgpack length " << serialize_str.size() << "value " << serialize_str << endl;
    }

    {
        xstore_data1 d2;
        d2.unpack(serialize_str);
        test_xstore_data1_is_valid(d2);
    }
}

TEST_F(test_serialize, stream_serialize) {
    xstream_t stream(xcontext_t::instance());
    string serialize_str;
    //serialize to
    {
        xstore_data1 d1;
        test_xstore_data1_set(d1);
        auto ret = d1.serialize_to(stream);
        ASSERT_NE(ret, 0);
        serialize_str = string((char*)stream.data(), stream.size());
        cout << "stream pack length " << serialize_str.size() << "value " << serialize_str << endl;
    }
    //serialize from
    {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)serialize_str.data(), serialize_str.size());
        xstore_data1 d2;
        auto ret = d2.serialize_from(stream);
        ASSERT_NE(ret, 0);
        test_xstore_data1_is_valid(d2);
    }
}


TEST_F(test_serialize, msgpack_serialize_2) {
    string serialize_str;
    {
        xstore_data1 d1;
        test_xstore_data1_set_2(d1);
        serialize_str = d1.pack();
        cout << "msgpack length " << serialize_str.size() << "value " << serialize_str << endl;
    }

    {
        xstore_data1 d2;
        d2.unpack(serialize_str);
        test_xstore_data1_is_valid_2(d2);
    }
}

TEST_F(test_serialize, stream_serialize_2) {
    xstream_t stream(xcontext_t::instance());
    string serialize_str;
    //serialize to
    {
        xstore_data1 d1;
        test_xstore_data1_set_2(d1);
        auto ret = d1.serialize_to(stream);
        ASSERT_NE(ret, 0);
        serialize_str = string((char*)stream.data(), stream.size());
        cout << "stream pack length " << serialize_str.size() << "value " << serialize_str << endl;
    }
    //serialize from
    {
        xstream_t stream(xcontext_t::instance(), (uint8_t*)serialize_str.data(), serialize_str.size());
        xstore_data1 d2;
        auto ret = d2.serialize_from(stream);
        ASSERT_NE(ret, 0);
        test_xstore_data1_is_valid_2(d2);
    }
}
