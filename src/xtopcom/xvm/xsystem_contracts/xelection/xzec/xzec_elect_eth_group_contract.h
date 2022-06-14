// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xlogic_time.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"

NS_BEG4(top, xvm, system_contracts, zec)

class xtop_zec_elect_eth_contract final : public xelect_consensus_group_contract_t {
    using xbase_t = xelect_consensus_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_zec_elect_eth_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_zec_elect_eth_contract);

    explicit xtop_zec_elect_eth_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override {
        return new xtop_zec_elect_eth_contract(network_id());
    }

    void setup();

    void on_timer(common::xlogic_time_t const current_time);
#ifdef STATIC_CONSENSUS
    void elect_config_nodes(common::xlogic_time_t const current_time);
#endif
    BEGIN_CONTRACT_WITH_PARAM(xtop_zec_elect_eth_contract)
    CONTRACT_FUNCTION_PARAM(xtop_zec_elect_eth_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
    void elect(common::xzone_id_t const zone_id, common::xcluster_id_t const cluster_id, std::uint64_t const random_seed, common::xlogic_time_t const election_timestamp);

    bool elect_eth_consensus(common::xzone_id_t const zone_id,
                             common::xcluster_id_t const cluster_id,
                             common::xgroup_id_t const auditor_group_id,
                             common::xgroup_id_t const validator_group_id,
                             common::xlogic_time_t const election_timestamp,
                             common::xlogic_time_t const start_time,
                             std::uint64_t const random_seed,
                             data::election::xstandby_network_result_t const & standby_network_result,
                             data::election::xelection_network_result_t & election_network_result);
};
using xzec_elect_eth_contract_t = xtop_zec_elect_eth_contract;

NS_END4
