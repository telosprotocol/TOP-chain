// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvm/xsystem_contracts/xelection/xelect_consensus_group_contract.h"

#include "xcodec/xmsgpack_codec.hpp"
#include "xcommon/xfts.h"
#include "xconfig/xconfig_register.h"
#include "xdata/xcodec/xmsgpack/xelection_network_result_codec.hpp"
#include "xdata/xcodec/xmsgpack/xstandby_result_store_codec.hpp"
#include "xdata/xgenesis_data.h"
#include "xconfig/xpredefined_configurations.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/xerror/xvm_error.h"

#include <cassert>
#include <cinttypes>

using top::data::election::xelection_group_result_t;
using top::data::election::xelection_info_bundle_t;
using top::data::election::xelection_info_t;
using top::data::election::xelection_network_result_t;
using top::data::election::xstandby_network_result_t;
using top::data::election::xstandby_node_info_t;
using top::data::election::xstandby_result_t;

#ifndef XSYSCONTRACT_MODULE
#    define XSYSCONTRACT_MODULE "sysContract_"
#endif
#define XCONTRACT_PREFIX "ConsensusElection_"
#define XCONSENSUS_ELECTION XSYSCONTRACT_MODULE XCONTRACT_PREFIX

NS_BEG3(top, xvm, system_contracts)

const uint64_t minimum_comprehensive_stake = 1;
const uint64_t basic_comprehensive_stake = 100000000;

static std::size_t calculate_rotation_count(xelection_group_result_t const & group, std::size_t const default_upper_limit) {
    assert(default_upper_limit != 0);

    if (group.empty()) {
        return default_upper_limit;
    }
    auto const percent_num = XGET_ONCHAIN_GOVERNANCE_PARAMETER(election_rotation_count_ratio);

    auto result = percent_num * group.size() / 100;
    if (result <= 1) {
        result = 1;
    }

    return result;
}

xtop_election_awared_data::xtop_election_awared_data(common::xnode_id_t const & account,
                                                     uint64_t const stake,
                                                     uint64_t const comprehensive_stake,
                                                     xpublic_key_t const & public_key)
  : m_account{ account }, m_stake{stake}, m_comprehensive_stake{comprehensive_stake}, m_public_key{public_key} {
}

xtop_election_awared_data::xtop_election_awared_data(common::xnode_id_t const & account,
                                                     uint64_t const stake,
                                                     xpublic_key_t const & public_key)
  : xtop_election_awared_data{account, stake, 0, public_key} {
}

bool xtop_election_awared_data::operator<(xtop_election_awared_data const & other) const noexcept {
    if (m_comprehensive_stake != other.m_comprehensive_stake) {
        return m_comprehensive_stake < other.m_comprehensive_stake;
    }

    if (m_stake != other.m_stake) {
        return m_stake < other.m_stake;
    }

    return m_account < other.m_account;
}

bool xtop_election_awared_data::operator==(xtop_election_awared_data const & other) const noexcept {
    return m_account == other.m_account                         &&
           m_stake == other.m_stake                             &&
           m_comprehensive_stake == other.m_comprehensive_stake &&
           m_public_key == other.m_public_key;
}

bool xtop_election_awared_data::operator>(xtop_election_awared_data const & other) const noexcept {
    return other < *this;
}

common::xnode_id_t const & xtop_election_awared_data::account() const noexcept {
    return m_account;
}

uint64_t xtop_election_awared_data::stake() const noexcept {
    return m_stake;
}

uint64_t xtop_election_awared_data::comprehensive_stake() const noexcept {
    return m_comprehensive_stake;
}

xpublic_key_t const & xtop_election_awared_data::public_key() const noexcept {
    return m_public_key;
}

void xtop_election_awared_data::comprehensive_stake(uint64_t const s) noexcept {
    m_comprehensive_stake = s;
}

xtop_elect_consensus_group_contract::xtop_elect_consensus_group_contract(common::xnetwork_id_t const & network_id) : xbase_t{network_id} {}

static uint64_t calc_comprehensive_stake(int i) {
    auto const auditor_nodes_per_segment = XGET_ONCHAIN_GOVERNANCE_PARAMETER(auditor_nodes_per_segment);
    uint64_t stake = basic_comprehensive_stake;

    const auto seg = static_cast<uint32_t>(i) / auditor_nodes_per_segment;
    for (uint32_t j = 0; j < seg; j++) {
        stake = stake * 9 / 10;
    }
    return std::max(stake, minimum_comprehensive_stake);  // comprehensive_stake has minimum value 1.
}

static void normalize_stake(common::xminer_type_t const role, std::vector<xelection_awared_data_t> & input) {
    auto & result = input;
    switch (role) {
    case common::xminer_type_t::advance: {
        std::sort(std::begin(result), std::end(result), [](xelection_awared_data_t const & lhs, xelection_awared_data_t const & rhs) { return lhs > rhs; });
        for (auto i = 0u; i < result.size(); ++i) {
            if (result[i].stake() > 0) {  // special condition check for genesis nodes.
                result[i].comprehensive_stake(calc_comprehensive_stake(i));
            } else {
                assert(result[i].stake() == 0);
                result[i].comprehensive_stake(minimum_comprehensive_stake);
            }
        }

        std::sort(std::begin(result), std::end(result), [](xelection_awared_data_t const & lhs, xelection_awared_data_t const & rhs) { return lhs < rhs; });
        break;
    }

    case common::xminer_type_t::validator: {
        for (auto & standby_node : result) {
            standby_node.comprehensive_stake(std::max(standby_node.stake(), minimum_comprehensive_stake));
        }

        std::sort(std::begin(result), std::end(result), [](xelection_awared_data_t const & lhs, xelection_awared_data_t const & rhs) { return lhs < rhs; });
        break;
    }

    default: {
        assert(false);
        break;
    }
    }
}

bool xtop_elect_consensus_group_contract::elect_group(common::xzone_id_t const & zid,
                                                      common::xcluster_id_t const & cid,
                                                      common::xgroup_id_t const & gid,
                                                      common::xlogic_time_t const election_timestamp,
                                                      common::xlogic_time_t const start_time,
                                                      std::uint64_t const random_seed,
                                                      xrange_t<config::xgroup_size_t> const & group_size_range,
                                                      data::election::xstandby_network_result_t const & standby_network_result,
                                                      data::election::xelection_network_result_t & election_network_result) {
    assert(!broadcast(zid));
    assert(!broadcast(cid));
    assert(!broadcast(gid));

    assert(zid == common::xcommittee_zone_id || zid == common::xzec_zone_id || zid == common::xconsensus_zone_id);

    auto const log_prefix = "[elect consensus group contract] zone " + zid.to_string() + " cluster " + cid.to_string() + " group " + gid.to_string() + ":";

    auto const max_group_size = group_size_range.end;

    common::xnode_type_t node_type{};
    common::xminer_type_t role_type{};

    switch (common::node_type_from(zid)) {
    case common::xnode_type_t::committee: {
        node_type = common::xnode_type_t::committee;
        role_type = common::xminer_type_t::advance;
        break;
    }

    case common::xnode_type_t::zec: {
        node_type = common::xnode_type_t::zec;
        role_type = common::xminer_type_t::advance;
        break;
    }

    case common::xnode_type_t::consensus: {
        if (gid < common::xauditor_group_id_end) {
            node_type = common::xnode_type_t::consensus_auditor;
            role_type = common::xminer_type_t::advance;
        } else {
            node_type = common::xnode_type_t::consensus_validator;
            role_type = common::xminer_type_t::validator;
        }
        break;
    }

    default: {
        xwarn("%s invalid parameter", log_prefix.c_str());
        assert(false);
        return false;
    }
    }

    assert(node_type != common::xnode_type_t::invalid && role_type != common::xminer_type_t::invalid);

    try {
        xwarn("%s starts electing", log_prefix.c_str());

        // current group nodes before election starts
        auto & current_group_nodes = election_network_result.result_of(node_type).result_of(cid).result_of(gid);

        auto next_version = current_group_nodes.group_version();
        if (!next_version.empty()) {
            next_version.increase();
        } else {
            next_version = common::xelection_round_t{0};
        }
        assert(!next_version.empty());

        // update new version/timestamp
        // if election failed (return false), property serialization won't be executed, thus nothing changed.
        current_group_nodes.group_version(next_version);
        current_group_nodes.election_committee_version(common::xelection_round_t{0});
        current_group_nodes.timestamp(election_timestamp);
        current_group_nodes.start_time(start_time);

        // read standby nodes.
        auto const & standby_result = standby_network_result.result_of(node_type);  // standbys in the pool
        if (standby_result.empty()) {
            xwarn("%s starts electing, but no standby nodes available", log_prefix.c_str());
            return false;
        }

        if (current_group_nodes.size() > max_group_size) {
            return do_shrink_election(zid, cid, gid, node_type, random_seed, current_group_nodes.size() - max_group_size, standby_result, current_group_nodes);
        } else {
            return do_normal_election(zid, cid, gid, node_type, role_type, random_seed, group_size_range, standby_result, current_group_nodes);
        }
    } catch (top::error::xtop_error_t const & eh) {
        xerror("%s xtop_error_t exception caught. category: %s msg: %s", log_prefix.c_str(), eh.code().category().name(), eh.what());
        throw;
    } catch (std::exception const & eh) {
        xerror("%s std::exception exception caught: %s", log_prefix.c_str(), eh.what());
        throw;
    }

    return true;
}

void xtop_elect_consensus_group_contract::handle_elected_out_data(std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> const & chosen_out,
                                                                  common::xzone_id_t const & zid,
                                                                  common::xcluster_id_t const & cid,
                                                                  common::xgroup_id_t const & gid,
                                                                  common::xnode_type_t const node_type,
                                                                  xelection_group_result_t & election_group_result) const {
    auto const log_prefix = "[elect consensus group contract] zone " + zid.to_string() + " cluster " + cid.to_string() + " group " + gid.to_string() + ":";

    xwarn("%s sees %zu nodes elected out", log_prefix.c_str(), chosen_out.size());

    for (auto const & out : chosen_out) {
        XMETRICS_PACKET_INFO(XCONSENSUS_ELECTION "elect_out",
                             "zone_id",
                             zid.to_string(),
                             "cluster_id",
                             cid.to_string(),
                             "group_id",
                             gid.to_string(),
                             "node_id",
                             top::get<common::xnode_id_t>(out).value(),
                             "stake",
                             static_cast<int64_t>(top::get<common::xstake_t>(out)));

        auto elect_out_pos = std::find_if(
            std::begin(election_group_result), std::end(election_group_result), [&out](std::pair<common::xslot_id_t const, xelection_info_bundle_t> const & node_info) {
                return top::get<common::xnode_id_t>(out) == top::get<xelection_info_bundle_t>(node_info).node_id();
            });

        if (elect_out_pos != std::end(election_group_result)) {
            election_group_result.reset(elect_out_pos);
        }
    }
}

void xtop_elect_consensus_group_contract::handle_elected_in_data(std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> const & chosen_in,
                                                                 std::vector<xelection_awared_data_t> const & effective_standby_data,
                                                                 common::xzone_id_t const & zid,
                                                                 common::xcluster_id_t const & cid,
                                                                 common::xgroup_id_t const & gid,
                                                                 common::xnode_type_t const node_type,
                                                                 xelection_group_result_t & election_group_result) const {
    auto const log_prefix = "[elect consensus group contract] zone " + zid.to_string() + " cluster " + cid.to_string() + " group " + gid.to_string() + ":";

    xwarn("%s sees %zu nodes elected in", log_prefix.c_str(), chosen_in.size());

    for (auto const & in : chosen_in) {
        auto const & node_id = top::get<common::xnode_id_t>(in);

        auto const elect_in_pos = std::find_if(std::begin(effective_standby_data), std::end(effective_standby_data), [&node_id](xelection_awared_data_t const & other) {
            return other.account() == node_id;
        });
        assert(elect_in_pos != std::end(effective_standby_data));

        xwarn("%s see elected in %s node %s with stake %" PRIu64 " comprehensive stake %" PRIu64,
              log_prefix.c_str(),
              common::to_string(node_type).c_str(),
              node_id.value().c_str(),
              elect_in_pos->stake(),
              elect_in_pos->comprehensive_stake());

        auto const find_result = election_group_result.find(node_id);
        if (top::get<bool>(find_result)) {
            xwarn("%s already has member %s", log_prefix.c_str(), node_id.to_string().c_str());
            continue;
        }

        xelection_info_t new_election_info{};
        new_election_info.joined_version = election_group_result.group_version();
        new_election_info.stake = elect_in_pos->stake();
        new_election_info.comprehensive_stake = elect_in_pos->comprehensive_stake();
        new_election_info.consensus_public_key = elect_in_pos->public_key();

        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(node_id);
        election_info_bundle.election_info(std::move(new_election_info));

        election_group_result.insert(std::move(election_info_bundle));

        XMETRICS_PACKET_INFO(XCONSENSUS_ELECTION "elect_in",
                             "zone_id",
                             zid.to_string(),
                             "cluster_id",
                             cid.to_string(),
                             "group_id",
                             gid.to_string(),
                             "node_id",
                             node_id.value(),
                             "group_version",
                             election_group_result.group_version().to_string());
    }
}

bool xtop_elect_consensus_group_contract::do_normal_election(common::xzone_id_t const & zid,
                                                             common::xcluster_id_t const & cid,
                                                             common::xgroup_id_t const & gid,
                                                             common::xnode_type_t const node_type,
                                                             common::xminer_type_t const role_type,
                                                              std::uint64_t const random_seed,
                                                             xrange_t<config::xgroup_size_t> const & group_size_range,
                                                             data::election::xstandby_result_t const & standby_result,
                                                             data::election::xelection_group_result_t & current_group_nodes) {
    auto const log_prefix = "[elect consensus group contract - normal] zone " + zid.to_string() + " cluster " + cid.to_string() + " group " + gid.to_string() + ":";
    auto const min_group_size = group_size_range.begin;
    auto const max_group_size = group_size_range.end;

    std::vector<xelection_awared_data_t> effective_standby_result;
    effective_standby_result.reserve(standby_result.size());
    for (auto const & standby_info : standby_result) {
        effective_standby_result.emplace_back(top::get<common::xnode_id_t const>(standby_info),
                                              top::get<xstandby_node_info_t>(standby_info).stake(node_type),
                                              minimum_comprehensive_stake,
                                              top::get<xstandby_node_info_t>(standby_info).consensus_public_key);
    }

    normalize_stake(role_type, effective_standby_result);

    // preparing the fts selection. rule:
    // when electing in, the higher the stake is, the higher the possibility is.
    // when electing out, the lower the stake is, the higher the possibility is.

    // filter the standbys by the current group nodes.
    std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> fts_standbys;
    for (auto it = std::begin(effective_standby_result); it != std::end(effective_standby_result);) {
        auto const & standby_node_id = it->account();
        auto const comprehensive_stake = it->comprehensive_stake();

        if (top::get<bool>(current_group_nodes.find(standby_node_id))) {
            // update the corresponding node in the group
            auto & node_election_info = current_group_nodes.result_of(standby_node_id);
            node_election_info.comprehensive_stake = comprehensive_stake;
            node_election_info.stake = it->stake();
            node_election_info.consensus_public_key = it->public_key();

            it = effective_standby_result.erase(it);
        } else {
            fts_standbys.push_back({static_cast<common::xstake_t>(comprehensive_stake), standby_node_id});
            ++it;
        }
    }

#if defined DEBUG
    for (auto const & standby : fts_standbys) {
        xdbg("%s fts node id %s stake %" PRIu64, log_prefix.c_str(), top::get<common::xnode_id_t>(standby).value().c_str(), top::get<common::xstake_t>(standby));
    }
#endif
    if (fts_standbys.empty()) {
        xwarn("%s fts standby empty", log_prefix.c_str());
        return false;
    }

    auto const rotation_count = calculate_rotation_count(current_group_nodes, min_group_size);
    auto const fts_standbys_size = fts_standbys.size();
    xwarn("%s expected rotation count %zu", log_prefix.c_str(), rotation_count);

    // filter the nodes by the stake value.
    std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> fts_current_nodes;
    std::size_t unqualified_node_count{0};
    std::size_t const rotation_count_upper_limit = std::min(rotation_count, fts_standbys_size);
    // make a copy, all later updates are on this copy and if election success, use this copy as the final result.
    // if there isn't any special reason, current_group_nodes shouldn't be used until it is overwritten by the copied object.
    auto result_nodes = current_group_nodes;

    // calculate:
    //  1. how many nodes should be kicked out since they are not in the standby pool
    //  2. nodes in the group that should participant in FTS (elect out) process
    for (auto & node_info : result_nodes) {
        auto & election_info_bundle = top::get<xelection_info_bundle_t>(node_info);

        auto const & node_id = election_info_bundle.node_id();

        auto const it = standby_result.find(node_id);
        if (it == std::end(standby_result)) {
            // this node isn't found in the standby pool. should be elected out immediately.

            if (unqualified_node_count >= rotation_count_upper_limit) {
                // but if we reach the rotation limitation, keep this node.

                fts_current_nodes.push_back({static_cast<common::xstake_t>(minimum_comprehensive_stake), node_id});

                // keep this node but reset fields.
                // the only valid operations are: clear the stake & comprehensive stake, but not the public key.
                election_info_bundle.election_info().stake = 0;
                election_info_bundle.election_info().comprehensive_stake = minimum_comprehensive_stake;

                xdbg("%s observes node %s with stake & comprehensive stake 1 (since it's not in the standby pool).", log_prefix.c_str(), node_id.value().c_str());

                continue;
            }

            xdbg("%s elects out (reset) node %s", log_prefix.c_str(), node_id.value().c_str());

            result_nodes.reset(node_id);
            ++unqualified_node_count;
        } else {
            fts_current_nodes.push_back({static_cast<common::xstake_t>(election_info_bundle.election_info().comprehensive_stake), node_id});

            xdbg("%s observes node %s with stake %" PRIu64 " comprehensive stake %" PRIu64,
                 log_prefix.c_str(),
                 node_id.value().c_str(),
                 election_info_bundle.election_info().stake,
                 election_info_bundle.election_info().comprehensive_stake);
        }
    }
    assert(fts_current_nodes.size() == result_nodes.size());
    assert(unqualified_node_count <= rotation_count_upper_limit);

    auto const current_fts_group_size = fts_current_nodes.size();
    xwarn("%s current group node size for fts election out %zu", log_prefix.c_str(), current_fts_group_size);

    // seems can not happened anymore.
    xdbg("%s min_group_size %zu current group size %zu", log_prefix.c_str(), min_group_size, current_fts_group_size);
    if (current_fts_group_size + fts_standbys_size < min_group_size) {
        xwarn("%s standby pool size doesn't meet the mininal requirement. size = %zu (current group size) + %zu (standby size) < %" PRIu16,
              log_prefix.c_str(),
              current_fts_group_size,
              fts_standbys_size,
              min_group_size);
        return false;
    }

    // calc the in & out count
    auto const origin_group_size = current_group_nodes.size();
    std::size_t elect_out_count{0}, elect_in_count{0};
    if (origin_group_size < max_group_size) {
        // if the group size doesn't reach its upper limit,
        // there are two cases.
        // first, the fts_standbys >= 20% of current_group size ELECT IN ONLY.
        // second, the fts_standbys <20% of current_group size ELECT IN & OUT (for consensus nodes only).
        // the rotation size is determined by (group size, upper limit size, rotation count)
        if (origin_group_size < min_group_size) {
            elect_in_count = min_group_size - origin_group_size;
        } else {
            elect_in_count = std::min({rotation_count, fts_standbys_size, static_cast<size_t>(max_group_size) - result_nodes.size()});
            if (node_type == common::xnode_type_t::consensus_auditor || node_type == common::xnode_type_t::consensus_validator) {
                const size_t election_minimum_rotation_percent = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cluster_election_minimum_rotation_ratio);
                if (fts_standbys_size <= (origin_group_size * election_minimum_rotation_percent + 99) / 100) {
                    elect_out_count = elect_in_count - unqualified_node_count;
                }
            }
        }

        xwarn("%s actual rotation count %zu", log_prefix.c_str(), elect_in_count);
    } else {
        elect_in_count = rotation_count_upper_limit;
        elect_out_count = elect_in_count - unqualified_node_count;
    }
    assert(fts_standbys_size >= elect_in_count);

    if (elect_out_count > 0) {
        // reverse the current nodes stake.
        std::sort(std::begin(fts_current_nodes),
                  std::end(fts_current_nodes),
                  [](common::xfts_merkle_tree_t<common::xnode_id_t>::value_type const & lhs, common::xfts_merkle_tree_t<common::xnode_id_t>::value_type const & rhs) {
                      auto const lhs_stake = top::get<common::xstake_t>(lhs);
                      auto const rhs_stake = top::get<common::xstake_t>(rhs);
                      if (lhs_stake != rhs_stake) {
                          return lhs_stake > rhs_stake;
                      }
                      return top::get<common::xnode_id_t>(lhs) > top::get<common::xnode_id_t>(rhs);
                  });
        auto const top_stake = top::get<common::xstake_t>(fts_current_nodes.front());
        std::for_each(std::begin(fts_current_nodes), std::end(fts_current_nodes), [&fts_current_nodes, top_stake](common::xfts_merkle_tree_t<common::xnode_id_t>::value_type & v) {
            auto const comprehensive_stake = top::get<common::xstake_t>(v);
            xdbg("watch_fts_start node:%s stake:%" PRIu64, top::get<common::xnode_id_t>(v).c_str(), comprehensive_stake);
            assert(comprehensive_stake > 0);
            top::get<common::xstake_t>(v) = top_stake * basic_comprehensive_stake / comprehensive_stake;
        });
        auto const chosen_out = common::select<common::xnode_id_t>(fts_current_nodes, random_seed + static_cast<std::uint64_t>(gid.value()), elect_out_count);
        handle_elected_out_data(chosen_out, zid, cid, gid, node_type, result_nodes);
    }

    assert(elect_in_count > 0);
    std::sort(std::begin(fts_standbys),
              std::end(fts_standbys),
              [](common::xfts_merkle_tree_t<common::xnode_id_t>::value_type const & lhs, common::xfts_merkle_tree_t<common::xnode_id_t>::value_type const & rhs) {
                  auto const lhs_stake = top::get<common::xstake_t>(lhs);
                  auto const rhs_stake = top::get<common::xstake_t>(rhs);
                  if (lhs_stake != rhs_stake) {
                      return lhs_stake < rhs_stake;
                  }
                  return top::get<common::xnode_id_t>(lhs) < top::get<common::xnode_id_t>(rhs);
              });
    auto const chosen_in = common::select<common::xnode_id_t>(fts_standbys, random_seed + static_cast<std::uint64_t>(gid.value()), elect_in_count);
    handle_elected_in_data(chosen_in, effective_standby_result, zid, cid, gid, node_type, result_nodes);

    assert(!result_nodes.empty());
#if defined DEBUG
    // for (auto const & node_info : result_nodes) {
    //     assert(!top::get<xelection_info_bundle_t>(node_info).empty());
    // }

    for (auto const & node_info : result_nodes) {
        xdbg("%s election finished seeing %s", log_prefix.c_str(), top::get<xelection_info_bundle_t>(node_info).node_id().value().c_str());
    }
#endif
    current_group_nodes = result_nodes;
    return true;
}

bool xtop_elect_consensus_group_contract::do_shrink_election(common::xzone_id_t const & zid,
                                                             common::xcluster_id_t const & cid,
                                                             common::xgroup_id_t const & gid,
                                                             common::xnode_type_t const node_type,
                                                             std::uint64_t const random_seed,
                                                             std::size_t shrink_size,
                                                             data::election::xstandby_result_t const & standby_result,
                                                             data::election::xelection_group_result_t & current_group_nodes) const {
    auto const log_prefix = "[elect consensus group contract - shrink] zone " + zid.to_string() + " cluster " + cid.to_string() + " group " + gid.to_string() + ":";
    std::size_t unqualified_node_count{0};
    std::size_t elect_out_count{0};

    auto result_nodes = current_group_nodes;

    std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> fts_current_nodes;
    for (auto & node_info : result_nodes) {
        auto & election_info_bundle = top::get<xelection_info_bundle_t>(node_info);
        auto const & node_id = election_info_bundle.node_id();
        auto const it = standby_result.find(node_id);
        if (it == std::end(standby_result)) {
            // this node not found in the standby pool. should be elected out immediately.
            if (unqualified_node_count >= shrink_size) {
                // but if we reach the shrink_size limitation, keep this node.

                fts_current_nodes.push_back({static_cast<common::xstake_t>(minimum_comprehensive_stake), node_id});

                // keep this node but reset fields.
                // the only valid operations are: clear the stake & comprehensive stake, but not the public key.
                election_info_bundle.election_info().stake = 0;
                election_info_bundle.election_info().comprehensive_stake = minimum_comprehensive_stake;
                continue;
            }

            xdbg("%s elects out (reset) node %s", log_prefix.c_str(), node_id.value().c_str());

            result_nodes.reset(node_id);
            ++unqualified_node_count;
        } else {
            fts_current_nodes.push_back({static_cast<common::xstake_t>(std::max(election_info_bundle.election_info().comprehensive_stake, minimum_comprehensive_stake)), node_id});

            xdbg("%s observes node %s with stake %" PRIu64 " comprehensive stake %" PRIu64,
                 log_prefix.c_str(),
                 node_id.value().c_str(),
                 election_info_bundle.election_info().stake,
                 election_info_bundle.election_info().comprehensive_stake);
        }
    }

    assert(fts_current_nodes.size() == result_nodes.size());
    elect_out_count = shrink_size - unqualified_node_count;
    if (elect_out_count > 0) {
        std::sort(std::begin(fts_current_nodes),
                  std::end(fts_current_nodes),
                  [](common::xfts_merkle_tree_t<common::xnode_id_t>::value_type const & lhs, common::xfts_merkle_tree_t<common::xnode_id_t>::value_type const & rhs) {
                      auto const lhs_stake = top::get<common::xstake_t>(lhs);
                      auto const rhs_stake = top::get<common::xstake_t>(rhs);
                      if (lhs_stake != rhs_stake) {
                          return lhs_stake > rhs_stake;
                      }
                      return top::get<common::xnode_id_t>(lhs) > top::get<common::xnode_id_t>(rhs);
                  });
        auto const top_stake = top::get<common::xstake_t>(fts_current_nodes.front());
        std::for_each(std::begin(fts_current_nodes), std::end(fts_current_nodes), [&fts_current_nodes, top_stake](common::xfts_merkle_tree_t<common::xnode_id_t>::value_type & v) {
            auto const comprehensive_stake = top::get<common::xstake_t>(v);
            xdbg("watch_fts_start node:%s stake:%" PRIu64, top::get<common::xnode_id_t>(v).c_str(), comprehensive_stake);
            assert(comprehensive_stake > 0);
            top::get<common::xstake_t>(v) = top_stake * basic_comprehensive_stake / comprehensive_stake;
        });
        auto const chosen_out = common::select<common::xnode_id_t>(fts_current_nodes, random_seed + static_cast<std::uint64_t>(gid.value()), elect_out_count);
        handle_elected_out_data(chosen_out, zid, cid, gid, node_type, result_nodes);
    }

    result_nodes.normalize();
    current_group_nodes = result_nodes;
    return true;
}

NS_END3
