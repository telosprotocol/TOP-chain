// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xrelay/xrelay_make_block_contract.h"

#include "xbasic/xhex.h"
#include "xbasic/xutility.h"
#include "xcodec/xmsgpack_codec.hpp"
#include "xdata/xblockextract.h"
#include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xrootblock.h"
#include "xevm_common/rlp.h"
#include "xvm/xserialization/xserialization.h"

NS_BEG4(top, xvm, system_contracts, relay)

#define RELAY_CHAIN_BIT_ETH (0x01)
#define RELAY_CHAIN_BIT_BSC (0x02)

xtop_relay_make_block_contract::xtop_relay_make_block_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

// same impl as zec_elect_relay setup
void xtop_relay_make_block_contract::setup() {
    STRING_CREATE(XPROPERTY_RELAY_NEXT_POLY_BLOCK_LOGIC_TIME);
    std::string zero_str{"0"};
    STRING_SET(XPROPERTY_RELAY_NEXT_POLY_BLOCK_LOGIC_TIME, zero_str);

    STRING_CREATE(XPROPERTY_RELAY_LAST_HEIGHT);
    STRING_SET(XPROPERTY_RELAY_LAST_HEIGHT, zero_str);

    STRING_CREATE(XPROPERTY_RELAY_LAST_HASH);
    data::xrelay_block genesis_relay_block = data::xrootblock_t::get_genesis_relay_block();
    std::string genesis_block_hash = from_bytes<std::string>(genesis_relay_block.get_block_hash().to_bytes());
    STRING_SET(XPROPERTY_RELAY_LAST_HASH, genesis_block_hash);

    STRING_CREATE(XPROPERTY_RELAY_LAST_EPOCH_ID);
    STRING_SET(XPROPERTY_RELAY_LAST_EPOCH_ID, zero_str);

    LIST_CREATE(XPROPERTY_RELAY_CROSS_TXS);

    LIST_CREATE(XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST);

    LIST_CREATE(XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_BLOCK_STR);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_BLOCK_STR, zero_str);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_INIT);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_ELECT_PACK_HEIGHT);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_ELECT_PACK_HEIGHT, zero_str);

    STRING_CREATE(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME);
    STRING_SET(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME, zero_str);

    data::election::v2::xelection_result_store_t election_result_store;
    common::xelection_round_t group_version{0};

    auto & current_group_nodes =
        election_result_store.result_of(network_id()).result_of(common::xnode_type_t::relay).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id);
    current_group_nodes.election_committee_version(common::xelection_round_t{0});
    current_group_nodes.timestamp(0);
    current_group_nodes.start_time(0);
    current_group_nodes.group_version(group_version);

    auto const max_relay_group_size = config::xmax_relay_group_size_onchain_goverance_parameter_t::value;

    const std::vector<data::node_info_t> & seeds = data::xrootblock_t::get_seed_nodes();
    for (auto i = 0u; i < seeds.size() && i < max_relay_group_size; ++i) {
        auto const & item = seeds[i];

        common::xnode_id_t node_id{item.m_account};

        data::election::v2::xelection_info_t election_info{};
        election_info.joined_version = group_version;
        election_info.stake = std::numeric_limits<uint64_t>::max();
        election_info.consensus_public_key = xpublic_key_t{item.m_publickey};
        election_info.genesis = true;
        election_info.miner_type = static_cast<common::xminer_type_t>(std::numeric_limits<uint32_t>::max());
        election_info.raw_credit_score = std::numeric_limits<uint64_t>::max();

        data::election::v2::xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.account_address(node_id);
        election_info_bundle.election_info(std::move(election_info));

        current_group_nodes.insert(std::move(election_info_bundle));
    }

    STRING_CREATE(data::election::get_property_by_group_id(common::xdefault_group_id));
    serialization::xmsgpack_t<data::election::v2::xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xdefault_group_id), election_result_store);
}

void xtop_relay_make_block_contract::on_receive_cross_txs(std::string const & data) {
    xdbg("xtop_relay_make_block_contract::on_receive_cross_txs in");
    data::xrelayblock_crosstx_infos_t all_crosstxs;
    std::error_code ec;
    all_crosstxs.serialize_from_string(data, ec);
    XCONTRACT_ENSURE(!ec, "unpack crosstxs fail " + ec.message());
    XCONTRACT_ENSURE(!all_crosstxs.tx_infos.empty(), "cross txs is empty");

    if (LIST_SIZE(XPROPERTY_RELAY_CROSS_TXS) == 0) {
        update_next_block_clock_for_a_type(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME, TIME() + (uint64_t)XGET_CONFIG(max_relay_tx_block_interval));
    }
    for (auto & crosstx : all_crosstxs.tx_infos) {
        xdbg("xtop_relay_make_block_contract::on_receive_cross_txs tx hash:%s", top::to_hex_prefixed(top::to_bytes(crosstx.tx.get_tx_hash())).c_str());
        auto tx_bytes = crosstx.encodeBytes();
        LIST_PUSH_BACK(XPROPERTY_RELAY_CROSS_TXS, to_string(tx_bytes));
    }
}

void xtop_relay_make_block_contract::update_next_block_clock_for_a_type(const string & key, uint64_t clock) {
    STRING_SET(key, std::to_string(clock));
    xdbg("xtop_relay_make_block_contract::update_next_tx_block_clock key:%s, next:%llu", key.c_str(), clock);
}

bool xtop_relay_make_block_contract::update_wrap_phase(uint64_t last_height) {
    auto wrap_phase = STRING_GET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    xdbg("xtop_relay_make_block_contract::update_wrap_phase. last wrap_phase=%s,height:%llu", wrap_phase.c_str(), last_height);
    if (wrap_phase == RELAY_WRAP_PHASE_0) {
        STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_1);
        return true;
    } else if (wrap_phase == RELAY_WRAP_PHASE_1) {
        STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_2);
        return true;
    }
    if ((wrap_phase != RELAY_WRAP_PHASE_2) || (wrap_phase != RELAY_WRAP_PHASE_INIT)) {
        xerror("xtop_relay_make_block_contract::update_wrap_phase wrap phase invalid:%s", wrap_phase.c_str());
    }
    return false;
}

void xtop_relay_make_block_contract::on_make_block(std::string const & data) {
    uint64_t last_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_HEIGHT)));
    bool ret = update_wrap_phase(last_height);
    if (ret) {
        return;
    }

    uint64_t clock = TIME();
    auto last_hash_str = STRING_GET(XPROPERTY_RELAY_LAST_HASH);
    evm_common::h256 last_hash(to_bytes(last_hash_str));
    uint64_t new_height = last_height + 1;

    ret = build_elect_relay_block(last_hash, new_height, clock, data);
    if (ret) {
        return;
    }

    ret = build_poly_relay_block(last_hash, new_height, clock);
    if (ret) {
        return;
    }

    ret = build_tx_relay_block(last_hash, new_height, clock);
    xdbg("xtop_relay_make_block_contract::on_make_block build new relay block height:%llu ret:%d", new_height, ret);
}

void xtop_relay_make_block_contract::pop_tx_block_hashs(const string & list_key,
                                                        bool for_poly_block,
                                                        std::vector<evm_common::h256> & tx_block_hash_vec,
                                                        evm_common::u256 & chain_bits) {
    auto tx_block_num = LIST_SIZE(list_key);
    if (tx_block_num > 0) {
        for (int32_t i = 0; i < tx_block_num; i++) {
            std::string str;
            LIST_POP_FRONT(list_key, str);
            evm_common::h256 block_hash;
            evm_common::u256 chain_bits_tmp;
            block_hash_chainid_from_string(str, block_hash, chain_bits_tmp);
            chain_bits |= chain_bits_tmp;
            tx_block_hash_vec.push_back(block_hash);
            if (for_poly_block) {
                LIST_PUSH_BACK(XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, str);
            }
        }
    }
}

void xtop_relay_make_block_contract::proc_created_relay_block(data::xrelay_block & relay_block, uint64_t clock, const evm_common::u256 & chain_bits) {
    relay_block.build_finish();

    if (!relay_block.get_all_transactions().empty()) {
        if (LIST_SIZE(XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST) == 0) {
            update_next_block_clock_for_a_type(XPROPERTY_RELAY_NEXT_POLY_BLOCK_LOGIC_TIME, clock + (uint64_t)XGET_CONFIG(max_relay_poly_interval));
        }
        LIST_PUSH_BACK(XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, block_hash_chainid_to_string(relay_block.get_block_hash(), chain_bits));
        update_next_block_clock_for_a_type(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME, clock + (uint64_t)XGET_CONFIG(max_relay_tx_block_interval));
    }

    STRING_SET(XPROPERTY_RELAY_LAST_HEIGHT, to_string(relay_block.get_block_height()));
    std::string block_hash = from_bytes<std::string>(relay_block.get_block_hash().to_bytes());
    STRING_SET(XPROPERTY_RELAY_LAST_HASH, block_hash);

    xbytes_t rlp_stream = relay_block.encodeBytes();
    std::string relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream));
    STRING_SET(data::system_contract::XPROPERTY_RELAY_BLOCK_STR, relay_block_data);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_0);
    xdbg("xtop_relay_make_block_contract::proc_created_relay_block new relayblock:%s", relay_block.dump().c_str());
}

bool xtop_relay_make_block_contract::build_elect_relay_block(const evm_common::h256 & prev_hash, uint64_t block_height, uint64_t clock, std::string const & data) {
    if (data.empty()) {
        xdbg("xtop_relay_process_election_data_contract::on_recv_election_data data empty");
        return false;
    }

    uint64_t last_epoch_id = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_LAST_EPOCH_ID)));

    auto const & coming_election_result_store = codec::msgpack_decode<data::election::v2::xelection_result_store_t>({std::begin(data), std::end(data)});

    auto const & group_result =
        coming_election_result_store.result_of(network_id()).result_of(common::xnode_type_t::relay).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id);

    uint64_t epoch = group_result.group_epoch().value();
    xinfo("xtop_relay_make_block_contract::build_elect_relay_block last epoch:%llu,epoch:%llu", last_epoch_id, epoch);
    XCONTRACT_ENSURE((last_epoch_id + 1 == epoch), "epoch id invalid");

    serialization::xmsgpack_t<data::election::v2::xelection_result_store_t>::serialize_to_string_prop(
        *this, data::election::get_property_by_group_id(common::xdefault_group_id), coming_election_result_store);

    data::xrelay_election_group_t reley_election_group;

    for (auto const & node_info : group_result) {
        auto const & election_info_bundle = top::get<data::election::v2::xelection_info_bundle_t>(node_info);
        xdbg("xtop_relay_make_block_contract::build_elect_relay_block get node:%s", election_info_bundle.account_address().c_str());
        auto const & election_info = election_info_bundle.election_info();
        auto pubkey_str = base::xstring_utl::base64_decode(election_info.consensus_public_key.to_string());
        xbytes_t bytes_x(pubkey_str.begin() + 1, pubkey_str.begin() + 33);
        xbytes_t bytes_y(pubkey_str.begin() + 33, pubkey_str.end());
        reley_election_group.elections_vector.push_back(data::xrelay_election_node_t(evm_common::h256(bytes_x), evm_common::h256(bytes_y)));
    }
    reley_election_group.election_epochID = epoch;

    std::vector<evm_common::h256> tx_block_hash_vec;
    evm_common::u256 chain_bits = 0;
    pop_tx_block_hashs(XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, false, tx_block_hash_vec, chain_bits);
    pop_tx_block_hashs(XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, false, tx_block_hash_vec, chain_bits);

    data::xrelay_block relay_block = data::xrelay_block(prev_hash, block_height, base::clock_to_gmtime(clock), chain_bits, reley_election_group);
    if (!tx_block_hash_vec.empty()) {
        relay_block.make_merkle_root_hash(tx_block_hash_vec);
    }

    uint64_t cur_height = get_blockchain_height(sys_contract_relay_make_block_addr) + 1;
    STRING_SET(data::system_contract::XPROPERTY_RELAY_ELECT_PACK_HEIGHT, to_string(cur_height));
    STRING_SET(XPROPERTY_RELAY_LAST_EPOCH_ID, std::to_string(epoch));
    proc_created_relay_block(relay_block, clock, 0);
    return true;
}

bool xtop_relay_make_block_contract::build_poly_relay_block(const evm_common::h256 & prev_hash, uint64_t block_height, uint64_t clock) {
    auto next_tx_block_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_NEXT_POLY_BLOCK_LOGIC_TIME)));
    if ((next_tx_block_time == 0) || (clock < next_tx_block_time)) {
        return false;
    }

    std::vector<evm_common::h256> tx_block_hash_vec;
    evm_common::u256 chain_bits = 0;
    pop_tx_block_hashs(XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, true, tx_block_hash_vec, chain_bits);
    xdbg("xtop_relay_make_block_contract::build_poly_relay_block tx block height:%llu,hash num:%u", block_height, tx_block_hash_vec.size());
    if (tx_block_hash_vec.empty()) {
        return false;
    }

    data::xrelay_block relay_block = data::xrelay_block(prev_hash, block_height, base::clock_to_gmtime(clock), chain_bits);
    relay_block.make_merkle_root_hash(tx_block_hash_vec);
    proc_created_relay_block(relay_block, clock, 0);
    return true;
}

bool xtop_relay_make_block_contract::build_tx_relay_block(const evm_common::h256 & prev_hash, uint64_t block_height, uint64_t clock) {
    auto next_tx_block_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPROPERTY_RELAY_NEXT_TX_BLOCK_LOGIC_TIME)));
    int32_t min_tx_num = ((next_tx_block_time != 0) && (clock >= next_tx_block_time)) ? 1 : XGET_CONFIG(relayblock_batch_tx_max_num);

    auto cross_tx_list_size = LIST_SIZE(XPROPERTY_RELAY_CROSS_TXS);
    xdbg("xtop_relay_make_block_contract::build_tx_relay_block height:%llu,min_num:%d,cross tx:%d", block_height, min_tx_num, cross_tx_list_size);
    if (cross_tx_list_size < min_tx_num) {
        return false;
    }
#ifndef CROSS_TX_DBG
    auto cross_chain_config = data::xblockextract_t::get_cross_chain_config();
#endif
    auto batch_num = XGET_CONFIG(relayblock_batch_tx_max_num);
    int32_t pack_num = cross_tx_list_size < batch_num ? cross_tx_list_size : batch_num;

    std::vector<data::xeth_transaction_t> transactions;
    std::vector<data::xeth_receipt_t> receipts;
    evm_common::u256 chain_bits = 0;
    for (int32_t i = 0; i < pack_num; i++) {
        std::string tx_str;
        LIST_POP_FRONT(XPROPERTY_RELAY_CROSS_TXS, tx_str);
        std::error_code ec;
        data::xrelayblock_crosstx_info_t cross_tx;
        cross_tx.decodeBytes(to_bytes(tx_str), ec);
        XCONTRACT_ENSURE(!ec, "xtop_relay_make_block_contract unpack crosstxs fail " + ec.message());
        transactions.push_back(cross_tx.tx);
        receipts.push_back(cross_tx.receipt);
#ifndef CROSS_TX_DBG
        evm_common::u256 tx_chain_bits = 0;
        auto ret = data::xblockextract_t::get_chain_bits(cross_tx.receipt.get_logs(), cross_chain_config, tx_chain_bits);
        if (!ret) {
            xerror("xtop_relay_make_block_contract::build_tx_relay_block invalid cross tx:%s", top::to_hex_prefixed(top::to_bytes(cross_tx.tx.get_tx_hash())).c_str());
            XCONTRACT_ENSURE(false, "invalid cross addr");
        }
        chain_bits |= tx_chain_bits;
#else
        chain_bits |= RELAY_CHAIN_BIT_ETH;
#endif
        xdbg("xtop_relay_make_block_contract::build_tx_relay_block height:%llu add tx:%s", block_height, top::to_hex_prefixed(top::to_bytes(cross_tx.tx.get_tx_hash())).c_str());
    }

    data::xrelay_block relay_block = data::xrelay_block(prev_hash, block_height, base::clock_to_gmtime(clock), chain_bits, transactions, receipts);
    proc_created_relay_block(relay_block, clock, chain_bits);
    return true;
}

const std::string xtop_relay_make_block_contract::block_hash_chainid_to_string(const evm_common::h256 & block_hash, const evm_common::u256 & chain_bits) {
    xdbg("xtop_relay_make_block_contract::block_hash_chainid_to_string block hash:%s,chain_bits:%s",
         block_hash.hex().c_str(),
         evm_common::toHex((evm_common::h256)chain_bits).c_str());
    evm_common::RLPStream rlp_stream;
    rlp_stream.appendList(2);
    rlp_stream << block_hash;
    rlp_stream << chain_bits;
    return from_bytes<std::string>(rlp_stream.out());
}

void xtop_relay_make_block_contract::block_hash_chainid_from_string(const std::string & str, evm_common::h256 & block_hash, evm_common::u256 & chain_bits) {
    xbytes_t bytes = to_bytes(str);
    evm_common::RLP _r(bytes);

    if (!_r.isList() || _r.itemCount() != 2) {
        xerror("xrelayblock_crosstx_info_t::decodeRLP fail item count,%d", _r.itemCount());
    }
    XCONTRACT_ENSURE((_r.isList() && _r.itemCount() == 2), "block hash and chain id decode fail");

    block_hash = (evm_common::h256)_r[0];
    chain_bits = (evm_common::u256)_r[1];
    xdbg("xtop_relay_make_block_contract::block_hash_chainid_from_string block hash:%s,chain_bits:%s",
         block_hash.hex().c_str(),
         evm_common::toHex((evm_common::h256)chain_bits).c_str());
}

NS_END4
