// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xfts.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xstake/xstake_algorithm.h"
#include "xvm/xsystem_contracts/xelection/xelect_group_contract.h"

#include <ratio>

NS_BEG3(top, xvm, system_contracts)

/**
 * @brief Holds node infomations in election algorithm
 */
class xtop_election_awared_data final {
private:
    common::xnode_id_t m_account;
    uint64_t m_stake{};
    uint64_t m_comprehensive_stake{};
    xpublic_key_t m_public_key{};

public:
    xtop_election_awared_data(xtop_election_awared_data const &) = default;
    xtop_election_awared_data & operator=(xtop_election_awared_data const &) = default;
    xtop_election_awared_data(xtop_election_awared_data &&) = default;
    xtop_election_awared_data & operator=(xtop_election_awared_data &&) = default;
    ~xtop_election_awared_data() = default;

    xtop_election_awared_data(common::xnode_id_t const & account,
                              uint64_t const stake,
                              uint64_t const comprehensive_stake,
                              xpublic_key_t const & public_key);

    xtop_election_awared_data(common::xnode_id_t const & account,
                              uint64_t const stake,
                              xpublic_key_t const & public_key);

    bool operator<(xtop_election_awared_data const & other) const noexcept;
    bool operator==(xtop_election_awared_data const & other) const noexcept;
    bool operator>(xtop_election_awared_data const & other) const noexcept;

    /**
     * @brief Get the node id
     * 
     * @return common::xnode_id_t const& 
     */
    common::xnode_id_t const & account() const noexcept;
    
    /**
     * @brief Get stake
     * 
     * @return uint64_t 
     */
    uint64_t stake() const noexcept;

    /**
     * @brief Get comprehensive_stake(Temporary values during the election process)
     * 
     * @return uint64_t 
     */
    uint64_t comprehensive_stake() const noexcept;

    /**
     * @brief Get public key
     * 
     * @return xpublic_key_t const& 
     */
    xpublic_key_t const & public_key() const noexcept;

    /**
     * @brief Set the comprehensive_stake
     * 
     * @param s Value
     */
    void comprehensive_stake(uint64_t const s) noexcept;
};
using xelection_awared_data_t = xtop_election_awared_data;
using xeffective_standby_data_t = xelection_awared_data_t;

class xtop_elect_consensus_group_contract : public xelect_group_contract_t {
    using xbase_t = xelect_group_contract_t;

public:
    XDECLARE_DELETED_COPY_DEFAULTED_MOVE_SEMANTICS(xtop_elect_consensus_group_contract);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_elect_consensus_group_contract);

protected:
    explicit xtop_elect_consensus_group_contract(common::xnetwork_id_t const & network_id);

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
    bool elect_group(common::xzone_id_t const & zid,
                     common::xcluster_id_t const & cid,
                     common::xgroup_id_t const & gid,
                     common::xlogic_time_t const election_timestamp,
                     common::xlogic_time_t const start_time,
                     std::uint64_t const random_seed,
                     xrange_t<config::xgroup_size_t> const & group_size_range,
                     data::election::xstandby_network_result_t const & standby_network_result,
                     data::election::xelection_network_result_t & election_network_result) override;

    /**
     * @brief Delete the chosen node out of election group result.
     */
    void handle_elected_out_data(std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> const & chosen_out,
                                 common::xzone_id_t const & zid,
                                 common::xcluster_id_t const & cid,
                                 common::xgroup_id_t const & gid,
                                 common::xnode_type_t const node_type,
                                 data::election::xelection_group_result_t & election_group_result) const;

    /**
     * @brief Insert the chosen node into election group result
     */
    void handle_elected_in_data(std::vector<common::xfts_merkle_tree_t<common::xnode_id_t>::value_type> const & chosen_in,
                                std::vector<xelection_awared_data_t> const & effective_registration_data,
                                common::xzone_id_t const & zid,
                                common::xcluster_id_t const & cid,
                                common::xgroup_id_t const & gid,
                                common::xnode_type_t const node_type,
                                data::election::xelection_group_result_t & election_group_result) const;

    /**
     * @brief Execute normal election.
     *
     * @param zid Zone id
     * @param cid Cluster id
     * @param gid Group id
     * @param node_type Group type
     * @param role_type Election role type
     * @param random_seed Random seed for FTS algorithm
     * @param group_size_range Group size range
     * @param standby_result Standby data for node_type
     * @param current_group_nodes Input/Output param. Stores the fianl election data
     */
    bool do_normal_election(common::xzone_id_t const & zid,
                            common::xcluster_id_t const & cid,
                            common::xgroup_id_t const & gid,
                            common::xnode_type_t const node_type,
                            common::xrole_type_t const role_type,
                            std::uint64_t const random_seed,
                            xrange_t<config::xgroup_size_t> const & group_size_range,
                            data::election::xstandby_result_t const & standby_result,
                            data::election::xelection_group_result_t & current_group_nodes);

    /**
     * @brief if current_group_size > max_group_size this round election will shrink to max size.
     *
     * @param zid Zone id
     * @param cid Cluster id
     * @param gid Group id
     * @param node_type
     * @param random_seed Random seed for FTS algorithm internally used by election process
     * @param shrink_size current_group_size minus max_group_size
     * @param standby_result Standby pool
     * @param current_group_nodes Election result
     * @return true shrink success which means elect success
     * @return false
     */
    bool do_shrink_election(common::xzone_id_t const & zid,
                            common::xcluster_id_t const & cid,
                            common::xgroup_id_t const & gid,
                            common::xnode_type_t const node_type,
                            std::uint64_t const random_seed,
                            std::size_t shrink_size,
                            data::election::xstandby_result_t const & standby_result,
                            data::election::xelection_group_result_t & current_group_nodes) const;
};
using xelect_consensus_group_contract_t = xtop_elect_consensus_group_contract;

NS_END3
