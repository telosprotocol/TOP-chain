// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xevm/xtable_cross_chain_txs_collection_contract.h"

#include "xbase/xmem.h"
#include "xbasic/xhex.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockextract.h"
#include "xmetrics/xmetrics.h"
// #include "xvm/manager/xcontract_manager.h"

using namespace top::base;
using namespace top::data;
using namespace top::data::system_contract;

NS_BEG3(top, xvm, system_contracts)

#define BATCH_PROCESS_BLOCKS_NUM        (10)
#define BATCH_PROCESS_CLOCK_INTERVAL    (10)   // TODO(jimmy) define para in configurations
#define BATCH_TXS_COUNT_MAX             (5)
#define UPDATE_MIN_BLOCKS_NUM           (6) // should larger than 3, avoiding drive self issue

xtable_cross_chain_txs_collection_contract::xtable_cross_chain_txs_collection_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtable_cross_chain_txs_collection_contract::setup() {
    // initialize map key
    STRING_CREATE(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT);
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT, std::to_string(0));
    STRING_CREATE(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME);
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME, std::to_string(0));
}

void xtable_cross_chain_txs_collection_contract::on_timer(common::xlogic_time_t const current_time) {
    XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().value(), "xtable_cross_chain_txs_collection_contract instance is triggled by " + SOURCE_ADDRESS());
    XCONTRACT_ENSURE(SELF_ADDRESS().value() == sys_contract_eth_table_cross_chain_txs_collection_addr, "xtable_cross_chain_txs_collection_contract instance is not triggled by sys_contract_eth_table_cross_chain_txs_collection_addr");

    auto latest_height = get_blockchain_height(sys_contract_eth_table_block_addr_with_suffix);
    auto latest_time = TIME();
    auto const last_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT)));
    auto const last_time = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME)));

    bool need_process = false;
    if (latest_height >= last_height + BATCH_PROCESS_BLOCKS_NUM) {
        need_process = true;
    } else if (latest_time >= last_time + BATCH_PROCESS_CLOCK_INTERVAL) {
        need_process = true;
    }

    xdbg("xtable_cross_chain_txs_collection_contract::on_timer,need_process:%d,height:%" PRIu64 ",%" PRIu64 ",time:%" PRIu64 ",%" PRIu64, 
        need_process, latest_height,last_height,latest_time,last_time);
    if (!need_process) {
        return;
    }
    std::error_code ec;
    xrelayblock_crosstx_infos_t all_crosstxs;
    uint64_t finish_height = latest_height;  // default read to latest_height
    for (uint64_t i = last_height+1; i <= latest_height; i++) {
        auto _block = get_block_by_height(sys_contract_eth_table_block_addr_with_suffix, i);
        XCONTRACT_ENSURE(nullptr != _block, "xtable_cross_chain_txs_collection_contract::on_timer, block nullptr " + std::to_string(i));

        xrelayblock_crosstx_infos_t crosstxs;
        data::xblockextract_t::unpack_crosschain_txs(_block.get(), crosstxs, ec);
        XCONTRACT_ENSURE(!ec, "xtable_cross_chain_txs_collection_contract::on_timer, unpack crosstxs fail " + ec.message());

        for (auto & v : crosstxs.tx_infos) {
            all_crosstxs.tx_infos.push_back(v);
            xdbg("xtable_cross_chain_txs_collection_contract::on_timer,packtx.height=%" PRIu64 ",tx=%s", i,top::to_hex_prefixed(top::to_bytes(v.tx.get_tx_hash())).c_str());
        }
        
        if (all_crosstxs.tx_infos.size() >= BATCH_TXS_COUNT_MAX) {
            finish_height = i;  // report tx size should not too large
            break;
        }
    }

    if ( (all_crosstxs.tx_infos.size() == 0)
        && (finish_height < last_height + UPDATE_MIN_BLOCKS_NUM)) {
        // ignore for avoiding drive self issue
        xdbg("xtable_cross_chain_txs_collection_contract::on_timer,ignore process.finish_height:%" PRIu64 ",last_height:%" PRIu64, finish_height,last_height);
        return;
    }

    if (all_crosstxs.tx_infos.size() > 0) {
        std::string param_str = all_crosstxs.serialize_to_string();
        xstream_t stream(xcontext_t::instance());
        stream << param_str;
        CALL(common::xaccount_address_t{sys_contract_relay_make_block_addr}, "on_receive_cross_txs", std::string((char *)stream.data(), stream.size()));
        xdbg("xtable_cross_chain_txs_collection_contract::on_timer,called.tx_size=%zu,param_size: %zu", all_crosstxs.tx_infos.size(),param_str.size());  
    }
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_TABLE_BLOCK_HEIGHT, std::to_string(finish_height));
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME, std::to_string(latest_time));    
    xdbg("xtable_cross_chain_txs_collection_contract::on_timer,update.finish_height: %" PRIu64 ",latest_time: %" PRIu64, finish_height,latest_time);
}

NS_END3
