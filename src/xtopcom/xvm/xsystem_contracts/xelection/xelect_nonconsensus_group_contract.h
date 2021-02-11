// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include "xvm/xsystem_contracts/xelection/xelect_group_contract.h"

NS_BEG3(top, xvm, system_contracts)

class xtop_elect_nonconsensus_group_contract : public xelect_group_contract_t {
    using xbase_t = xelect_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_elect_nonconsensus_group_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_elect_nonconsensus_group_contract);

protected:
    explicit xtop_elect_nonconsensus_group_contract(common::xnetwork_id_t const & network_id);

public:
    /**
     * @brief elect consensus group
     * 
     * @param zid Zone id
     * @param cid Cluster id
     * @param gid Group id
     * @param election_timestamp Timestamp that triggers the election
     * @param start_time The time that this election result starts to work
     * @param group_size_range Maximum and minimum values for the group
     * @param standby_network_result Standby pool
     * @param election_network_result Election result
     * @return true election successful
     * @return false election failed
     */
    bool elect_group(common::xzone_id_t const & zid,
                     common::xcluster_id_t const & cid,
                     common::xgroup_id_t const & gid,
                     common::xlogic_time_t const election_timestamp,
                     common::xlogic_time_t const start_time,
                     xrange_t<config::xgroup_size_t> const & group_size_range,
                     data::election::xstandby_network_result_t & standby_network_result,
                     data::election::xelection_network_result_t & election_network_result) override;
};
using xelect_nonconsensus_group_contract_t = xtop_elect_nonconsensus_group_contract;

NS_END3
