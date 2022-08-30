// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xmem.h"
#include "xbase/xutl.h"
#include "xcommon/xaccount_address.h"
#include "xevm_common/common.h"

#include <algorithm>
#include <cassert>
#include <cinttypes>
#include <cstdint>
#include <ctime>
#include <set>
#include <string>
#include <vector>

NS_BEG3(top, data, system_contract)

class xtop_allowance {
public:
    using data_type = std::map<common::xaccount_address_t, evm_common::u256>;
    using allocator_type = data_type::allocator_type;
    using const_iterator = data_type::const_iterator;
    using const_pointer = data_type::const_pointer;
    using const_reference = data_type::const_reference;
    using difference_type = data_type::difference_type;
    using iterator = data_type::iterator;
    using key_type = data_type::key_type;
    using mapped_type = data_type::mapped_type;
    using pointer = data_type::pointer;
    using reference = data_type::reference;
    using size_type = data_type::size_type;
    using value_type = data_type::value_type;

    xtop_allowance() = default;
    xtop_allowance(xtop_allowance const &) = delete;
    xtop_allowance & operator=(xtop_allowance const &) = delete;
    xtop_allowance(xtop_allowance &&) = default;
    xtop_allowance & operator=(xtop_allowance &&) = default;
    ~xtop_allowance() = default;

    explicit xtop_allowance(data_type d) noexcept(std::is_nothrow_move_constructible<data_type>::value);

    // iterators
    iterator begin() noexcept;
    const_iterator begin() const noexcept;
    const_iterator cbegin() const noexcept;

    iterator end() noexcept;
    const_iterator end() const noexcept;
    const_iterator cend() const noexcept;

    // capacity
    bool empty() const noexcept;
    size_type size() const noexcept;

    // modifier
    std::pair<iterator, bool> insert(const value_type & value);
    iterator insert(const_iterator hint, const value_type & value);

    // lookup
    size_type count(key_type const & key) const;
    iterator find(key_type const & key);
    const_iterator find(key_type const & key) const;

    data_type const & raw_data() const noexcept;

private:
    data_type data_;
};
using xallowance_t = xtop_allowance;

NS_END3
