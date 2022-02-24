// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xelection/xelection_association_result_store.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_result_store.h"
#include "xvm/xcontract/xcontract_exec.h"
#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"

NS_BEG4(top, xvm, system_contracts, zec)

class xtop_zec_elect_consensus_group_contract final : public xelect_consensus_group_contract_t {
    using xbase_t = xelect_consensus_group_contract_t;

    common::xelection_round_t m_zec_round_version{0};
    mutable bool m_update_registration_contract_read_status{ false };

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_zec_elect_consensus_group_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_zec_elect_consensus_group_contract);

    explicit xtop_zec_elect_consensus_group_contract(common::xnetwork_id_t const & network_id);

    xcontract_base * clone() override { return new xtop_zec_elect_consensus_group_contract(this->m_network_id); }

    void setup() override;

    void on_timer(common::xlogic_time_t const current_time);

    BEGIN_CONTRACT_WITH_PARAM(xtop_zec_elect_consensus_group_contract)
    CONTRACT_FUNCTION_PARAM(xtop_zec_elect_consensus_group_contract, on_timer);
    END_CONTRACT_WITH_PARAM

private:
#ifdef STATIC_CONSENSUS
    void swap_election_result(common::xlogic_time_t const current_time);
    void elect_config_nodes(common::xlogic_time_t const current_time);
#endif

    void elect(common::xzone_id_t const zone_id, common::xcluster_id_t const cluster_id, std::uint64_t const random_seed, common::xlogic_time_t const election_timestamp);

    bool elect_auditor_validator(common::xzone_id_t const & zone_id,
                                 common::xcluster_id_t const & cluster_id,
                                 common::xgroup_id_t const & auditor_group_id,
                                 std::uint64_t const random_seed,
                                 common::xlogic_time_t const election_timestamp,
                                 common::xlogic_time_t const start_time,
                                 data::election::xelection_association_result_store_t const & association_result_store,
                                 data::election::xstandby_network_result_t const & standby_network_result,
                                 std::unordered_map<common::xgroup_id_t, data::election::xelection_result_store_t> & all_cluster_election_result_store);

    bool elect_auditor(common::xzone_id_t const & zid,
                       common::xcluster_id_t const & cid,
                       common::xgroup_id_t const & gid,
                       common::xlogic_time_t const election_timestamp,
                       common::xlogic_time_t const start_time,
                       std::uint64_t const random_seed,
                       data::election::xstandby_network_result_t const & standby_network_result,
                       data::election::xelection_network_result_t & election_network_result);

    bool elect_validator(common::xzone_id_t const & zid,
                         common::xcluster_id_t const & cid,
                         common::xgroup_id_t const & auditor_gid,
                         common::xgroup_id_t const & validator_gid,
                         common::xlogic_time_t const election_timestamp,
                         common::xlogic_time_t const start_time,
                         std::uint64_t const random_seed,
                         data::election::xstandby_network_result_t const & standby_network_result,
                         data::election::xelection_network_result_t & election_network_result);

    bool genesis_elected() const;
};
using xzec_elect_consensus_group_contract_t = xtop_zec_elect_consensus_group_contract;

NS_END4
