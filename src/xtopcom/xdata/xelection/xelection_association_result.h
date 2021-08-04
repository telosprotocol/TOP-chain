// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xcommon/xversion.h"

#include <map>

NS_BEG3(top, data, election)

class xtop_election_association_result final
{
private:
    using container_t = std::map<common::xgroup_id_t, common::xgroup_id_t>; // std::set<common::xgroup_id_t>??? should we support multiple associations?
    container_t m_assocation_result{};
    common::xelection_round_t m_cluster_version{};

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

    common::xelection_round_t const &
    cluster_version() const noexcept;

    common::xelection_round_t &
    cluster_version() noexcept;

    void
    cluster_version(common::xelection_round_t && cluster_ver) noexcept;

    std::map<common::xgroup_id_t, common::xgroup_id_t> const &
    results() const noexcept;

    void
    results(std::map<common::xgroup_id_t, common::xgroup_id_t> && r) noexcept;

    void
    increase_cluster_version();

    common::xgroup_id_t const &
    result_of(common::xgroup_id_t const & gid) const noexcept;

    common::xgroup_id_t &
    result_of(common::xgroup_id_t const & gid) noexcept;

    std::pair<iterator, bool>
    insert(value_type && value);

    std::pair<iterator, bool>
    insert(value_type const & value);

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

    std::size_t
    erase(key_type const & key);

    iterator
    erase(const_iterator pos);
};
using xelection_association_result_t = xtop_election_association_result;

NS_END3
