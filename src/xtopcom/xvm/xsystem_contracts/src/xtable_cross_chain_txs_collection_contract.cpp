// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xevm/xtable_cross_chain_txs_collection_contract.h"

#include "xbase/xmem.h"
#include "xcertauth/xcertauth_face.h"
#include "xchain_fork/xchain_upgrade_center.h"
#include "xcommon/xip.h"
#include "xdata/xdata_common.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xdata/xblockextract.h"
#include "xmetrics/xmetrics.h"
#include "xvm/manager/xcontract_manager.h"

using namespace top::base;
using namespace top::data;
using namespace top::data::system_contract;

NS_BEG3(top, xvm, system_contracts)

#define MIN_BLOCKS_NUM  (1)
#define MIN_CLOCK_INTERVAL  (1)
#define MAX_TXS_COUNT  (5)

xtable_cross_chain_txs_collection_contract::xtable_cross_chain_txs_collection_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtable_cross_chain_txs_collection_contract::setup() {
    // initialize map key
    MAP_CREATE(XPORPERTY_CONTRACT_CROSSCHAIN_TX_RECEIPT_KEY);
    STRING_CREATE(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT);
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT, std::to_string(0));
    STRING_CREATE(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME);
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME, std::to_string(0));
}

void xtable_cross_chain_txs_collection_contract::on_timer(common::xlogic_time_t const current_time) {
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xtable_cross_chain_txs_collection_contract instance is triggled by " + SOURCE_ADDRESS());
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_eth_table_cross_chain_txs_collection_addr, "xtable_cross_chain_txs_collection_contract instance is not triggled by sys_contract_eth_table_cross_chain_txs_collection_addr");

    xdbg("[xtable_cross_chain_txs_collection_contract] on_timer: %" PRIu64, current_time);    

    auto latest_height = get_blockchain_height(sys_contract_eth_table_block_addr_with_suffix);
    auto latest_time = TIME();
    auto const last_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT)));
    auto const last_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME)));

    xdbg("[xtable_cross_chain_txs_collection_contract] latest_height: %" PRIu64 ",last_height: %" PRIu64 ",last_time: %" PRIu64, latest_height,last_height,last_time);

    if (latest_height < last_height + MIN_BLOCKS_NUM
        && latest_time < last_time + MIN_CLOCK_INTERVAL) {        
        return;
    }
    std::error_code ec;
    xrelayblock_crosstx_infos_t all_crosstxs;
    uint64_t finish_height = 0;
    for (uint64_t i = last_height+1; i <= latest_height; i++) {
        auto _block = get_block_by_height(sys_contract_eth_table_block_addr_with_suffix, i);
        XCONTRACT_ENSURE(nullptr != _block, "xtable_cross_chain_txs_collection_contract block nullptr " + std::to_string(i));

        xrelayblock_crosstx_infos_t crosstxs;
        data::xblockextract_t::unpack_crosschain_txs(_block.get(), crosstxs, ec);
        XCONTRACT_ENSURE(!ec, "xtable_cross_chain_txs_collection_contract unpack crosstxs fail " + ec.message());

        if (crosstxs.tx_infos.size() > 0) {
            all_crosstxs.tx_infos.insert(all_crosstxs.tx_infos.end(), crosstxs.tx_infos.begin(), crosstxs.tx_infos.end());
            xdbg("[xtable_cross_chain_txs_collection_contract] height: %" PRIu64 ",crosstx count: %" PRIu64 , i,crosstxs.tx_infos.size());
        }

        finish_height = i;
        if (all_crosstxs.tx_infos.size() >= MAX_TXS_COUNT) {
            break;
        }
    }

    std::string call_param;
    if (all_crosstxs.tx_infos.size() > 0) {
        call_param = all_crosstxs.serialize_to_string();
        // call relay block build contract
        // CALL();
        CALL(common::xaccount_address_t{sys_contract_relay_make_block_addr}, "on_receive_cross_txs", call_param);
        xdbg("[xtable_cross_chain_txs_collection_contract] called: %" PRIu64 ",latest_time: %" PRIu64 ",param_size: %zu", finish_height,current_time,call_param.size());  
    }
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT, std::to_string(finish_height));
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME, std::to_string(latest_time));
    xdbg("[xtable_cross_chain_txs_collection_contract] update.finish_height: %" PRIu64 ",latest_time: %" PRIu64 ",param_size: %zu", finish_height,current_time,call_param.size());  
}

NS_END3
