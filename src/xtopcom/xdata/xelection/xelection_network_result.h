// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_type.h"
#include "xdata/xdata_common.h"
#include "xdata/xelection/xelection_result.h"

#include <cstdint>

NS_BEG3(top, data, election)

class xtop_election_network_result final {
private:
    using container_t = std::map<common::xnode_type_t, xelection_result_t>;
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

    std::map<common::xnode_type_t, xelection_result_t> const &
    results() const noexcept;

    void
    results(std::map<common::xnode_type_t, xelection_result_t> && r) noexcept;

    std::pair<iterator, bool>
    insert(value_type const & value);

    std::pair<iterator, bool>
    insert(value_type && value);

    xelection_result_t const &
    result_of(common::xnode_type_t const type) const;

    xelection_result_t &
    result_of(common::xnode_type_t const type);

    std::size_t
    size(common::xnode_type_t const type) const noexcept;

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

    iterator
    erase(const_iterator pos);

    size_type
    erase(key_type const & key);
};
using xelection_network_result_t = xtop_election_network_result;

NS_END3
