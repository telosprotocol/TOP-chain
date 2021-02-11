#include <vector>
#include <iostream>

#include "gtest/gtest.h"
#include "xbase/xint.h"
#include "xbase/xmem.h"
#include "xbase/xcontext.h"
#include "xbase/xutl.h"

using namespace top;

class test_xbitmap : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_xbitmap, bitmap_set_get) {
    uint256_t value;
    for (size_t i = 0; i < 256; i++) {
        ASSERT_FALSE(value.bit_is_set(i));
    }

    value.bit_set(0);
    std::cout << base::xstring_utl::to_hex(std::string((const char*)value.data(), value.size())) << std::endl;
    std::cout << value.raw_uint32[0] << std::endl;
    value.bit_clear(0);

    value.bit_set(1);
    std::cout << base::xstring_utl::to_hex(std::string((const char*)value.data(), value.size())) << std::endl;
    std::cout << value.raw_uint32[0] << std::endl;
    value.bit_clear(1);

    value.bit_set(2);
    std::cout << base::xstring_utl::to_hex(std::string((const char*)value.data(), value.size())) << std::endl;
    std::cout << value.raw_uint32[0] << std::endl;
    value.bit_clear(2);

    value.bit_set(255);
    std::cout << base::xstring_utl::to_hex(std::string((const char*)value.data(), value.size())) << std::endl;
    std::cout << value.raw_uint32[7] << std::endl;
    value.bit_clear(255);

    value.bit_set(20);
    value.bit_set(50);
    std::cout << base::xstring_utl::to_hex(std::string((const char*)value.data(), value.size())) << std::endl;

    for (size_t i = 0; i < 256; i++) {
        if (i == 20) {
            ASSERT_TRUE(value.bit_is_set(i));
        } else if (i == 50) {
            ASSERT_TRUE(value.bit_is_set(i));
        } else {
            ASSERT_FALSE(value.bit_is_set(i));
        }
    }

    value.bit_clear(20);
    value.bit_clear(50);

    for (size_t i = 0; i < 256; i++) {
        ASSERT_FALSE(value.bit_is_set(i));
    }
}


