// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xenable_to_string.h"
#include "xbasic/xhashable.hpp"

#include <cassert>
#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

NS_BEG1(top)

class xtop_bad_version_access : public std::exception {
public:
    const char *
    what() const noexcept override {
        return "bad xversion_t access";
    }
};
using xbad_version_access_t = xtop_bad_version_access;

template <typename TagT, typename ValueT>
class xtop_version final : public xhashable_t<xtop_version<TagT, std::size_t>>
                         , public xenable_to_string_t<xtop_version<TagT, ValueT>>
{
    XSTATIC_ASSERT(std::is_integral<ValueT>::value);

public:
    using hash_result_type = typename xhashable_t<xtop_version<TagT, std::size_t>>::hash_result_type;
    using value_type = ValueT;

    XSTATIC_ASSERT(std::is_integral<hash_result_type>::value && std::is_unsigned<hash_result_type>::value);

private:
    value_type m_value{};
    bool m_initialized{ false };

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    value_type
    get_value() const {
        if (!m_initialized) {
            assert(false);
            throw xbad_version_access_t{};
        }

        return m_value;
    }

public:
    constexpr xtop_version()                       = default;
    xtop_version(xtop_version const &)             = default;
    xtop_version & operator=(xtop_version const &) = default;
    xtop_version(xtop_version &&)                  = default;
    xtop_version & operator=(xtop_version &&)      = default;
    ~xtop_version()                                = default;

    constexpr
    explicit
    xtop_version(value_type const v) noexcept
        : m_value{ v }, m_initialized{ true } {
    }

    void
    swap(xtop_version & other) noexcept {
        std::swap(m_value, other.m_value);
        std::swap(m_initialized, other.m_initialized);
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    operator==(xtop_version const & other) const noexcept {
        if (m_initialized != other.m_initialized) {
            return false;
        }

        if (m_initialized == false) {
            assert(other.m_initialized == false);
            return true;
        }

        return m_value == other.m_value;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    operator!=(xtop_version const & other) const noexcept {
        return !(*this == other);
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    operator<(xtop_version const & other) const noexcept {
        if (other.m_initialized == false) {
            return false;
        }

        if (m_initialized == false) {
            return true;
        }

        return m_value < other.m_value;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    operator>(xtop_version const & other) const noexcept {
        return other < *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    operator<=(xtop_version const & other) const noexcept {
        if (m_initialized == false) {
            return true;
        }

        if (other.m_initialized == false) {
            return false;
        }

        return m_value <= other.m_value;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    operator>=(xtop_version const & other) const noexcept {
        return other <= *this;
    }

#if defined(XCXX14_OR_ABOVE)
    constexpr
#endif
    xtop_version next(int32_t step = 1) {
        auto this_copy = *this;
        if (step > 0) {
            this_copy.increase(step);
        } else if (step < 0) {
            this_copy.decrease(-step);
        }

        return this_copy;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_version &
    operator++() {
        if (empty()) {
            assert(false);
            throw xbad_version_access_t{};
        }

        if (m_value == std::numeric_limits<value_type>::max()) {
            assert(false);
            throw std::overflow_error{ "version overflow " + std::to_string(m_value) };
        }

        ++m_value;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_version
    operator++(int) {
        if (empty()) {
            assert(false);
            throw xbad_version_access_t{};
        }

        if (m_value == std::numeric_limits<value_type>::max()) {
            assert(false);
            throw std::overflow_error{ "version overflow " + std::to_string(m_value) };
        }

        auto old_value = m_value;
        ++m_value;
        return xtop_version{ old_value };
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_version &
    operator--() {
        if (empty()) {
            assert(false);
            throw xbad_version_access_t{};
        }

        if (m_value == std::numeric_limits<value_type>::min()) {
            assert(false);
            throw std::underflow_error{ "version underflow " + std::to_string(m_value) };
        }

        --m_value;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_version
    operator--(int) {
        if (empty()) {
            assert(false);
            throw xbad_version_access_t{};
        }

        if (m_value == std::numeric_limits<value_type>::min()) {
            assert(false);
            throw std::underflow_error{ "version underflow " + std::to_string(m_value) + u8"--"};
        }

        auto old_value = m_value;
        --m_value;
        return xtop_version{ old_value };
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_version &
    increase(value_type step = 1) {
        if (empty()) {
            assert(false);
            throw xbad_version_access_t{};
        }

        auto increament_space = std::numeric_limits<value_type>::max() - step;
        if (step > increament_space) {
            assert(false);
            throw std::overflow_error{ "version overflow " + std::to_string(m_value) + u8" increase " + std::to_string(step) };
        }

        m_value += step;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    xtop_version &
    decrease(value_type step = 1) {
        if (empty()) {
            assert(false);
            throw xbad_version_access_t{};
        }

        auto decreament_space = step - std::numeric_limits<value_type>::min();
        if (step > decreament_space) {
            assert(false);
            throw std::underflow_error{ "version underflow " + std::to_string(m_value) + u8" decrease " + std::to_string(step) };
        }

        m_value -= step;
        return *this;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    value_type
    value() const noexcept {
        return get_value();
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    value_type
    value_or(value_type const default_value) const noexcept {
        if (!has_value()) {
            return default_value;
        }

        return get_value();
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    empty() const noexcept {
        return !m_initialized;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    bool
    has_value() const noexcept {
        return m_initialized;
    }

#if defined XCXX14_OR_ABOVE
    constexpr
#endif
    hash_result_type
    hash() const override {
        if (has_value()) {
            return static_cast<hash_result_type>(m_value);
        } else {
            return 0;
        }
    }

    std::string
    to_string() const override {
        if (has_value()) {
            return std::to_string(m_value);
        }

        return u8"(null)";
    }

    void
    clear() {
        m_value = value_type{};
        m_initialized = false;
    }

    constexpr
    static
    xtop_version
    max() {
        return xtop_version{ std::numeric_limits<value_type>::max() };
    }

    constexpr
    static
    xtop_version
    min() {
        return xtop_version{ std::numeric_limits<value_type>::min() };
    }
};

template <typename TagT, typename ValueT>
using xversion_t = xtop_version<TagT, ValueT>;

NS_END1

NS_BEG1(std)

template <typename TagT, typename ValueT>
struct hash<top::xversion_t<TagT, ValueT>> {
    std::size_t
    operator()(top::xversion_t<TagT, ValueT> const & version) const noexcept {
        return version.hash();
    }
};

NS_END1
