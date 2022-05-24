#include "xbase/xcontext.h"
#include "xbasic/xbyte_buffer.h"

#include <gtest/gtest.h>

#define private public
#include "xevm_common/xeth/xeth_header.h"

namespace top {
namespace tests {

TEST(stream, basic_test) {
    base::xstream_t stream(base::xcontext_t::instance());
    xbytes_t bytes1{1, 3, 5, 7, 9};
    xbytes_t bytes2{2, 4, 6, 8, 0};
    stream << bytes1;
    stream << bytes2;
    auto stream_str = std::string(reinterpret_cast<char *>(stream.data()), static_cast<uint32_t>(stream.size()));

    base::xstream_t stream_decode(base::xcontext_t::instance(), (uint8_t *)(stream_str.c_str()), static_cast<uint32_t>(stream_str.size()));
    xbytes_t bytes1_decode;
    xbytes_t bytes2_decode;
    stream_decode >> bytes1_decode;
    stream_decode >> bytes2_decode;

    EXPECT_EQ(bytes1, bytes1_decode);
    EXPECT_EQ(bytes2, bytes2_decode);
}

TEST(eth_header, encode_decode_test) {
    evm_common::eth::xeth_block_header_t header;
    header.m_parentHash = static_cast<evm_common::h256>(UINT32_MAX - 1);
    header.m_uncleHash = static_cast<evm_common::h256>(UINT32_MAX - 2);
    header.m_miner = static_cast<evm_common::eth::Address>(UINT32_MAX - 3);
    header.m_stateMerkleRoot = static_cast<evm_common::h256>(UINT32_MAX - 4);
    header.m_txMerkleRoot = static_cast<evm_common::h256>(UINT32_MAX - 5);
    header.m_receiptMerkleRoot = static_cast<evm_common::h256>(UINT32_MAX - 6);
    header.m_bloom = static_cast<evm_common::eth::LogBloom>(UINT32_MAX - 7);
    header.m_mixDigest = static_cast<evm_common::h256>(UINT32_MAX - 8);
    header.m_nonce = static_cast<evm_common::h64>(UINT32_MAX - 9);
    header.m_hash = static_cast<evm_common::h256>(UINT32_MAX - 10);
    header.m_difficulty = static_cast<evm_common::bigint>(UINT64_MAX - 1);
    header.m_number = UINT64_MAX - 2;
    header.m_gasLimit = UINT64_MAX - 3;
    header.m_gasUsed = UINT64_MAX - 4;
    header.m_time = UINT64_MAX - 5;
    header.m_baseFee = static_cast<evm_common::bigint>(UINT64_MAX - 6);
    header.m_hashed = true;
    header.m_isBaseFee = true;
    header.m_extra = {1, 3, 5, 7};

    auto header_str = header.to_string();
    evm_common::eth::xeth_block_header_t header_decode;
    header_decode.from_string(header_str);

    EXPECT_EQ(header.m_parentHash, header_decode.m_parentHash);
    EXPECT_EQ(header.m_uncleHash, header_decode.m_uncleHash);
    EXPECT_EQ(header.m_miner, header_decode.m_miner);
    EXPECT_EQ(header.m_stateMerkleRoot, header_decode.m_stateMerkleRoot);
    EXPECT_EQ(header.m_txMerkleRoot, header_decode.m_txMerkleRoot);
    EXPECT_EQ(header.m_receiptMerkleRoot, header_decode.m_receiptMerkleRoot);
    EXPECT_EQ(header.m_bloom, header_decode.m_bloom);
    EXPECT_EQ(header.m_difficulty, header_decode.m_difficulty);
    EXPECT_EQ(header.m_number, header_decode.m_number);
    EXPECT_EQ(header.m_gasLimit, header_decode.m_gasLimit);
    EXPECT_EQ(header.m_gasUsed, header_decode.m_gasUsed);
    EXPECT_EQ(header.m_time, header_decode.m_time);
    EXPECT_EQ(header.m_extra, header_decode.m_extra);
    EXPECT_EQ(header.m_mixDigest, header_decode.m_mixDigest);
    EXPECT_EQ(header.m_nonce, header_decode.m_nonce);
    EXPECT_EQ(header.m_baseFee, header_decode.m_baseFee);
    EXPECT_EQ(header.m_hash, header_decode.m_hash);
    EXPECT_EQ(header.m_hashed, header_decode.m_hashed);
    EXPECT_EQ(header.m_isBaseFee, header_decode.m_isBaseFee);
}

}
}