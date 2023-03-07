#include <gtest/gtest.h>

#include "xcommon/xeth_address.h"
#include "xevm_common/xabi_decoder.h"

#include <random>
#include <cstring>

#include <endian.h>

NS_BEG3(top, evm_common, tests)

// recommending reading:
// https://docs.soliditylang.org/en/develop/abi-spec.html
// abi encode tools: https://abi.hashex.org/

TEST(test_abi, integer_only_1) {
    // function i8_16(int8 x, int16 y) public pure {}
    // > contract.i8_16.getData(-1,2)

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0x7c4df3eeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff0000000000000000000000000000000000000000000000000000000000000002", ec);
    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0x7c4df3ee);

    auto fi = t.extract<int8_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(fi, (int8_t)-1);
    auto si = t.extract<int16_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(si, (int16_t)2);
}

TEST(test_abi, integer_only_2) {
    // function u16_32_64(uint16 x, uint32 y, uint64 z) public pure {}
    // > contract.u16_32_64.getData(1,2,3)
    // "0x4ab51583
    // 0000000000000000000000000000000000000000000000000000000000000001
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000003"

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0x4ab515830000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000"
        "00000000000000000000000000000003",
        ec);
    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0x4ab51583);

    auto fu = t.extract<uint16_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(fu, (uint16_t)1);
    auto su = t.extract<uint32_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(su, (uint32_t)2);
    auto tu = t.extract<uint64_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(tu, (uint64_t)3);
}

TEST(test_abi, integer_only_3) {
    // function ui256(int256 x, uint256 y) public pure {}
    // > contract.ui256.getData(-12345,54321)
    // "0xac8d63f0
    // ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffcfc7
    // 000000000000000000000000000000000000000000000000000000000000d431"

    //std::error_code ec;
    //auto t = xabi_decoder_t::build_from_hex_string(
    //    "0xac8d63f0ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffcfc7000000000000000000000000000000000000000000000000000000000000d431", ec);
    //ASSERT_TRUE(!ec);

    //auto selector = t.extract<xfunction_selector_t>(ec);
    //ASSERT_EQ(selector.method_id, 0xac8d63f0);

    //auto fi = t.extract<s256>(ec);
    //ASSERT_TRUE(!ec);
    //ASSERT_EQ(fi, (s256)-12345);
    //auto su = t.extract<u256>(ec);
    //ASSERT_TRUE(!ec);
    //ASSERT_EQ(su, (u256)54321);
}

TEST(test_abi, address_only) {
    // function addr(address a, address[] memory aa) public pure {}
    // 0x4dce5c8961e283786cb31ad7fc072347227d7ea2, ["0x9251e7932e2c941e0ee1f370a1c387754af9cfdb","0x96932b7a373d8586c4a2d3c98517803ff2818cec"]
    // 0xb6333743
    // 0000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea2
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000009251e7932e2c941e0ee1f370a1c387754af9cfdb
    // 00000000000000000000000096932b7a373d8586c4a2d3c98517803ff2818cec

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0xb63337430000000000000000000000004dce5c8961e283786cb31ad7fc072347227d7ea200000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000"
        "0000000000000000000000000000000000020000000000000000000000009251e7932e2c941e0ee1f370a1c387754af9cfdb00000000000000000000000096932b7a373d8586c4a2d3c98517803ff2818cec",
        ec);
    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0xb6333743);

    auto addr1 = t.extract<common::xeth_address_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(addr1.to_hex_string(), "0x4dce5c8961e283786cb31ad7fc072347227d7ea2");

    auto addr_arr = t.decode_array<common::xeth_address_t>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_TRUE(addr_arr.size() == 2);
    ASSERT_EQ(addr_arr[0].to_hex_string(), "0x9251e7932e2c941e0ee1f370a1c387754af9cfdb");
    ASSERT_EQ(addr_arr[1].to_hex_string(), "0x96932b7a373d8586c4a2d3c98517803ff2818cec");
}

TEST(test_abi, string_only) {
    // function s_s(string memory x, string memory y) public pure {}
    // > contract.s_s.getData("first","second")
    // "0xcad1ec28
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 0000000000000000000000000000000000000000000000000000000000000080
    // 0000000000000000000000000000000000000000000000000000000000000005
    // 6669727374000000000000000000000000000000000000000000000000000000
    // 0000000000000000000000000000000000000000000000000000000000000006
    // 7365636f6e640000000000000000000000000000000000000000000000000000"

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0xcad1ec280000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000000000008000000000000000000000000000000000"
        "00000000000000000000000000000005666972737400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000067365636f6e"
        "640000000000000000000000000000000000000000000000000000",
        ec);
    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0xcad1ec28);

    //auto s1 = t.extract<std::string>(ec);
    //ASSERT_TRUE(!ec);
    //ASSERT_EQ(s1, "first");

    //auto s2 = t.extract<std::string>(ec);
    //ASSERT_TRUE(!ec);
    //ASSERT_EQ(s2, "second");
}

TEST(test_abi, string_only_no_ec) {
    // function s_s(string memory x, string memory y) public pure {}
    // > contract.s_s.getData("first","second")
    // "0xcad1ec28
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 0000000000000000000000000000000000000000000000000000000000000080
    // 0000000000000000000000000000000000000000000000000000000000000005
    // 6669727374000000000000000000000000000000000000000000000000000000
    // 0000000000000000000000000000000000000000000000000000000000000006
    // 7365636f6e640000000000000000000000000000000000000000000000000000"
    try {
        auto t = xabi_decoder_t::build_from_hex_string(
            "0xcad1ec28000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000800000000000000000000000000000"
            "0000"
            "0000000000000000000000000000000566697273740000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000006736563"
            "6f6e"
            "640000000000000000000000000000000000000000000000000000");
        auto selector = t.extract<xfunction_selector_t>();
        ASSERT_EQ(selector.method_id, 0xcad1ec28);

        //auto s1 = t.extract<std::string>();
        //ASSERT_EQ(s1, "first");

        //auto s2 = t.extract<std::string>();
        //ASSERT_EQ(s2, "second");
    } catch (const std::exception & e) {
        GTEST_FAIL();
    }
}

TEST(test_abi, wiki_sample_1) {
    // https://docs.soliditylang.org/en/develop/abi-spec.html#examples
    //
    // function bar(bytes3[2] memory) public pure {}
    // > contract.bar.getData(["abc","def"])
    // "0xfce353f6
    // 6162630000000000000000000000000000000000000000000000000000000000
    // 6465660000000000000000000000000000000000000000000000000000000000"
    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0xfce353f661626300000000000000000000000000000000000000000000000000000000006465660000000000000000000000000000000000000000000000000000000000", ec);

    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0xfce353f6);

    auto b1 = t.decode_bytes(3, ec);
    auto expected = top::to_bytes(std::string{"abc"});
    ASSERT_TRUE(!ec);
    ASSERT_EQ(b1, expected);

    auto b2 = t.decode_bytes(3, ec);
    expected = top::to_bytes(std::string{"def"});
    ASSERT_TRUE(!ec);
    ASSERT_EQ(b2, expected);
}

TEST(test_abi, wiki_sample_2) {
    // https://docs.soliditylang.org/en/develop/abi-spec.html#examples
    //
    // function sam(bytes memory, bool, uint256[] memory) public pure {}
    // > contract.sam.getData("0x123456789000000000000000000000000111111111111111111111",true,[1,2,3])
    // 0xa5643bf2
    // 0000000000000000000000000000000000000000000000000000000000000060 pos of parameter one
    // 0000000000000000000000000000000000000000000000000000000000000001
    // 00000000000000000000000000000000000000000000000000000000000000a0 pos of parameter two
    // 000000000000000000000000000000000000000000000000000000000000001b
    // 1234567890000000000000000000000001111111111111111111110000000000
    // 0000000000000000000000000000000000000000000000000000000000000003
    // 0000000000000000000000000000000000000000000000000000000000000001
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000003

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0xa5643bf20000000000000000000000000000000000000000000000000000000000000060000000000000000000000000000000000000000000000000000000000000000100000000000000000"
        "000000000000000000000000000000000000000000000a0000000000000000000000000000000000000000000000000000000000000001b12345678900000000000000000000000011111111111"
        "11111111110000000000000000000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000000000000000000000000000000000010000000"
        "0000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000003",
        ec);

    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0xa5643bf2);

    auto bs = t.extract<xbytes_t>(ec);
    auto expected = top::from_hex("0x123456789000000000000000000000000111111111111111111111", ec);
    ASSERT_EQ(bs, expected);
    ASSERT_TRUE(!ec);

    auto b = t.extract<bool>(ec);
    ASSERT_EQ(b, true);
    ASSERT_TRUE(!ec);

    auto au = t.decode_array<u256>(ec);
    auto expected_array = std::vector<u256>{1, 2, 3};
    ASSERT_EQ(au, expected_array);
    ASSERT_TRUE(!ec);
}

TEST(test_abi, wiki_sample_3) {
    // https://docs.soliditylang.org/en/develop/abi-spec.html#use-of-dynamic-types
    // sample f:
    //
    // function f(uint, uint32[] memory, bytes10, bytes memory) public pure{}
    // > contract.f.getData(0x123,[0x456,0x789],"1234567890","Hello, world!")
    // "0x8be65246
    // 0000000000000000000000000000000000000000000000000000000000000123
    // 0000000000000000000000000000000000000000000000000000000000000080
    // 3132333435363738393000000000000000000000000000000000000000000000
    // 00000000000000000000000000000000000000000000000000000000000000e0
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000456
    // 0000000000000000000000000000000000000000000000000000000000000789
    // 000000000000000000000000000000000000000000000000000000000000000d
    // 48656c6c6f2c20776f726c642100000000000000000000000000000000000000"
    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "0x8be652460000000000000000000000000000000000000000000000000000000000000123000000000000000000000000000000000000000000000000000000000000008031323334353637383"
        "9300000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000e000000000000000000000000000000000000000000000"
        "00000000000000000002000000000000000000000000000000000000000000000000000000000000045600000000000000000000000000000000000000000000000000000000000007890000000"
        "00000000000000000000000000000000000000000000000000000000d48656c6c6f2c20776f726c642100000000000000000000000000000000000000",
        ec);

    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0x8be65246);

    auto ui = t.extract<u256>(ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(ui, u256(0x123));

    auto ua = t.decode_array<uint32_t>(ec);
    ASSERT_TRUE(!ec);
    auto expected_res = std::vector<uint32_t>{0x456, 0x789};
    ASSERT_EQ(ua, expected_res);

    auto b10 = t.decode_bytes(10, ec);
    ASSERT_TRUE(b10.size() == 10);
    ASSERT_TRUE(!ec);
    auto expected = top::to_bytes(std::string{"1234567890"});
    ASSERT_EQ(b10, expected);

    auto b = t.extract<xbytes_t>(ec);
    ASSERT_TRUE(!ec);
    expected = top::to_bytes(std::string{"Hello, world!"});
    ASSERT_EQ(b, expected);
}

TEST(test_abi, wiki_sample_4) {
    // https://docs.soliditylang.org/en/develop/abi-spec.html#use-of-dynamic-types
    // sample g:
    //
    // function g(uint[][] memory, string[] memory) public pure {}
    // > contract.g.getData([[1,2],[3]],["one","two","three"])
    // "0x2289b18c
    // 0000000000000000000000000000000000000000000000000000000000000040  0    // position of [[1,2],[3]] 64/32 = 2
    // 0000000000000000000000000000000000000000000000000000000000000140  1    // position of ["one","two","three"] 320/32 = 10
    // 0000000000000000000000000000000000000000000000000000000000000002  2    // sz of [[1,2],[3]]
    // 0000000000000000000000000000000000000000000000000000000000000040  3  0 // offset of [1,2] 64/32 = 2
    // 00000000000000000000000000000000000000000000000000000000000000a0  4  1 // offset of [3] 160/32 = 5
    // 0000000000000000000000000000000000000000000000000000000000000002  5  2 // sz of [1,2]
    // 0000000000000000000000000000000000000000000000000000000000000001  6  3 // 1
    // 0000000000000000000000000000000000000000000000000000000000000002  7  4 // 2
    // 0000000000000000000000000000000000000000000000000000000000000001  8  5 // sz of [3]
    // 0000000000000000000000000000000000000000000000000000000000000003  9  6 // 3
    // 0000000000000000000000000000000000000000000000000000000000000003  10   // sz of ["one","two","three"]
    // 0000000000000000000000000000000000000000000000000000000000000060  11 0 // offset of "one" 96/32 = 3
    // 00000000000000000000000000000000000000000000000000000000000000a0  12 1 // offset of "two"
    // 00000000000000000000000000000000000000000000000000000000000000e0  13 2 // offset of "three"
    // 0000000000000000000000000000000000000000000000000000000000000003  14 3 // size of "one"
    // 6f6e650000000000000000000000000000000000000000000000000000000000  15 4 // "one"
    // 0000000000000000000000000000000000000000000000000000000000000003  16 5 // size of "two"
    // 74776f0000000000000000000000000000000000000000000000000000000000  17 6 // "two"
    // 0000000000000000000000000000000000000000000000000000000000000005  18   // size of "three"
    // 7468726565000000000000000000000000000000000000000000000000000000" 19   // "three""

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "2289b18c000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000001400000000000000000000000000000000000"
        "000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000000a0000000000000"
        "00000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000"
        "00000000020000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000300000000000000000000000000000000"
        "00000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000000000000000a00000000000"
        "0000000000000000000000000000000000000000000000000000e000000000000000000000000000000000000000000000000000000000000000036f6e650000000000000000000000000000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000374776f0000000000000000000000000000000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000057468726565000000000000000000000000000000000000000000000000000000",
        ec);

    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0x2289b18c);

    auto vvu256 = t.decode_array<std::vector<u256>>(ec);
    ASSERT_TRUE(!ec);
    auto expected_vvu = std::vector<std::vector<u256>>{{1, 2}, {3}};
    ASSERT_EQ(vvu256, expected_vvu);

    auto vstr = t.decode_array<std::string>(ec);
    ASSERT_TRUE(!ec);
    auto expected_vstr = std::vector<std::string>{"one", "two", "three"};
    ASSERT_EQ(vstr, expected_vstr);
}

TEST(test_abi, array_recursive_test) {
    // sample g2:
    //
    // function g2(uint256[][][] memory, string[] memory) public pure {}
    // > [[[1,2],[3]],[[4,5],[6]]] , ["one","two","three"]
    // "0x65bf4b82
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 00000000000000000000000000000000000000000000000000000000000002a0
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 0000000000000000000000000000000000000000000000000000000000000140
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 00000000000000000000000000000000000000000000000000000000000000a0
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000001
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000001
    // 0000000000000000000000000000000000000000000000000000000000000003
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000040
    // 00000000000000000000000000000000000000000000000000000000000000a0
    // 0000000000000000000000000000000000000000000000000000000000000002
    // 0000000000000000000000000000000000000000000000000000000000000004
    // 0000000000000000000000000000000000000000000000000000000000000005
    // 0000000000000000000000000000000000000000000000000000000000000001
    // 0000000000000000000000000000000000000000000000000000000000000006
    // 0000000000000000000000000000000000000000000000000000000000000003
    // 0000000000000000000000000000000000000000000000000000000000000060
    // 00000000000000000000000000000000000000000000000000000000000000a0
    // 00000000000000000000000000000000000000000000000000000000000000e0
    // 0000000000000000000000000000000000000000000000000000000000000003
    // 6f6e650000000000000000000000000000000000000000000000000000000000
    // 0000000000000000000000000000000000000000000000000000000000000003
    // 74776f0000000000000000000000000000000000000000000000000000000000
    // 0000000000000000000000000000000000000000000000000000000000000005
    // 7468726565000000000000000000000000000000000000000000000000000000

    std::error_code ec;
    auto t = xabi_decoder_t::build_from_hex_string(
        "65bf4b82000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000002a00000000000000000000000000000000000"
        "00000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000000000000140000000000000"
        "00000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000040000000000000000000000000000000000000000000000000000000"
        "00000000a00000000000000000000000000000000000000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000"
        "00000000000000000000000000000002000000000000000000000000000000000000000000000000000000000000000100000000000000000000000000000000000000000000000000000000000000030000000000"
        "00000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000400000000000000000000000000000000000000000000000000000"
        "0000000000a000000000000000000000000000000000000000000000000000000000000000020000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000"
        "00000000000000000000000000000000050000000000000000000000000000000000000000000000000000000000000001000000000000000000000000000000000000000000000000000000000000000600000000"
        "00000000000000000000000000000000000000000000000000000003000000000000000000000000000000000000000000000000000000000000006000000000000000000000000000000000000000000000000000"
        "000000000000a000000000000000000000000000000000000000000000000000000000000000e000000000000000000000000000000000000000000000000000000000000000036f6e650000000000000000000000"
        "000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000374776f0000000000000000000000000000000000000000000000000000000000000000"
        "00000000000000000000000000000000000000000000000000000000057468726565000000000000000000000000000000000000000000000000000000",
        ec);

    ASSERT_TRUE(!ec);

    auto selector = t.extract<xfunction_selector_t>(ec);
    ASSERT_EQ(selector.method_id, 0x65bf4b82);

    auto vvvu256 = t.decode_array<std::vector<std::vector<u256>>>(ec);
    ASSERT_TRUE(!ec);
    auto expected_vvvu = std::vector<std::vector<std::vector<u256>>>{{{1, 2}, {3}}, {{4, 5}, {6}}};
    ASSERT_EQ(vvvu256, expected_vvvu);

    auto vstr = t.decode_array<std::string>(ec);
    ASSERT_TRUE(!ec);
    auto expected_vstr = std::vector<std::string>{"one", "two", "three"};
    ASSERT_EQ(vstr, expected_vstr);
}

TEST(abi_decoder, fuzzy_bytes_less_than_4) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(0, 3);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 5000; ++n) {
        auto const sz = size_distrib(gen);

        xbytes_t raw_data(sz);
        for (auto i = 0u; i < sz; ++i) {
            raw_data[i] = byte_distrib(gen);
        }

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(std::move(raw_data), ec);
            ASSERT_FALSE(!ec);
            ASSERT_TRUE(decoder.empty());

            ec.clear();
            auto const fs = decoder.extract<xfunction_selector_t>(ec);
            ASSERT_FALSE(!ec);
            ASSERT_TRUE(fs.method_id == 0);
        }
    }
}

TEST(abi_decoder, fuzzy_bytes_equal_4) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 5000; ++n) {
        xbytes_t raw_data(xabi_decoder_t::function_selector_size);
        for (auto i = 0u; i < xabi_decoder_t::function_selector_size; ++i) {
            raw_data[i] = byte_distrib(gen);
        }

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_TRUE(!ec);
            ASSERT_TRUE(decoder.empty());

            auto const fs = decoder.extract<xfunction_selector_t>(ec);
            uint32_t method_id = htobe32(fs.method_id);
            ASSERT_EQ(0, std::memcmp(raw_data.data(), &method_id, xabi_decoder_t::function_selector_size));
        }
    }
}

TEST(abi_decoder, fuzzy_bytes_invalid_size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(5, 10000);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 5000; ++n) {
        auto sz = size_distrib(gen);
        while (sz % xabi_decoder_t::solidity_word_size == xabi_decoder_t::function_selector_size) {
            sz = size_distrib(gen);
        }

        xbytes_t raw_data(sz);
        for (auto i = 0u; i < sz; ++i) {
            raw_data[i] = byte_distrib(gen);
        }

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_FALSE(!ec);
            ASSERT_TRUE(decoder.empty());

            ec.clear();
            auto const fs = decoder.extract<xfunction_selector_t>(ec);
            ASSERT_EQ(0, fs.method_id);
        }
    }
}

TEST(abi_decoder, fuzzy_bytes_valid_size) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(5, 10000);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 5000; ++n) {
        auto sz = size_distrib(gen);
        while (sz % xabi_decoder_t::solidity_word_size != xabi_decoder_t::function_selector_size) {
            sz = size_distrib(gen);
        }

        ASSERT_EQ(0, (sz - xabi_decoder_t::function_selector_size) % xabi_decoder_t::solidity_word_size);

        xbytes_t raw_data(sz);
        for (auto i = 0u; i < sz; ++i) {
            raw_data[i] = byte_distrib(gen);
        }

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_TRUE(!ec);
            ASSERT_TRUE(!decoder.empty());
            ASSERT_EQ(sz / xabi_decoder_t::solidity_word_size, decoder.size());

            ec.clear();
            auto const fs = decoder.extract<xfunction_selector_t>(ec);
            uint32_t method_id = htobe32(fs.method_id);
            ASSERT_EQ(0, std::memcmp(raw_data.data(), &method_id, xabi_decoder_t::function_selector_size));
        }
    }
}

TEST(abi_decoder, fuzzy_bytes_eth_address) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(5, 10000);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 5000; ++n) {
        auto sz = size_distrib(gen);
        while (sz % xabi_decoder_t::solidity_word_size != xabi_decoder_t::function_selector_size) {
            sz = size_distrib(gen);
        }

        ASSERT_EQ(0, (sz - xabi_decoder_t::function_selector_size) % xabi_decoder_t::solidity_word_size);
        ASSERT_TRUE(sz >= xabi_decoder_t::solidity_word_size + xabi_decoder_t::function_selector_size);

        xbytes_t raw_data;
        raw_data.reserve(sz);
        for (auto i = 0u; i < xabi_decoder_t::function_selector_size; ++i) {
            raw_data.emplace_back(byte_distrib(gen));
        }

        for (size_t i = xabi_decoder_t::function_selector_size, j = 0; i < sz; ++i, ++j) {
            raw_data.emplace_back(j % xabi_decoder_t::solidity_word_size >= 12u ? byte_distrib(gen) : xbyte_t{});
        }

        ASSERT_EQ(sz, raw_data.size());

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_TRUE(!ec);
            ASSERT_TRUE(!decoder.empty());
            ASSERT_EQ(sz / xabi_decoder_t::solidity_word_size, decoder.size());

            ec.clear();
            auto const fs = decoder.extract<xfunction_selector_t>(ec);
            uint32_t method_id = htobe32(fs.method_id);
            ASSERT_EQ(0, std::memcmp(raw_data.data(), &method_id, xabi_decoder_t::function_selector_size));

            while (!decoder.empty()) {
                decoder.extract<common::xeth_address_t>(ec);
                ASSERT_TRUE(!ec);
            }
        }
    }

    for (int n = 0; n < 5000; ++n) {
        auto sz = size_distrib(gen);
        while (sz % xabi_decoder_t::solidity_word_size != xabi_decoder_t::function_selector_size) {
            sz = size_distrib(gen);
        }

        ASSERT_EQ(0, (sz - xabi_decoder_t::function_selector_size) % xabi_decoder_t::solidity_word_size);
        ASSERT_TRUE(sz >= xabi_decoder_t::solidity_word_size + xabi_decoder_t::function_selector_size);

        xbytes_t raw_data;
        raw_data.reserve(sz);
        for (auto i = 0u; i < xabi_decoder_t::function_selector_size; ++i) {
            raw_data.emplace_back(byte_distrib(gen));
        }

        for (size_t i = xabi_decoder_t::function_selector_size, j = 0; i < sz; ++i, ++j) {
            raw_data.emplace_back(byte_distrib(gen));
        }

        ASSERT_EQ(sz, raw_data.size());

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_TRUE(!ec);
            ASSERT_TRUE(!decoder.empty());
            ASSERT_EQ(sz / xabi_decoder_t::solidity_word_size, decoder.size());

            auto const fs = decoder.extract<xfunction_selector_t>(ec);
            uint32_t method_id = htobe32(fs.method_id);
            ASSERT_EQ(0, std::memcmp(raw_data.data(), &method_id, xabi_decoder_t::function_selector_size));

            while (!decoder.empty()) {
                ec.clear();
                auto const eth_address = decoder.extract<common::xeth_address_t>(ec);
                ASSERT_FALSE(!ec);
                ASSERT_TRUE(eth_address.is_zero());
            }

            ec.clear();
            ASSERT_TRUE(decoder.extract<common::xeth_address_t>(ec).is_zero());
            ASSERT_FALSE(!ec);
        }
    }
}

TEST(abi_decoder, fuzzy_bytes) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(5, 100000);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 500; ++n) {
        auto sz = size_distrib(gen);
        while (sz % xabi_decoder_t::solidity_word_size != xabi_decoder_t::function_selector_size) {
            sz = size_distrib(gen);
        }

        ASSERT_EQ(0, (sz - xabi_decoder_t::function_selector_size) % xabi_decoder_t::solidity_word_size);

        xbytes_t raw_data(sz);
        for (auto i = 0u; i < sz; ++i) {
            raw_data[i] = byte_distrib(gen);
        }

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_TRUE(!ec);
            ASSERT_TRUE(!decoder.empty());
            ASSERT_EQ(sz / xabi_decoder_t::solidity_word_size, decoder.size());

            while (!decoder.empty()) {
                ec.clear();
                auto const bytes = decoder.extract<xbytes_t>(ec);
                ASSERT_FALSE(!ec);
                ASSERT_TRUE(bytes.empty());
            }
        }
    }
}

TEST(abi_decoder, fuzzy_string) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<std::size_t> size_distrib(5, 100000);
    std::uniform_int_distribution<xbyte_t> byte_distrib{};

    for (int n = 0; n < 500; ++n) {
        auto sz = size_distrib(gen);
        while (sz % xabi_decoder_t::solidity_word_size != xabi_decoder_t::function_selector_size) {
            sz = size_distrib(gen);
        }

        ASSERT_EQ(0, (sz - xabi_decoder_t::function_selector_size) % xabi_decoder_t::solidity_word_size);

        xbytes_t raw_data(sz);
        for (auto i = 0u; i < sz; ++i) {
            raw_data[i] = byte_distrib(gen);
        }

        {
            std::error_code ec;
            auto const decoder = xabi_decoder_t::build_from(raw_data, ec);
            ASSERT_TRUE(!ec);
            ASSERT_TRUE(!decoder.empty());
            ASSERT_EQ(sz / xabi_decoder_t::solidity_word_size, decoder.size());

            while (!decoder.empty()) {
                ec.clear();
                auto const string = decoder.extract<std::string>(ec);
                ASSERT_FALSE(!ec);
                ASSERT_TRUE(string.empty());
            }
        }
    }
}

NS_END3
