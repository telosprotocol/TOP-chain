// Copyright (c) 2023-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbytes.h"

#include <boost/functional/hash.hpp>

#include <cstring>
#include <algorithm>
#include <array>

NS_BEG2(top, details)

template <std::size_t N>
class xtop_fixed_bytes {
private:
    using internal_type = std::array<xbyte_t, N>;
    internal_type data_{};

public:
    using value_type = typename internal_type::value_type;
    using size_type = typename internal_type::size_type;
    using difference_type = typename internal_type::difference_type;
    using reference = typename internal_type::reference;
    using const_reference = typename internal_type::const_reference;
    using pointer = typename internal_type::pointer;
    using const_pointer = typename internal_type::const_pointer;
    using iterator = typename internal_type::iterator;
    using const_iterator = typename internal_type::const_iterator;
    using reverse_iterator = typename internal_type::reverse_iterator;
    using const_reverse_iterator = typename internal_type::const_reverse_iterator;

    xtop_fixed_bytes(xtop_fixed_bytes const &) = default;
    xtop_fixed_bytes & operator=(xtop_fixed_bytes const &) = default;
    xtop_fixed_bytes(xtop_fixed_bytes &&) = default;
    xtop_fixed_bytes & operator=(xtop_fixed_bytes &&) = default;
    ~xtop_fixed_bytes() = default;

    xtop_fixed_bytes() {
        std::fill(data_.begin(), data_.end(), static_cast<xbyte_t>(0));
    }

    explicit xtop_fixed_bytes(internal_type const & data) : data_{ data } {
    }

    struct hash {
        size_t operator()(xtop_fixed_bytes const & value) const {
            return boost::hash_range(value.cbegin(), value.cend());
        }
    };

    xtop_fixed_bytes & operator=(internal_type const & data) noexcept {
        data_ = data;
        return *this;
    }

    bool operator==(xtop_fixed_bytes const & other) const noexcept {
        return data_ == other.data_;
    }

    bool operator!=(xtop_fixed_bytes const & other) const noexcept {
        return !(*this == other);
    }

#if defined(XCXX20)
    friend bool operator==(xtop_fixed_bytes const & lhs, xbytes_t const & rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }

        return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
    }
#else
    friend bool operator==(xtop_fixed_bytes const & lhs, xbytes_t const & rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }

        return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
    }

    friend bool operator==(xbytes_t const & lhs, xtop_fixed_bytes const & rhs) noexcept {
        if (lhs.size() != rhs.size()) {
            return false;
        }

        return std::memcmp(lhs.data(), rhs.data(), lhs.size()) == 0;
    }
#endif

    reference operator[](std::size_t index) noexcept {
        return data_[index];
    }

    value_type operator[](std::size_t index) const noexcept {
        return data_[index];
    }

    const_pointer data() const noexcept {
        return data_.data();
    }

    pointer data() noexcept {
        return data_.data();
    }

    std::size_t size() const noexcept {
               return data_.size();
    }

    bool empty() const noexcept {
        return std::all_of(data_.begin(), data_.end(), [](xbyte_t const v) { return v == 0; });
    }

    iterator begin() noexcept {
        return data_.begin();
    }

    const_iterator begin() const noexcept {
        return data_.begin();
    }

    const_iterator cbegin() const noexcept {
        return data_.cbegin();
    }

    reverse_iterator rbegin() noexcept {
        return data_.rbegin();
    }

    const_reverse_iterator rbegin() const noexcept {
        return data_.rbegin();
    }

    const_reverse_iterator crbegin() const noexcept {
        return data_.crbegin();
    }

    iterator end() noexcept {
        return data_.end();
    }

    const_iterator end() const noexcept {
        return data_.end();
    }

    const_iterator cend() const noexcept {
        return data_.cend();
    }

    reverse_iterator rend() noexcept {
        return data_.rend();
    }

    const_reverse_iterator rend() const noexcept {
        return data_.rend();
    }

    const_reverse_iterator crend() const noexcept {
        return data_.crend();
    }
};

NS_END2

NS_BEG1(top)

template <size_t N>
using xfixed_bytes_t = details::xtop_fixed_bytes<N>;

NS_END1

NS_BEG1(std)

template <std::size_t N>
struct hash<top::xfixed_bytes_t<N>> : top::xfixed_bytes_t<N> {};

NS_END1
