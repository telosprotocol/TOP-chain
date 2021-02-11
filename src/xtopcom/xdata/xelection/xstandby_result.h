// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_id.h"
#include "xdata/xelection/xstandby_node_info.h"

#include <map>
#include <string>

NS_BEG3(top, data, election)

class xtop_standby_result final
{
private:
    using container_t = std::map<common::xnode_id_t, xstandby_node_info_t>;
    container_t m_nodes{};

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

    std::pair<iterator, bool>
    update(value_type const & value);

    // update: if (not exist or changed), .second = true .else false
    std::pair<iterator, bool>
    update(value_type && value);

    std::pair<iterator, bool>
    insert(value_type const & value);

    std::pair<iterator, bool>
    insert(value_type && value);

    bool
    empty() const noexcept;

    std::map<common::xnode_id_t, xstandby_node_info_t> const &
    results() const noexcept;

    std::map<common::xnode_id_t, xstandby_node_info_t> &
    results() noexcept;

    void
    results(std::map<common::xnode_id_t, xstandby_node_info_t> && r) noexcept;

    xstandby_node_info_t const &
    result_of(common::xnode_id_t const & nid) const;

    xstandby_node_info_t &
    result_of(common::xnode_id_t const & nid);

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

    const_iterator
    find(common::xnode_id_t const & id) const noexcept;

    iterator
    erase(const_iterator pos);

    size_type
    erase(key_type const & key);
};
using xstandby_result_t = xtop_standby_result;

NS_END3
