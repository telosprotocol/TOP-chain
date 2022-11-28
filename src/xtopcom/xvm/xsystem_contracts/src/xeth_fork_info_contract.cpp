// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xfork/xeth_fork_info_contract.h"

#include "xchain_fork/xutility.h"

NS_BEG4(top, xvm, system_contracts, fork)

xtop_eth_fork_info_contract::xtop_eth_fork_info_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

void xtop_eth_fork_info_contract::setup() {
    xinfo("xtop_eth_fork_info_contract::setup");
    STRING_CREATE(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY);
    STRING_CREATE(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY);
    std::string fork_info{""};
    std::string fork_index{"0"};
    STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY, fork_info);
    STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY, fork_index);
}

void xtop_eth_fork_info_contract::on_timer(common::xlogic_time_t const trigger_time) {
    auto current_time = TIME();
    xinfo("xtop_eth_fork_info_contract::on_timer % " PRIu64 " triggered at %" PRIu64 "", current_time, trigger_time);

    /// @brief Sample fork code, one and for all.
    /// if (chain_fork::xutility_t::is_forked(fork_points::TEST_FORK, current_time) && STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY) == "") {
    ///     xinfo("xtop_eth_fork_info_contract::do_fork: TEST_FORK");
    ///     STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY, "fork_info");
    /// }

    /// @brief Sample fork code, continues block 
    /// if (chain_fork::xutility_t::is_forked(fork_points::TEST_FORK, current_time) && STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY) == "") {
    ///     xinfo("xtop_eth_fork_info_contract::do_fork: TEST_FORK");
    ///     // let's say we need have no more than 100 * 5's account in one table.
    ///     if (STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY) == "0" ) { STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY, "1"); return; }
    ///     if (STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY) == "1" ) { STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY, "2"); return; }
    ///     if (STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY) == "2" ) { STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY, "3"); return; }
    ///     if (STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY) == "3" ) { STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY, "4"); return; }
    ///     if (STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY) == "4" ) {
    ///         STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INDEX_KEY, "0");
    ///         STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY, "fork_info");
    ///         return;
    ///     }
    ///     assert(false);
    /// }
}

NS_END4