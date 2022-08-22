#include <sstream>
#define private public
#include "xbase/xutl.h"
#include "xbasic/xhex.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_node_info.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xrelayblock_build.h"
#include "xelection/xcache/xdata_accessor.h"
#include "xvm/manager/xcontract_manager.h"
#include "xvm/xcontract_helper.h"
#include "xvm/xsystem_contracts/xrelay/xrelay_make_block_contract.h"

#include <gtest/gtest.h>

using namespace top;
using namespace top::contract;
using namespace top::xvm;
using namespace top::xvm::system_contracts::relay;
using top::common::xbroadcast_id_t;
using top::common::xnode_id_t;
using top::common::xnode_type_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_result_store_t;
using top::data::election::xstandby_node_info_t;

class xtop_test_relay_make_block_contract
  : public xtop_relay_make_block_contract
  , public testing::Test {
    using xbase_t = xtop_relay_make_block_contract;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_test_relay_make_block_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_test_relay_make_block_contract);

    xtop_test_relay_make_block_contract() : xtop_relay_make_block_contract(common::xnetwork_id_t{0}){};

    xcontract_base * clone() override {
        return {};
    }

    void exec(top::xvm::xvm_context * vm_ctx) {
        return;
    }

    static void SetUpTestCase() {
    }
    static void TearDownTestCase() {
    }
};

#define PREPAIR                                                                                                                                                                    \
    auto exe_addr = std::string{sys_contract_relay_make_block_addr};                                                                                                               \
    auto contract_addr = common::xnode_id_t{sys_contract_relay_make_block_addr};                                                                                                   \
    auto vbstate = make_object_ptr<xvbstate_t>(sys_contract_relay_make_block_addr, 1, 1, std::string{}, std::string{}, 0, 0, 0);                                                   \
    auto unitstate = std::make_shared<xunit_bstate_t>(vbstate.get());                                                                                                              \
    auto account_context = std::make_shared<xaccount_context_t>(unitstate);                                                                                                        \
    auto contract_helper = std::make_shared<xcontract_helper>(account_context.get(), contract_addr, exe_addr);                                                                     \
    set_contract_helper(contract_helper);                                                                                                                                          \
    setup();

TEST_F(xtop_test_relay_make_block_contract, update_wrap_phase) {
    PREPAIR

    bool ret = update_wrap_phase(0);
    EXPECT_EQ(ret, false);

    STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_0);
    ret = update_wrap_phase(0);
    EXPECT_EQ(ret, true);
    auto wrap_phase = STRING_GET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    EXPECT_EQ(wrap_phase, RELAY_WRAP_PHASE_1);

    ret = update_wrap_phase(0);
    EXPECT_EQ(ret, true);
    wrap_phase = STRING_GET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    EXPECT_EQ(wrap_phase, RELAY_WRAP_PHASE_2);

    ret = update_wrap_phase(0);
    EXPECT_EQ(ret, false);
    wrap_phase = STRING_GET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    EXPECT_EQ(wrap_phase, RELAY_WRAP_PHASE_2);
}

TEST_F(xtop_test_relay_make_block_contract, update_next_block_clock_for_a_type) {
    PREPAIR

    update_next_block_clock_for_a_type(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME, 10000);
    auto next_tx_block_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME)));
    EXPECT_EQ(next_tx_block_time, 10000);
}

TEST_F(xtop_test_relay_make_block_contract, block_hash_chainid) {
    PREPAIR

    evm_common::h256 block_hash(top::evm_common::fromHex("3963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits = 0x3;

    auto str = block_hash_chainid_to_string(block_hash, chain_bits);

    evm_common::h256 block_hash_1;
    evm_common::u256 chain_bits_1;

    block_hash_chainid_from_string(str, block_hash_1, chain_bits_1);

    EXPECT_EQ(block_hash_1, block_hash);
    EXPECT_EQ(chain_bits_1, chain_bits);
}

TEST_F(xtop_test_relay_make_block_contract, pop_tx_block_hashs) {
    PREPAIR

    evm_common::h256 block_hash1(top::evm_common::fromHex("3963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits1 = 0x1;

    evm_common::h256 block_hash2(top::evm_common::fromHex("4963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits2 = 0x1;

    evm_common::h256 block_hash3(top::evm_common::fromHex("5963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits3 = 0x2;

    std::string list_key = XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST;

    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash1, chain_bits1));
    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash2, chain_bits2));
    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash3, chain_bits3));

    std::vector<evm_common::h256> tx_block_hash_vec;
    evm_common::u256 chain_bits;
    pop_tx_block_hashs(list_key, true, tx_block_hash_vec, chain_bits);

    EXPECT_EQ(3, tx_block_hash_vec.size());
    EXPECT_EQ(block_hash1, tx_block_hash_vec[0]);
    EXPECT_EQ(block_hash2, tx_block_hash_vec[1]);
    EXPECT_EQ(block_hash3, tx_block_hash_vec[2]);
    EXPECT_EQ(chain_bits, chain_bits1 | chain_bits2 | chain_bits3);

    evm_common::h256 block_hash4(top::evm_common::fromHex("6963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits4 = 0x4;
    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash4, chain_bits4));

    std::string list_key_e = XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST;

    std::vector<evm_common::h256> tx_block_hash_vec_e;
    evm_common::u256 chain_bits_e;
    pop_tx_block_hashs(list_key_e, false, tx_block_hash_vec_e, chain_bits_e);
    pop_tx_block_hashs(list_key, false, tx_block_hash_vec_e, chain_bits_e);

    EXPECT_EQ(4, tx_block_hash_vec_e.size());
    EXPECT_EQ(block_hash1, tx_block_hash_vec_e[0]);
    EXPECT_EQ(block_hash2, tx_block_hash_vec_e[1]);
    EXPECT_EQ(block_hash3, tx_block_hash_vec_e[2]);
    EXPECT_EQ(block_hash4, tx_block_hash_vec_e[3]);
    EXPECT_EQ(chain_bits_e, chain_bits1 | chain_bits2 | chain_bits3 | chain_bits4);
}

TEST_F(xtop_test_relay_make_block_contract, build_elect_relay_block_no_data) {
    PREPAIR

    evm_common::h256 prev_hash;
    std::string data = "";
    bool ret = build_elect_relay_block(prev_hash, 1, 1000, data);
    EXPECT_EQ(ret, false);
}

common::xaccount_address_t build_account_address(size_t const i) {
    std::string account_address_prefix = "T00000LMZLAYynftsjQiKZ5W7TQncDL";

    std::string account_string = account_address_prefix + std::to_string(i);
    if (account_string.size() < top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH) {
        account_string.append(top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH - account_string.size(), 'x');
    }
    assert(account_string.size() == top::common::xaccount_base_address_t::LAGACY_USER_ACCOUNT_LENGTH);

    return common::xaccount_address_t{account_string};
}

TEST_F(xtop_test_relay_make_block_contract, build_elect_relay_block) {
    PREPAIR

    std::string data = "";
    bool ret = build_elect_relay_block({}, 1, 1000, data);
    EXPECT_EQ(ret, false);

    top::common::xnetwork_id_t network_id{top::common::xtopchain_network_id};
    top::common::xzone_id_t zone_id{top::common::xrelay_zone_id};
    top::common::xcluster_id_t cluster_id{top::common::xdefault_cluster_id};
    top::common::xgroup_id_t group_id{top::common::xdefault_group_id};

    xelection_result_store_t election_result_store;
    auto & group_result = election_result_store.result_of(network_id).result_of(xnode_type_t::relay).result_of(cluster_id).result_of(group_id);
    group_result.group_version(top::common::xelection_round_t{1});
    group_result.election_committee_version(top::common::xelection_round_t{1});
    group_result.start_time(0);

    for (auto i = 0u; i < 8u; ++i) {
        xelection_info_t new_election_info{};
        new_election_info.joined_version = top::common::xelection_round_t{1};
        new_election_info.consensus_public_key = top::xpublic_key_t{"BNRHeRGw4YZnTHeNGxYtuAsvSslTV7THMs3A9RJM+1Vg63gyQ4XmK2i8HW+f3IaM7KavcH7JMhTPFzKtWp7IXW4="};

        xelection_info_bundle_t election_info_bundle;
        election_info_bundle.account_address(build_account_address(i));
        election_info_bundle.election_info(std::move(new_election_info));

        group_result.insert(std::move(election_info_bundle));
    }

    auto bytes = codec::msgpack_encode(election_result_store);
    std::string obj_str{std::begin(bytes), std::end(bytes)};

    ret = build_elect_relay_block({}, 1, 1000, obj_str);
    EXPECT_EQ(ret, true);

    uint64_t epoch_id = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_EPOCH_ID)));
    EXPECT_EQ(epoch_id, 1);
    uint64_t height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_HEIGHT)));
    EXPECT_EQ(height, 1);
}

TEST_F(xtop_test_relay_make_block_contract, build_poly_relay_block) {
    PREPAIR

    uint64_t clock = 10000;
    std::string clock_str = std::to_string(clock);

    STRING_SET(XPROPERTY_RELAY_NEXT_POLY_BLOCK_LOGIC_TIME, clock_str);

    bool ret = build_poly_relay_block({}, 1, clock - 1);
    EXPECT_EQ(ret, false);

    ret = build_poly_relay_block({}, 1, clock + 1);
    EXPECT_EQ(ret, false);

    evm_common::h256 block_hash1(top::evm_common::fromHex("3963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits1 = 0x1;

    evm_common::h256 block_hash2(top::evm_common::fromHex("4963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits2 = 0x1;

    evm_common::h256 block_hash3(top::evm_common::fromHex("5963ed01bce983f8829174b13d3417716ff604bfb0fed07634288df8b146f20f"));
    evm_common::u256 chain_bits3 = 0x2;

    std::string list_key = XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST;

    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash1, chain_bits1));
    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash2, chain_bits2));
    LIST_PUSH_BACK(list_key, block_hash_chainid_to_string(block_hash3, chain_bits3));

    ret = build_poly_relay_block({}, 1, clock + 1);
    EXPECT_EQ(ret, true);

    uint64_t height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_HEIGHT)));
    EXPECT_EQ(height, 1);

    EXPECT_EQ(LIST_SIZE(list_key), 0);
    EXPECT_EQ(LIST_SIZE(XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST), 3);
}

TEST_F(xtop_test_relay_make_block_contract, on_make_block_invalid) {
    PREPAIR

    on_make_block("");
}

static xeth_transaction_t create_test_eth() {
    xeth_transaction_t tx;
    std::error_code ec;
    tx.set_tx_version(EIP_1559);
    tx.set_chainid(0x26b);
    tx.set_nonce(0x2);
    tx.set_max_priority_fee_per_gas(0x59682f00);
    tx.set_max_fee_per_gas(0x59682f08);
    tx.set_gas(0x5208);
    tx.set_to(common::xeth_address_t::build_from("0xc8e6615f4c0ca0f44c0ac05daadb2aaad9720c98"));
    tx.set_value(0x1bc16d674ec80000);
    tx.set_data(top::from_hex("0x", ec));
    tx.set_signV(0x1);
    tx.set_signR(evm_common::xh256_t(top::from_hex("0x3aa2d1b9ca2c95f5bcf3dc4076241cb0552359ebfa523ad9c045aa3c1953779c", ec)));
    tx.set_signS(evm_common::xh256_t(top::from_hex("0x385b0d94ee10c5325ae4960a616c9c2aaad9e8549dd43d68bb5ca14206d62ded", ec)));
    return tx;
}

TEST_F(xtop_test_relay_make_block_contract, build_tx_relay_block) {
    PREPAIR

    std::error_code ec;
    std::string cross_addr = "0xbc9b5f068bc20a5b12030fcb72975d8bddc4e84c";
    std::string cross_topic_str = "0x342827c97908e5e2f71151c08502a66d44b6f758e3ac2f1de95f02eb95f0a735";

    std::string cross_config_addr = cross_addr + ":" + cross_topic_str + ":1";
    top::config::config_register.get_instance().set(config::xcross_chain_contract_list_onchain_goverance_parameter_t::name, cross_config_addr);

    xrelayblock_crosstx_infos_t txinfos;
    for (uint32_t i = 0; i < 1; i++) {
        xeth_transaction_t _tx = create_test_eth();
        xeth_receipt_t _receipt;
        evm_common::xevm_logs_t logs;
        evm_common::xevm_log_t log;
        log.address = common::xtop_eth_address::build_from(cross_addr);
        logs.push_back(log);
        _receipt.set_logs(logs);
        xrelayblock_crosstx_info_t txinfo(_tx, _receipt);
        txinfos.tx_infos.push_back(txinfo);
    }
    std::string param_str = txinfos.serialize_to_string();

    on_receive_cross_txs(param_str);
    EXPECT_EQ(LIST_SIZE(XPROPERTY_RELAY_CROSS_TXS), 1);

    auto clock_to_pack = TIME() + (uint64_t)XGET_CONFIG(max_relay_tx_block_interval);

    auto ret = build_tx_relay_block({}, 1, clock_to_pack - 1);
    EXPECT_EQ(ret, false);
    uint64_t height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_HEIGHT)));
    EXPECT_EQ(height, 0);

    ret = build_tx_relay_block({}, 1, clock_to_pack + 1);
    EXPECT_EQ(ret, true);
    height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_HEIGHT)));
    EXPECT_EQ(height, 1);
    EXPECT_EQ(LIST_SIZE(XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST), 1);

    ret = build_tx_relay_block({}, 1, clock_to_pack + 1);
    EXPECT_EQ(ret, false);
    height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_HEIGHT)));
    EXPECT_EQ(height, 1);
}
