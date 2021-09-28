// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xstandby/xstandby_data_struct.h"

NS_BEG3(top, data, standby)

// data::election::xstandby_network_result_t to_standby_network_result(data::standby::xzec_standby_result_t const & zec_standby_result) {
//     data::election::xstandby_network_result_t standby_network_result;
//     for (auto const & p : zec_standby_result) {
//         auto const & node_id = top::get<const common::xnode_id_t>(p);
//         auto const & standby_node_info = top::get<xzec_standby_node_info_t>(p);
//         data::election::xstandby_node_info_t old_standby_node_info;
//         for (auto const & stake : standby_node_info.stake_container) {
//             old_standby_node_info.stake_container.insert(stake);
//         }
//         old_standby_node_info.program_version = standby_node_info.program_version;
//         old_standby_node_info.consensus_public_key = standby_node_info.public_key;
//         old_standby_node_info.is_genesis_node = standby_node_info.is_genesis_node;

//         // todo mainnet activated.
//         // if (m_mainnet_activated || standby_node_info.is_genesis_node) {
//         for (auto const & stake : standby_node_info.stake_container) {
//             auto const & node_type = get<common::xnode_type_t const>(stake);
//             standby_network_result.result_of(node_type).insert(std::make_pair(node_id, old_standby_node_info));
//         }
//         // }
//     }
//     return standby_network_result;
// }

/*
| genesis_nodes_only | is_genesis_node |   result |
|          -         |        -        |     -    |
|        true        |      true       |   true   |
|        true        |      false      |   false  |
|        false       |      true       |   true   |
|        false       |      flase      |   true   |

-> if( (!genesis_nodes_only) || node_info.is_genesis_node ) { selected }

*/
data::standby::xsimple_standby_result_t select_standby_nodes(data::standby::xzec_standby_result_t const & zec_standby_result,
                                                             common::xnode_type_t const & node_type,
                                                             bool genesis_nodes_only) {
    data::standby::xsimple_standby_result_t simple_standby_result;
    for (auto const & p : zec_standby_result) {
        auto const & node_id = top::get<const common::xnode_id_t>(p);
        auto const & zec_standby_node_info = top::get<top::data::standby::xzec_standby_node_info_t>(p);
        if ((zec_standby_node_info.stake_container.find(node_type) != zec_standby_node_info.stake_container.end()) &&
            ((!genesis_nodes_only) || zec_standby_node_info.is_genesis_node)) {
            xsimple_standby_node_info_t simple_node_info;
            simple_node_info.stake = zec_standby_node_info.stake_container.result_of(node_type);
            simple_node_info.is_genesis_node = zec_standby_node_info.is_genesis_node;
            simple_node_info.program_version = zec_standby_node_info.program_version;
            simple_node_info.public_key = zec_standby_node_info.public_key;
            simple_standby_result.insert(std::make_pair(node_id, simple_node_info));
        }
    }

    return simple_standby_result;
}

NS_END3