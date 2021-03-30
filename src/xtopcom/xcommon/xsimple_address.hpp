// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xenable_to_string.h"
#include "xbasic/xhashable.hpp"

#include <ostream>
#include <utility>

NS_BEG2(top, common)

template <typename AddressTagType, typename AddressValueType>
class xtop_simple_address final : public xhashable_t<xtop_simple_address<AddressTagType, AddressValueType>>
                                , public xenable_to_string_t<xtop_simple_address<AddressTagType, AddressValueType>>
{
public:
    using value_type = AddressValueType;
    using hash_result_type = typename xhashable_t<xtop_simple_address<AddressTagType, AddressValueType>>::hash_result_type;

private:
    value_type m_address{};

public:
    xtop_simple_address()                                        = default;
    xtop_simple_address(xtop_simple_address const &)             = default;
    xtop_simple_address & operator=(xtop_simple_address const &) = default;
    xtop_simple_address(xtop_simple_address &&)                  = default;
    xtop_simple_address & operator=(xtop_simple_address &&)      = default;
    ~xtop_simple_address() override                              = default;

    explicit
    xtop_simple_address(value_type v) noexcept
        : m_address{ std::move(v) }
    {
    }

    bool
    operator==(xtop_simple_address const & other) const noexcept {
        return m_address == other.m_address;
    }

    bool
    operator!=(xtop_simple_address const & other) const noexcept {
        return !(*this == other);
    }

    bool
    operator<(xtop_simple_address const & other) const noexcept {
        return m_address < other.m_address;
    }

    explicit
    operator
    bool() const noexcept {
        return !m_address.empty();
    }

    explicit
    operator value_type() const noexcept {
        return m_address;
    }

    bool
    empty() const noexcept {
        return m_address.empty();
    }

    value_type const &
    value() const noexcept {
        return m_address;
    }

    bool
    has_value() const noexcept {
        return !m_address.empty();
    }

    hash_result_type
    hash() const override {
        return static_cast<hash_result_type>(std::hash<value_type>{}(m_address));
    }

    std::string
    to_string() const override {
        return m_address.to_string();
    }
};

template <typename AddressTagType, typename AddressValueType>
using xsimple_address_t = xtop_simple_address<AddressTagType, AddressValueType>;

NS_END2

template <typename AddressTagT, typename AddressValueT>
std::ostream &
operator<<(std::ostream & o, top::common::xsimple_address_t<AddressTagT, AddressValueT> const & address) {
    if (address.empty()) {
        o << u8"(null)";
    } else {
        o << address.value();
    }
    return o;
}
