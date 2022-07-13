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
#include "xvm/xserialization/xserialization.h"

NS_BEG4(top, xvm, system_contracts, relay)

#define RELAY_WRAP_PHASE_0 "0"
#define RELAY_WRAP_PHASE_1 "1"
#define RELAY_WRAP_PHASE_2 "2"
#define RELAY_WRAP_PHASE_INIT "2"

xtop_relay_make_block_contract::xtop_relay_make_block_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

// same impl as zec_elect_relay setup
void xtop_relay_make_block_contract::setup() {
    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME);
    std::string zero_str{"0"};
    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME, zero_str);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_LAST_HEIGHT);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_HEIGHT, zero_str);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_LAST_HASH);
    data::xrelay_block genesis_relay_block = data::xrootblock_t::get_genesis_relay_block();
    std::string genesis_block_hash = from_bytes<std::string>(genesis_relay_block.get_block_hash().to_bytes());
    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_HASH, genesis_block_hash);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_LAST_EPOCH_ID);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_EPOCH_ID, zero_str);

    LIST_CREATE(data::system_contract::XPROPERTY_RELAY_CROSS_TXS);

    LIST_CREATE(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST);

    LIST_CREATE(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_BLOCK_STR);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_BLOCK_STR, zero_str);

    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_INIT);
}

void xtop_relay_make_block_contract::on_receive_cross_txs(std::string const & data) {
    xdbg("xtop_relay_make_block_contract::on_receive_cross_txs in");
    data::xrelayblock_crosstx_infos_t all_crosstxs;
    std::error_code ec;
    all_crosstxs.serialize_from_string(data, ec);
    XCONTRACT_ENSURE(!ec, "xtop_relay_make_block_contract unpack crosstxs fail " + ec.message());
    for (auto & crosstx : all_crosstxs.tx_infos) {
        xdbg("xtop_relay_make_block_contract::on_receive_cross_txs tx hash:%s", top::to_hex_prefixed(top::to_bytes(crosstx.tx.get_tx_hash())).c_str());
        auto tx_bytes = crosstx.encodeBytes();
        LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_CROSS_TXS, to_string(tx_bytes));
    }
}

void xtop_relay_make_block_contract::on_make_block(std::string const & data) {
    auto wrap_phase = STRING_GET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE);
    uint64_t last_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_RELAY_LAST_HEIGHT)));
    xdbg("xtop_relay_make_block_contract::on_make_block enter. wrap_phase=%s,height:%llu", wrap_phase.c_str(), last_height);
    if (wrap_phase == RELAY_WRAP_PHASE_0) {
        STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_1);
        return;
    } else if (wrap_phase == RELAY_WRAP_PHASE_1) {
        STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_2);
        return;
    }

    XCONTRACT_ENSURE((wrap_phase == RELAY_WRAP_PHASE_2) || (wrap_phase == RELAY_WRAP_PHASE_INIT), "wrap phase invalid:" + wrap_phase);

    uint64_t cur_time = TIME() * 10;
    auto last_hash_str = STRING_GET(data::system_contract::XPROPERTY_RELAY_LAST_HASH);
    evm_common::h256 last_hash(to_bytes(last_hash_str));
    bool ret;

    data::xrelay_block relay_block;

    ret = build_elect_relay_block(last_hash, last_height + 1, cur_time, relay_block);
    if (ret) {
        return;
    }

    auto last_poly_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME)));
    if (last_poly_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_relay_poly_interval) * 10 < cur_time) {
        ret = build_poly_relay_block(last_hash, last_height + 1, cur_time, relay_block);
        if (!ret) {
            ret = build_tx_relay_block(last_hash, last_height + 1, cur_time, 1, relay_block);
        }
    } else {
        ret = build_tx_relay_block(last_hash, last_height + 1, cur_time, XGET_CONFIG(relayblock_batch_tx_max_num), relay_block);
    }
    xdbg("xtop_relay_make_block_contract::on_make_block build new relay block height:%llu ret:%d", last_height + 1, ret);
}

void xtop_relay_make_block_contract::pop_tx_block_hashs(const string & list_key, bool for_poly_block, std::vector<evm_common::h256> & tx_block_hash_vec) {
    auto tx_block_num = LIST_SIZE(list_key);
    if (tx_block_num > 0) {
        for (int32_t i = 0; i < tx_block_num; i++) {
            std::string block_hash_str;
            LIST_POP_FRONT(list_key, block_hash_str);
            evm_common::h256 block_hash(to_bytes(block_hash_str));
            tx_block_hash_vec.push_back(block_hash);
            if (for_poly_block) {
                LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, block_hash_str);
            }
        }
    }
}

void xtop_relay_make_block_contract::proc_created_relay_block(data::xrelay_block & relay_block, uint64_t timestamp) {
    relay_block.build_finish();

    std::string block_hash = from_bytes<std::string>(relay_block.get_block_hash().to_bytes());
    if (relay_block.get_all_transactions().empty()) {
        STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME, std::to_string(timestamp));
        if (!relay_block.get_elections_sets().empty()) {
            STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_EPOCH_ID, std::to_string(relay_block.get_elections_sets().election_epochID));
        }
    } else {
        LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, block_hash);
        if (relay_block.get_block_height() == 1) {
            STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME, std::to_string(timestamp));
        }
    }

    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_HEIGHT, to_string(relay_block.get_block_height()));
    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_HASH, block_hash);

    xbytes_t rlp_stream = relay_block.encodeBytes();
    std::string relay_block_data = from_bytes<std::string>((xbytes_t)(rlp_stream));
    STRING_SET(data::system_contract::XPROPERTY_RELAY_BLOCK_STR, relay_block_data);
    STRING_SET(data::system_contract::XPROPERTY_RELAY_WRAP_PHASE, RELAY_WRAP_PHASE_0);
    xdbg("xtop_relay_make_block_contract::proc_created_relay_block new relayblock:%s", relay_block.dump().c_str());
}

bool xtop_relay_make_block_contract::build_elect_relay_block(evm_common::h256 prev_hash, uint64_t block_height, uint64_t timestamp, data::xrelay_block & relay_block) {
    uint64_t last_epoch_id = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_RELAY_LAST_EPOCH_ID)));

    auto election_result_store = serialization::xmsgpack_t<data::election::v2::xelection_result_store_t>::deserialize_from_string_prop(
        *this, sys_contract_relay_repackage_election_addr, data::election::get_property_by_group_id(common::xdefault_group_id));

    auto const & group_result =
        election_result_store.result_of(network_id()).result_of(common::xnode_type_t::relay).result_of(common::xdefault_cluster_id).result_of(common::xdefault_group_id);

    uint64_t epoch = group_result.group_epoch().value();
    xdbg("xtop_relay_make_block_contract::build_elect_relay_block last epoch:%llu,epoch:%llu", last_epoch_id, epoch);
    if (last_epoch_id + 1 != epoch) {
        XCONTRACT_ENSURE((last_epoch_id == epoch), "epoch id invalid");
        if (last_epoch_id != epoch) {
            xerror("xtop_relay_make_block_contract::build_elect_relay_block last epoch:%llu,invalid epoch:%llu", last_epoch_id, epoch);
        }
        return false;
    }

    data::xrelay_election_group_t reley_election_group;

    for (auto const & node_info : group_result) {
        auto const & election_info = top::get<data::election::v2::xelection_info_bundle_t>(node_info).election_info();
        auto pubkey_str = base::xstring_utl::base64_decode(election_info.consensus_public_key.to_string());
        xbytes_t bytes_x(pubkey_str.begin() + 1, pubkey_str.begin() + 33);
        xbytes_t bytes_y(pubkey_str.begin() + 33, pubkey_str.end());
        reley_election_group.elections_vector.push_back(data::xrelay_election_node_t(evm_common::h256(bytes_x), evm_common::h256(bytes_y)));
    }
    reley_election_group.election_epochID = epoch;

    std::vector<evm_common::h256> tx_block_hash_vec;
    pop_tx_block_hashs(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, false, tx_block_hash_vec);
    pop_tx_block_hashs(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, false, tx_block_hash_vec);

    // todo(nathan):remove epoch id
    // todo(nathan):set block root hash
    relay_block = data::xrelay_block(prev_hash, block_height, 0, timestamp, reley_election_group);
    xdbg("xtop_relay_make_block_contract::build_elect_relay_block new elect block epoch:%llu,height:%llu,tx block hash num:%u", epoch, block_height, tx_block_hash_vec.size());
    proc_created_relay_block(relay_block, timestamp);
    return true;
}

bool xtop_relay_make_block_contract::build_poly_relay_block(evm_common::h256 prev_hash, uint64_t block_height, uint64_t timestamp, data::xrelay_block & relay_block) {
    std::vector<evm_common::h256> tx_block_hash_vec;
    pop_tx_block_hashs(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, true, tx_block_hash_vec);
    xdbg("xtop_relay_make_block_contract::build_poly_relay_block tx block height:%llu,hash num:%u", block_height, tx_block_hash_vec.size());
    if (tx_block_hash_vec.empty()) {
        return false;
    }

    // todo(nathan):remove epoch id
    // todo(nathan):set block root hash
    relay_block = data::xrelay_block(prev_hash, block_height, 0, timestamp);
    proc_created_relay_block(relay_block, timestamp);
    xdbg("xtop_relay_make_block_contract::build_elect_relay_block new poly block,height:%llu,tx block hash num:%u", block_height, tx_block_hash_vec.size());
    return true;
}

bool xtop_relay_make_block_contract::build_tx_relay_block(evm_common::h256 prev_hash,
                                                          uint64_t block_height,
                                                          uint64_t timestamp,
                                                          int32_t min_num,
                                                          data::xrelay_block & relay_block) {
    auto cross_tx_list_size = LIST_SIZE(data::system_contract::XPROPERTY_RELAY_CROSS_TXS);
    xdbg("xtop_relay_make_block_contract::build_tx_relay_block height:%llu,min_num:%d,cross tx:%d", block_height, min_num, cross_tx_list_size);
    if (cross_tx_list_size < min_num) {
        return false;
    }
    auto batch_num = XGET_CONFIG(relayblock_batch_tx_max_num);
    int32_t pack_num = cross_tx_list_size < batch_num ? cross_tx_list_size : batch_num;

    std::vector<data::xeth_transaction_t> transactions;
    std::vector<data::xeth_receipt_t> receipts;
    for (int32_t i = 0; i < pack_num; i++) {
        std::string tx_str;
        LIST_POP_FRONT(data::system_contract::XPROPERTY_RELAY_CROSS_TXS, tx_str);
        std::error_code ec;
        data::xrelayblock_crosstx_info_t cross_tx;
        cross_tx.decodeBytes(to_bytes(tx_str), ec);
        XCONTRACT_ENSURE(!ec, "xtop_relay_make_block_contract unpack crosstxs fail " + ec.message());
        transactions.push_back(cross_tx.tx);
        receipts.push_back(cross_tx.receipt);
        xdbg("xtop_relay_make_block_contract::build_tx_relay_block height:%llu add tx:%s", block_height, top::to_hex_prefixed(top::to_bytes(cross_tx.tx.get_tx_hash())).c_str());
    }

    // todo(nathan):remove epoch id
    relay_block = data::xrelay_block(prev_hash, block_height, 0, timestamp, transactions, receipts);
    proc_created_relay_block(relay_block, timestamp);
    xdbg("xtop_relay_make_block_contract::build_tx_relay_block new tx block,height:%llu,tx num:%u", block_height, transactions.size());
    return true;
}

NS_END4
