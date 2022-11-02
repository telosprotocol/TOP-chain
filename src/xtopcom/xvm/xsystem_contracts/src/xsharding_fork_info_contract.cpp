// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xfork/xsharding_fork_info_contract.h"

// #include "xchain_fork/xchain_upgrade_center.h"

NS_BEG4(top, xvm, system_contracts, fork)

xtop_sharding_fork_info_contract::xtop_sharding_fork_info_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {
}

void xtop_sharding_fork_info_contract::setup() {
    xinfo("xtop_sharding_fork_info_contract::setup");
    STRING_CREATE(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY);
    std::string fork_info{""};
    STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY, fork_info);
}

void xtop_sharding_fork_info_contract::on_timer(common::xlogic_time_t const current_time) {
    xinfo("xtop_sharding_fork_info_contract::on_timer % " PRIu64 "", current_time);

    // auto const & fork_config = chain_fork::xchain_fork_config_center_t::chain_fork_config();
    // auto if_forked = chain_fork::xchain_fork_config_center_t::is_forked(fork_config.__TODO__SOME_FORK_POINT, current_time);
    // if (if_forked && STRING_GET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY) == "__TODO__LAST_FORK_POINT_STRING") {
    //     STRING_SET(data::XPROPERTY_CONTRACT_TABLE_FORK_INFO_KEY, "__TODO__THIS_FORK_POINT_STRING")
    // }
}

NS_END4