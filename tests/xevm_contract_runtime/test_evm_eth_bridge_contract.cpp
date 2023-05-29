#include "tests/xevm_contract_runtime/test_evm_eth_bridge_contract_fixture.h"
#include "xbasic/xhex.h"
#include "xcommon/rlp.h"
#include "xevm_common/xabi_decoder.h"
#include "xevm_common/xcrosschain/xethash.h"
#include "ethash/ethash.hpp"

#define private public
// #include "gperftools/src/base/logging.h"
#include "xevm_common/xcrosschain/xeth_header.h"
#include "xevm_contract_runtime/xevm_sys_contract_face.h"

namespace top {
namespace tests {

using namespace contract_runtime::evm::sys_contract;
using namespace evm_common;
using namespace evm_common::eth;
using namespace ethash;

TEST_F(xcontract_fixture_t, header_encode_decode) {
    evm_common::xeth_header_t header;
    evm_common::xeth_header_t header_decode;
    header.parent_hash = static_cast<evm_common::h256>(UINT32_MAX - 1);
    header.uncle_hash = static_cast<evm_common::h256>(UINT32_MAX - 2);
    header.miner = common::xeth_address_t::build_from(static_cast<evm_common::Address>(UINT32_MAX - 3).asBytes());
    header.state_root = static_cast<evm_common::h256>(UINT32_MAX - 4);
    header.transactions_root = static_cast<evm_common::h256>(UINT32_MAX - 5);
    header.receipts_root = static_cast<evm_common::h256>(UINT32_MAX - 6);
    header.bloom = static_cast<evm_common::LogBloom>(UINT32_MAX - 7);
    header.mix_digest = static_cast<evm_common::h256>(UINT32_MAX - 8);
    header.nonce = static_cast<xh64_t>(UINT32_MAX - 9);
    header.difficulty = UINT64_MAX - 1;
    header.number = UINT64_MAX - 2;
    header.gas_limit = UINT64_MAX - 3;
    header.gas_used = UINT64_MAX - 4;
    header.time = UINT64_MAX - 5;
    header.base_fee_per_gas = UINT64_MAX - 6;
    header.extra = {1, 3, 5, 7};

    EXPECT_TRUE(contract.set_header(header.calc_hash(), header, contract_state));
    EXPECT_TRUE(contract.get_header(header.calc_hash(), header_decode, contract_state));

    EXPECT_TRUE(header.parent_hash == header_decode.parent_hash);
    EXPECT_TRUE(header.uncle_hash == header_decode.uncle_hash);
    EXPECT_EQ(header.miner, header_decode.miner);
    EXPECT_TRUE(header.state_root == header_decode.state_root);
    EXPECT_TRUE(header.transactions_root == header_decode.transactions_root);
    EXPECT_TRUE(header.receipts_root == header_decode.receipts_root);
    EXPECT_EQ(header.bloom, header_decode.bloom);
    EXPECT_TRUE(header.difficulty == header_decode.difficulty);
    EXPECT_TRUE(header.number == header_decode.number);
    EXPECT_EQ(header.gas_limit, header_decode.gas_limit);
    EXPECT_EQ(header.gas_used, header_decode.gas_used);
    EXPECT_EQ(header.time, header_decode.time);
    EXPECT_EQ(header.extra, header_decode.extra);
    EXPECT_TRUE(header.mix_digest == header_decode.mix_digest);
    EXPECT_TRUE(header.nonce == header_decode.nonce);
    EXPECT_TRUE(header.base_fee_per_gas == header_decode.base_fee_per_gas);
}

TEST_F(xcontract_fixture_t, test_last_hash_property) {
    EXPECT_TRUE(contract.get_last_hash(contract_state) == h256());
    EXPECT_TRUE(contract.set_last_hash(h256(9999), contract_state));
    EXPECT_TRUE(contract.get_last_hash(contract_state) == h256(9999));
}

TEST_F(xcontract_fixture_t, test_headers_property) {
    for (auto i = 0; i < 10; i++) {
        xeth_header_t header;
        EXPECT_FALSE(contract.get_header(h256(i), header, contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        xeth_header_t header;
        header.number = i;
        EXPECT_TRUE(contract.set_header(h256(i), header, contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        xeth_header_t header;
        header.number = i;
        xeth_header_t get_header;
        EXPECT_TRUE(contract.get_header(h256(i), get_header, contract_state));
        EXPECT_TRUE(header.calc_hash() == get_header.calc_hash());
        return;
    }
    for (auto i = 0; i < 50; i++) {
        EXPECT_TRUE(contract.remove_header(h256(i), contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        if (i < 50) {
            xeth_header_t get_header;
            EXPECT_FALSE(contract.get_header(h256(i), get_header, contract_state));
        } else {
            xeth_header_t header;
            header.number = i;
            xeth_header_t get_header;
            EXPECT_TRUE(contract.get_header(h256(i), get_header, contract_state));
            EXPECT_TRUE(header.calc_hash() == get_header.calc_hash());
        }
    }
}

TEST_F(xcontract_fixture_t, test_headers_summary_property) {
    for (auto i = 0; i < 10; i++) {
        xeth_header_info_t info;
        EXPECT_FALSE(contract.get_header_info(h256(i), info, contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        xeth_header_info_t info;
        info.number = i;
        EXPECT_TRUE(contract.set_header_info(h256(i), info, contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        xeth_header_info_t info;
        info.number = i;
        xeth_header_info_t get_info;
        EXPECT_TRUE(contract.get_header_info(h256(i), get_info, contract_state));
        EXPECT_TRUE(info.encode_rlp() == get_info.encode_rlp());
    }
    for (auto i = 0; i < 50; i++) {
        EXPECT_TRUE(contract.remove_header_info(h256(i), contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        if (i < 50) {
            xeth_header_info_t info;
            EXPECT_FALSE(contract.get_header_info(h256(i), info, contract_state));
        } else {
            xeth_header_info_t info;
            info.number = i;
            xeth_header_info_t get_info;
            EXPECT_TRUE(contract.get_header_info(h256(i), get_info, contract_state));
            EXPECT_TRUE(info.encode_rlp() == get_info.encode_rlp());
        }
    }
}

TEST_F(xcontract_fixture_t, test_effective_hash_property) {
    for (auto i = 0; i < 10; i++) {
        h256 info;
        EXPECT_TRUE(contract.get_effective_hash(i, contract_state) == h256());
    }
    for (auto i = 0; i < 100; i++) {
        EXPECT_TRUE(contract.set_effective_hash(i, h256(i), contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        EXPECT_TRUE(contract.get_effective_hash(i, contract_state) == h256(i));
    }
    for (auto i = 0; i < 50; i++) {
        EXPECT_TRUE(contract.remove_effective_hash(i, contract_state));
    }
    for (auto i = 0; i < 100; i++) {
        if (i < 50) {
            EXPECT_TRUE(contract.get_effective_hash(i, contract_state) == h256());
        } else {
            EXPECT_TRUE(contract.get_effective_hash(i, contract_state) == h256(i));
        }
    }
}

TEST_F(xcontract_fixture_t, test_all_hashes_property) {
    std::set<h256> h10{h256(1), h256(2), h256(3), h256(4), h256(5)};
    EXPECT_TRUE(contract.set_hashes(10, h10, contract_state));
    auto hashes = contract.get_hashes(10, contract_state);
    EXPECT_TRUE(h10 == hashes);
    EXPECT_TRUE(contract.remove_hashes(10, contract_state));
    hashes = contract.get_hashes(10, contract_state);
    EXPECT_TRUE(hashes.empty());
}

#include "test_evm_eth_bridge_contract_data.cpp"
// parent_hash: 688cbef142d64af10ff3bfed7463d11cc096493b926c8257e6dc3ab8ea8ee830
// uncle_hash: 1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347
// miner: 5a0b54d5dc17e0aadc383d2db43b0a0d3e029c4c
// state_merkleroot: 0dde65ccd7eb91b96ddd017e5b6983b0a86ca49f9b0ad8edb3f6bbcf4818af5c
// tx_merkleroot: 5e5b28e01547c6b46eab83f82062a513e42dec27a1a2ad3dc51dea6889f6dce0
// receipt_merkleroot: a8a266ccda44bc2fa160c0190b447932a2f2b5b34a2a178175baa38331b713e7
// bloom: 54f7de3e69a6dfc15bc046f2a2c8423b8286907297b5400c083566f9beca8107be8cf1393feedefcf7587f9350232df55aa7a2519d2d6d37ab96c0d420e02a702a40486da339e7abaaa6222fbb32f0e40b460c44b4723e160b465d6ac5c513fe5f02200b423e0b6c254551cd28956ff9783abc5bb87fcccacae3e5f8b85bc0369cb087b88e4f1b71937d15fa2997673fc2059d6731f22568a7e32cd4e3fd2da0e2fb96a333bca54b567b639ea8039e8e9ea0820b7ae6d04d4f2b77e21aadd117c03fb6a27f201feb3b652293b09b31504b3e026c1edebcd573c263b32b75f882d5b8284eae1445252217341091f022c6a6e3a7d12d5889508e08adf86acfd26d
// difficulty: 7652263722236960
// number: 12969999
// gas_limit: 29970301
// gas_used: 18001521
// time: 1628233653
// extra: d883010a06846765746888676f312e31362e36856c696e7578
// mix_digest: 51b6613a3f989905ea1c55a095ad8b1e1d7caabb9f890a60c880710db40eb294
// nonce: 99dcdb535b6987c1
// hash: 749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6
// base_fee: 27986467855

// parent_hash: 749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6
// uncle_hash: 1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347
// miner: 00192fb10df37c9fb26829eb2cc623cd1bf599e8
// state_merkleroot: dde529c57dd9924f169825c4415e4779c95f0e02c59af1d8b1f7a833520e5446
// tx_merkleroot: 9c3c8bbffa9e86ffd84711ad649edc4ae9b657c732028cb3f22acb6822c8a741
// receipt_merkleroot: 72cf29f67b3a97c4dabf832725d1ef0780bbc8cb8faf68e7c9c7a124461b622c
// bloom: 10200000410000000000000080000000000000000200000000010000040000000000000000000200000048000000000882048000080028000000000000200010000000080000013808000408000000200000000004401000080000000000000000000000020000200000000000000840010000000000040000000010000800000000000002000010064000000000000000000000010000180000004000002000120001000000200000004000000000000001020000000000002000021008000000000002400000002000100000000000000000000000001040000002000020000010a00000000000000010000000000010800000200000000200020000000000
// difficulty: 7656001252874407
// number: 12970000
// gas_limit: 29999567
// gas_used: 645288
// time: 1628233655
// extra: 457468657265756d50504c4e532f326d696e6572735f455533
// mix_digest: 6af002e55cd5e5c4f4e04c15ac48e9a2d2e88e364639b0713b70a77509caa556
// nonce: 8af5a4039d877336
// hash: 13049bb8cfd97fe2333829f06df37c569db68d42c23097fbac64f2c61471f281
// base_fee: 28690644740

// parent_hash: 13049bb8cfd97fe2333829f06df37c569db68d42c23097fbac64f2c61471f281
// uncle_hash: 1dcc4de8dec75d7aab85b567b6ccd41ad312451b948a7413f0a142fd40d49347
// miner: ea674fdde714fd979de3edf0f56aa9716b898ec8
// state_merkleroot: 9f1c3e7aa02f252555f4de00f87f270779fbe6a1c90f3f43e6e0f420eead6c36
// tx_merkleroot: 8baa0ea7ef6acbc834e1e707df765e85b265f51ff238d6c940f526233bbb0aae
// receipt_merkleroot: 1164615a3f7f890089fc16b16fe3d4c7d7e9fac71debea357db154fdbec60411
// bloom: 70f15b52e5937b2910f025def3a1903bff1994d2be5235645331206abe92e0576c6f775a22cbbed0a059df8f863d2398ff0fbbc95d3dff2183c29b3463f5ec5f38bcf846c9be513cddf6b02e17d975fceda3f46c68e0bf156e27df6aca6f03827fe0ec0d239e4b209eaaffb0cb530b7bb04f8d11088b4e63dee04d9e9b7acd9ef60821253cbffa90b6780d69ffa65eff543d952965142c18aaab6676a85116b082dd70ab5fe9e5c25c7bcbd6e934cfb1f361efdf1683e1c06136cb4b3f1d61747356d6423a3b81e33467327e27af5b2df91d9323d50f92127ea467fe167ce676f9397a2eaad61561c5159a326d9a868d56f6b4c1e772e2fe067c8921526b74db
// difficulty: 7659740608477986
// number: 12970001
// gas_limit: 30000000
// gas_used: 29980167
// time: 1628233658
// extra: 65746865726d696e652d75732d6561737431
// mix_digest: 79c1383210280b431543525da4950695a902920e0ac3133f25f8ddf537514bb0
// nonce: 13d0d10002c6671e
// hash: ffcaf92863a2cf8bee2ac451a06c93a839403d9a384cbfa4df36d9cb59e028fe
// base_fee: 25258597453

static void verify_ethash(xbytes_t & left_bytes) {
    while (left_bytes.size() != 0) {
        RLP::DecodedItem item = RLP::decode(left_bytes);
        left_bytes = std::move(item.remainder);
        xeth_header_t header;
        {
            auto item_header = RLP::decode_once(item.decoded[0]);
            auto header_bytes = item_header.decoded[0];
            header.decode_rlp(header_bytes);
        }
        auto proofs_per_node = evm_common::fromBigEndian<uint64_t>(item.decoded[item.decoded.size() - 1]);
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
}

TEST(ethash, ethash_1270000) {
    std::error_code ec;
    xbytes_t output_bytes = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    verify_ethash(output_bytes);
}

TEST(ethash, ethash_1270001) {
    std::error_code ec;
    xbytes_t output_bytes = top::from_hex(relayer_hex_output_1270001, ec);
    EXPECT_EQ(ec.value(), 0);
    verify_ethash(output_bytes);
}

TEST(ethash, ethash_batch) {
    std::error_code ec;
    xbytes_t left_bytes = top::from_hex(relayer_hex_output_batch, ec);
    EXPECT_EQ(ec.value(), 0);
    verify_ethash(left_bytes);
}

TEST_F(xcontract_fixture_t, test_init_and_sync) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param1 = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param2 = top::from_hex(relayer_hex_output_1270001, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param, contract_state));
    EXPECT_TRUE(contract.sync(sync_param1, contract_state));
    EXPECT_TRUE(contract.sync(sync_param2, contract_state));

    auto bytes12969999 = from_hex("749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6", ec);
    EXPECT_TRUE(contract.get_effective_hash(12969999, contract_state) == static_cast<h256>(bytes12969999));
    auto bytes12970000 = from_hex("13049bb8cfd97fe2333829f06df37c569db68d42c23097fbac64f2c61471f281", ec);
    EXPECT_TRUE(contract.get_effective_hash(12970000, contract_state) == static_cast<h256>(bytes12970000));
    auto bytes12970001 = from_hex("ffcaf92863a2cf8bee2ac451a06c93a839403d9a384cbfa4df36d9cb59e028fe", ec);
    EXPECT_TRUE(contract.get_effective_hash(12970001, contract_state) == static_cast<h256>(bytes12970001));

    EXPECT_TRUE(contract.get_last_hash(contract_state) == static_cast<h256>(bytes12970001));
    EXPECT_EQ(12970001, contract.get_height(contract_state));

    auto hashes = contract.get_hashes(12969999, contract_state);
    EXPECT_EQ(hashes.size(), 1);
    EXPECT_TRUE(hashes.count(static_cast<h256>(bytes12969999)));
    hashes = contract.get_hashes(12970000, contract_state);
    EXPECT_EQ(hashes.size(), 1);
    EXPECT_TRUE(hashes.count(static_cast<h256>(bytes12970000)));
    hashes = contract.get_hashes(12970001, contract_state);
    EXPECT_EQ(hashes.size(), 1);
    EXPECT_TRUE(hashes.count(static_cast<h256>(bytes12970001)));

    xeth_header_info_t info;
    EXPECT_TRUE(contract.get_header_info(static_cast<h256>(bytes12969999), info, contract_state));
    EXPECT_EQ(info.number, 12969999);
    EXPECT_EQ(info.difficult_sum, 7652263722236960);
    EXPECT_TRUE(contract.get_header_info(static_cast<h256>(bytes12970000), info, contract_state));
    EXPECT_EQ(info.number, 12970000);
    EXPECT_EQ(info.difficult_sum, 7652263722236960 + 7656001252874407);
    EXPECT_TRUE(info.parent_hash == static_cast<h256>(bytes12969999));
    EXPECT_TRUE(contract.get_header_info(static_cast<h256>(bytes12970001), info, contract_state));
    EXPECT_EQ(info.number, 12970001);
    EXPECT_EQ(info.difficult_sum, 7652263722236960 + 7656001252874407 + 7659740608477986);
    EXPECT_TRUE(info.parent_hash == static_cast<h256>(bytes12970000));
}

TEST_F(xcontract_fixture_t, test_double_init) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param, contract_state));
    EXPECT_FALSE(contract.init(init_param, contract_state));
}

TEST_F(xcontract_fixture_t, test_sync_not_init) {
    std::error_code ec;
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.sync(sync_param, contract_state));
}

TEST_F(xcontract_fixture_t, test_sync_get_parent_header_error) {
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270001, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param, contract_state));
    EXPECT_FALSE(contract.sync(sync_param, contract_state));
}

TEST_F(xcontract_fixture_t, test_verify_common_time) {
    xeth_header_t h1;
    xeth_header_t h2;
    h1.number = 10;
    h2.number = 11;
    EXPECT_FALSE(contract.verify(h1, h2, {}));
}

TEST_F(xcontract_fixture_t, test_verify_common_extra_size) {
    xeth_header_t h1;
    xeth_header_t h2;
    h1.number = 10;
    h2.number = 11;
    h1.time = 10;
    h2.time = 9;
    h2.extra = xbytes_t(64, 1);
    EXPECT_FALSE(contract.verify(h1, h2, {}));
}

TEST_F(xcontract_fixture_t, test_verify_common_gaslimit) {
    xeth_header_t h1;
    xeth_header_t h2;
    h1.number = 10;
    h2.number = 11;
    h2.gas_limit = UINT64_MAX;
    EXPECT_FALSE(contract.verify(h1, h2, {}));
}

TEST_F(xcontract_fixture_t, test_verify_common_gasused) {
    xeth_header_t h1;
    xeth_header_t h2;
    h1.number = 10;
    h2.number = 11;
    h2.gas_limit = 5000;
    h2.gas_used = 6000;
    EXPECT_FALSE(contract.verify(h1, h2, {}));
}

TEST_F(xcontract_fixture_t, test_is_confirmed) {
    const char * hash1_hex = "749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6";
    const char * hash2_hex = "13049bb8cfd97fe2333829f06df37c569db68d42c23097fbac64f2c61471f281";
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    auto sync_param = top::from_hex(relayer_hex_output_1270000, ec);
    EXPECT_EQ(ec.value(), 0);
    auto hash1 = top::from_hex(hash1_hex, ec);
    EXPECT_EQ(ec.value(), 0);
    auto hash2 = top::from_hex(hash2_hex, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param, contract_state));
    EXPECT_TRUE(contract.sync(sync_param, contract_state));
    for (auto i = 12970001; i < 12970025; i++) {
        EXPECT_TRUE(contract.set_effective_hash(i, h256(i), contract_state));
        xeth_header_info_t info;
        info.parent_hash = h256(i - 1);
        info.number = i;
        EXPECT_TRUE(contract.set_header_info(h256(i), info, contract_state));
    }
    EXPECT_TRUE(contract.set_last_hash(h256(12970024), contract_state));

    EXPECT_TRUE(contract.is_confirmed(12969999, hash1, contract_state));
    EXPECT_FALSE(contract.is_confirmed(12970000, hash2, contract_state));
}

static xeth_header_t create_header(h256 parent_hash, uint32_t number, uint32_t difficulty) {
    xeth_header_t header;
    header.parent_hash = parent_hash;
    header.uncle_hash = static_cast<evm_common::h256>(0);
    header.miner = common::xeth_address_t::build_from(static_cast<evm_common::Address>(0).asBytes());
    header.state_root = static_cast<evm_common::h256>(0);
    header.transactions_root = static_cast<evm_common::h256>(0);
    header.receipts_root = static_cast<evm_common::h256>(0);
    header.bloom = static_cast<evm_common::LogBloom>(0);
    header.mix_digest = static_cast<evm_common::h256>(0);
    header.nonce = xh64_t{0};
    header.difficulty = difficulty;
    header.number = number;
    header.gas_limit = 0;
    header.gas_used = 0;
    header.time = 0;
    header.base_fee_per_gas = static_cast<uint64_t>(0);
    return header;
}

TEST_F(xcontract_fixture_t, test_rebuild1) {
    auto h10 = create_header(static_cast<evm_common::h256>(0), 10, 10);
    auto h11 = create_header(h10.calc_hash(), 11, 10);
    auto h12 = create_header(h11.calc_hash(), 12, 10);
    auto h13 = create_header(h12.calc_hash(), 13, 10);
    auto h14 = create_header(h13.calc_hash(), 14, 10);
    auto h15 = create_header(h14.calc_hash(), 15, 10);
    auto h12_fork = create_header(h11.calc_hash(), 12, 5);
    auto h13_fork = create_header(h12_fork.calc_hash(), 13, 100);

    contract.set_header_info(h10.calc_hash(), xeth_header_info_t{10, static_cast<evm_common::h256>(0), 10}, contract_state);
    contract.set_header_info(h11.calc_hash(), xeth_header_info_t{20, h10.calc_hash(), 11}, contract_state);
    contract.set_header_info(h12.calc_hash(), xeth_header_info_t{30, h11.calc_hash(), 12}, contract_state);
    contract.set_header_info(h13.calc_hash(), xeth_header_info_t{40, h12.calc_hash(), 13}, contract_state);
    contract.set_header_info(h14.calc_hash(), xeth_header_info_t{50, h13.calc_hash(), 14}, contract_state);
    contract.set_header_info(h15.calc_hash(), xeth_header_info_t{60, h14.calc_hash(), 15}, contract_state);
    contract.set_header_info(h12_fork.calc_hash(), xeth_header_info_t{15, h11.calc_hash(), 12}, contract_state);
    contract.set_header_info(h13_fork.calc_hash(), xeth_header_info_t{115, h12_fork.calc_hash(), 13}, contract_state);
    EXPECT_TRUE(contract.set_effective_hash(10, h10.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(11, h11.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(12, h12.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(13, h13.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(14, h14.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(15, h15.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_last_hash(h15.calc_hash(), contract_state));
    EXPECT_TRUE(contract.rebuild(h13_fork, xeth_header_info_t{60, h14.calc_hash(), 15}, xeth_header_info_t{115, h12_fork.calc_hash(), 13}, contract_state));

    EXPECT_TRUE(contract.get_last_hash(contract_state) == h13_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(15, contract_state) == h256());
    EXPECT_TRUE(contract.get_effective_hash(14, contract_state) == h256());
    EXPECT_TRUE(contract.get_effective_hash(13, contract_state) == h13_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(12, contract_state) == h12_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(11, contract_state) == h11.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(10, contract_state) == h10.calc_hash());
}

TEST_F(xcontract_fixture_t, test_rebuild2) {
    auto h10 = create_header(static_cast<evm_common::h256>(0), 10, 10);
    auto h11 = create_header(h10.calc_hash(), 11, 10);
    auto h12 = create_header(h11.calc_hash(), 12, 10);
    auto h13 = create_header(h12.calc_hash(), 13, 10);
    auto h14 = create_header(h13.calc_hash(), 14, 10);
    auto h15 = create_header(h14.calc_hash(), 15, 10);
    auto h12_fork = create_header(h11.calc_hash(), 12, 5);
    auto h13_fork = create_header(h12_fork.calc_hash(), 13, 5);
    auto h14_fork = create_header(h13_fork.calc_hash(), 14, 5);
    auto h15_fork = create_header(h14_fork.calc_hash(), 15, 5);
    auto h16_fork = create_header(h15_fork.calc_hash(), 16, 5);
    auto h17_fork = create_header(h16_fork.calc_hash(), 17, 100);

    contract.set_header_info(h10.calc_hash(), xeth_header_info_t{10, static_cast<evm_common::h256>(0), 10}, contract_state);
    contract.set_header_info(h11.calc_hash(), xeth_header_info_t{20, h10.calc_hash(), 11}, contract_state);
    contract.set_header_info(h12.calc_hash(), xeth_header_info_t{30, h11.calc_hash(), 12}, contract_state);
    contract.set_header_info(h13.calc_hash(), xeth_header_info_t{40, h12.calc_hash(), 13}, contract_state);
    contract.set_header_info(h14.calc_hash(), xeth_header_info_t{50, h13.calc_hash(), 14}, contract_state);
    contract.set_header_info(h15.calc_hash(), xeth_header_info_t{60, h14.calc_hash(), 15}, contract_state);
    contract.set_header_info(h12_fork.calc_hash(), xeth_header_info_t{15, h11.calc_hash(), 12}, contract_state);
    contract.set_header_info(h13_fork.calc_hash(), xeth_header_info_t{20, h12_fork.calc_hash(), 13}, contract_state);
    contract.set_header_info(h14_fork.calc_hash(), xeth_header_info_t{25, h13_fork.calc_hash(), 14}, contract_state);
    contract.set_header_info(h15_fork.calc_hash(), xeth_header_info_t{30, h14_fork.calc_hash(), 15}, contract_state);
    contract.set_header_info(h16_fork.calc_hash(), xeth_header_info_t{35, h15_fork.calc_hash(), 16}, contract_state);
    contract.set_header_info(h17_fork.calc_hash(), xeth_header_info_t{135, h16_fork.calc_hash(), 17}, contract_state);
    EXPECT_TRUE(contract.set_effective_hash(10, h10.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(11, h11.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(12, h12.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(13, h13.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(14, h14.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_effective_hash(15, h15.calc_hash(), contract_state));
    EXPECT_TRUE(contract.set_last_hash(h15.calc_hash(), contract_state));
    EXPECT_TRUE(contract.rebuild(h17_fork, xeth_header_info_t{60, h14.calc_hash(), 15}, xeth_header_info_t{135, h16_fork.calc_hash(), 17}, contract_state));

    EXPECT_TRUE(contract.get_last_hash(contract_state) == h17_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(17, contract_state) == h17_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(16, contract_state) == h16_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(15, contract_state) == h15_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(14, contract_state) == h14_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(13, contract_state) == h13_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(12, contract_state) == h12_fork.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(11, contract_state) == h11.calc_hash());
    EXPECT_TRUE(contract.get_effective_hash(10, contract_state) == h10.calc_hash());
}

TEST_F(xcontract_fixture_t, execute_input_with_error_account) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    context.caller.build_from("f8a1e199c49c2ae2682ecc5b4a8838b39bab1a39");
    EXPECT_FALSE(contract.execute({}, 0, context, false, statectx_observer, output, err));
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
    init_param.insert(init_param.begin(), 0x78);
    init_param.insert(init_param.begin(), 0x30);
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
    EXPECT_TRUE(contract.init(init_param, contract_state));
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

TEST_F(xcontract_fixture_t, execute_init_static_error) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_output_init_1269999_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_FALSE(contract.execute(init_param, 0, context, true, statectx_observer, output, err));
}

TEST_F(xcontract_fixture_t,  execute_sync_ok) {
    contract_runtime::evm::sys_contract_precompile_output output;
    contract_runtime::evm::sys_contract_precompile_error err;
    std::error_code ec;
    auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.init(init_param, contract_state));

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
    EXPECT_TRUE(contract.init(init_param, contract_state));

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
    EXPECT_TRUE(contract.init(init_param, contract_state));

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
    EXPECT_TRUE(contract.init(init_param, contract_state));

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
    EXPECT_TRUE(contract.init(init_param, contract_state));
    auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
    auto height_param = top::from_hex(relayer_hex_output_get_height_tx_date, ec);
    EXPECT_EQ(ec.value(), 0);
    EXPECT_TRUE(contract.execute(height_param, 0, context, true, statectx_observer, output, err));
    u256 h = evm_common::fromBigEndian<u256>(output.output);
    EXPECT_TRUE(h == 12970000);
}

// TEST_F(xcontract_fixture_t,  execute_is_confirmed) {
//     contract_runtime::evm::sys_contract_precompile_output output;
//     contract_runtime::evm::sys_contract_precompile_error err;
//     std::error_code ec;
//     auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
//     EXPECT_EQ(ec.value(), 0);
//     EXPECT_TRUE(contract.init(init_param));
//     auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
//     EXPECT_EQ(ec.value(), 0);
//     EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));
//     auto is_confirmed_param = top::from_hex(relayer_hex_output_is_confirmed_tx_data, ec);
//     EXPECT_EQ(ec.value(), 0);
//     EXPECT_TRUE(contract.execute(is_confirmed_param, 0, context, false, statectx_observer, output, err));
//     EXPECT_EQ(evm_common::fromBigEndian<u256>(output.output), 0);
// }

// TEST_F(xcontract_fixture_t,  execute_is_confirmed_extract_error) {
//     contract_runtime::evm::sys_contract_precompile_output output;
//     contract_runtime::evm::sys_contract_precompile_error err;
//     std::error_code ec;
//     auto init_param = top::from_hex(relayer_hex_init_12969999, ec);
//     EXPECT_EQ(ec.value(), 0);
//     EXPECT_TRUE(contract.init(init_param));
//     auto sync_param = top::from_hex(relayer_hex_output_sync_1270000_tx_data, ec);
//     EXPECT_EQ(ec.value(), 0);
//     EXPECT_TRUE(contract.execute(sync_param, 0, context, false, statectx_observer, output, err));

//     std::string str{relayer_hex_output_is_confirmed_tx_data};
//     str = {str.begin(), str.begin() + 70};
//     auto is_confirmed_param = top::from_hex(str, ec);
//     EXPECT_EQ(ec.value(), 0);
//     EXPECT_FALSE(contract.execute(is_confirmed_param, 0, context, false, statectx_observer, output, err));
//     EXPECT_EQ(evm_common::fromBigEndian<u256>(output.output), 0);
// }

TEST_F(xcontract_fixture_t, test_release) {
    // num = 40000
    for (auto i = 10000; i <= 60000; i++) {
        EXPECT_TRUE(contract.set_effective_hash(i, h256(i), contract_state));
    }
    // num = 500
    xeth_header_t header;
    xeth_header_info_t info;
    for (auto i = 59000; i <= 60000; i++) {
        EXPECT_TRUE(contract.set_header(h256(i), header, contract_state));
        info.difficult_sum = i;
        info.parent_hash = h256(i - 1);
        info.number = i;
        EXPECT_TRUE(contract.set_header_info(h256(i), info, contract_state));
        EXPECT_TRUE(contract.set_hashes(i, {h256(i)}, contract_state));
    }
    contract.release(60000, contract_state);
    for (auto i = 10000; i <= 20000; i++) {
        EXPECT_TRUE(contract.get_effective_hash(i, contract_state) == h256());
    }
    for (auto i = 20001; i <= 60000; i++) {
        EXPECT_TRUE(contract.get_effective_hash(i, contract_state) == h256(i));
    }
    for (auto i = 59000; i <= 59500; i++) {
        EXPECT_FALSE(contract.get_header_info(h256(i), info, contract_state));
        EXPECT_FALSE(contract.get_header(h256(i), header, contract_state));
        EXPECT_TRUE((contract.get_hashes(i, contract_state)).empty());
    }
    for (auto i = 59501; i <= 60000; i++) {
        EXPECT_TRUE(contract.get_header(h256(i), header, contract_state));
        EXPECT_TRUE(contract.get_header_info(h256(i), info, contract_state));
        EXPECT_TRUE(info.difficult_sum == i);
        EXPECT_TRUE(info.number == i);
        EXPECT_TRUE(info.parent_hash == h256(i - 1));
        EXPECT_FALSE((contract.get_hashes(i, contract_state)).empty());
    }
}

TEST_F(xcontract_fixture_t, test_extract_base32) {
    const char * text = "efd8beed749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6";

    std::error_code ec;
    auto bytes = top::from_hex(text, ec);
    evm_common::xabi_decoder_t abi_decoder = evm_common::xabi_decoder_t::build_from(xbytes_t{std::begin(bytes), std::end(bytes)}, ec);
    abi_decoder.extract<evm_common::xfunction_selector_t>(ec);
    // function_selector;
    auto hash_bytes = abi_decoder.decode_bytes(32, ec);
    h256 hash = static_cast<h256>(hash_bytes);
    EXPECT_EQ("749c4c2c82582cba3489bce142d6c776d50a5c18b3aeaf3cdc68b27650d0a6b6", hash.hex());
}

}
}