// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_id.h"
#include "xdata/xregistration/xregistration_node_info.h"

#include <map>

NS_BEG3(top, data, registration)

class xtop_registration_chain_result {
private:
    using container_t = std::map<common::xnode_id_t, xregistration_node_info>;
    container_t m_result;

public:
    using key_type = container_t::key_type;
    using mapped_type = container_t::mapped_type;
    using value_type = container_t::value_type;
    using size_type = container_t::size_type;
    using difference_type = container_t::difference_type;
    using key_compare = container_t::key_compare;
    using reference = container_t::reference;
    using const_reference = container_t::const_reference;
    using pointer = container_t::pointer;
    using const_pointer = container_t::const_pointer;
    using iterator = container_t::iterator;
    using const_iterator = container_t::const_iterator;

    std::pair<iterator, bool>
    insert(value_type const & value);

    std::pair<iterator, bool>
    insert(value_type && value);

    bool
    empty(key_type const & key) const noexcept;

    bool
    empty() const noexcept;

    std::map<common::xnode_id_t, xregistration_node_info> const &
    results() const noexcept;

    void
    results(std::map<common::xnode_id_t, xregistration_node_info> && r) noexcept;

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

using xregistration_chain_result_t = xtop_registration_chain_result;

NS_END3