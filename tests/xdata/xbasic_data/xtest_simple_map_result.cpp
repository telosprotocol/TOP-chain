#include "xbasic/xcodec/xmsgpack/xsimple_map_result_codec.hpp"
#include "xbasic/xsimple_map_result.hpp"
#include "xcodec/xmsgpack_codec.hpp"

#include <gtest/gtest.h>

#include <string>

using xtest_simple_map_t = top::xsimple_map_result_t<std::string, std::string>;

TEST(xdata_test_simple_map, test_insert) {
    xtest_simple_map_t m;
    EXPECT_TRUE(m.size() == 0 && m.empty());
    m.insert(std::make_pair("key1", "value1"));
    EXPECT_TRUE(m.size() == 1);

    EXPECT_FALSE(m.insert(std::make_pair("key1", "value1")).second);
    EXPECT_TRUE(m.size() == 1);

    EXPECT_FALSE(m.insert(std::make_pair("key1", "value2")).second);
    EXPECT_TRUE(m.size() == 1);

    EXPECT_TRUE(m.insert(std::make_pair("key2", "value2")).second);
    EXPECT_TRUE(m.size() == 2);
}

TEST(xdata_test_simple_map, test_update) {
    xtest_simple_map_t m;
    EXPECT_TRUE(m.size() == 0 && m.empty());

    EXPECT_TRUE(m.update(std::make_pair("key1", "value1")).second);
    EXPECT_TRUE(m.size() == 1);

    EXPECT_FALSE(m.update(std::make_pair("key1", "value1")).second);
    EXPECT_TRUE(m.size() == 1);
    EXPECT_TRUE(m.result_of("key1") == "value1");

    EXPECT_TRUE(m.update(std::make_pair("key1", "value2")).second);
    EXPECT_TRUE(m.size() == 1);
    EXPECT_TRUE(m.result_of("key1") == "value2");

    EXPECT_TRUE(m.update(std::make_pair("key2", "value2")).second);
    EXPECT_TRUE(m.size() == 2);
    EXPECT_TRUE(m.result_of("key2") == "value2");
}

TEST(xdata_test_simple_map, test_serde) {
    xtest_simple_map_t m;
    m.insert(std::make_pair("key1", "value1"));

    auto bytes = top::codec::msgpack_encode(m);
    std::string object = {std::begin(bytes), std::end(bytes)};

    xtest_simple_map_t res = top::codec::msgpack_decode<xtest_simple_map_t>({object.begin(), object.end()});
    EXPECT_TRUE(res.result_of("key1") == "value1");
}