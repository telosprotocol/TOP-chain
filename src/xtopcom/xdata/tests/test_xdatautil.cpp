#include <map>
#include "gtest/gtest.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xdata/xdata_defines.h"
#include "xbasic/xobject_ptr.h"
#include "xdata/xblock.h"
#include "xdata/xproperty.h"
#include "xdata/xdatautil.h"
#include "xutility/xhash.h"

using namespace top;
using namespace top::data;

class test_xdatautil : public testing::Test {
 protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xdatautil, native_property_check) {
    ASSERT_TRUE(xnative_property_name_t::is_native_property("@1"));
    ASSERT_TRUE(xnative_property_name_t::is_native_property("@11111"));
    ASSERT_FALSE(xnative_property_name_t::is_native_property("111"));
    ASSERT_TRUE(xnative_property_name_t::is_native_property("@@"));
}

TEST_F(test_xdatautil, digest_xsha2_256) {
    {
        std::string input = "hello world";
        uint256_t output = utl::xsha2_256_t::digest(input.c_str(), input.size());
        // std::cout << "input: " << input << std::endl;
        // std::cout << "output: " << to_hex_str(output) << std::endl;
        ASSERT_EQ("b94d27b9934d3e08a52e52d7da7dabfac484efe37a5380ee9088f7ace2efcde9", to_hex_str(output));
    }
    {
        std::string input = "123456";
        uint256_t output = utl::xsha2_256_t::digest(input.c_str(), input.size());
        // std::cout << "input: " << input << std::endl;
        // std::cout << "output: " << to_hex_str(output) << std::endl;
        ASSERT_EQ("8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92", to_hex_str(output));
    }
    {
        std::string input = "0";
        uint256_t output = utl::xsha2_256_t::digest(input.c_str(), input.size());
        // std::cout << "input: " << input << std::endl;
        // std::cout << "output: " << to_hex_str(output) << std::endl;
        ASSERT_EQ("5feceb66ffc86f38d952786c6d696c79c2dbc239dd4e91b46729d73a27fb57e9", to_hex_str(output));
    }
}
