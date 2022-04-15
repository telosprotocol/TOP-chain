#include "xevm_common/common_data.h"
#include "xevm_common/xborsh.hpp"


#include <iostream>
#include <limits>
#include <fstream>
#include <gtest/gtest.h>

using namespace top::evm_common;



TEST(test_evm_xborsh, test_xborsh_bool) {
    std::cout << " test_type_bool " << std::endl;
    //test bool true
    {
        xBorshEncoder encoder;
        encoder.EncodeInteger(true);
        std::ofstream fin("type_bool_true.bin", std::ios::binary);
        for (auto c : encoder.GetBuffer()) {
            fin.write((char*)&c, sizeof(uint8_t));
        }
        fin.close();

        char buf[256] = {0};
        std::ifstream fout("type_bool_true.bin", std::ios::binary);
        fout.read(buf, sizeof(uint8_t));
        fout.close();
        xBorshDecoder decoder;
        bool testFlag = decoder.getBool(buf);
        ASSERT_EQ(testFlag , true);
    }
     //test bool false
    {
        xBorshEncoder encoder;
        encoder.EncodeInteger(false);
        std::ofstream fin("type_bool_false.bin", std::ios::binary);
        for (auto c : encoder.GetBuffer()) {
            fin.write((char*)&c, sizeof(uint8_t));
        }
        fin.close();

        char buf[256] = {0};
        std::ifstream fout("type_bool_false.bin", std::ios::binary);
        fout.read(buf, sizeof(uint8_t));
        fout.close();
        xBorshDecoder decoder;
        bool testFlag = decoder.getBool(buf);
        ASSERT_EQ(testFlag , false);
    }
}

TEST(test_evm_xborsh, test_xborsh_u8) {

    std::cout << " test_type_u8 " << std::endl;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint8_t)0x12);
    std::ofstream fin("type_u8_0x12.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    char buf[256] = {0};
    std::ifstream fout("type_u8_0x12.bin", std::ios::binary);
    fout.read(buf, sizeof(uint8_t));
    fout.close();
    xBorshDecoder decoder;
    uint8_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x12);
}


TEST(test_evm_xborsh, test_xborsh_u16) {

    std::cout << " test_type_u16 " << std::endl;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint16_t)0x1234);
    std::ofstream fin("type_u16_0x1234.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    char buf[256] = {0};
    std::ifstream fout("type_u16_0x1234.bin", std::ios::binary);
    fout.read(buf, sizeof(uint16_t));
    fout.close();
    xBorshDecoder decoder;
    uint16_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x1234);
}


TEST(test_evm_xborsh, test_xborsh_u32) {

    std::cout << " test_type_u32 " << std::endl;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint32_t)0x12345678);
    std::ofstream fin("type_u32_0x12345678.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    char buf[256] = {0};
    std::ifstream fout("type_u32_0x12345678.bin", std::ios::binary);
    fout.read(buf, sizeof(uint32_t));
    fout.close();
    xBorshDecoder decoder;
    uint32_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x12345678);
}


TEST(test_evm_xborsh, test_xborsh_u64) {

    std::cout << " test_type_u64 " << std::endl;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint64_t)0x1234567812345678);
    std::ofstream fin("type_u64_0x1234567812345678.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    char buf[256] = {0};
    std::ifstream fout("type_u64_0x1234567812345678.bin", std::ios::binary);
    fout.read(buf, sizeof(uint64_t));
    fout.close();
    xBorshDecoder decoder;
    uint64_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x1234567812345678);

}


TEST(test_evm_xborsh, test_xborsh_string) {

    std::cout << " test_type_string " << std::endl;
    xBorshEncoder encoder;
    encoder.EncodeString("abcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyz");
    std::ofstream fin("type_string_abcdefghjklmnopqrstuvwxyz.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    std::string bufStr;
    std::ifstream fout("type_string_abcdefghjklmnopqrstuvwxyz.bin", std::ios::binary);
    fout >> bufStr;
    fout.close();
    xBorshDecoder decoder;
    std::string result;
    decoder.getString(bufStr, result);

    ASSERT_EQ(result , "abcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyz");
}


TEST(test_evm_xborsh, test_xborsh_128) {

    const u128 s_err{"0xacedce6af48a03bbfd25e8cd036414"}; 
    const u128 s_max{"0xaaedce6af48a03bbfd25e8cd036414"}; 
    auto testbuffer = toBigEndian(s_max);
    std::ofstream fin("type_u128.bin", std::ios::binary);
    for (auto c : testbuffer) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    std::string buffer;
    std::ifstream fout("type_u128.bin", std::ios::binary);
    fout >> buffer;
    fout.close(); // <class T, class _In>
    u256 result =  fromBigEndian<u256, std::string>(buffer);
    ASSERT_TRUE((result == s_max));
    ASSERT_TRUE((result != s_err));
}


TEST(test_evm_xborsh, test_xborsh_256) {
  //  u256 test;
    const u256 s_err{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364321"}; 
    const u256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"}; 
    auto testbuffer = toBigEndian(s_max);
    std::ofstream fin("type_u256.bin", std::ios::binary);
    for (auto c : testbuffer) {
        fin.write((char*)&c, sizeof(uint8_t));
    }
    fin.close();

    std::string buffer;
    std::ifstream fout("type_u256.bin", std::ios::binary);
    fout >> buffer; 
    fout.close(); 
    u256 result =  fromBigEndian<u256, std::string>(buffer);//    fromBigEndian<u256, std::string>(buffer);
    ASSERT_TRUE((result == s_max));
    ASSERT_TRUE((result != s_err));
}