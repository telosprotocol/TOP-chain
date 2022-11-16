#include "nlohmann/fifo_map.hpp"
#include "nlohmann/json.hpp"
#include "test_evm_eth2_client_contract_fixture.h"
#include "test_evm_eth2_client_contract_kiln_header_data.inc"
#include "test_evm_eth2_client_contract_sepolia_header_data.inc"
#include "test_evm_eth2_client_contract_update_101_data_.inc"
#include "test_evm_eth2_client_contract_update_data.inc"
#include "xbasic/xhex.h"
#include "xdata/xdatautil.h"
#include "xevm_common/rlp.h"
#include "xevm_common/xabi_decoder.h"

#include <fstream>

namespace top {
namespace tests {

template <class K, class V, class dummy_compare, class A>
using my_workaround_fifo_map = nlohmann::fifo_map<K, V, nlohmann::fifo_map_compare<K>, A>;
using json = nlohmann::basic_json<my_workaround_fifo_map>;

TEST_F(xeth2_contract_fixture_t, property_finalized_execution_blocks) {
    // get empty
    for (auto i = 0; i < 10; i++) {
        EXPECT_EQ(m_contract.get_finalized_execution_blocks(m_contract_state, i), h256());
    }
    // set
    for (auto i = 0; i < 5; i++) {
        EXPECT_TRUE(m_contract.set_finalized_execution_blocks(m_contract_state, i, h256(i + 1)));
    }
    // get value
    for (auto i = 0; i < 10; i++) {
        if (i < 5) {
            EXPECT_EQ(m_contract.get_finalized_execution_blocks(m_contract_state, i), h256(i + 1));
        } else {
            EXPECT_EQ(m_contract.get_finalized_execution_blocks(m_contract_state, i), h256());
        }
    }
    // del
    for (auto i = 0; i < 3; i++) {
        EXPECT_TRUE(m_contract.del_finalized_execution_blocks(m_contract_state, i));
    }
    // get value
    for (auto i = 0; i < 10; i++) {
        if (i >= 3 && i < 5) {
            EXPECT_EQ(m_contract.get_finalized_execution_blocks(m_contract_state, i), h256(i + 1));
        } else {
            EXPECT_EQ(m_contract.get_finalized_execution_blocks(m_contract_state, i), h256());
        }
    }
}

TEST_F(xeth2_contract_fixture_t, property_unfinalized_headers) {
    // get empty
    for (auto i = 0; i < 10; i++) {
        EXPECT_TRUE(m_contract.get_unfinalized_headers(m_contract_state, h256(i + 1)).empty());
    }
    // set
    for (auto i = 0; i < 5; i++) {
        EXPECT_TRUE(m_contract.set_unfinalized_headers(m_contract_state, h256(i + 1), xexecution_header_info_t(h256(i), i)));
    }
    // get value
    for (auto i = 0; i < 10; i++) {
        if (i < 5) {
            EXPECT_EQ(m_contract.get_unfinalized_headers(m_contract_state, h256(i + 1)), xexecution_header_info_t(h256(i), i));
        } else {
            EXPECT_EQ(m_contract.get_unfinalized_headers(m_contract_state, h256(i + 1)), xexecution_header_info_t());
        }
    }
    // del
    for (auto i = 0; i < 3; i++) {
        EXPECT_TRUE(m_contract.del_unfinalized_headers(m_contract_state, h256(i + 1)));
    }
    // get value
    for (auto i = 0; i < 10; i++) {
        if (i >= 3 && i < 5) {
            EXPECT_EQ(m_contract.get_unfinalized_headers(m_contract_state, h256(i + 1)), xexecution_header_info_t(h256(i), i));
        } else {
            EXPECT_EQ(m_contract.get_unfinalized_headers(m_contract_state, h256(i + 1)), xexecution_header_info_t());
        }
    }
}

TEST_F(xeth2_contract_fixture_t, property_finalized_beacon_header) {
    // get empty
    EXPECT_TRUE(m_contract.get_finalized_beacon_header(m_contract_state).empty());
    // set
    xextended_beacon_block_header_t header_ext;
    header_ext.header.slot = 1;
    header_ext.header.proposer_index = 2;
    header_ext.header.body_root = h256(3);
    header_ext.header.parent_root = h256(4);
    header_ext.header.state_root = h256(5);
    header_ext.beacon_block_root = h256(6);
    header_ext.execution_block_hash = h256(7);
    EXPECT_TRUE(m_contract.set_finalized_beacon_header(m_contract_state, header_ext));
    // get value
    EXPECT_EQ(m_contract.get_finalized_beacon_header(m_contract_state), header_ext);
}

TEST_F(xeth2_contract_fixture_t, property_finalized_execution_header) {
    // get empty
    EXPECT_TRUE(m_contract.get_finalized_execution_header(m_contract_state).empty());
    // set
    xexecution_header_info_t info(h256(rand()), rand());
    EXPECT_TRUE(m_contract.set_finalized_execution_header(m_contract_state, info));
    // get value
    EXPECT_EQ(m_contract.get_finalized_execution_header(m_contract_state), info);
}

TEST_F(xeth2_contract_fixture_t, property_current_sync_committee) {
    // get empty
    EXPECT_TRUE(m_contract.get_current_sync_committee(m_contract_state).empty());
    // set
    xsync_committee_t committee;
    committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        auto b = h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32);
        committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    EXPECT_TRUE(m_contract.set_current_sync_committee(m_contract_state, committee));
    // get value
    EXPECT_EQ(m_contract.get_current_sync_committee(m_contract_state), committee);
}

TEST_F(xeth2_contract_fixture_t, property_next_sync_committee) {
    // get empty
    EXPECT_TRUE(m_contract.get_next_sync_committee(m_contract_state).empty());
    // set
    xsync_committee_t committee;
    committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    EXPECT_TRUE(m_contract.set_next_sync_committee(m_contract_state, committee));
    // get value
    EXPECT_EQ(m_contract.get_next_sync_committee(m_contract_state), committee);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_committee_update) {
    xsync_committee_update_t update;
    update.next_sync_committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        update.next_sync_committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    for (auto i = 0; i < 16; i++) {
        update.next_sync_committee_branch.emplace_back(h256(rand()).to_bytes());
    }
    auto b = update.encode_rlp();
    xsync_committee_update_t update_decode;
    EXPECT_TRUE(update_decode.decode_rlp(b));
    EXPECT_EQ(update, update_decode);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_header_update) {
    xheader_update_t update;
    update.beacon_header.slot = 1;
    update.beacon_header.proposer_index = 2;
    update.beacon_header.body_root = h256(3);
    update.beacon_header.parent_root = h256(4);
    update.beacon_header.state_root = h256(5);
    update.execution_block_hash = h256(6);
    auto b = update.encode_rlp();
    xheader_update_t update_decode;
    EXPECT_TRUE(update_decode.decode_rlp(b));
    EXPECT_EQ(update, update_decode);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_finalized_header_update) {
    xfinalized_header_update_t update;
    update.header_update.beacon_header.slot = 1;
    update.header_update.beacon_header.proposer_index = 2;
    update.header_update.beacon_header.body_root = h256(3);
    update.header_update.beacon_header.parent_root = h256(4);
    update.header_update.beacon_header.state_root = h256(5);
    update.header_update.execution_block_hash = h256(6);
    for (auto i = 0; i < 16; i++) {
        update.finality_branch.emplace_back(h256(rand()).to_bytes());
    }
    auto b = update.encode_rlp();
    xfinalized_header_update_t update_decode;
    EXPECT_TRUE(update_decode.decode_rlp(b));
    EXPECT_EQ(update, update_decode);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_sync_aggregate) {
    xsync_aggregate_t sync;
    sync.sync_committee_bits = xbytes_t(32, 'a');
    sync.sync_committee_signature = xbytes_t(96, 'b');
    auto b = sync.encode_rlp();
    xsync_aggregate_t sync_decode;
    EXPECT_TRUE(sync_decode.decode_rlp(b));
    EXPECT_EQ(sync, sync_decode);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_light_client_update) {
    xlight_client_update_t update;
    update.attested_beacon_header.slot = 1;
    update.attested_beacon_header.proposer_index = 2;
    update.attested_beacon_header.body_root = h256(3);
    update.attested_beacon_header.parent_root = h256(4);
    update.attested_beacon_header.state_root = h256(5);
    update.sync_aggregate.sync_committee_bits = xbytes_t(32, 'a');
    update.sync_aggregate.sync_committee_signature = xbytes_t(96, 'b');
    update.signature_slot = 10;
    update.finality_update.header_update.beacon_header.slot = 1;
    update.finality_update.header_update.beacon_header.proposer_index = 2;
    update.finality_update.header_update.beacon_header.body_root = h256(3);
    update.finality_update.header_update.beacon_header.parent_root = h256(4);
    update.finality_update.header_update.beacon_header.state_root = h256(5);
    update.finality_update.header_update.execution_block_hash = h256(6);
    for (auto i = 0; i < 16; i++) {
        update.finality_update.finality_branch.emplace_back(h256(rand()).to_bytes());
    }
    update.sync_committee_update.next_sync_committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        update.sync_committee_update.next_sync_committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    for (auto i = 0; i < 16; i++) {
        update.sync_committee_update.next_sync_committee_branch.emplace_back(h256(rand()).to_bytes());
    }
    auto b = update.encode_rlp();
    xlight_client_update_t update_decode;
    EXPECT_TRUE(update_decode.decode_rlp(b));
    EXPECT_EQ(update, update_decode);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_light_client_state) {
    xlight_client_state_t state;
    state.finalized_beacon_header.header.slot = 1;
    state.finalized_beacon_header.header.proposer_index = 2;
    state.finalized_beacon_header.header.body_root = h256(3);
    state.finalized_beacon_header.header.parent_root = h256(4);
    state.finalized_beacon_header.header.state_root = h256(5);
    state.finalized_beacon_header.beacon_block_root = h256(6);
    state.finalized_beacon_header.execution_block_hash = h256(7);
    state.current_sync_committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        state.current_sync_committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    state.next_sync_committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        state.next_sync_committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    auto b = state.encode_rlp();
    xlight_client_state_t state_decode;
    EXPECT_TRUE(state_decode.decode_rlp(b));
    EXPECT_EQ(state, state_decode);
}

TEST_F(xeth2_contract_fixture_t, encode_decode_init_input) {
    xinit_input_t init;
    init.finalized_execution_header.parent_hash = static_cast<evm_common::h256>(UINT32_MAX - 1);
    init.finalized_execution_header.uncle_hash = static_cast<evm_common::h256>(UINT32_MAX - 2);
    init.finalized_execution_header.miner = static_cast<evm_common::Address>(UINT32_MAX - 3);
    init.finalized_execution_header.state_merkleroot = static_cast<evm_common::h256>(UINT32_MAX - 4);
    init.finalized_execution_header.tx_merkleroot = static_cast<evm_common::h256>(UINT32_MAX - 5);
    init.finalized_execution_header.receipt_merkleroot = static_cast<evm_common::h256>(UINT32_MAX - 6);
    init.finalized_execution_header.bloom = static_cast<evm_common::LogBloom>(UINT32_MAX - 7);
    init.finalized_execution_header.mix_digest = static_cast<evm_common::h256>(UINT32_MAX - 8);
    init.finalized_execution_header.nonce = static_cast<evm_common::h64>(UINT32_MAX - 9);
    init.finalized_execution_header.difficulty = static_cast<evm_common::bigint>(UINT64_MAX - 1);
    init.finalized_execution_header.number = UINT64_MAX - 2;
    init.finalized_execution_header.gas_limit = UINT64_MAX - 3;
    init.finalized_execution_header.gas_used = UINT64_MAX - 4;
    init.finalized_execution_header.time = UINT64_MAX - 5;
    init.finalized_execution_header.base_fee = static_cast<evm_common::bigint>(UINT64_MAX - 6);
    init.finalized_execution_header.extra = {1, 3, 5, 7};
    init.finalized_beacon_header.header.slot = 1;
    init.finalized_beacon_header.header.proposer_index = 2;
    init.finalized_beacon_header.header.body_root = h256(3);
    init.finalized_beacon_header.header.parent_root = h256(4);
    init.finalized_beacon_header.header.state_root = h256(5);
    init.finalized_beacon_header.beacon_block_root = h256(6);
    init.finalized_beacon_header.execution_block_hash = h256(7);
    init.current_sync_committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        init.current_sync_committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    init.next_sync_committee.aggregate_pubkey = xbytes_t(PUBLIC_KEY_BYTES_LEN - 32) + h256(rand()).to_bytes();
    for (auto i = 0; i < 32; i++) {
        init.next_sync_committee.pubkeys.emplace_back(h256(rand()).to_bytes() + xbytes_t(PUBLIC_KEY_BYTES_LEN - 32));
    }
    auto b = init.encode_rlp();
    xinit_input_t init_decode;
    EXPECT_TRUE(init_decode.decode_rlp(b));
    EXPECT_EQ(init, init_decode);
}

TEST_F(xeth2_contract_fixture_t, test_release_finalized_execution_blocks) {
    for (auto i = 50; i < 100; i++) {
        EXPECT_TRUE(m_contract.set_finalized_execution_blocks(m_contract_state, i, h256(i)));
    }
    EXPECT_FALSE(m_contract.del_finalized_execution_blocks(m_contract_state, 100));
    m_contract.release_finalized_execution_blocks(m_contract_state, 80 - 1);
    for (auto i = 50; i < 80; ++i) {
        auto b = m_contract.get_finalized_execution_blocks(m_contract_state, i);
        EXPECT_EQ(b, h256());
    }
    for (auto i = 80; i < 100; ++i) {
        auto b = m_contract.get_finalized_execution_blocks(m_contract_state, i);
        EXPECT_EQ(b, h256(i));
    }
}

std::vector<xeth_header_t> parse_header_data() {
    std::vector<xeth_header_t> res;
    auto j = json::parse(kiln_header_json_data_ptr);
    for (auto it = j.begin(); it != j.end(); ++it) {
        xeth_header_t h;
        h.parent_hash = static_cast<h256>(from_hex(it->at("parent_hash").get<std::string>()));
        h.uncle_hash = static_cast<h256>(from_hex(it->at("uncles_hash").get<std::string>()));
        h.miner = static_cast<h160>(from_hex(it->at("author").get<std::string>()));
        h.state_merkleroot = static_cast<h256>(from_hex(it->at("state_root").get<std::string>()));
        h.tx_merkleroot = static_cast<h256>(from_hex(it->at("transactions_root").get<std::string>()));
        h.receipt_merkleroot = static_cast<h256>(from_hex(it->at("receipts_root").get<std::string>()));
        h.bloom = static_cast<h2048>(from_hex(it->at("log_bloom").get<std::string>()));
        auto difficulty = it->at("difficulty").get<std::string>();
        h.difficulty = data::hex_to_uint64(difficulty);
        auto number = it->at("number").get<std::string>();
        h.number = data::hex_to_uint64(number);
        auto gas_limit = it->at("gas_limit").get<std::string>();
        h.gas_limit = data::hex_to_uint64(gas_limit);
        auto gas_used = it->at("gas_used").get<std::string>();
        h.gas_used = data::hex_to_uint64(gas_used);
        auto time = it->at("timestamp").get<std::string>();
        h.time = data::hex_to_uint64(time);
        h.mix_digest = static_cast<h256>(from_hex(it->at("mix_hash").get<std::string>()));
        h.nonce = static_cast<h64>(from_hex(it->at("nonce").get<std::string>()));
        auto base_fee = it->at("base_fee").get<std::string>();
        h.base_fee = bigint(data::hex_to_uint64(base_fee));
        res.emplace_back(h);
    }
    return res;
}

xlight_client_update_t parse_update_data(const char * data_ptr) {
    xlight_client_update_t res;
    auto j = json::parse(data_ptr);
    res.attested_beacon_header.slot = std::stoll(j["attested_beacon_header"]["slot"].get<std::string>());
    res.attested_beacon_header.proposer_index = std::stoll(j["attested_beacon_header"]["proposer_index"].get<std::string>());
    res.attested_beacon_header.parent_root = static_cast<h256>(from_hex(j["attested_beacon_header"]["parent_root"].get<std::string>()));
    res.attested_beacon_header.state_root = static_cast<h256>(from_hex(j["attested_beacon_header"]["state_root"].get<std::string>()));
    res.attested_beacon_header.body_root = static_cast<h256>(from_hex(j["attested_beacon_header"]["body_root"].get<std::string>()));
    res.sync_aggregate.sync_committee_bits = to_bytes(j["sync_aggregate"]["sync_committee_bits"].get<std::string>());
    res.sync_aggregate.sync_committee_signature = from_hex(j["sync_aggregate"]["sync_committee_signature"].get<std::string>());
    res.signature_slot = std::stoll(j["signature_slot"].get<std::string>());
    res.finality_update.header_update.beacon_header.slot = std::stoll(j["finality_update"]["header_update"]["beacon_header"]["slot"].get<std::string>());
    res.finality_update.header_update.beacon_header.proposer_index = std::stoll(j["finality_update"]["header_update"]["beacon_header"]["proposer_index"].get<std::string>());
    res.finality_update.header_update.beacon_header.parent_root =
        static_cast<h256>(from_hex(j["finality_update"]["header_update"]["beacon_header"]["parent_root"].get<std::string>()));
    res.finality_update.header_update.beacon_header.state_root =
        static_cast<h256>(from_hex(j["finality_update"]["header_update"]["beacon_header"]["state_root"].get<std::string>()));
    res.finality_update.header_update.beacon_header.body_root = static_cast<h256>(from_hex(j["finality_update"]["header_update"]["beacon_header"]["body_root"].get<std::string>()));
    res.finality_update.header_update.execution_block_hash = static_cast<h256>(from_hex(j["finality_update"]["header_update"]["execution_block_hash"].get<std::string>()));
    auto const & finality_branch_array = j["finality_update"]["finality_branch"];
    for (auto const b : finality_branch_array) {
        res.finality_update.finality_branch.emplace_back(from_hex(b.get<std::string>()));
    }
    auto const & pubkeys_array = j["sync_committee_update"]["next_sync_committee"]["pubkeys"];
    for (auto const & p : pubkeys_array) {
        res.sync_committee_update.next_sync_committee.pubkeys.emplace_back(from_hex(p.get<std::string>()));
    }
    res.sync_committee_update.next_sync_committee.aggregate_pubkey = from_hex(j["sync_committee_update"]["next_sync_committee"]["aggregate_pubkey"].get<std::string>());
    auto const & next_sync_committee_branch_array = j["sync_committee_update"]["next_sync_committee_branch"];
    for (auto const b : next_sync_committee_branch_array) {
        res.sync_committee_update.next_sync_committee_branch.emplace_back(from_hex(b.get<std::string>()));
    }
    return res;
}

TEST_F(xeth2_contract_fixture_t, test_submit_update_two_periods) {
    auto headers = parse_header_data();
    auto update_101 = parse_update_data(update_101_data_ptr);

    xinit_input_t init;
    init.finalized_execution_header = headers[0];
    init.finalized_beacon_header.header.slot = 823648;
    init.finalized_beacon_header.header.proposer_index = 9369;
    init.finalized_beacon_header.header.parent_root = static_cast<h256>(from_hex("87d8a2dd1072fcd8cc631e42f9ddeb7e821660ec27b05d2857c9e5ccbad4851b"));
    init.finalized_beacon_header.header.state_root = static_cast<h256>(from_hex("8f5e215cef7eb1026bfa4f4b66e82eab2d77b1a0425513136ec042124f71974f"));
    init.finalized_beacon_header.header.body_root = static_cast<h256>(from_hex("44c9d4b7b97a9e147cff85f90e68f8c30dae846fd6b969e6b8298e4d8311769e"));
    init.finalized_beacon_header.beacon_block_root = static_cast<h256>(("dfb0d6f3164271032200b94138f0b1cceaee36bad55748a1efc830f3c62f5c9c"));
    init.finalized_beacon_header.execution_block_hash = static_cast<h256>(("603c233dbb57105d0a73b47b6a6936cbf59f9d6005fbf8d4a7def7f35b5f6a4f"));
    init.current_sync_committee.aggregate_pubkey = from_hex(aggregate_pubkey_99);
    for (auto const p : sync_committee_99) {
        init.current_sync_committee.pubkeys.emplace_back(from_hex(p));
    }
    init.next_sync_committee.aggregate_pubkey = from_hex(aggregate_pubkey_100);
    for (auto const p : sync_committee_100) {
        init.next_sync_committee.pubkeys.emplace_back(from_hex(p));
    }
    EXPECT_TRUE(m_contract.init(m_contract_state, init));
    EXPECT_EQ(m_contract.last_block_number(m_contract_state), 0xbb247);

    headers.erase(headers.begin());
    for (auto const & header : headers) {
        m_contract.submit_execution_header(m_contract_state, header);
        EXPECT_TRUE(m_contract.is_known_execution_header(m_contract_state, header.hash()));
        EXPECT_EQ(m_contract.block_hash_safe(m_contract_state, static_cast<uint64_t>(header.number)), h256());
    }

    EXPECT_TRUE(m_contract.submit_beacon_chain_light_client_update(m_contract_state, update_101));
    EXPECT_EQ(m_contract.last_block_number(m_contract_state), headers.back().number);
    EXPECT_FALSE(m_contract.is_known_execution_header(m_contract_state, m_contract.get_finalized_beacon_header(m_contract_state).execution_block_hash));
}

TEST_F(xeth2_contract_fixture_t, test_init_and_update) {
    m_contract.m_network = xeth2_client_net_t::eth2_net_sepolia;

    auto init_param_rlp = from_hex(init_param_rlp_hex);
    xinit_input_t init_param;
    EXPECT_TRUE(init_param.decode_rlp(init_param_rlp));
    EXPECT_TRUE(m_contract.init(m_contract_state, init_param));

    {
        auto cur_committee = m_contract.get_current_sync_committee(m_contract_state);
        EXPECT_EQ(cur_committee.pubkeys.size(), 512);
        EXPECT_EQ(cur_committee.aggregate_pubkey, from_hex("0xab2f06f681f10383788e9c7ab89275d602cbedbcfbaedcd4ed92c3bff7d15561ec7df684d43c531370157357806a8453"));
        EXPECT_EQ(cur_committee.pubkeys[0], from_hex("0xab7eff4ef8696db334bce564bc273af0412bb4de547056326dff2037e1eca7abde039a51953948dd61d3d15925cd92f6"));
        EXPECT_EQ(cur_committee.pubkeys[1], from_hex("0x87c5670e16a84e27529677881dbedc5c1d6ebb4e4ff58c13ece43d21d5b42dc89470f41059bfa6ebcf18167f97ddacaa"));
        EXPECT_EQ(cur_committee.pubkeys[511], from_hex("0x8c64035c18e2d684b5800039a4e273b2d08a1ba037c72609fd9e73595d980637ef2b812204710e32dc91147bf034c19c"));

        auto next_committee = m_contract.get_next_sync_committee(m_contract_state);
        EXPECT_EQ(next_committee.pubkeys.size(), 512);
        EXPECT_EQ(next_committee.aggregate_pubkey, from_hex("0xb99e31d04473fa778efc8a22ed4fa3b1048043244d33cbdcb509d11f5e3a74bdb501d7db06919cc2844209ab32cdd629"));
        EXPECT_EQ(next_committee.pubkeys[0], from_hex("0xb01ee30d120b97e7b60ea89b9b6c537cdf20b6e36337e70d289ed5949355dd32679dc0a747525d6f2076f5be051d3a89"));
        EXPECT_EQ(next_committee.pubkeys[1], from_hex("0xb549cef11bf7c8bcf4bb11e5cdf5a289fc4bf145826e96a446fb4c729a2c839a4d8d38629cc599eda7efa05f3cf3425b"));
        EXPECT_EQ(next_committee.pubkeys[511], from_hex("0x949cf015ce50e27cf5c2ff1b8e2e066679905ac91164e3423d3fb7e05c64429e77e432db0f549acb99f91fb134b6edad"));
    }
    auto fin_header = m_contract.get_finalized_beacon_header(m_contract_state);
    EXPECT_EQ(fin_header.header.slot, 1024000);
    EXPECT_EQ(fin_header.header.proposer_index, 257);
    EXPECT_EQ(fin_header.header.parent_root, h256(from_hex("0x96bd1ead9d2932de8b1c626d3a24884a867d01357842c2a73c2bf1e4791cc9e3")));
    EXPECT_EQ(fin_header.header.state_root, h256(from_hex("0xb7fc02c07bb70b0cd9ff4f58067a058e3990c69f2066a167c1a7b9b6f7873335")));
    EXPECT_EQ(fin_header.header.body_root, h256(from_hex("0x026612cc702610f126f8ac4fd7b0604628ff2acf8d34da41a89fdda31bfa9710")));
    auto beacon_header_rlp = fin_header.header.encode_rlp();
    xbytes_t beacon_header_hash(32);
    EXPECT_TRUE(unsafe_beacon_header_root(beacon_header_rlp.data(), beacon_header_rlp.size(), beacon_header_hash.data()));
    EXPECT_EQ(fin_header.beacon_block_root, h256(beacon_header_hash));
    auto fin_exe_header = m_contract.get_finalized_execution_header(m_contract_state);
    EXPECT_EQ(fin_exe_header.block_number, 2256927);
    EXPECT_EQ(fin_exe_header.parent_hash, h256(from_hex("2dd3836685ab8c30353c295e078fdfe37d76386d3a2af2aa44025a46f247711f")));

    auto update_param_rlp = from_hex(update_param_rlp_hex);
    xlight_client_update_t update_param;
    EXPECT_TRUE(update_param.decode_rlp(update_param_rlp));
    EXPECT_EQ(update_param.attested_beacon_header.slot, 1027776);
    EXPECT_EQ(update_param.attested_beacon_header.proposer_index, 1902);
    EXPECT_EQ(update_param.attested_beacon_header.parent_root, h256(from_hex("0xbdd9a3c97ba3a5d5d3e50aa48bbf91c558f0af732326af9be1e32e2ddefb9cba")));
    EXPECT_EQ(update_param.attested_beacon_header.state_root, h256(from_hex("0x3922928c57f96df1093729b3f69f3740ffc7b76195fe1949455e63db91c5e339")));
    EXPECT_EQ(update_param.attested_beacon_header.body_root, h256(from_hex("0xbf457d4bdc6569d5099d781a074f7c33c8ba450d98606bcc3cdea5c61c24fda7")));
    EXPECT_EQ(update_param.finality_update.header_update.beacon_header.slot, 1027712);
    EXPECT_EQ(update_param.finality_update.header_update.beacon_header.proposer_index, 1870);
    EXPECT_EQ(update_param.finality_update.header_update.beacon_header.parent_root, h256(from_hex("0xec5bbe8719d81005f0e5ac5631dd50e7e3047c6356040d27062514ba091b9678")));
    EXPECT_EQ(update_param.finality_update.header_update.beacon_header.state_root, h256(from_hex("0x4140d8a7d5e23a386565228e38d4470730e8a7c5fc9f54dbf63d32005f5c10b1")));
    EXPECT_EQ(update_param.finality_update.header_update.beacon_header.body_root, h256(from_hex("0x5b4abf0a33bc7e423c7c0e623843cab4d6bc70a9b238ad7f1cac8afb7d17f82e")));
    EXPECT_EQ(update_param.finality_update.finality_branch.size(), 6);
    EXPECT_EQ(update_param.finality_update.finality_branch[0], from_hex("0x747d000000000000000000000000000000000000000000000000000000000000"));
    EXPECT_EQ(update_param.finality_update.finality_branch[1], from_hex("0x3d4be5d019ba15ea3ef304a83b8a067f2e79f46a3fac8069306a6c814a0a35eb"));
    EXPECT_EQ(update_param.finality_update.finality_branch[5], from_hex("0x22f383afe7f12a9ddf390d58f08b63ecd1ca8acb176a505d9c2a7ab19b4d628b"));
    EXPECT_TRUE(update_param.sync_committee_update.next_sync_committee_branch.empty());
    EXPECT_TRUE(update_param.sync_committee_update.next_sync_committee.aggregate_pubkey.empty());
    EXPECT_TRUE(update_param.sync_committee_update.next_sync_committee.pubkeys.empty());
    EXPECT_EQ(update_param.signature_slot, 1027777);
    std::string str{"0xfbfffdf7fdffffffffffbffff7fffeefffdfffa7efffeffdbffff3ffffffebd7fffdefffb7fffdbefafffeffefebdffffff7fffffdefffffffdfffffeffffdf7"};
    xbytes_t bytes(str.begin(), str.end());
    EXPECT_EQ(update_param.sync_aggregate.sync_committee_bits, bytes);
    EXPECT_EQ(update_param.sync_aggregate.sync_committee_signature,
              from_hex("0x8aead7714b224c274fe5ab8ca44fcfcb3c607a77ad37f2c8fbedfb88dac99916fb83913675c324140e6aef81e35351460ddf778246baf8d9f1176e9016fb4c67748eda559957d745d03f22667"
                       "d5362708c911c99939c1f3abbd1e251927f5490"));
    EXPECT_EQ(update_param.finality_update.header_update.execution_block_hash, h256(from_hex("351da499932c0b29f02dd639bf8576a028055758e07af9b4539ad2e0690680d2")));

    auto j = json::parse(sepolia_header_json_data_ptr);
    int cnt{0};
    auto it = j.begin() + 1;
    for (; it != j.end(); ++it) {
        xeth_header_t header;
        EXPECT_TRUE(header.decode_rlp(from_hex(it->get<std::string>())));
        EXPECT_TRUE(m_contract.submit_execution_header(m_contract_state, header));
        EXPECT_TRUE(m_contract.is_known_execution_header(m_contract_state, header.hash()));
        EXPECT_EQ(m_contract.block_hash_safe(m_contract_state, static_cast<uint64_t>(header.number)), h256());
        if (header.number == 2260221) {
            EXPECT_FALSE(m_contract.submit_beacon_chain_light_client_update(m_contract_state, update_param));
        }
        if (header.number == 2260230) {
            break;
        }
    }

    {
        auto cur_committee = m_contract.get_current_sync_committee(m_contract_state);
        EXPECT_EQ(cur_committee.pubkeys.size(), 512);
        EXPECT_EQ(cur_committee.aggregate_pubkey, from_hex("0xab2f06f681f10383788e9c7ab89275d602cbedbcfbaedcd4ed92c3bff7d15561ec7df684d43c531370157357806a8453"));
        EXPECT_EQ(cur_committee.pubkeys[0], from_hex("0xab7eff4ef8696db334bce564bc273af0412bb4de547056326dff2037e1eca7abde039a51953948dd61d3d15925cd92f6"));
        EXPECT_EQ(cur_committee.pubkeys[1], from_hex("0x87c5670e16a84e27529677881dbedc5c1d6ebb4e4ff58c13ece43d21d5b42dc89470f41059bfa6ebcf18167f97ddacaa"));
        EXPECT_EQ(cur_committee.pubkeys[511], from_hex("0x8c64035c18e2d684b5800039a4e273b2d08a1ba037c72609fd9e73595d980637ef2b812204710e32dc91147bf034c19c"));
        auto next_committee = m_contract.get_next_sync_committee(m_contract_state);
        EXPECT_EQ(next_committee.pubkeys.size(), 512);
        EXPECT_EQ(next_committee.aggregate_pubkey, from_hex("0xb99e31d04473fa778efc8a22ed4fa3b1048043244d33cbdcb509d11f5e3a74bdb501d7db06919cc2844209ab32cdd629"));
        EXPECT_EQ(next_committee.pubkeys[0], from_hex("0xb01ee30d120b97e7b60ea89b9b6c537cdf20b6e36337e70d289ed5949355dd32679dc0a747525d6f2076f5be051d3a89"));
        EXPECT_EQ(next_committee.pubkeys[1], from_hex("0xb549cef11bf7c8bcf4bb11e5cdf5a289fc4bf145826e96a446fb4c729a2c839a4d8d38629cc599eda7efa05f3cf3425b"));
        EXPECT_EQ(next_committee.pubkeys[511], from_hex("0x949cf015ce50e27cf5c2ff1b8e2e066679905ac91164e3423d3fb7e05c64429e77e432db0f549acb99f91fb134b6edad"));
    }

    EXPECT_TRUE(m_contract.submit_beacon_chain_light_client_update(m_contract_state, update_param));
    EXPECT_EQ(m_contract.last_block_number(m_contract_state), 2260222);
    EXPECT_FALSE(m_contract.is_known_execution_header(m_contract_state, m_contract.get_finalized_beacon_header(m_contract_state).execution_block_hash));
    // 2260223
    EXPECT_TRUE(m_contract.is_known_execution_header(m_contract_state, h256(from_hex("7eb26ba7a27b0ef431287046c0d30d0958c94f7519bad3361a77b9723b34ac4a"))));
    // 2260230
    EXPECT_TRUE(m_contract.is_known_execution_header(m_contract_state, h256(from_hex("27d771cacab35d0fc448dabaf7fbd1d1722dbd0a1165e7076cacdc3d8f251758"))));

    int32_t size{0};
    m_contract_state->map_size(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, size);
    EXPECT_EQ(size, 8);

    auto update_param_full_rlp = from_hex(update_param_full_rlp_hex);
    xlight_client_update_t update_param_full;
    EXPECT_TRUE(update_param_full.decode_rlp(update_param_full_rlp));

    for (; it != j.end(); ++it) {
        xeth_header_t header;
        EXPECT_TRUE(header.decode_rlp(from_hex(it->get<std::string>())));
        EXPECT_TRUE(m_contract.submit_execution_header(m_contract_state, header));
        EXPECT_TRUE(m_contract.is_known_execution_header(m_contract_state, header.hash()));
        EXPECT_EQ(m_contract.block_hash_safe(m_contract_state, static_cast<uint64_t>(header.number)), h256());
    }
    EXPECT_TRUE(m_contract.submit_beacon_chain_light_client_update(m_contract_state, update_param_full));
    EXPECT_EQ(m_contract.last_block_number(m_contract_state), 2264207);
    EXPECT_FALSE(m_contract.is_known_execution_header(m_contract_state, m_contract.get_finalized_beacon_header(m_contract_state).execution_block_hash));
    {
        auto cur_committee = m_contract.get_current_sync_committee(m_contract_state);
        EXPECT_EQ(cur_committee.pubkeys.size(), 512);
        EXPECT_EQ(cur_committee.aggregate_pubkey, from_hex("0xb99e31d04473fa778efc8a22ed4fa3b1048043244d33cbdcb509d11f5e3a74bdb501d7db06919cc2844209ab32cdd629"));
        EXPECT_EQ(cur_committee.pubkeys[0], from_hex("0xb01ee30d120b97e7b60ea89b9b6c537cdf20b6e36337e70d289ed5949355dd32679dc0a747525d6f2076f5be051d3a89"));
        EXPECT_EQ(cur_committee.pubkeys[1], from_hex("0xb549cef11bf7c8bcf4bb11e5cdf5a289fc4bf145826e96a446fb4c729a2c839a4d8d38629cc599eda7efa05f3cf3425b"));
        EXPECT_EQ(cur_committee.pubkeys[511], from_hex("0x949cf015ce50e27cf5c2ff1b8e2e066679905ac91164e3423d3fb7e05c64429e77e432db0f549acb99f91fb134b6edad"));
        auto next_committee = m_contract.get_next_sync_committee(m_contract_state);
        EXPECT_EQ(next_committee.pubkeys.size(), 512);
        EXPECT_EQ(next_committee.aggregate_pubkey, from_hex("0xb3f15a9a420fb103f15c8aea436c761f6e9a40f6f046712ad8bc8b05928419d51c12d4384568f4bf92b237b4891da267"));
        EXPECT_EQ(next_committee.pubkeys[0], from_hex("0xa23710308d8e25a0bb1db53c8598e526235c5e91e4605e402f6a25c126687d9de146b75c39a31c69ab76bab514320e05"));
        EXPECT_EQ(next_committee.pubkeys[1], from_hex("0xb01ee30d120b97e7b60ea89b9b6c537cdf20b6e36337e70d289ed5949355dd32679dc0a747525d6f2076f5be051d3a89"));
        EXPECT_EQ(next_committee.pubkeys[511], from_hex("0x86b3ec14a8ffb811a0ecc3771f600d8b08c098537d100fba66def19e7ee4d1c397a311977bf37e6cd2d47a8a2ee8c223"));
    }
}

}  // namespace tests
}  // namespace top