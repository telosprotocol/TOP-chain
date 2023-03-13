// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xhashable.hpp"
#include "xutility/xhash.h"

#include <cassert>
#include <functional>
#include <string>

NS_BEG2(top, common)

template <typename IdTagType>
class xtop_string_id : public xhashable_t<xtop_string_id<IdTagType>>
{
public:
    using hash_result_type = typename xhashable_t<xtop_string_id<IdTagType>>::hash_result_type;
    using value_type = std::string;

protected:
    value_type m_id{};

public:
    xtop_string_id()                                   = default;
    xtop_string_id(xtop_string_id const &)             = default;
    xtop_string_id & operator=(xtop_string_id const &) = default;
    xtop_string_id(xtop_string_id &&)                  = default;
    xtop_string_id & operator=(xtop_string_id &&)      = default;
    ~xtop_string_id() override                         = default;

    explicit
    xtop_string_id(std::string v)
        : m_id{ std::move(v) } {
    }

    void
    swap(xtop_string_id & other) noexcept {
        m_id.swap(other.m_id);
    }

    bool
    operator==(xtop_string_id const & other) const noexcept {
        return m_id == other.m_id;
    }

    bool
    operator!=(xtop_string_id const & other) const noexcept {
        return !(*this == other);
    }

    bool
    operator<(xtop_string_id const & other) const noexcept {
        return m_id < other.m_id;
    }

    bool
    operator>(xtop_string_id const & other) const noexcept {
        return other.m_id < m_id;
    }

    bool
    operator<=(xtop_string_id const & other) const noexcept {
        return !(other.m_id < m_id);
    }

    bool
    operator>=(xtop_string_id const & other) const noexcept {
        return !(m_id < other.m_id);
    }

    explicit
    operator
    bool() const noexcept {
        return !m_id.empty();
    }

    bool
    empty() const noexcept {
        return m_id.empty();
    }

    bool
    has_value() const noexcept {
        return !empty();
    }

    value_type &
    value() & {
        return m_id;
    }

    value_type const &
    value() const & {
        return m_id;
    }

    value_type &&
    value() && {
        return std::move(m_id);
    }

#if defined(XCXX14)    // C++14 and above
    value_type const &&
    value() const && {
        return std::move(m_id);
    }
#endif

    hash_result_type
    hash() const override {
        if (has_value()) {
            return utl::xxh64_t::digest(m_id.data(), m_id.size());
        }

        return 0;
    }

    std::string
    to_string() const {
        if (empty()) {
            return "(null)";
        }

        return m_id;
    }

    void
    clear() noexcept {
        m_id.clear();
    }
};

template <typename IdTagType>
using xstring_id_t = xtop_string_id<IdTagType>;

NS_END2

NS_BEG1(std)

template <typename IdTagType>
struct hash<top::common::xstring_id_t<IdTagType>> final
{
    std::size_t
    operator()(top::common::xstring_id_t<IdTagType> const & id) const noexcept {
        return static_cast<std::size_t>(id.hash());
    }
};

NS_END1

