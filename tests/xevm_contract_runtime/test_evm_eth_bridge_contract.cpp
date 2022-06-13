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

void print_header(xeth_block_header_t & header) {
    printf("header.m_parentHash: %s\n", header.m_parentHash.hex().c_str());
    printf("header.m_uncleHash: %s\n", header.m_uncleHash.hex().c_str());
    printf("header.m_miner: %s\n", header.m_miner.hex().c_str());
    printf("header.m_stateMerkleRoot: %s\n", header.m_stateMerkleRoot.hex().c_str());
    printf("header.m_txMerkleRoot: %s\n", header.m_txMerkleRoot.hex().c_str());
    printf("header.m_receiptMerkleRoot: %s\n", header.m_receiptMerkleRoot.hex().c_str());
    printf("header.m_bloom: %s\n", header.m_bloom.hex().c_str());
    printf("header.m_difficulty: %s\n", header.m_difficulty.str().c_str());
    printf("header.m_number: %s\n", header.m_number.str().c_str());
    printf("header.m_gasLimit: %lu\n", header.m_gasLimit);
    printf("header.m_gasUsed: %lu\n", header.m_gasUsed);
    printf("header.m_time: %lu\n", header.m_time);
    printf("header.m_extra: %s\n", top::to_hex(header.m_extra).c_str());
    printf("header.m_mixDigest: %s\n", header.m_mixDigest.hex().c_str());
    printf("header.m_nonce: %s\n", header.m_nonce.hex().c_str());
    printf("header.hash: %s\n", header.hash().hex().c_str());
    printf("header.m_baseFee: %s\n", header.m_baseFee.str().c_str());
    printf("header.m_hashed: %d\n", header.m_hashed);
    printf("header.m_isBaseFee: %d\n", header.m_isBaseFee);
}

void header_rlp_bytes_decode(const char * hex_input) {
    std::error_code ec;
    std::vector<xeth_block_header_t> headers;
    auto bytes = top::from_hex(hex_input, ec);
    auto item = RLP::decode_once(bytes);
    auto header_bytes = item.decoded[0];
    xeth_block_header_t header;
    header.from_rlp(header_bytes);
    print_header(header);
}

void headers_rlp_bytes_decode(const char * hex_input) {
    std::error_code ec;
    xbytes_t input = top::from_hex(hex_input, ec);
    std::vector<xeth_block_header_t> headers;

    auto decode_item = RLP::decode_once(input);
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
    for (size_t i = 0; i < headers.size(); i++) {
        auto header = headers[i];
        print_header(header);
    }
}

void raw_input_decode(const char * hex_input) {
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
    for (size_t i = 0; i < headers.size(); i++) {
        auto header = headers[i];
        print_header(header);
    }
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

TEST(eth_hash, ethash_with_basefee) {
    // "parentHash":"0x6d70280cdec5d2cfa1a3dee20d3d1fc1aa65c80ac742a3c7e8c298a697fccb82",
    // "sha3Uncles":"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347",
    // "miner":"0x0000000000000000000000000000000000000000",
    // "stateRoot":"0xc3ecb60b288fa9c23a0a427733aab8dfcc5cddbcbcd7b39b5f7650869082c82c",
    // "transactionsRoot":"0x55ef1e24d76cffa46675b69682003f6424034a888ef57227d043bfd01e0d3fef",
    // "receiptsRoot":"0x593ade3839f1cf8533f17205cf41f29755e2f0f37efc4cd1b7ad66abe805820a",
    // "logsBloom":"0x0c2010080200000040800000400060100000000004000100008100000c84020080080000000002001001000000004000000412220624a00000200000002c20418200384040000000a20000280000008001090108022400000008800040000008088884000a00c008080841500100080000001001400000800000111000000060008108040244040001510422000000100000380280000000000100044000010082000000000088044000010840000008000081004081200420a00c48002440014004002201a000080000000000000000820004008008000001000130000065000012001002000400000000020002019000001000008020000000080402842413",
    // "difficulty":"0x1",
    // "number":"0xa4814e",
    // "gasLimit":"0x1c9c380",
    // "gasUsed":"0x322351",
    // "timestamp":"0x62981b2b",
    // "extraData":"0x696e667572612e696f00000000000000000000000000000000000000000000007cddbf742833598f47b9641981105fb1b32d5eafc8e114e7685af196fbde6cca1fcfba80770080afd0f7094406da275eb16d5526c25a9d135b6b0ee68f30fee800",
    // "mixHash":"0x0000000000000000000000000000000000000000000000000000000000000000",
    // "nonce":"0x0000000000000000",
    // "baseFeePerGas":"0x9",
    // "hash":"0xe1160b7e6941ffea73a9fb031cd86fdfc9a87573449da4d1a186038ae8288b42"}
    xeth_block_header_t header;
    std::error_code ec;
    header.m_parentHash = static_cast<evm_common::h256>(top::from_hex("6d70280cdec5d2cfa1a3dee20d3d1fc1aa65c80ac742a3c7e8c298a697fccb82", ec));
    header.m_uncleHash = static_cast<evm_common::h256>(top::from_hex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347", ec));
    header.m_miner = static_cast<evm_common::h160>(top::from_hex("0000000000000000000000000000000000000000", ec));
    header.m_stateMerkleRoot = static_cast<evm_common::h256>(top::from_hex("c3ecb60b288fa9c23a0a427733aab8dfcc5cddbcbcd7b39b5f7650869082c82c", ec));
    header.m_txMerkleRoot = static_cast<evm_common::h256>(top::from_hex("55ef1e24d76cffa46675b69682003f6424034a888ef57227d043bfd01e0d3fef", ec));
    header.m_receiptMerkleRoot = static_cast<evm_common::h256>(top::from_hex("593ade3839f1cf8533f17205cf41f29755e2f0f37efc4cd1b7ad66abe805820a", ec));
    header.m_bloom = static_cast<evm_common::h2048>(top::from_hex("0c2010080200000040800000400060100000000004000100008100000c84020080080000000002001001000000004000000412220624a00000200000002c20418200384040000000a20000280000008001090108022400000008800040000008088884000a00c008080841500100080000001001400000800000111000000060008108040244040001510422000000100000380280000000000100044000010082000000000088044000010840000008000081004081200420a00c48002440014004002201a000080000000000000000820004008008000001000130000065000012001002000400000000020002019000001000008020000000080402842413", ec));
    header.m_difficulty = 0x1;
    header.m_number = 0xa4814e;
    header.m_gasLimit = 0x1c9c380;
    header.m_gasUsed = 0x322351;
    header.m_time = 0x62981b2b;
    header.m_extra = top::from_hex("696e667572612e696f00000000000000000000000000000000000000000000007cddbf742833598f47b9641981105fb1b32d5eafc8e114e7685af196fbde6cca1fcfba80770080afd0f7094406da275eb16d5526c25a9d135b6b0ee68f30fee800", ec);
    header.m_mixDigest = static_cast<evm_common::h256>(top::from_hex("0000000000000000000000000000000000000000000000000000000000000000", ec));
    header.m_nonce = static_cast<evm_common::h64>(top::from_hex("0000000000000000", ec));
    header.m_baseFee = 0x9;
    header.m_isBaseFee = true;
    header.m_hashed = false;
    EXPECT_EQ(header.hash().hex(), "e1160b7e6941ffea73a9fb031cd86fdfc9a87573449da4d1a186038ae8288b42");
}

TEST(eth_hash, ethash_without_basefee) {
    /*std::string str = "{\
        \"difficulty\": \"0x200c0\",\
        \"extraData\": \"0xd983010a0f846765746889676f312e31352e3135856c696e7578\",\
        \"gasLimit\": \"0x47ff2c\",\
        \"gasUsed\": \"0x0\",\
        \"hash\": \"0x8946a06b7aab41b68ceb49cda930d595fef38b8d8df13dff29cb92dc7507e2ac\",\
        \"logsBloom\": \"0x00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000\",\
        \"miner\": \"0x55b82560cf9f787705a106b8142a82be7f13c266\",\
        \"mixHash\": \"0x076d53f285449f2d4508da903bd63b4817341d8543b534ce94bc38c6715fd191\",\
        \"nonce\": \"0x3c6c5cd91a9cbd8e\",\
        \"number\": \"0x4\",\
        \"parentHash\": \"0x2b1ec97e4d25b99e006079c851c3ca1435ff5881c480517a3278af35a87e6a3a\",\
        \"receiptsRoot\": \"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421\",\
        \"sha3Uncles\": \"0x1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347\",\
        \"size\": \"0x219\",\
        \"stateRoot\": \"0xe3638dd1d7693222e09ac20388cce125fde4f61707b6ba77ad5e76f51f8f9662\",\
        \"timestamp\": \"0x62625f01\",\
        \"totalDifficulty\": \"0x80182\",\
        \"transactions\": [],\
        \"transactionsRoot\": \"0x56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421\",\
        \"uncles\": []\
        }";*/
    xeth_block_header_t header;
    std::error_code ec;
    header.m_parentHash = static_cast<evm_common::h256>(top::from_hex("2b1ec97e4d25b99e006079c851c3ca1435ff5881c480517a3278af35a87e6a3a", ec));
    header.m_uncleHash = static_cast<evm_common::h256>(top::from_hex("1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347", ec));
    header.m_miner = static_cast<evm_common::h160>(top::from_hex("55b82560cf9f787705a106b8142a82be7f13c266", ec));
    header.m_stateMerkleRoot = static_cast<evm_common::h256>(top::from_hex("e3638dd1d7693222e09ac20388cce125fde4f61707b6ba77ad5e76f51f8f9662", ec));
    header.m_txMerkleRoot = static_cast<evm_common::h256>(top::from_hex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421", ec));
    header.m_receiptMerkleRoot = static_cast<evm_common::h256>(top::from_hex("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421", ec));
    header.m_bloom = static_cast<evm_common::h2048>(top::from_hex("00000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000", ec));
    header.m_difficulty = 0x200c0;
    header.m_number = 0x4;
    header.m_gasLimit = 0x47ff2c;
    header.m_gasUsed = 0x0;
    header.m_time = 0x62625f01;
    header.m_extra = top::from_hex("d983010a0f846765746889676f312e31352e3135856c696e7578", ec);
    header.m_mixDigest = static_cast<evm_common::h256>(top::from_hex("076d53f285449f2d4508da903bd63b4817341d8543b534ce94bc38c6715fd191", ec));
    header.m_nonce = static_cast<evm_common::h64>(top::from_hex("3c6c5cd91a9cbd8e", ec));
    EXPECT_EQ(header.hash().hex(), "8946a06b7aab41b68ceb49cda930d595fef38b8d8df13dff29cb92dc7507e2ac");
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
}

TEST(eth_header, rlp_decode_init) {
    const char * hex_input = "78dcb6c7000000000000000000000000000000000000000000000000000000000000004000000000000000000000000000000000000000000000000000000000000002c00000000000000000000000000000000000000000000000000000000000000260f9025da09f08f9313e362e1faa0fbb4a3534416dd82ce7797e8f194bc22b7c76dcdd7911a01dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347940000000000000000000000000000000000000000a0c9864d89fd30d28e1e05402d1fc12244ae5067a2f0dd21edbf10e62bf78465aca01be9adc56250943063e10082284c744156ed4580c8be0deae48b1fa057845697a0d3e02d72b7070748469fb93e794205c0e6357402520436660dd9106fe18eb42db9010040a6010c02400000108800c080004a50408a080000022000848918002c0002022010152401000280410004100000020080200000400020002220008180a420400000081042202120a50001081000442683010709420000420008000000040000008b80404244800888204822010008011a01202140106c2001001034010814481010000002400400004004020001181420008010c61064282111004460480440c2208010008288444c8110480800000a000094c00000020c03900e000681080000040203a0c00004040060009000200082800c02000c0010400400210804b088001620180108086000002402000a0032800010180290a00484140990900440180183a4814d8401c9c364838d31238462981b1cb861d883010a11846765746888676f312e31372e38856c696e757800000000000000a26ffe223af0fd86a323a3068735dd0fe36a8a0bc51547c85e756d42f7bd26db73f0227251f63142f73488ade3638c0632654b42a5c013e6d923805eca4b2dbe01a00000000000000000000000000000000000000000000000000000000000000000880000000000000000";
    std::error_code ec;
    xbytes_t input = top::from_hex(hex_input, ec);
    EXPECT_EQ(ec.value(), 0);
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(input, ec);
    EXPECT_EQ(ec.value(), 0);
    auto function_selector = abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_EQ(function_selector.method_id, 0x78dcb6c7);
    auto bytes = abi_decoder.extract<xbytes_t>(ec);
    EXPECT_EQ(ec.value(), 0);
    std::vector<xeth_block_header_t> headers;
    while (bytes.size() > 0) {
        auto item = RLP::decode_once(bytes);
        bytes = item.remainder;
        xassert(item.decoded.size() == 1);
        auto header_bytes = item.decoded[0];
        xeth_block_header_t header;
        header.from_rlp(header_bytes);
        headers.emplace_back(header);
    }
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