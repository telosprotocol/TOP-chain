#include <memory>

#include "gtest/gtest.h"
#include "xrpc/xuint_format.h"

TEST(test_uint, hex_to_uint64_t) {
    std::string hex = "0xb4ae99d7575b481";
    std::string save_hex(hex);
    auto ret = top::xrpc::hex_to_uint64(hex);
    EXPECT_EQ(ret, 813719545183581313);
    auto str = top::xrpc::uint64_to_str(ret);
    EXPECT_EQ(str, save_hex);
    std::string hex1 = "0x0b4ae99d7575b481";
    auto ret2 = top::xrpc::hex_to_uint64(hex1);
    EXPECT_EQ(ret2, 813719545183581313);

    hex1 = "0b4ae99d7575b481";
    ret2 = top::xrpc::hex_to_uint64(hex1);
    EXPECT_EQ(ret2, 0);
}

TEST(test_uint, hex_to_dec) {
    char c('B');
    auto ic = top::xrpc::hex_to_dec(c);
    EXPECT_EQ(ic, 11);
    c = '-';
    ic = top::xrpc::hex_to_dec(c);
    EXPECT_EQ(ic, -1);
}

TEST(test_uint, hex_to_uint) {
    std::string hex = "0x-b4ae99d7575b481";
    auto ret = top::xrpc::hex_to_uint(hex);
    EXPECT_EQ(0, ret.size());
}

TEST(test_uint, hex_to_uint256) {
    int cnt(0);
    std::string hex = "0x-4ae99d71";
    try{
        top::xrpc::hex_to_uint256(hex);
    }catch(...){
        cnt++;
    }
    EXPECT_EQ(1, cnt);
}
