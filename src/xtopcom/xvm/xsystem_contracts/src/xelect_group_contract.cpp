// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xelect_group_contract.h"

#include <cassert>

NS_BEG3(top, xvm, system_contracts)

xtop_elect_group_contract::xtop_elect_group_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

bool xtop_elect_group_contract::elect_group(common::xzone_id_t const &,
                                            common::xcluster_id_t const &,
                                            common::xgroup_id_t const &,
                                            common::xlogic_time_t const,
                                            common::xlogic_time_t const,
                                            std::uint64_t const,
                                            xrange_t<config::xgroup_size_t> const &,
                                            data::election::xstandby_network_result_t const &,
                                            data::election::xelection_network_result_t &) {
    assert(false);
    return false;
}

bool xtop_elect_group_contract::elect_group(common::xzone_id_t const &,
                                            common::xcluster_id_t const &,
                                            common::xgroup_id_t const &,
                                            common::xlogic_time_t const,
                                            common::xlogic_time_t const,
                                            xrange_t<config::xgroup_size_t> const &,
                                            data::election::xstandby_network_result_t &,
                                            data::election::xelection_network_result_t &) {
    assert(false);
    return false;
}

NS_END3
