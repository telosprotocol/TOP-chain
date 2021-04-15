// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <functional>
#include <limits>
#include <type_traits>

NS_BEG3(top, network, p2p)

template <typename T, T LowerBoundValue, T UpperBoundValue>
class xtop_distance final
{
private:
    XSTATIC_ASSERT(std::is_integral<T>::value);

    XSTATIC_ASSERT(UpperBoundValue > 0);
    XSTATIC_ASSERT(LowerBoundValue >= 0);

    XSTATIC_ASSERT(UpperBoundValue > LowerBoundValue);

    XSTATIC_ASSERT(std::numeric_limits<T>::max() > UpperBoundValue);
    XSTATIC_ASSERT(std::numeric_limits<T>::min() <= LowerBoundValue);

    static constexpr T m_upper_bound{ static_cast<T>(UpperBoundValue) };
    static constexpr T m_lower_bound{ static_cast<T>(LowerBoundValue) };

    T m_value { static_cast<T>(UpperBoundValue) };

public:
    constexpr xtop_distance()                        = default;
    xtop_distance(xtop_distance const &)             = default;
    xtop_distance & operator=(xtop_distance const &) = default;
    xtop_distance(xtop_distance &&)                  = default;
    xtop_distance & operator=(xtop_distance &&)      = default;
    ~xtop_distance()                                 = default;

    explicit
    xtop_distance(T const value) noexcept
        : m_value{ value }
    {
    }

    bool
    operator<(xtop_distance const & other) const noexcept {
        return m_value < other.m_value;
    }

    bool
    operator==(xtop_distance const & other) const noexcept {
        return m_value == other.m_value;
    }

    void
    swap(xtop_distance & other) noexcept {
        std::swap(m_value, other.m_value);
    }

    explicit
    operator T() const noexcept {
        return m_value;
    }

    bool
    empty() const noexcept {
        return m_value >= UpperBoundValue || m_value < LowerBoundValue;
    }

    static constexpr
    T
    upper_bound() noexcept {
        return static_cast<T>(m_upper_bound);
    }

    static constexpr
    T
    lower_bound() noexcept {
        return static_cast<T>(m_lower_bound);
    }
};

using xdistance_t = xtop_distance<std::size_t, 0, 256>;

NS_END3

NS_BEG1(std)

template <>
struct hash<top::network::p2p::xdistance_t> final
{
    std::size_t
    operator()(top::network::p2p::xdistance_t const & distance) const noexcept;
};

void
swap(top::network::p2p::xdistance_t & lhs,
     top::network::p2p::xdistance_t & rhs) noexcept;

NS_END1

