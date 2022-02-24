// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

// only used in static_consensus election
#ifdef STATIC_CONSENSUS
#pragma once
#include "xconfig/xconfig_register.h"
#include "xconfig/xpredefined_configurations.h"
#include "xdata/xcodec/xmsgpack/xelection_result_store_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_node_info_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xelection/xelection_result_property.h"
#include "xdata/xgenesis_data.h"

#include <atomic>
#include <csignal>
#include <iostream>

NS_BEG3(top, xvm, system_contracts)

void set_signal_true(int signal);

struct node_info {
    common::xnode_id_t node_id;
    uint64_t stake;
    top::xpublic_key_t pub_key;
    node_info(std::string _node_id, uint64_t _stake, std::string _pub_key) : node_id{_node_id}, stake{_stake}, pub_key{_pub_key} {
    }
};

struct standby_node_info{
    common::xnode_id_t node_id;
    top::xpublic_key_t pub_key;
    std::vector<std::pair<common::xnode_type_t,uint64_t>> type_stake_pair;

    standby_node_info(std::string _node_id, std::string _pub_key, std::vector<std::pair<common::xnode_type_t,uint64_t>> _type_stake_pair):node_id{_node_id},pub_key{_pub_key},type_stake_pair{_type_stake_pair}{}
};

class xstatic_election_center {
public:
    xstatic_election_center(xstatic_election_center const &) = delete;
    xstatic_election_center & operator=(xstatic_election_center const &) = delete;
    xstatic_election_center(xstatic_election_center &&) = delete;
    xstatic_election_center & operator=(xstatic_election_center &&) = delete;
    static xstatic_election_center & instance() {
        static xstatic_election_center instance;
        return instance;
    }

    // only used by set_signal_true trigged by signal.
    void allow_elect() {
        allow_election.store(true);
    }

    bool if_allow_elect() {
        return allow_election.load();
    }

    std::map<std::string,common::xnode_type_t> node_type_dict = {
       {"rec",common::xnode_type_t::rec}, 
       {"zec",common::xnode_type_t::zec}, 
       {"adv",common::xnode_type_t::consensus_auditor}, 
       {"con",common::xnode_type_t::consensus_validator},
       {"edge",common::xnode_type_t::edge},
       {"archive",common::xnode_type_t::storage_archive},
       {"exchange", common::xnode_type_t::storage_exchange},
       {"fullnode", common::xnode_type_t::fullnode},
    };

    std::vector<standby_node_info> get_standby_config();

    std::vector<node_info> get_static_election_nodes(std::string const & key);

    void calc_static_consensus_election_nodes();
    
    std::vector<node_info> get_static_consensus_election_nodes(uint8_t group_id_value);

private:
    std::map<uint8_t,std::vector<node_info>> consensus_group_map;

    xstatic_election_center() {
        xinfo("[static_consensus][xstatic_election_center] init allow election false");
        std::signal(SIGUSR1, set_signal_true);
        calc_static_consensus_election_nodes();
    }
    std::atomic<bool> allow_election{false};
};

NS_END3
#endif