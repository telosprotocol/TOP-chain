// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xip.h"
#include "xdata/xelection/xelection_association_result.h"

#include <map>

NS_BEG3(top, data, election)

class xtop_election_association_result_store final
{
private:
    using container_t = std::map<common::xcluster_id_t, xelection_association_result_t>;
    container_t m_results{};

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

    std::pair<iterator, bool>
    insert(value_type const & value);

    std::pair<iterator, bool>
    insert(value_type && value);

    iterator
    erase(const_iterator pos);

    std::size_t
    erase(key_type const & key);

    std::map<common::xcluster_id_t, xelection_association_result_t> const &
    results() const noexcept;

    void
    results(std::map<common::xcluster_id_t, xelection_association_result_t> && r) noexcept;

    xelection_association_result_t const &
    result_of(common::xcluster_id_t const & cid) const;

    xelection_association_result_t &
    result_of(common::xcluster_id_t const & cid);

    std::size_t
    size() const noexcept;

    bool
    empty() const noexcept;
};
using xelection_association_result_store_t = xtop_election_association_result_store;

NS_END3
