// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#ifdef STATIC_CONSENSUS
#include "xvm/xsystem_contracts/xelection/xstatic_election_center.h"

NS_BEG3(top, xvm, system_contracts)

void set_signal_true(int signal) {
    std::cout << "[static_consensus][xstatic_election_center]recv signal,allow election" << std::endl;
    xinfo("[static_consensus][xstatic_election_center]recv signal,set allow election true");
    xstatic_election_center::instance().allow_elect();
}

std::vector<standby_node_info> xstatic_election_center::get_standby_config() {
    std::vector<standby_node_info> res;
    auto & config_register = top::config::xconfig_register_t::get_instance();
    std::string key = "standby_start_nodes";
    std::string config_infos;
    config_register.get(key, config_infos);
    xinfo("[xstatic_election_center][get_standby_config] read_key:%s content:%s", key.c_str(), config_infos.c_str());
    std::vector<std::string> node_standby_infos;
    top::base::xstring_utl::split_string(config_infos, ',', node_standby_infos);
    // node_id:pub_key:type.stake|type.stake,node_id:pub_key:type.stake|type.stake,...
    for (auto const & nodes : node_standby_infos) {
        std::vector<std::string> one_node_info;
        top::base::xstring_utl::split_string(nodes, ':', one_node_info);
        std::string node_id_str = one_node_info[0];
        std::string node_pub_key_str = one_node_info[1];
        std::vector<std::string> type_stake_pairs;
        top::base::xstring_utl::split_string(one_node_info[2], '|', type_stake_pairs);
        std::vector<std::pair<common::xnode_type_t, uint64_t>> _type_stake_pair;
        for (auto const & each_pair : type_stake_pairs) {
            std::vector<std::string> type_stake;
            top::base::xstring_utl::split_string(each_pair, '.', type_stake);
            common::xnode_type_t node_type = node_type_dict.at(type_stake[0]);
            uint64_t stake = static_cast<std::uint64_t>(std::atoi(type_stake[1].c_str()));
            _type_stake_pair.push_back(std::make_pair(node_type, stake));
        }
        res.push_back(standby_node_info{node_id_str, node_pub_key_str, _type_stake_pair});
    }
    return res;
}

std::vector<node_info> xstatic_election_center::get_static_election_nodes(std::string const & key) {
    assert(key == "zec_start_nodes" || key == "archive_start_nodes" || key == "edge_start_nodes" || key == "exchange_start_nodes");
    std::vector<node_info> res;

    auto & config_register = top::config::xconfig_register_t::get_instance();
    std::string config_infos;

    config_register.get(key, config_infos);
    xinfo("[xstatic_election_center][get_static_election_nodes] read_key:%s content:%s", key.c_str(), config_infos.c_str());
    std::vector<std::string> nodes_info;
    top::base::xstring_utl::split_string(config_infos, ',', nodes_info);
    for (auto nodes : nodes_info) {
        std::vector<std::string> one_node_info;
        top::base::xstring_utl::split_string(nodes, '.', one_node_info);
        res.push_back(node_info{one_node_info[0], static_cast<std::uint64_t>(std::atoi(one_node_info[1].c_str())), one_node_info[2]});
    }
    return res;
}

void xstatic_election_center::calc_static_consensus_election_nodes() {
    auto & config_register = top::config::xconfig_register_t::get_instance();
    std::string consensus_infos;
    config_register.get(std::string("consensus_start_nodes"), consensus_infos);
    std::vector<std::string> group_string_value;
    top::base::xstring_utl::split_string(consensus_infos, '|', group_string_value);
    assert(group_string_value.size() == XGET_CONFIG(validator_group_count) + XGET_CONFIG(auditor_group_count));

    uint8_t auditor_group_id_start = common::xauditor_group_id_begin.value();
    uint8_t validator_group_id_start = common::xvalidator_group_id_begin.value();
    for (auto const & group_info : group_string_value) {
        std::vector<std::string> type_detail;
        top::base::xstring_utl::split_string(group_info, ':', type_detail);
        auto group_type = type_detail[0];          // auditor or validator
        auto detailed_node_info = type_detail[1];  // multi nodes info.
        std::vector<std::string> nodes_info;
        top::base::xstring_utl::split_string(detailed_node_info, ',', nodes_info);
        std::vector<node_info> res;
        for (auto nodes : nodes_info) {
            std::vector<std::string> one_node_info;
            top::base::xstring_utl::split_string(nodes, '.', one_node_info);
            res.push_back(node_info{one_node_info[0], static_cast<std::uint64_t>(std::atoi(one_node_info[1].c_str())), one_node_info[2]});
        }

        assert(group_type == "auditor" || group_type == "validator");
        uint8_t group_id_value = (group_type == "auditor") ? (auditor_group_id_start++) : (validator_group_id_start++);
        consensus_group_map.insert({group_id_value, res});
    }
}

std::vector<node_info> xstatic_election_center::get_static_consensus_election_nodes(uint8_t group_id_value) {
    assert(group_id_value == 1 || group_id_value == 2 || group_id_value == 64 || group_id_value == 65 || group_id_value == 66 || group_id_value == 67);
    return consensus_group_map.at(group_id_value);
}

NS_END3
#endif