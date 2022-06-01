#include "tests/xevm_contract_runtime/test_vem_eth_bridge_contract_fixture.h"
#include "xbasic/xhex.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xabi_decoder.h"

#define private public
#include "xevm_common/xeth/xeth_header.h"

namespace top {
namespace tests {

using namespace contract_runtime::evm::sys_contract;
using namespace evm_common;
using namespace evm_common::eth;

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

TEST(eth_header, stream_encode_decode_test) {
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

TEST(eth_header, rlp_decode) {
    const char * hex_input = "7024323700000000000000000000000000000000000000000000000000000000000000200000000000000000000000000000000000000000000000000000000000000429f90426f90210a0397cb4acc8fe4ac39c6cdcb50a28c727ad257f084020912029a2d6f2ad11d431a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0b5d83f8283dba9ba397287db1f6c0675e9a66a51aef8d7796c32b7da5cd6a982a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302192a64834f11298084628f482099d883010a12846765746888676f312e31372e38856c696e7578a063e47112f6e95c00322701868d568e5d08e393e9430a0fd152e71e7c8be86cac8829ebdb8e839fffacf90210a0fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0be5f8f7c84539b81c621a029b3083c37e0b63dde1372545c38edb792fcb65883a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302196d65834f24ec8084628f482199d883010a12846765746888676f312e31372e38856c696e7578a0d8f6c88a59b42eafc6bc6e048e257a415bbb17822d57566153b73a72666b297b880afe7b9f331361e70000000000000000000000000000000000000000000000";
    std::error_code ec;
    xbytes_t input = top::from_hex(hex_input, ec);
    EXPECT_EQ(ec.value(), 0);
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(input, ec);
    EXPECT_EQ(ec.value(), 0);
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    EXPECT_EQ(ec.value(), 0);
    function_selector.method_id;
    auto headers_bytes = abi_decoder.extract<xbytes_t>(ec);
    EXPECT_EQ(ec.value(), 0);
    std::vector<xeth_block_header_t> headers;
    auto decode_item = RLP::decode_once(headers_bytes);
    auto bytes = decode_item.decoded[0];
    while (bytes.size() > 0) {
        auto item = RLP::decode_once(bytes);
        bytes = item.remainder;
        xassert(item.decoded.size() == 1);
        auto header_bytes = item.decoded[0];
        xeth_block_header_t header;
        header.from_rlp(header_bytes);
        headers.emplace_back(header);
    }
    // {"parentHash":"0x397cb4acc8fe4ac39c6cdcb50a28c727ad257f084020912029a2d6f2ad11d431","sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347","miner":"0xcf4d39b0edb0b69cd15f687fd45c8fc8eb687dae","stateRoot":"0xb5d83f8283dba9ba397287db1f6c0675e9a66a51aef8d7796c32b7da5cd6a982","transactionsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421","receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421","logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","difficulty":"0x2192a","number":"0x64","gasLimit":"0x4f1129","gasUsed":"0x0","timestamp":"0x628f4820","extraData":"0xd883010a12846765746888676f312e31372e38856c696e7578","mixHash":"0x63e47112f6e95c00322701868d568e5d08e393e9430a0fd152e71e7c8be86cac","nonce":"0x29ebdb8e839fffac","baseFeePerGas":null,"hash":"0xfc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107"}
    // {"parentHash":"0xfc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107","sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347","miner":"0xcf4d39b0edb0b69cd15f687fd45c8fc8eb687dae","stateRoot":"0xbe5f8f7c84539b81c621a029b3083c37e0b63dde1372545c38edb792fcb65883","transactionsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421","receiptsRoot":"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421","logsBloom":"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000","difficulty":"0x2196d","number":"0x65","gasLimit":"0x4f24ec","gasUsed":"0x0","timestamp":"0x628f4821","extraData":"0xd883010a12846765746888676f312e31372e38856c696e7578","mixHash":"0xd8f6c88a59b42eafc6bc6e048e257a415bbb17822d57566153b73a72666b297b","nonce":"0x0afe7b9f331361e7","baseFeePerGas":null,"hash":"0x3a5c3c19ab2906d03fccde42623bda65153ac0045abba0ef3db998c41be38553"}
    EXPECT_EQ(headers[0].m_parentHash.hex(), "397cb4acc8fe4ac39c6cdcb50a28c727ad257f084020912029a2d6f2ad11d431");
    EXPECT_EQ(headers[0].m_uncleHash.hex(), "1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    EXPECT_EQ(headers[0].m_miner.hex(), "cf4d39b0edb0b69cd15f687fd45c8fc8eb687dae");
    EXPECT_EQ(headers[0].m_stateMerkleRoot.hex(), "b5d83f8283dba9ba397287db1f6c0675e9a66a51aef8d7796c32b7da5cd6a982");
    EXPECT_EQ(headers[0].m_txMerkleRoot.hex(), "56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    EXPECT_EQ(headers[0].m_receiptMerkleRoot.hex(), "56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    EXPECT_EQ(headers[0].m_bloom.hex(), "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(headers[0].m_difficulty.str(), std::to_string(0x2192a));
    EXPECT_EQ(headers[0].m_number.str(), std::to_string(0x64));
    EXPECT_EQ(headers[0].m_gasLimit, 0x4f1129);
    EXPECT_EQ(headers[0].m_gasUsed, 0x0);
    EXPECT_EQ(headers[0].m_time, 0x628f4820);
    EXPECT_EQ(headers[0].m_mixDigest.hex(), "63e47112f6e95c00322701868d568e5d08e393e9430a0fd152e71e7c8be86cac");
    EXPECT_EQ(headers[0].m_nonce.hex(), "29ebdb8e839fffac");
    EXPECT_EQ(headers[0].hash().hex() , "fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107");
    EXPECT_EQ(headers[1].m_parentHash.hex(), "fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107");
    EXPECT_EQ(headers[1].m_uncleHash.hex(), "1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    EXPECT_EQ(headers[1].m_miner.hex(), "cf4d39b0edb0b69cd15f687fd45c8fc8eb687dae");
    EXPECT_EQ(headers[1].m_stateMerkleRoot.hex(), "be5f8f7c84539b81c621a029b3083c37e0b63dde1372545c38edb792fcb65883");
    EXPECT_EQ(headers[1].m_txMerkleRoot.hex(), "56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    EXPECT_EQ(headers[1].m_receiptMerkleRoot.hex(), "56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    EXPECT_EQ(headers[1].m_bloom.hex(), "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(headers[1].m_difficulty.str(), std::to_string(0x2196d));
    EXPECT_EQ(headers[1].m_number.str(), std::to_string(0x65));
    EXPECT_EQ(headers[1].m_gasLimit, 0x4f24ec);
    EXPECT_EQ(headers[1].m_gasUsed, 0x0);
    EXPECT_EQ(headers[1].m_time, 0x628f4821);
    EXPECT_EQ(headers[1].m_mixDigest.hex(), "d8f6c88a59b42eafc6bc6e048e257a415bbb17822d57566153b73a72666b297b");
    EXPECT_EQ(headers[1].m_nonce.hex(), "0afe7b9f331361e7");
    EXPECT_EQ(headers[1].hash().hex() , "3a5c3c19ab2906d03fccde42623bda65153ac0045abba0ef3db998c41be38553");
    // for (size_t i = 0; i < headers.size(); i++) {
    //     auto header = headers[i];
    //     printf("header.m_parentHash: %s\n", header.m_parentHash.hex().c_str());
    //     printf("header.m_uncleHash: %s\n", header.m_uncleHash.hex().c_str());
    //     printf("header.m_miner: %s\n", header.m_miner.hex().c_str());
    //     printf("header.m_stateMerkleRoot: %s\n", header.m_stateMerkleRoot.hex().c_str());
    //     printf("header.m_txMerkleRoot: %s\n", header.m_txMerkleRoot.hex().c_str());
    //     printf("header.m_receiptMerkleRoot: %s\n", header.m_receiptMerkleRoot.hex().c_str());
    //     printf("header.m_bloom: %s\n", header.m_bloom.hex().c_str());
    //     printf("header.m_difficulty: %s\n", header.m_difficulty.str().c_str());
    //     printf("header.m_number: %s\n", header.m_number.str().c_str());
    //     printf("header.m_gasLimit: %lu\n", header.m_gasLimit);
    //     printf("header.m_gasUsed: %lu\n", header.m_gasUsed);
    //     printf("header.m_time: %lu\n", header.m_time);
    //     printf("header.m_mixDigest: %s\n", header.m_mixDigest.hex().c_str());
    //     printf("header.m_nonce: %s\n", header.m_nonce.hex().c_str());
    // }
}

TEST_F(xcontract_fixture_t, test_init_and_sync) {
    const char * rlp_headers = "f90213f90210a0397cb4acc8fe4ac39c6cdcb50a28c727ad257f084020912029a2d6f2ad11d431a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0b5d83f8283dba9ba397287db1f6c0675e9a66a51aef8d7796c32b7da5cd6a982a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302192a64834f11298084628f482099d883010a12846765746888676f312e31372e38856c696e7578a063e47112f6e95c00322701868d568e5d08e393e9430a0fd152e71e7c8be86cac8829ebdb8e839fffac00000000000000000000";
    std::error_code ec;
    xbytes_t input = top::from_hex(rlp_headers, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(input, ""));

    bigint height{0};
    EXPECT_TRUE(contract.get_height(height));
    EXPECT_EQ(height, 100);
    h256 hash{0};
    EXPECT_TRUE(contract.get_hash(height, hash));
    EXPECT_EQ(hash.hex(), "fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107");

    xeth_block_header_t header;
    bigint d{0};
    EXPECT_TRUE(contract.get_header(hash, header, d));
    EXPECT_EQ(header.m_parentHash.hex(), "397cb4acc8fe4ac39c6cdcb50a28c727ad257f084020912029a2d6f2ad11d431");
    EXPECT_EQ(header.m_uncleHash.hex(), "1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347");
    EXPECT_EQ(header.m_miner.hex(), "cf4d39b0edb0b69cd15f687fd45c8fc8eb687dae");
    EXPECT_EQ(header.m_stateMerkleRoot.hex(), "b5d83f8283dba9ba397287db1f6c0675e9a66a51aef8d7796c32b7da5cd6a982");
    EXPECT_EQ(header.m_txMerkleRoot.hex(), "56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    EXPECT_EQ(header.m_receiptMerkleRoot.hex(), "56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
    EXPECT_EQ(header.m_bloom.hex(), "00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000");
    EXPECT_EQ(header.m_difficulty.str(), std::to_string(0x2192a));
    EXPECT_EQ(header.m_number.str(), std::to_string(0x64));
    EXPECT_EQ(header.m_gasLimit, 0x4f1129);
    EXPECT_EQ(header.m_gasUsed, 0x0);
    EXPECT_EQ(header.m_time, 0x628f4820);
    EXPECT_EQ(header.m_mixDigest.hex(), "63e47112f6e95c00322701868d568e5d08e393e9430a0fd152e71e7c8be86cac");
    EXPECT_EQ(header.m_nonce.hex(), "29ebdb8e839fffac");
    EXPECT_EQ(header.m_difficulty, d);
    EXPECT_EQ(header.m_hash.hex(), "fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107");

    const char * sync_rlp_headers = "f90213f90210a0fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0be5f8f7c84539b81c621a029b3083c37e0b63dde1372545c38edb792fcb65883a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302196d65834f24ec8084628f482199d883010a12846765746888676f312e31372e38856c696e7578a0d8f6c88a59b42eafc6bc6e048e257a415bbb17822d57566153b73a72666b297b880afe7b9f331361e700000000000000000000";
    xbytes_t sync_input = top::from_hex(sync_rlp_headers, ec);
    EXPECT_EQ(ec.value(), 0);
}

TEST_F(xcontract_fixture_t, test_height_property) {
    bigint height(0);
    EXPECT_TRUE(contract.get_height(height));
    EXPECT_EQ(height, bigint(0));

    bigint set_height(9999);
    EXPECT_TRUE(contract.set_height(set_height));
    bigint get_height(0);
    EXPECT_TRUE(contract.get_height(get_height));
    EXPECT_EQ(get_height, bigint(9999));
}

TEST_F(xcontract_fixture_t, test_hash_property) {
    h256 hash{0};
    for (auto i = 0; i < 10; i++) {
        bigint height(i);
        EXPECT_FALSE(contract.get_hash(height, hash));
    }
    for (auto i = 0; i < 100; i++) {
        bigint height(i*10);
        h256 hash(UINT64_MAX - i);
        EXPECT_TRUE(contract.set_hash(height, hash));
    }
    for (auto i = 0; i < 100; i++) {
        bigint height(i*10);
        EXPECT_TRUE(contract.get_hash(height, hash));
        EXPECT_EQ(h256(UINT64_MAX - i), hash);
    }
}

}
}