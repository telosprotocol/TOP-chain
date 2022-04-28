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
        int writeLen = 0, fileLen;
        xBorshEncoder encoder;
        encoder.EncodeInteger(true);
        std::ofstream fin("type_bool_true.bin", std::ios::binary);
        for (auto c : encoder.GetBuffer()) {
            fin.write((char*)&c, sizeof(uint8_t));
            writeLen++;
        }
        fin.close();

        std::string buffer;
        std::ifstream fout("type_bool_true.bin", std::ios::binary);
        fout >> buffer;
        fout.seekg(0,  std::ios::end);   
        fileLen = fout.tellg();
        fout.close();
        xBorshDecoder decoder;
        bool testFlag = decoder.getBool(buffer);
        ASSERT_EQ(testFlag , true);
        ASSERT_EQ(writeLen , fileLen);
        std::cout << " test_type_bool true fileLen  " << writeLen << std::endl;
    }
     //test bool false
    {
        int writeLen = 0, fileLen;
        xBorshEncoder encoder;
        encoder.EncodeInteger(false);
        std::ofstream fin("type_bool_false.bin", std::ios::binary);
        for (auto c : encoder.GetBuffer()) {
            fin.write((char*)&c, sizeof(uint8_t));
            writeLen++;
        }
        fin.close();

        std::string buffer;
        std::ifstream fout("type_bool_false.bin", std::ios::binary);
        fout >> buffer;
        fout.seekg(0,  std::ios::end);   
        fileLen = fout.tellg();
        fout.close();
        xBorshDecoder decoder;
        bool testFlag = decoder.getBool(buffer);
        ASSERT_EQ(testFlag , false);
        ASSERT_EQ(writeLen , fileLen);
        ASSERT_EQ(fileLen , 1);
        std::cout << " test_type_bool false fileLen  " << writeLen << std::endl;
    }
}


TEST(test_evm_xborsh, test_xborsh_u8) {
    int writeLen = 0, fileLen;
    std::cout << " test_type_u8 " << std::endl;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint8_t)0x12);
    std::ofstream fin("type_u8_0x12.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

     std::string buffer;
    std::ifstream fout("type_u8_0x12.bin", std::ios::binary);
    fout >> buffer; 
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close();
    xBorshDecoder decoder;
    uint8_t result = 0;
    decoder.getInteger(buffer, result);
    ASSERT_EQ(result , 0x12);
    ASSERT_EQ(writeLen , fileLen);
    ASSERT_EQ(fileLen , 1);
    std::cout << " test_type_u8  fileLen  " << writeLen << std::endl;
}


TEST(test_evm_xborsh, test_xborsh_u16) {

    std::cout << " test_type_u16 " << std::endl;
    int writeLen = 0, fileLen;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint16_t)0x1234);
    std::ofstream fin("type_u16_0x1234.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

    std::string buf;
    std::ifstream fout("type_u16_0x1234.bin", std::ios::binary);
    fout >> buf; 
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close();
    xBorshDecoder decoder;
    uint16_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x1234);
    ASSERT_EQ(writeLen , fileLen);
    ASSERT_EQ(fileLen , 2);
    std::cout << " test_type_u16  fileLen  " << writeLen << std::endl;
}


TEST(test_evm_xborsh, test_xborsh_u32) {

    std::cout << " test_type_u32 " << std::endl;
    int writeLen = 0, fileLen;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint32_t)0x12345678);
    std::ofstream fin("type_u32_0x12345678.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

    std::string buf;
    std::ifstream fout("type_u32_0x12345678.bin", std::ios::binary);
    fout >> buf; 
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close();
    xBorshDecoder decoder;
    uint32_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x12345678);
    ASSERT_EQ(writeLen , fileLen);
    ASSERT_EQ(fileLen , 4);
    std::cout << " test_type_u32  fileLen  " << writeLen << std::endl;
}


TEST(test_evm_xborsh, test_xborsh_u64) {

    std::cout << " test_type_u64 " << std::endl;
    int writeLen = 0, fileLen;
    xBorshEncoder encoder;
    encoder.EncodeInteger((uint64_t)0x1234567812345678);
    std::ofstream fin("type_u64_0x1234567812345678.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

    std::string buf;
    std::ifstream fout("type_u64_0x1234567812345678.bin", std::ios::binary);
    fout >> buf; 
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close();
    xBorshDecoder decoder;
    uint64_t result = 0;
    decoder.getInteger(buf, result);
    ASSERT_EQ(result , 0x1234567812345678);
    ASSERT_EQ(writeLen , fileLen);
    ASSERT_EQ(fileLen , 8);
    std::cout << " test_type_u64  fileLen  " << writeLen << std::endl;

}


TEST(test_evm_xborsh, test_xborsh_string) {

    std::cout << " test_type_string " << std::endl;
     int writeLen = 0, fileLen;
    xBorshEncoder encoder;
    encoder.EncodeString("abcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyz");
    std::ofstream fin("type_string_abcdefghjklmnopqrstuvwxyz.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

    std::string bufStr;
    std::ifstream fout("type_string_abcdefghjklmnopqrstuvwxyz.bin", std::ios::binary);
    fout >> bufStr;
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close();
    xBorshDecoder decoder;
    std::string result;
    decoder.getString(bufStr, result);
    ASSERT_EQ(result , "abcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyz");
    ASSERT_EQ(writeLen , fileLen);
    ASSERT_EQ((writeLen - sizeof(uint32_t)) , strlen("abcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyzabcdefghjklmnopqrstuvwxyz"));
    std::cout << " test_xborsh_string fileLen  " << writeLen << std::endl;
}


TEST(test_evm_xborsh, test_xborsh_128) {

    int writeLen = 0, fileLen;
    const u128 s_err{"0xacedce6af48a03bbfd25e8cd036414"}; 
    const u128 s_max{"0xaaedce6af48a03bbfd25e8cd036414"}; 

    xBorshEncoder encoder;
    encoder.EncodeInteger(s_max);
    std::ofstream fin("type_u128.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

    std::string buffer;
    std::ifstream fout("type_u128.bin", std::ios::binary);
    fout >> buffer;
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close(); 

    u128 result;
    xBorshDecoder decoder;
    decoder.getInteger(buffer, result);
    ASSERT_TRUE((result == s_max));
    ASSERT_TRUE((result != s_err));
    ASSERT_EQ(writeLen , fileLen);
    std::cout << " test_xborsh_128 fileLen  " << writeLen << std::endl;
}


TEST(test_evm_xborsh, test_xborsh_256) {
  //  u256 test;


    int writeLen = 0, fileLen;
    const u256 s_err{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364321"}; 
    const u256 s_max{"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141"}; 

    xBorshEncoder encoder;
    encoder.EncodeInteger(s_max);
    std::ofstream fin("type_u256.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();

    std::string buffer;
    std::ifstream fout("type_u256.bin", std::ios::binary);
    fout >> buffer;
    fout.seekg(0,  std::ios::end);   
    fileLen = fout.tellg();
    fout.close(); 

    u256 result;
    xBorshDecoder decoder;
    decoder.getInteger(buffer, result);
    ASSERT_TRUE((result == s_max));
    ASSERT_TRUE((result != s_err));
    ASSERT_EQ(writeLen , fileLen);
    std::cout << " type_u256 fileLen  " << writeLen << std::endl;

}
