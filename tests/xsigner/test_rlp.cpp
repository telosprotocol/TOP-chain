#include <iostream>
#include <string>
#include "gtest/gtest.h"
#include "xcrypto/xckey.h"
#include "xbase/xint.h"
#include "xdata/xgenesis_data.h"
#include "xutility/xhash.h"
#include "xpbase/rlp/Data.h"
#include "xpbase/rlp/RLP.h"
#include "xpbase/rlp/Address.h"

using std::string;
using namespace top::utl;
using namespace top;

const char kHexAlphabet[] = "0123456789abcdef";
const char kHexLookup[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7,  8,  9,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                            0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15 };
const char kBase64Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

std::string HexEncode(const std::string& str) {
    auto size(str.size());
    std::string hex_output(size * 2, 0);
    for (size_t i(0), j(0); i != size; ++i) {
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) / 16];
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) % 16];
    }
    return hex_output;
}

std::string HexDecode(const std::string& str) {
    auto size(str.size());
    if (size % 2) return "";

    std::string non_hex_output(size / 2, 0);
    for (size_t i(0), j(0); i != size / 2; ++i) {
        non_hex_output[i] = (kHexLookup[static_cast<int>(str[j++])] << 4);
        non_hex_output[i] |= kHexLookup[static_cast<int>(str[j++])];
    }
    return non_hex_output;
}

class test_rlp : public testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
    }
};

TEST_F(test_rlp, encode) {
    Address from("T8000066ab344963eaa071f9636faac26b0d1a39900325");
    Address to("T8000066ab344963eaa071f9636faac26b0d1a39900325");

    Data encoded = Data();
    append(encoded, Ethereum::RLP::encode(from.bytes));
    append(encoded, Ethereum::RLP::encode(to.bytes));
    append(encoded, Ethereum::RLP::encode(0x010203040506));
    append(encoded, Ethereum::RLP::encode(0x030405));
    append(encoded, Ethereum::RLP::encode(0x0102030405060708));
    append(encoded, Ethereum::RLP::encode(std::string("12345678")));
    append(encoded, Ethereum::RLP::encode(std::string("hello world")));
    append(encoded, Ethereum::RLP::encode(std::string("top unit test")));

    encoded = Ethereum::RLP::encodeList(encoded);
    std::cout << "encoded: " << HexEncode(std::string((char*)encoded.data(), encoded.size())) <<std::endl;
    EXPECT_TRUE(HexEncode(std::string((char*)encoded.data(), encoded.size())) == "f8619466ab344963eaa071f9636faac26b0d1a399003259466ab344963eaa071f9636faac26b0d1a3990032586010203040506830304058801020304050607088831323334353637388b68656c6c6f20776f726c648d746f7020756e69742074657374");

    std::string pri = HexDecode("2ff271ab38849388c49e24fbc52d357384c24ed929df4f0c7b81afca7c775b62");
    xecprikey_t privk((uint8_t*)pri.c_str());
    xecpubkey_t pub_key = privk.get_public_key();
    std::cout << "pub key: " << HexEncode(std::string((char*)pub_key.data(), pub_key.size())) <<std::endl;
    uint256_t hash_value = utl::xsha2_256_t::digest((const char*)encoded.data(), encoded.size());
    xecdsasig_t sign = privk.sign(hash_value);
    uint8_t* p_comp_data = sign.get_compact_signature();
    int size = sign.get_compact_signature_size();
    std::cout << "sign: " << HexEncode(std::string((char*)p_comp_data, size)) <<std::endl;
    // 0051a134afd1fc323b4477d774a249742860c0d200f874ad6f3299c5270304e7f501423897a3d8e1613d339102af7f3011f901d85b0f848a27434a261563e259ee

    std::string node_addr = from.string();
    top::utl::xkeyaddress_t xaddr{node_addr};

    EXPECT_TRUE(xaddr.verify_signature(sign, hash_value));
}

TEST_F(test_rlp, decode) {
    std::string rawTx = HexDecode("f8d8ae54383030303036366162333434393633656161303731663936333666616163323662306431613339393030333235ae5438303030303636616233343439363365616130373166393633366661616332366230643161333939303033323586010203040506830304058801020304050607088831323334353637388b68656c6c6f20776f726c648d746f7020756e69742074657374b841006c0a03419f26882c0bb2037821a75c87ef3111e229ad741e490549ea884321d833c45432d9a131160c3adeb10db1f940cd30eaa6aeb4c593e50bf73277a51130");
    Ethereum::RLP::DecodedItem decoded = Ethereum::RLP::decode(::data(rawTx));

    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++)
    {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
        std::cout << "data" << i << ": " << HexEncode(str) << std::endl;
    }
    EXPECT_EQ(vecData[0], "T8000066ab344963eaa071f9636faac26b0d1a39900325");
    EXPECT_EQ(vecData[1], "T8000066ab344963eaa071f9636faac26b0d1a39900325");
    EXPECT_EQ(HexEncode(vecData[2]), "010203040506");
    EXPECT_EQ(HexEncode(vecData[3]), "030405");
    EXPECT_EQ(HexEncode(vecData[4]), "0102030405060708");

    EXPECT_EQ(to_uint64(vecData[2]), 0x010203040506);
    EXPECT_EQ(to_uint32(vecData[3]), 0x030405);
    EXPECT_EQ(to_uint64(vecData[4]), 0x0102030405060708);

    EXPECT_EQ(vecData[5], "12345678");
    EXPECT_EQ(vecData[6], "hello world");
    EXPECT_EQ(vecData[7], "top unit test");
    EXPECT_EQ(HexEncode(vecData[8]), "006c0a03419f26882c0bb2037821a75c87ef3111e229ad741e490549ea884321d833c45432d9a131160c3adeb10db1f940cd30eaa6aeb4c593e50bf73277a51130");

    std::string amount(vecData[2]);
    std::reverse(amount.begin(), amount.end());
    amount += std::string(sizeof(uint64_t)-vecData[2].size(), 0);
    std::cout << std::hex << std::setfill('0') << std::setw(2) << *(uint64_t*)amount.data() <<std::endl;
    std::cout << std::hex << std::setfill('0') << std::setw(2) << 0x010203040506 << std::endl;
}
TEST_F(test_rlp, decode2) {
    std::string rawTx = HexDecode("f8cfae54383030303039363839323731303066336362376232336538643437373239383331313634383937386438363133a85430303030304c674750714570694b36584c434b526a396756504e38456a31614d6279416233487584010000000101923078366634663761303938333262303137318b68656c6c6f20776f726c648d746f7020756e69742074657374b8410050d558d2ab0ff34094e9d5c042ff705523a103dbb2098635a7eeddc67bdad86d51909d8cc6c5b4789cc18fb78e5c5b07031564d1870a1238405863ed03ab681f");
    Ethereum::RLP::DecodedItem decoded = Ethereum::RLP::decode(::data(rawTx));

    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++)
    {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
        std::cout << "data" << i << ": " << HexEncode(str) << std::endl;
    }
}
TEST_F(test_rlp, sign_check) {
    std::string node_addr("T80000968927100f3cb7b23e8d477298311648978d8613");
    top::utl::xkeyaddress_t xaddr{node_addr};

    std::string hash("676cb3d7d9d2fba08e8b1ec03dbc465f168f08002709e4413d400b57fbbddf87");
    hash = HexDecode(hash);
    uint256_t hash_value((uint8_t*)hash.c_str());

    std::string s("01536244fd2699c0810f3646d02fee7f9c5f8775426c61d9be87aa035bc18335894caed6fa0f9d6ae98385b9abd9e118d7e6b38d8ae8a78f7b5c0a1c9f3a618d53");
    s = HexDecode(s);
    xecdsasig_t sign((uint8_t*)s.c_str());
    EXPECT_TRUE(xaddr.verify_signature(sign, hash_value));

    std::string pri = HexDecode("2ff271ab38849388c49e24fbc52d357384c24ed929df4f0c7b81afca7c775b62");
    xecprikey_t privk((uint8_t*)pri.c_str());
    xecdsasig_t sign2 = privk.sign(hash_value);
    uint8_t* p_comp_data = sign2.get_compact_signature();
    int size = sign2.get_compact_signature_size();
    std::cout << "sign: " << HexEncode(std::string((char*)p_comp_data, size)) <<std::endl;
}

TEST_F(test_rlp, json_test) {
    std::string pri = HexDecode("2ff271ab38849388c49e24fbc52d357384c24ed929df4f0c7b81afca7c775b62");
    xecprikey_t privk((uint8_t*)pri.c_str());
    xecpubkey_t pub_key = privk.get_public_key();
    std::cout << "pub key: " << HexEncode(std::string((char*)pub_key.data(), pub_key.size())) <<std::endl;
    std::string encoded("hello world!");
    uint256_t hash_value = utl::xsha2_256_t::digest((const char*)encoded.data(), encoded.size());
    xecdsasig_t sign = privk.sign(hash_value);
    uint8_t* p_comp_data = sign.get_compact_signature();
    int size = sign.get_compact_signature_size();
    std::cout << "sign: " << HexEncode(std::string((char*)p_comp_data, size)) <<std::endl;
}