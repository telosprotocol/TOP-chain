#include <stdio.h>
#include <iostream>
#include <string>
#include "trezor-crypto/sha3.h"
#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xbasic/xfixed_hash.h"
#include "xcommon/common.h"
#include "xcommon/rlp.h"
#include "xcommon/xeth_address.h"
#include "xevm_common/xevm_transaction_result.h"

#include <gtest/gtest.h>

using namespace top::evm_common;
using namespace top;

TEST(test_evm_log, logsbloom) {
    std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
    std::string strAddress = base::xstring_utl::from_hex(address_str);
    top::evm_common::h2048 bloom;
    char szDigest[32] = {0};
    keccak_256((const unsigned char *)strAddress.data(), strAddress.size(), (unsigned char *)szDigest);
    top::evm_common::h256 hash_h256;
    bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
    bloom.shiftBloom<3>(hash_h256);

    std::vector<std::string> topics = {"342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735",
                                       "0000000000000000000000000000000000000000000000000000000000000000",
                                       "000000000000000000000000047b2c1e1e9258ca2a7549d5b7987096f55109d1"};
    for (auto & topic : topics) {
        std::string strTopic = base::xstring_utl::from_hex(topic);
        char szDigest[32] = {0};
        keccak_256((const unsigned char *)strTopic.data(), strTopic.size(), (unsigned char *)szDigest);
        top::evm_common::h256 hash_h256;
        bytesConstRef((const unsigned char *)szDigest, 32).copyTo(hash_h256.ref());
        bloom.shiftBloom<3>(hash_h256);
    }
    std::stringstream outstrbloom;
    outstrbloom << bloom;
    std::string bloom_str = outstrbloom.str();

    EXPECT_EQ(bloom_str,
            "00000000000000000000000000000000000000000400000000000000000000000000000000000000000080000000000000000000000000000000000000000000000000000000000000000000800000000000"
            "00000000000000000000000000400000000002000000000000000000080000000000000000000000000000000000000001000020000000040000000000000000000000000000000000000000000000000001"
            "00000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000"
            "00000000000000000000");


// compare log bloom
{
    common::xeth_address_t ethaddr = common::xeth_address_t::build_from(address_str);
    xh256s_t topics2;
    for (auto & topic : topics) {
        topics2.push_back(xh256_t(topic));
    }
    xbytes_t data;
    evm_common::xevm_log_t log(ethaddr, topics2, data);
    evm_common::xbloom9_t _bloom9 = log.bloom();
    std::string _bloom9_hex = _bloom9.to_hex_string();
    EXPECT_EQ(_bloom9_hex,
            "0x00000000000000000000000000000000000000000400000000000000000000000000000000000000000080000000000000000000000000000000000000000000000000000000000000000000800000000000"
            "00000000000000000000000000400000000002000000000000000000080000000000000000000000000000000000000001000020000000040000000000000000000000000000000000000000000000000001"
            "00000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000"
            "00000000000000000000");
}
}

TEST(test_evm_log, log_rlpstream) {
    std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
    std::vector<std::string> topics = {"342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735",
                                       "0000000000000000000000000000000000000000000000000000000000000000",
                                       "000000000000000000000000047b2c1e1e9258ca2a7549d5b7987096f55109d1"};
    common::xeth_address_t ethaddr = common::xeth_address_t::build_from(address_str);
    xh256s_t topics2;
    for (auto & topic : topics) {
        topics2.push_back(xh256_t(topic));
    }
    xbytes_t data;
    evm_common::xevm_log_t log(ethaddr, topics2, data);
    evm_common::xbloom9_t _bloom9 = log.bloom();
    std::string _bloom9_hex = _bloom9.to_hex_string();
    EXPECT_EQ(_bloom9_hex,
            "0x00000000000000000000000000000000000000000400000000000000000000000000000000000000000080000000000000000000000000000000000000000000000000000000000000000000800000000000"
            "00000000000000000000000000400000000002000000000000000000080000000000000000000000000000000000000001000020000000040000000000000000000000000000000000000000000000000001"
            "00000000000000000400000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000"
            "00000000000000000000");

    evm_common::RLPStream s;
    log.streamRLP(s);
    auto s_out = s.out();
    std::cout << "streamRLP:" << s_out.size() << std::endl;

    evm_common::xevm_log_t log2;
    evm_common::RLP d(s_out);
    std::error_code ec;
    log2.decodeRLP(d, ec);
    if (ec) {
        assert(false);
    }
    ASSERT_EQ(log2.address.to_string(), log.address.to_string());
}

TEST(test_evm_log, log_xstream) {
    std::string address_str = "e71d898e741c743326bf045959221cc39e0718d2";
    std::vector<std::string> topics = {"342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735",
                                       "0000000000000000000000000000000000000000000000000000000000000000",
                                       "000000000000000000000000047b2c1e1e9258ca2a7549d5b7987096f55109d1"};
    common::xeth_address_t ethaddr = common::xeth_address_t::build_from(address_str);
    xh256s_t topics2;
    for (auto & topic : topics) {
        topics2.push_back(xh256_t(topic));
    }
    xbytes_t data;
    evm_common::xevm_log_t log(ethaddr, topics2, data);

    base::xstream_t _stream(base::xcontext_t::instance());
    log.do_write(_stream);
    std::cout << "xstream:" << _stream.size() << std::endl;
    
    evm_common::xevm_log_t log2;
    auto ret = log2.do_read(_stream);
    ASSERT_TRUE(ret > 0);
}
