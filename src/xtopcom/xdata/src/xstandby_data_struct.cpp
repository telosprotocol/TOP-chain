// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xstandby/xstandby_data_struct.h"

NS_BEG3(top, data, standby)

data::election::xstandby_network_result_t to_standby_network_result(data::standby::xzec_standby_result_t const & zec_standby_result) {
    data::election::xstandby_network_result_t standby_network_result;
    for (auto const & p : zec_standby_result) {
        auto const & node_id = top::get<const common::xnode_id_t>(p);
        auto const & standby_node_info = top::get<xzec_standby_node_info_t>(p);
        data::election::xstandby_node_info_t old_standby_node_info;
        for (auto const & stake : standby_node_info.stake_container) {
            old_standby_node_info.stake_container.insert(stake);
        }
        old_standby_node_info.program_version = standby_node_info.program_version;
        old_standby_node_info.consensus_public_key = standby_node_info.public_key;
        old_standby_node_info.is_genesis_node = standby_node_info.is_genesis_node;

        // todo mainnet activated.
        // if (m_mainnet_activated || standby_node_info.is_genesis_node) {
        for (auto const & stake : standby_node_info.stake_container) {
            auto const & node_type = get<common::xnode_type_t const>(stake);
            standby_network_result.result_of(node_type).insert(std::make_pair(node_id, old_standby_node_info));
        }
        // }
    }
    return standby_network_result;
}
NS_END3