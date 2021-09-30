// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xid.hpp"
#include "xcommon/xip.h"
#include "xcommon/xlogic_time.h"
#include "xcommon/xnode_id.h"
#include "xcommon/xversion.h"
#include "xdata/xelection/xelection_info.h"
#include "xdata/xelection/xelection_info_bundle.h"

#include <cstdint>
#include <limits>
#include <map>
#include <string>

NS_BEG3(top, data, election)

class xtop_election_group_result final {
private:
    using container_t = std::map<common::xslot_id_t, xelection_info_bundle_t>;
    container_t m_nodes{};
    common::xlogic_time_t m_timestamp{ 0 };
    common::xlogic_time_t m_start_time{ common::xjudgement_day };
    common::xelection_round_t m_group_version{};
    common::xelection_round_t m_associated_group_version{};
    common::xelection_round_t m_cluster_version{};
    common::xelection_round_t m_election_committee_version{};
    // std::uint64_t m_associated_election_blk_height{ 0 };
    common::xgroup_id_t m_associated_group_id{};

public:
    using key_type        = container_t::key_type;
    using mapped_type     = container_t::mapped_type;
    using value_type      = container_t::value_type;
    using size_type       = container_t::size_type;
    using difference_type = container_t::difference_type;
    using key_compare     = container_t::key_compare;
    using reference       = container_t::reference;
    using const_reference = container_t::const_reference;
    using pointer         = container_t::pointer;
    using const_pointer   = container_t::const_pointer;
    using iterator        = container_t::iterator;
    using const_iterator  = container_t::const_iterator;

    bool
    operator==(xtop_election_group_result const & other) const noexcept;

    bool
    operator!=(xtop_election_group_result const & other) const noexcept;

    common::xgroup_id_t const &
    associated_group_id() const noexcept;

    void
    associated_group_id(common::xgroup_id_t gid) noexcept;

    common::xelection_round_t const &
    group_version() const noexcept;

    common::xelection_round_t &
    group_version() noexcept;

    void
    group_version(common::xelection_round_t ver) noexcept;

    common::xelection_round_t const &
    associated_group_version() const noexcept;

    void
    associated_group_version(common::xelection_round_t associated_gp_ver) noexcept;

    common::xelection_round_t const &
    cluster_version() const noexcept;

    void
    cluster_version(common::xelection_round_t ver) noexcept;

    common::xelection_round_t const &
    election_committee_version() const noexcept;

    common::xelection_round_t &
    election_committee_version() noexcept;

    void
    election_committee_version(common::xelection_round_t zec_ver) noexcept;

    common::xlogic_time_t
    timestamp() const noexcept;

    void
    timestamp(common::xlogic_time_t const time) noexcept;

    common::xlogic_time_t
    start_time() const noexcept;

    common::xlogic_time_t start_time(std::error_code & ec) const noexcept;

    void
    start_time(common::xlogic_time_t const time) noexcept;

    void start_time(common::xlogic_time_t const time, std::error_code & ec) noexcept;

    //std::uint64_t
    //associated_election_blk_height() const noexcept;

    //void
    //associated_election_blk_height(std::uint64_t const height) noexcept;

    std::pair<iterator, bool>
    insert(xelection_info_bundle_t const & value);

    std::pair<iterator, bool>
    insert(xelection_info_bundle_t && value);

    bool
    empty() const noexcept;

    std::map<common::xslot_id_t, xelection_info_bundle_t> const &
    results() const noexcept;

    void
    results(std::map<common::xslot_id_t, xelection_info_bundle_t> && r) noexcept;

    xelection_info_t const &
    result_of(common::xnode_id_t const & node_id) const;

    xelection_info_t &
    result_of(common::xnode_id_t const & node_id);

    std::size_t
    size() const noexcept;

    iterator
    begin() noexcept;

    const_iterator
    begin() const noexcept;

    const_iterator
    cbegin() const noexcept;

    iterator
    end() noexcept;

    const_iterator
    end() const noexcept;

    const_iterator
    cend() const noexcept;

    void
    reset(common::xnode_id_t const & id);

    void
    reset(iterator pos);

    void
    clear(common::xslot_id_t const & slot_id);

    std::pair<common::xslot_id_t, bool>
    find(common::xnode_id_t const & nid) const noexcept;

    void
    normalize() noexcept;

private:
    void
    do_clear(common::xslot_id_t const & slot_id);
};
using xelection_group_result_t = xtop_election_group_result;

NS_END3
