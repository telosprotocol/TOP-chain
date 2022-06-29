#include "tests/xevm_contract_runtime/test_vem_eth_bridge_contract_fixture.h"
#include "xbasic/xhex.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xeth/xethash.h"
#include "xdepends/include/ethash/ethash.hpp"

#define private public
#include "xevm_common/xeth/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

namespace top {
namespace tests {

using namespace contract_runtime::evm::sys_contract;
using namespace evm_common;
using namespace evm_common::eth;
using namespace ethash;

void header_rlp_bytes_decode(const char * hex_input) {
    std::error_code ec;
    std::vector<xeth_block_header_t> headers;
    auto bytes = top::from_hex(hex_input, ec);
    auto item = RLP::decode_once(bytes);
    auto header_bytes = item.decoded[0];
    xeth_block_header_t header;
    header.from_rlp(header_bytes);
    header.print();
}

TEST(stream, bytes_encode_decode) {
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

TEST(stream, header_encode_decode) {
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

#include "test_evm_eth_bridge_contract_data.cpp"

TEST(ethash, ethash_1270000) {
// header.m_parentHash: 749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6
// header.m_uncleHash: 1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347
// header.m_miner: 00192fb10df37c9fb26829eb2cc623cd1bf599e8
// header.m_stateMerkleRoot: dde529c57dd9924f169825c4415e4779c95f0e02c59af1d8b1f7a833520e5446
// header.m_txMerkleRoot: 9c3c8bbffa9e86ffd84711ad649edc4ae9b657c732028cb3f22acb6822c8a741
// header.m_receiptMerkleRoot: 72cf29f67b3a97c4dabf832725d1ef0780bbc8cb8faf68e7c9c7a124461b622c
// header.m_bloom: 10200000410000000000000080000000000000000200000000010000040000000000000000000200000048000000000882048000080028000000000000200010000000080000013808000408000000200000000004401000080000000000000000000000020000200000000000000840010000000000040000000010000800000000000002000010064000000000000000000000010000180000004000002000120001000000200000004000000000000001020000000000002000021008000000000002400000002000100000000000000000000000001040000002000020000010a00000000000000010000000000010800000200000000200020000000000
// header.m_difficulty: 7656001252874407
// header.m_number: 12970000
// header.m_gasLimit: 29999567
// header.m_gasUsed: 645288
// header.m_time: 1628233655
// header.m_extra: 457468657265756d50504c4e532f326d696e6572735f455533
// header.m_mixDigest: 6af002e55cd5e5c4f4e04c15ac48e9a2d2e88e364639b0713b70a77509caa556
// header.m_nonce: 8af5a4039d877336
// header.hash: 13049bb8cfd97fe2333829f06df37c569db68d42c23097fbac64f2c61471f281
// header.m_baseFee: 28690644740
// header.m_hashed: 1
    std::error_code ec;
    xbytes_t output_bytes = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);

    auto item = RLP::decode(output_bytes);
    xeth_block_header_t header;
    {
        auto item_header = RLP::decode_once(item.decoded[0]);
        auto header_bytes = item_header.decoded[0];
        header.from_rlp(header_bytes);
    }
    auto proofs_per_node = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[item.decoded.size() - 1]));
    std::vector<double_node_with_merkle_proof> nodes;
    uint32_t nodes_size{64};
    uint32_t nodes_start_index{2};
    uint32_t proofs_start_index{2 + nodes_size * 2};
    for (size_t i = 0; i < nodes_size; i++) {
        double_node_with_merkle_proof node;
        node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i]));
        node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i + 1]));
        for (size_t j = 0; j < proofs_per_node; j++) {
            node.proof.emplace_back(static_cast<h128>(item.decoded[proofs_start_index + proofs_per_node * i + j]));
        }
        nodes.emplace_back(node);
    }
    EXPECT_TRUE(xethash_t::instance().verify_seal(header, nodes));
}

TEST(ethash, ethash_1270001) {
    std::error_code ec;
    xbytes_t output_bytes = top::from_hex(relayer_hex_output_1270001, ec);
    EXPECT_EQ(ec.value(), 0);

    auto item = RLP::decode(output_bytes);
    xeth_block_header_t header;
    {
        auto item_header = RLP::decode_once(item.decoded[0]);
        auto header_bytes = item_header.decoded[0];
        header.from_rlp(header_bytes);
    }
    auto proofs_per_node = static_cast<uint64_t>(evm_common::fromBigEndian<u64>(item.decoded[item.decoded.size() - 1]));
    std::vector<double_node_with_merkle_proof> nodes;
    uint32_t nodes_size{64};
    uint32_t nodes_start_index{2};
    uint32_t proofs_start_index{2 + nodes_size * 2};
    for (size_t i = 0; i < nodes_size; i++) {
        double_node_with_merkle_proof node;
        node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i]));
        node.dag_nodes.emplace_back(static_cast<h512>(item.decoded[nodes_start_index + 2 * i + 1]));
        for (size_t j = 0; j < proofs_per_node; j++) {
            node.proof.emplace_back(static_cast<h128>(item.decoded[proofs_start_index + proofs_per_node * i + j]));
        }
        nodes.emplace_back(node);
    }
    EXPECT_TRUE(xethash_t::instance().verify_seal(header, nodes));
}

TEST_F(xcontract_fixture_t, test_init_and_sync) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param1 = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param2 = top::from_hex(relayer_hex_output_1270001, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    EXPECT_TRUE(contract.sync(sync_param1));
    EXPECT_TRUE(contract.sync(sync_param2));
}

TEST_F(xcontract_fixture_t, test_double_init) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    EXPECT_FALSE(contract.init(init_param));
}

TEST_F(xcontract_fixture_t, test_error_init_param) {
    const char * hex_init_param = "f90210a0fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0be5f8f7c84539b81c621a029b3083c37e0b63dde1372545c38edb792fcb65883a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302196d65834f24ec8084628f482199d883010a12846765746888676f312e31372e38856c696e7578a0d8f6c88a59b42eafc6bc6e048e257a415bbb17822d57566153b73a72666b297b880afe7b9f331361e7";
    std::error_code ec;
    auto init_param = top::from_hex(hex_init_param, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.init(init_param));
}

TEST_F(xcontract_fixture_t, test_sync_not_init) {
    std::error_code ec;
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_error_header) {
    const char * hex_init_param = "f90426f90210a0397cb4acc8fe4ac39c6cdcb50a28c727ad257f084020912029a2d6f2ad11d431a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0b5d83f8283dba9ba397287db1f6c0675e9a66a51aef8d7796c32b7da5cd6a982a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302192a64834f11298084628f482099d883010a12846765746888676f312e31372e38856c696e7578a063e47112f6e95c00322701868d568e5d08e393e9430a0fd152e71e7c8be86cac8829ebdb8e839fffacf90210a0fc40ad4f64dd51d09e94d9b7f1136cdcaaf03fb9a75c32661228bd3212547107a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d4934794cf4d39b0edb0b69cd15f687fd45c8fc8eb687daea0be5f8f7c84539b81c621a029b3083c37e0b63dde1372545c38edb792fcb65883a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421a056e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421b90100000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000008302196d65834f24ec8084628f482199d883010a12846765746888676f312e31372e38856c696e7578a0d8f6c88a59b42eafc6bc6e048e257a415bbb17822d57566153b73a72666b297b880afe7b9f331361e7";
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[sync_param.size() - 1] = '9';
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_merkle_proof_error) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[sync_param.size() - 1] = 25;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_get_parent_header_error) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270001, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_verify_common_error1) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[958/2] = 0;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_verify_common_error2) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[930/2] = 0;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_verify_common_error3) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[936/2] = 0;
    sync_param[938/2] = 0;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_verify_common_error4) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[936/2] = 0xff;
    sync_param[938/2] = 0xff;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_verify_common_time) {
    xeth_block_header_t h1;
    xeth_block_header_t h2;
    h1.m_number = 10;
    h2.m_number = 11;
    EXPECT_FALSE(contract.verify_common(h1, h2));
}

TEST_F(xcontract_fixture_t, test_verify_common_extra_size) {
    xeth_block_header_t h1;
    xeth_block_header_t h2;
    h1.m_number = 10;
    h2.m_number = 11;
    h1.m_time = 10;
    h2.m_time = 9;
    h2.m_extra = xbytes_t(64, 1);
    EXPECT_FALSE(contract.verify_common(h1, h2));
}

TEST_F(xcontract_fixture_t, test_verify_common_gaslimit) {
    xeth_block_header_t h1;
    xeth_block_header_t h2;
    h1.m_number = 10;
    h2.m_number = 11;
    h2.m_gasLimit = UINT64_MAX;
    EXPECT_FALSE(contract.verify_common(h1, h2));
}

TEST_F(xcontract_fixture_t, test_verify_common_gasused) {
    xeth_block_header_t h1;
    xeth_block_header_t h2;
    h1.m_number = 10;
    h2.m_number = 11;
    h2.m_gasLimit = 5000;
    h2.m_gasUsed = 6000;
    EXPECT_FALSE(contract.verify_common(h1, h2));
}

TEST_F(xcontract_fixture_t, test_sync_verify_eip1559_error) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[1106/2] = 0;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_difficulty_error) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[922/2] = 0;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_sync_verify_seal_error) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    sync_param[1076/2] = 0;
    EXPECT_FALSE(contract.sync(sync_param));
}

TEST_F(xcontract_fixture_t, test_is_confirmed) {
    const char * hash1_hex = "13049bb8cfd97fe2333829f06df37c569db68d42c23097fbac64f2c61471f281";
    const char * hash2_hex = "98941087c5e71acde89189ef750ee08c7ef265a7d3c2303ef4d07ccaec0de42a";
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    auto hash1 = top::from_hex(hash1_hex, ec);
    EXPECT_EQ(ec.value(), 0);
    auto hash2 = top::from_hex(hash2_hex, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    EXPECT_TRUE(contract.sync(sync_param));
    EXPECT_FALSE(contract.is_confirmed(hash1));
    EXPECT_FALSE(contract.is_confirmed(hash2));
}

static xeth_block_header_t create_header(h256 parent_hash, uint32_t number, uint32_t difficulty) {
    xeth_block_header_t header;
    header.m_parentHash = parent_hash;
    header.m_uncleHash = static_cast<evm_common::h256>(0);
    header.m_miner = static_cast<evm_common::eth::Address>(0);
    header.m_stateMerkleRoot = static_cast<evm_common::h256>(0);
    header.m_txMerkleRoot = static_cast<evm_common::h256>(0);
    header.m_receiptMerkleRoot = static_cast<evm_common::h256>(0);
    header.m_bloom = static_cast<evm_common::eth::LogBloom>(0);
    header.m_mixDigest = static_cast<evm_common::h256>(0);
    header.m_nonce = static_cast<evm_common::h64>(0);
    header.m_difficulty = difficulty;
    header.m_number = number;
    header.m_gasLimit = 0;
    header.m_gasUsed = 0;
    header.m_time = 0;
    header.m_baseFee = static_cast<evm_common::bigint>(0);
    return header;
}

TEST_F(xcontract_fixture_t, test_rebuild1) {
    auto h10 = create_header(static_cast<evm_common::h256>(0), 10, 10);
    auto h11 = create_header(h10.hash(), 11, 10);
    auto h12 = create_header(h11.hash(), 12, 10);
    auto h13 = create_header(h12.hash(), 13, 10);
    auto h14 = create_header(h13.hash(), 14, 10);
    auto h15 = create_header(h14.hash(), 15, 10);

    EXPECT_TRUE(contract.set_header(h10, 10));
    EXPECT_TRUE(contract.set_header(h11, 20));
    EXPECT_TRUE(contract.set_header(h12, 30));
    EXPECT_TRUE(contract.set_header(h13, 40));
    EXPECT_TRUE(contract.set_header(h14, 50));
    EXPECT_TRUE(contract.set_header(h15, 60));
    EXPECT_TRUE(contract.set_hash(10, h10.hash()));
    EXPECT_TRUE(contract.set_hash(11, h11.hash()));
    EXPECT_TRUE(contract.set_hash(12, h12.hash()));
    EXPECT_TRUE(contract.set_hash(13, h13.hash()));
    EXPECT_TRUE(contract.set_hash(14, h14.hash()));
    EXPECT_TRUE(contract.set_hash(15, h15.hash()));
    EXPECT_TRUE(contract.set_height(15));

    auto h12_fork = create_header(h11.hash(), 12, 5);
    auto h13_fork = create_header(h12_fork.hash(), 13, 100);
    EXPECT_TRUE(contract.set_header(h12_fork, 15));
    EXPECT_TRUE(contract.set_header(h13_fork, 115));
    EXPECT_TRUE(contract.rebuild(h15, h13_fork));

    h256 hash;
    EXPECT_TRUE(contract.get_hash(10, hash));
    EXPECT_EQ(hash, h10.hash());
    EXPECT_TRUE(contract.get_hash(11, hash));
    EXPECT_EQ(hash, h11.hash());
    EXPECT_TRUE(contract.get_hash(12, hash));
    EXPECT_EQ(hash, h12_fork.hash());
    EXPECT_TRUE(contract.get_hash(13, hash));
    EXPECT_EQ(hash, h13_fork.hash());
    EXPECT_FALSE(contract.get_hash(14, hash));
    EXPECT_FALSE(contract.get_hash(15, hash));
    bigint height{0};
    EXPECT_TRUE(contract.get_height(height));
    EXPECT_EQ(height, 13);
}

TEST_F(xcontract_fixture_t, test_rebuild2) {
    auto h10 = create_header(static_cast<evm_common::h256>(0), 10, 10);
    auto h11 = create_header(h10.hash(), 11, 10);
    auto h12 = create_header(h11.hash(), 12, 10);
    auto h13 = create_header(h12.hash(), 13, 10);
    auto h14 = create_header(h13.hash(), 14, 10);
    auto h15 = create_header(h14.hash(), 15, 10);

    EXPECT_TRUE(contract.set_header(h10, 10));
    EXPECT_TRUE(contract.set_header(h11, 20));
    EXPECT_TRUE(contract.set_header(h12, 30));
    EXPECT_TRUE(contract.set_header(h13, 40));
    EXPECT_TRUE(contract.set_header(h14, 50));
    EXPECT_TRUE(contract.set_header(h15, 60));
    EXPECT_TRUE(contract.set_hash(10, h10.hash()));
    EXPECT_TRUE(contract.set_hash(11, h11.hash()));
    EXPECT_TRUE(contract.set_hash(12, h12.hash()));
    EXPECT_TRUE(contract.set_hash(13, h13.hash()));
    EXPECT_TRUE(contract.set_hash(14, h14.hash()));
    EXPECT_TRUE(contract.set_hash(15, h15.hash()));
    EXPECT_TRUE(contract.set_height(15));

    auto h12_fork = create_header(h11.hash(), 12, 5);
    auto h13_fork = create_header(h12_fork.hash(), 13, 5);
    auto h14_fork = create_header(h13_fork.hash(), 14, 5);
    auto h15_fork = create_header(h14_fork.hash(), 15, 5);
    auto h16_fork = create_header(h15_fork.hash(), 16, 5);
    auto h17_fork = create_header(h16_fork.hash(), 17, 100);
    EXPECT_TRUE(contract.set_header(h12_fork, 15));
    EXPECT_TRUE(contract.set_header(h13_fork, 20));
    EXPECT_TRUE(contract.set_header(h14_fork, 25));
    EXPECT_TRUE(contract.set_header(h15_fork, 30));
    EXPECT_TRUE(contract.set_header(h16_fork, 35));
    EXPECT_TRUE(contract.set_header(h17_fork, 135));
    EXPECT_TRUE(contract.rebuild(h15, h17_fork));

    h256 hash;
    EXPECT_TRUE(contract.get_hash(10, hash));
    EXPECT_EQ(hash, h10.hash());
    EXPECT_TRUE(contract.get_hash(11, hash));
    EXPECT_EQ(hash, h11.hash());
    EXPECT_TRUE(contract.get_hash(12, hash));
    EXPECT_EQ(hash, h12_fork.hash());
    EXPECT_TRUE(contract.get_hash(13, hash));
    EXPECT_EQ(hash, h13_fork.hash());
    EXPECT_TRUE(contract.get_hash(14, hash));
    EXPECT_EQ(hash, h14_fork.hash());
    EXPECT_TRUE(contract.get_hash(15, hash));
    EXPECT_EQ(hash, h15_fork.hash());
    EXPECT_TRUE(contract.get_hash(16, hash));
    EXPECT_EQ(hash, h16_fork.hash());
    EXPECT_TRUE(contract.get_hash(17, hash));
    EXPECT_EQ(hash, h17_fork.hash());
    bigint height{0};
    EXPECT_TRUE(contract.get_height(height));
    EXPECT_EQ(height, 17);
}

TEST_F(xcontract_fixture_t, execute_input_empty) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    EXPECT_FALSE(contract.execute({}, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_input_invalid) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::string str{relayer_hex_output_init_1269999_tx_data};
    str.insert(str.begin(), 'x');
    str.insert(str.begin(), '0');
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_output_init_1269999_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_input_invalid_method_id) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::string str{relayer_hex_output_init_1269999_tx_data};
    str[0] = '5';
    std::error_code ec;
    auto init_param = top::from_hex(str, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t,  execute_method_id_extract_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
    std::string str{relayer_hex_output_get_height_tx_date};
    str = {str.begin(), str.begin() + 4};
    auto height_param = top::from_hex(str, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(height_param, 0, context, true, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_init_extract_error1) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::string str{relayer_hex_output_init_1269999_tx_data};
    str = {str.begin(), str.begin() + 8};
    std::error_code ec;
    auto init_param = top::from_hex(str, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_init_extract_error2) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::string str{relayer_hex_output_init_1269999_tx_data};
    str = {str.begin(), str.begin() + 1298};
    std::error_code ec;
    auto init_param = top::from_hex(str, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_init_static_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_output_init_1269999_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, true, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_init_verify_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_output_init_1269999_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t,  execute_sync_ok) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));

    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t, execute_sync_extract_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));

    std::string str{relayer_hex_output_sync_1270000_tx_data};
    str = {str.begin(), str.begin() + 8};
    auto sync_param = top::from_hex(str, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t,  execute_sync_static_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));

    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(sync_param, 0, context, true, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t,  execute_sync_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));

    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
    EXPECT_FALSE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t,  execute_get_height) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
    auto height_param = top::from_hex(relayer_hex_output_get_height_tx_date, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(height_param, 0, context, true, statectx_observer, output, err));
    u256 h = evm_common::fromBigEndian<u256>(output.output);
    EXPECT_EQ(h, 12970000);
}

TEST_F(xcontract_fixture_t,  execute_is_confirmed) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
    auto is_confirmed_param = top::from_hex(relayer_hex_output_is_confirmed_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(is_confirmed_param, 0, context, false, statectx_observer, output, err));
    EXPECT_EQ(evm_common::fromBigEndian<u256>(output.output), 0);
}

TEST_F(xcontract_fixture_t,  execute_is_confirmed_extract_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param));
    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));

    std::string str{relayer_hex_output_is_confirmed_tx_data};
    str = {str.begin(), str.begin() + 136};
    auto is_confirmed_param = top::from_hex(str, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(is_confirmed_param, 0, context, false, statectx_observer, output, err));
    EXPECT_EQ(evm_common::fromBigEndian<u256>(output.output), 0);
}

}
}