// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xrange.hpp"
#include "xcommon/xip.h"
#include "xcommon/xlogic_time.h"
#include "xdata/xelection/xelection_association_result.h"
#include "xdata/xelection/xelection_network_result.h"
#include "xdata/xelection/xelection_result_store.h"
#include "xdata/xelection/xstandby_network_result.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/xcontract/xcontract_base.h"

NS_BEG3(top, xvm, system_contracts)

class xtop_elect_group_contract : public xcontract::xcontract_base {
    using xbase_t = xcontract::xcontract_base;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_elect_group_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_elect_group_contract);

protected:
    explicit xtop_elect_group_contract(common::xnetwork_id_t const & network_id);

    /**
     * @brief elect consensus group
     * 
     * @param zid Zone id
     * @param cid Cluster id
     * @param gid Group id
     * @param election_timestamp Timestamp that triggers the election
     * @param start_time The time that this election result starts to work
     * @param random_seed Random seed for FTS algorithm internally used by election process
     * @param group_size_range Maximum and minimum values for the group
     * @param standby_network_result Standby pool
     * @param election_network_result Election result
     * @return true election successful
     * @return false election failed
     */
    virtual bool elect_group(common::xzone_id_t const & zid,
                             common::xcluster_id_t const & cid,
                             common::xgroup_id_t const & gid,
                             common::xlogic_time_t const election_timestamp,
                             common::xlogic_time_t const start_time,
                             std::uint64_t const random_seed,
                             xrange_t<config::xgroup_size_t> const & group_size_range,
                             data::election::xstandby_network_result_t const & standby_network_result,
                             data::election::xelection_network_result_t & election_network_result);

    /**
     * @brief elect non-consensus group
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
    virtual bool elect_group(common::xzone_id_t const & zid,
                             common::xcluster_id_t const & cid,
                             common::xgroup_id_t const & gid,
                             common::xlogic_time_t const election_timestamp,
                             common::xlogic_time_t const start_time,
                             xrange_t<config::xgroup_size_t> const & group_size_range,
                             data::election::xstandby_network_result_t & standby_network_result,
                             data::election::xelection_network_result_t & election_network_result);
};
using xelect_group_contract_t = xtop_elect_group_contract;

NS_END3
