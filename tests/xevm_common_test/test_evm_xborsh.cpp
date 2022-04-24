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

/*
template <unsigned N>
class xop_bytes_array 
{
public:

    xop_bytes_array(std::string const &str) {    
       // std::memcpy(charray.data(), cstr, );
    }

private:
    
    std::array<byte, N> m_data; 

}
*/

/*
struct BlockHeaderInnerLite {
    uint64_t height; // Height of this block since the genesis block (height 0).
    std::string epoch_id; // byt32 Epoch start hash of this block's epoch. Used for retrieving validator information
    std::string next_epoch_id;
    std::string prev_state_root; // Root hash of the state at the previous block.
    std::string outcome_root; // Root of the outcomes of transactions and receipts.
    uint64_t timestamp; // Timestamp at which the block was built.
    std::string next_bp_hash; // Hash of the next epoch block producers set
    std::string block_merkle_root;
    std::string hash; // Additional computable element
}
*/

typedef  struct PublicKey {
    uint8_t version;    //if version =0 ,isChunkOnly not borsh,  version = 1 isChunkOnly  borsh
    uint32_t  keyType;  //no used
    uint8_t  bt8;    //no used
    std::string k;
}x_PublicKey;

typedef   struct BlockProducer {
    x_PublicKey publicKey;
    u128 stake;
    // Flag indicating if this validator proposed to be a chunk-only producer (i.e. cannot become a block producer).
   // bool isChunkOnly;  no send it
}x_BlockProducer;

typedef   struct OptionalBlockProducers {
    bool some;
    std::vector<x_BlockProducer> blockProducers;  //
   // std::string hash; // not send it 
}x_OptionalBlockProducers;

typedef   struct Signature {
    uint8_t signatureType;    //no used
    std::string r;
    std::string s;
}x_Signature;

typedef  struct OptionalSignature {
    bool some;
    x_Signature signature;
}x_OptionalSignature;


typedef struct LightClientBlock {
    std::string prev_block_hash;   //32 byte
    std::string next_block_inner_hash;  //32 byte
    // BlockHeaderInnerLite inner_lite;

    uint64_t height; // Height of this block since the genesis block (height 0).
    std::string epoch_id; // byt32 Epoch start hash of this block's epoch. Used for retrieving validator information
    std::string next_epoch_id;
    std::string prev_state_root; // Root hash of the state at the previous block.
    std::string outcome_root; // Root of the outcomes of transactions and receipts.
    uint64_t timestamp; // Timestamp at which the block was built.
    std::string next_bp_hash; // Hash of the next epoch block producers set
    std::string block_merkle_root;
   // std::string inner_hash; // no send 

    std::string inner_rest_hash;//todo   //32 byte
    x_OptionalBlockProducers next_bps;
    std::vector<x_OptionalSignature> approvals_after_next;
  //  std::string hash;   //32 byte  not send it
  //  std::string next_hash; //todo  //32 byte
}x_LightClientBlock;


TEST(test_evm_xborsh, test_xborsh_block) {

    x_LightClientBlock clientBlock;
    clientBlock.prev_block_hash = "7bRffWHtH2pZ6X9vXQ3ZnDCkdpTutNs4";
    clientBlock.next_block_inner_hash = "BVJ3UPLFvh6e6qKoQqXzNwGg3arJfERr";
    clientBlock.height = 100;
    clientBlock.epoch_id = "5nm5qwBYQH9MfrHQ1R645MXXHgAz4jBR";
    clientBlock.next_epoch_id = "556dMzVQusducJypg5Q4Kbq3bZ5mJigy";
    clientBlock.prev_state_root = "BhqfFFrnFj7EfLc4ognhZGsPz8UsELyL";
    clientBlock.outcome_root= "7tkzFg8RHBmMw1ncRJZCCZAizgq4rwCf";
    clientBlock.timestamp= 1590578830167531000;
    clientBlock.next_bp_hash = "4EHb8bzoefQNHcbxeXKkbWpygFzgeoUK";
    clientBlock.block_merkle_root = "4fFB5xWb5vzQ1aj7NAEFQwwNzLuMhDyq";
   // clientBlock.inner_hash = "HX2u2p4XPLPMiBydcF9riFKoh6vqwsam";
    
    clientBlock.inner_rest_hash = "97zbp3ivM3bgN78ia1gGquqKtGyGtJWP";
    clientBlock.next_bps.some = true;

    for (int  i = 0; i < 2; i++) {
        x_BlockProducer block_pro;

 
        block_pro.publicKey.version =  0;
        block_pro.publicKey.keyType = 0;
        block_pro.publicKey.bt8 = 0;
        block_pro.publicKey.k = "811Zje7UUQhRoYYsZCj4t71reETSBgQ7";

        u128 testData {"0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364321"};
        block_pro.stake = testData;
       // block_pro.isChunkOnly = true;
        clientBlock.next_bps.blockProducers.push_back(block_pro);
    }
   // clientBlock.next_bps.hash = "CQq9wqQ5bdAKjFM1AjAz8UFGmSZfQp5w";

    for (int  i = 0; i < 3; i++)
    {
        x_OptionalSignature opt_signatrue;
        opt_signatrue.some = true;
         opt_signatrue.signature.signatureType  = 0;    //no used
        opt_signatrue.signature.r = "5JHR5e66KRasgGAveg9eK3iLvDuuTZ3A";
        opt_signatrue.signature.s = "G2Y2Dw5zie84v4UF7XLvcXMGnUMCjzsh";
        clientBlock.approvals_after_next.push_back(opt_signatrue);
    }
 //   clientBlock.hash = "7An3RU7j9paDzTpHtrXK8vy1cW41mYCh";
 //   clientBlock.next_hash = "QNpRL2pUBjQ95QRcFdeDUMF2hAxRtHGa";


    
    xBorshEncoder encoder;
    uint32_t len = 0xffff; //noused , solidity not use it

    //std::string nousedstring = "5JHR5e66KRasgGAveg9eK3iLvDuuTZ3A";

    //cala calculate
    encoder.EncodeStringFixArray(clientBlock.prev_block_hash).EncodeStringFixArray(clientBlock.next_block_inner_hash)

    .EncodeInteger(clientBlock.height).EncodeStringFixArray(clientBlock.epoch_id).EncodeStringFixArray(clientBlock.next_epoch_id)
    .EncodeStringFixArray(clientBlock.prev_state_root).EncodeStringFixArray(clientBlock.outcome_root)
    .EncodeInteger(clientBlock.timestamp).EncodeStringFixArray(clientBlock.next_bp_hash).EncodeStringFixArray(clientBlock.block_merkle_root);
    
    
    encoder.EncodeStringFixArray(clientBlock.inner_rest_hash).EncodeInteger(clientBlock.next_bps.some);
    if (clientBlock.next_bps.some == true) {
         //add len before blockProducers
        uint32_t blockProducersLen = clientBlock.next_bps.blockProducers.size();
        encoder.EncodeInteger(blockProducersLen);
        std::cout << " blockProducersLen   " << blockProducersLen << std::endl;
        for (auto blockpro : clientBlock.next_bps.blockProducers) {

             encoder.EncodeInteger(blockpro.publicKey.version).EncodeInteger(blockpro.publicKey.keyType)
             .EncodeInteger(blockpro.publicKey.bt8).EncodeStringFixArray(blockpro.publicKey.k)
             .EncodeInteger(blockpro.stake);
        }
    }
    
                //add len before blockProducers
        uint32_t approvals_after_nextLen = clientBlock.approvals_after_next.size();
        encoder.EncodeInteger(approvals_after_nextLen);
        std::cout << " approvals_after_nextLen   " << approvals_after_nextLen << std::endl;



    for (auto opt_signatrue :clientBlock.approvals_after_next)
    {
        encoder.EncodeInteger(opt_signatrue.some).EncodeInteger(opt_signatrue.signature.signatureType)
        .EncodeStringFixArray(opt_signatrue.signature.r).EncodeStringFixArray(opt_signatrue.signature.s);
    }


    int writeLen = 0;
    std::ofstream fin("block_test.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();
    
}


TEST(test_evm_xborsh, test_bytes_block) {

    h256 test256;
    test256.copyFromStr("G2Y2Dw5zie84v4UF7XLvcXMGn");
   
    int writeLen, readLen, fileLen;
    xBorshEncoder encoder;
    encoder.EncodeBytesFixArray(test256);
    std::ofstream fin("h256_test.bin", std::ios::binary);
    for (auto c : encoder.GetBuffer()) {
        fin.write((char*)&c, sizeof(uint8_t));
        writeLen++;
    }
    fin.close();


}





