#include <stdio.h>
#include <iostream>
#include <string>
#include "xdata/xserial_transfrom.h"
#include <gtest/gtest.h>

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

TEST(test_transfrom, signature_test_3) {
    string strEth = HexDecode("f86e0285174876e80082520894a6d2b331b03fddb8c6a8830a63fe47e42c4bdf4e881bc16d674ec8000080820a94a04025663f417a36b44757d33d1e71069d9d124e6bbef809019f33d7b7d5b37e11a07ac5fe86086ed1432fb5a9b3f8548e0148c5a9ea3d30ee9156a6d95b865eace6");
    std::string rawTx = HexDecode("f86e0185174876e8008252089482e1c56d8bb42c0d9049439c00c6c51660670b43880de0b6b3a764000080820a93a03871ae8cb94eb8fa5fb18c2cb25e9e7578b4603ff23de3471a28ceced3d9b52ea06254d4faa63e64f276987553a020e38d4d26f506325c02760c7cbe9bb19f5920");
    //std::string rawTx = HexDecode("f786010203040506830304058801020304050607088831323334353637388b68656c6c6f20776f726c648d746f7020756e69742074657374");
    string strTop;
    int nRet = serial_transfrom::eth_to_top(strEth, strTop);

    string strEth1;
    nRet = serial_transfrom::top_to_eth(strTop, strEth1);
    cout << "eth" << HexEncode(strEth1) << endl;

    return 0;
}

TEST_F(test_transfrom, signature_test_4) {
    std::string hash_str = base::xstring_utl::from_hex("1c299ec068dd832a3b89c6d2b1476dca756873fdc36a3fdb4ce89f1f552ea5da");

    uint256_t hash((uint8_t *)(&hash_str[0]));

    std::cout << "hash = " << base::xstring_utl::to_hex(hash_str) << std::endl;

    std::string sig_str =
        base::xstring_utl::from_hex("014025663f417a36b44757d33d1e71069d9d124e6bbef809019f33d7b7d5b37e117ac5fe86086ed1432fb5a9b3f8548e0148c5a9ea3d30ee9156a6d95b865eace6");

    std::cout << "sig = " << base::xstring_utl::to_hex(sig_str) << std::endl;

    xecdsasig_t sig((uint8_t *)(&sig_str[0]));

    uint8_t out_publickey_data[65];

    xassert(true == xsecp256k1_t::get_publickey_from_signature(sig, hash, out_publickey_data));

    std::string recpub_str((const char *)out_publickey_data, 65);

    std::cout << "recpub = " << base::xstring_utl::to_hex(recpub_str) << std::endl;

    xecpubkey_t pub(out_publickey_data);

    std::string pub_str((const char *)pub.data(), pub.size());

    std::cout << "pub = " << base::xstring_utl::to_hex(pub_str) << std::endl;

    xassert(pub_str == recpub_str);

    ASSERT_EQ("041ac37dafdf7b078ded8b021f78edeab19db39c3ce2ce8d1e2d039786416ced703f1911bd0207713ae92393f30e9b9b3c7a12e0e466671095703dae3fc5afc583", pub_str);

    std::string address = pub.to_raw_eth_address();

    std::cout << "address = " << address << std::endl;

    ASSERT_EQ("0xd8aE0197425C0eA651264b06978580DcB62f3c91", address);
}