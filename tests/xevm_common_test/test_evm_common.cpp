#include "xbase/xbase.h"
#include "xbase/xdata.h"
#include "xbase/xint.h"
#include "xdata/xdatautil.h"
#include "xevm_common/address.h"
#include "xevm_common/common.h"
#include "xevm_common/rlp.h"
#include "xutility/xhash.h"

#include <gtest/gtest.h>

using namespace top::evm_common;

using namespace top::evm_common::rlp;

TEST(test_boost, uint) {
    u512 n = 1;
    for (int index = 1; index < 90; ++index) {
        n = n * index;
        std::cout << n << std::endl;
    }
}

TEST(test_boost, hash) {
    auto raddr = Address::random();
    std::cout << raddr << std::endl;
}

const char kHexAlphabet[] = "0123456789abcdef";
const char kHexLookup[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 0, 0, 0, 0,  0,  0,  0,  0,  0, 0, 0,
                           0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 10, 11, 12, 13, 14, 15};

std::string HexEncode(const std::string & str) {
    auto size(str.size());
    std::string hex_output(size * 2, 0);
    for (size_t i(0), j(0); i != size; ++i) {
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) / 16];
        hex_output[j++] = kHexAlphabet[static_cast<unsigned char>(str[i]) % 16];
    }
    return hex_output;
}

std::string HexDecode(const std::string & str) {
    auto size(str.size());
    if (size % 2)
        return "";

    std::string non_hex_output(size / 2, 0);
    for (size_t i(0), j(0); i != size / 2; ++i) {
        non_hex_output[i] = (kHexLookup[static_cast<int>(str[j++])] << 4);
        non_hex_output[i] |= kHexLookup[static_cast<int>(str[j++])];
    }
    return non_hex_output;
}
TEST(test_rlp, encode) {
    bytes encoded = bytes();
    append(encoded, RLP::encode(0x010203040506));
    append(encoded, RLP::encode(0x030405));
    append(encoded, RLP::encode(0x0102030405060708));
    append(encoded, RLP::encode(std::string("12345678")));
    append(encoded, RLP::encode(std::string("hello world")));
    append(encoded, RLP::encode(std::string("top unit test")));

    encoded = RLP::encodeList(encoded);
    std::cout << "encoded: " << HexEncode(std::string((char *)encoded.data(), encoded.size())) << std::endl;
    EXPECT_TRUE(HexEncode(std::string((char *)encoded.data(), encoded.size())) ==
                "f786010203040506830304058801020304050607088831323334353637388b68656c6c6f20776f726c648d746f7020756e69742074657374");
}

TEST(test_rlp, decode) {
    std::string rawTx = HexDecode("f786010203040506830304058801020304050607088831323334353637388b68656c6c6f20776f726c648d746f7020756e69742074657374");
    RLP::DecodedItem decoded = RLP::decode(::data(rawTx));

    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
        std::cout << "data" << i << ": " << HexEncode(str) << std::endl;
    }
    EXPECT_EQ(HexEncode(vecData[0]), "010203040506");
    EXPECT_EQ(HexEncode(vecData[1]), "030405");
    EXPECT_EQ(HexEncode(vecData[2]), "0102030405060708");

    EXPECT_EQ(to_uint64(vecData[0]), 0x010203040506);
    EXPECT_EQ(to_uint32(vecData[1]), 0x030405);
    EXPECT_EQ(to_uint64(vecData[2]), 0x0102030405060708);

    EXPECT_EQ(vecData[3], "12345678");
    EXPECT_EQ(vecData[4], "hello world");
    EXPECT_EQ(vecData[5], "top unit test");
}

TEST(test_rlp, wiki_example) {
    std::string raw = HexDecode("c88363617483646f67");  // The list [ “cat”, “dog” ] = [ 0xc8, 0x83, 'c', 'a', 't', 0x83, 'd', 'o', 'g' ]
    RLP::DecodedItem decoded = RLP::decode(::data(raw));

    std::vector<std::string> vecData;
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
        std::cout << "data" << i << ": " << (str) << std::endl;
    }

    raw = HexDecode("c7c0c1c0c3c0c1c0");  // [ [], [[]], [ [], [[]] ] ] = [ 0xc7, 0xc0, 0xc1, 0xc0, 0xc3, 0xc0, 0xc1, 0xc0 ]
    decoded = RLP::decode(::data(raw));

    vecData.clear();
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
        std::cout << "data" << i << ": " << (str) << std::endl;
    }

    raw = HexDecode("820400");  // The encoded integer 1024 (’\x04\x00’) = [ 0x82, 0x04, 0x00 ]
    decoded = RLP::decode(::data(raw));

    vecData.clear();
    for (int i = 0; i < (int)decoded.decoded.size(); i++) {
        std::string str(decoded.decoded[i].begin(), decoded.decoded[i].end());
        vecData.push_back(str);
        std::cout << "data" << i << ": " << HexEncode(str) << std::endl;
    }
}

TEST(test_rlp, logsbloom) {
    std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
    h2048 bloom;
    h160 address{address_str};
    top::uint256_t hash = top::utl::xsha3_256_t::digest(address.ref().data(), address.ref().size());

    bytes data = fromHex(top::data::to_hex_str(hash));
    // bytes data = top::evm_common::toCompactBigEndian(hash);
    h256 hash_h256(data);
    // h256 hash_h256 = top::evm_common::fromBigEndian<u256>(data);


    // h256 hash_h256 = fromBigEndian<u256>(std::string(reinterpret_cast<char *>(hash.data()), hash.size()));
    bloom.shiftBloom<3>(hash_h256);

    std::vector<std::string> topics = {"342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735",
                                       "0000000000000000000000000000000000000000000000000000000000000000",
                                       "000000000000000000000000047b2c1e1e9258ca2a7549d5b7987096f55109d1"};
    for (auto & topic : topics) {
        h256 topic_h256{topic};
        top::uint256_t topic_hash = top::utl::xsha3_256_t::digest(topic_h256.ref().data(), topic_h256.ref().size());

        bytes topic_data = fromHex(top::data::to_hex_str(topic_hash));
        // bytes data = top::evm_common::toCompactBigEndian(topic_hash);
        h256 topic_hash_h256(topic_data);
        // h256 topic_hash_h256 = top::evm_common::fromBigEndian<u256>(topic_data);

        // h256 topic_hash_h256 = fromBigEndian<u256>(std::string(reinterpret_cast<char *>(topic_hash.data()), topic_hash.size()));
        bloom.shiftBloom<3>(topic_hash_h256);
    }

    std::stringstream outstrbloom;
    outstrbloom << bloom;
    std::string bloom_str = outstrbloom.str();

    EXPECT_EQ(bloom_str,
              "00000000000000000000000000000000000000000400000000000000000000000000000000000000000080000000000000000000000000000000000000000000000000000000000000000000800000000000"
              "00000000000000000000000000400000000002000000000000000000080000000000000000000000000000000000000001000020000000040000000000000000000000000000000000000000000000000001"
              "00000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000"
              "00000000000000000000");
}
