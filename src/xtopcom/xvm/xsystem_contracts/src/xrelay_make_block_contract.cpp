// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xrelay/xrelay_make_block_contract.h"

#include "xbasic/xhex.h"
#include "xcodec/xmsgpack_codec.hpp"
// #include "xdata/xcodec/xmsgpack/xelection/xelection_result_store_codec.hpp"
// #include "xdata/xelection/xelection_result_property.h"
#include "xdata/xblockextract.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xrootblock.h"
#include "xvm/xserialization/xserialization.h"

#include "xbasic/xutility.h"

NS_BEG4(top, xvm, system_contracts, relay)

#define RELAY_PACK_TYPE_ELECT "e"
#define RELAY_PACK_TYPE_POLY "p"
#define RELAY_PACK_TYPE_TX "t"
#define RELAY_PACK_TYPE_INVALID "i"

xtop_relay_make_block_contract::xtop_relay_make_block_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

// same impl as zec_elect_relay setup
void xtop_relay_make_block_contract::setup() {
    STRING_CREATE(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME);
    std::string last_pack_logic_time{"0"};
    STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME, last_pack_logic_time);

    // STRING_CREATE(data::system_contract::XPROPERTY_RELAY_LAST_BLOCK_TYPE);
    // STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_BLOCK_TYPE, RELAY_PACK_TYPE_ELECT);

    LIST_CREATE(data::system_contract::XPROPERTY_RELAY_CROSS_TXS);

    LIST_CREATE(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST);

    LIST_CREATE(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST);
}

void xtop_relay_make_block_contract::on_receive_cross_txs(xbytes_t const & data) {
    // todo(nathan):把跨链交易数据存入属性的list中XPROPERTY_RELAY_CROSS_TXS
    xdbg("xtop_relay_make_block_contract::on_receive_cross_txs in");
    data::xrelayblock_crosstx_infos_t all_crosstxs;
    std::error_code ec;
    all_crosstxs.serialize_from_string(from_bytes<std::string>(data), ec);
    XCONTRACT_ENSURE(!ec, "xtop_relay_make_block_contract unpack crosstxs fail " + ec.message());
    for (auto & crosstx : all_crosstxs.tx_infos) {
        xdbg("xtop_relay_make_block_contract::on_receive_cross_txs tx hash:%s", top::to_hex_prefixed(top::to_bytes(crosstx.tx.get_tx_hash())).c_str());
        auto tx_bytes = crosstx.encodeBytes();
        LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_CROSS_TXS, to_string(tx_bytes));
    }
}

void xtop_relay_make_block_contract::on_make_block(xbytes_t const & data) {
    uint64_t cur_time = TIME();
    std::string cur_time_str = std::to_string(cur_time);
    if (!data.empty()) {
        // todo(nathan):data is new elect info, make elect block
        LIST_CLEAR(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST);
        LIST_CLEAR(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST);
        STRING_SET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME, cur_time_str);
    } else {
        // auto last_block_type = STRING_GET(data::system_contract::XPROPERTY_RELAY_LAST_BLOCK_TYPE);
        auto last_poly_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(data::system_contract::XPROPERTY_RELAY_LAST_POLY_BLOCK_LOGIC_TIME)));
        auto cross_tx_list_size = LIST_SIZE(data::system_contract::XPROPERTY_RELAY_CROSS_TXS);
        if (last_poly_time + XGET_ONCHAIN_GOVERNANCE_PARAMETER(max_relay_poly_interval) < cur_time) {
            auto tx_block_num = LIST_SIZE(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST);
            if (tx_block_num > 0) {
                for (int32_t i = 0; i < tx_block_num; i++) {
                    std::string block_hash;
                    LIST_POP_FRONT(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_FROM_LAST_POLY_LIST, block_hash);
                    // todo(nathan):calculate block tree.
                    LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, block_hash);
                }
                // todo(nathan):make poly block
            } else {
                if (cross_tx_list_size > 0) {
                    // todo(nathan):make tx block
                    // todo(nathan):get relay block hash
                    std::string block_hash;
                    LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, block_hash);
                }
            }
        } else {
            if (cross_tx_list_size > XGET_CONFIG(relayblock_batch_tx_max_num)) {
                // todo(nathan):make tx block
                // todo(nathan):get relay block hash
                std::string block_hash;
                LIST_PUSH_BACK(data::system_contract::XPROPERTY_RELAY_BLOCK_HASH_LAST_ELECT_TO_LAST_POLY_LIST, block_hash);
            }
        }
    }
}

NS_END4
