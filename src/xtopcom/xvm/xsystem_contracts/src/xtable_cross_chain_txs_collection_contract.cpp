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
#define BATCH_TXS_COUNT_MAX             (20)
#define UPDATE_MIN_BLOCKS_NUM           (6) // should larger than 3, avoiding drive self issue

xtable_cross_chain_txs_collection_contract::xtable_cross_chain_txs_collection_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

void xtable_cross_chain_txs_collection_contract::setup() {
    // initialize map key
    STRING_CREATE(XPORPERTY_CONTRACT_PROCESSED_TABLE_TX_COUNT);
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_TABLE_TX_COUNT, std::to_string(0));
  /*   STRING_CREATE(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME);
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_LOGIC_TIME, std::to_string(0));*/
}

void xtable_cross_chain_txs_collection_contract::updateCrossTx(std::string const& cross_txs_data) {
    //  XCONTRACT_ENSURE(SOURCE_ADDRESS() == SELF_ADDRESS().to_string(), "xtable_cross_chain_txs_collection_contract instance is triggled by " + SOURCE_ADDRESS());
    XCONTRACT_ENSURE(SELF_ADDRESS().to_string() == sys_contract_eth_table_cross_chain_txs_collection_addr,
        "xtable_cross_chain_txs_collection_contract instance is not triggled by sys_contract_eth_table_cross_chain_txs_collection_addr");

    if (!cross_txs_data.empty()) {
        xstream_t stream(xcontext_t::instance());
        stream << cross_txs_data;
        CALL(common::xaccount_address_t { sys_contract_relay_make_block_addr }, "on_receive_cross_txs", std::string((char*)stream.data(), stream.size()));
        xdbg("xtable_cross_chain_txs_collection_contract::on_timer,stream_size: %zu", stream.size());
    }
    auto const last_height = static_cast<std::uint64_t>(std::stoull(STRING_GET(XPORPERTY_CONTRACT_PROCESSED_TABLE_TX_COUNT)));
    STRING_SET(XPORPERTY_CONTRACT_PROCESSED_TABLE_TX_COUNT, std::to_string(last_height + 1));
}

NS_END3
