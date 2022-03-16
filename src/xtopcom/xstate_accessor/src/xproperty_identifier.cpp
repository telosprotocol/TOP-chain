// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xproperties/xproperty_identifier.h"
#include "xstate_accessor/xerror/xerror.h"

#include <cassert>

NS_BEG3(top, state_accessor, properties)

xtop_typeless_property_identifier::xtop_typeless_property_identifier(std::string name, xproperty_category_t category)
    : m_name{ std::move(name) }, m_category{ category } {
    assert(!m_name.empty());
    assert(category != xproperty_category_t::invalid);
    // if (m_name.front() == category_character(m_category)) {
    //     m_name = m_name.substr(1);
        if (m_name.empty()) {
            assert(false);
            top::error::throw_error(state_accessor::error::xerrc_t::empty_property_name);
        }
    // }
}

xtop_typeless_property_identifier::xtop_typeless_property_identifier(std::string name) : xtop_typeless_property_identifier {std::move(name), xproperty_category_t::user} {
}

xtop_typeless_property_identifier::xtop_typeless_property_identifier(xtop_property_identifier const & property_identifier)
    : xtop_typeless_property_identifier{property_identifier.name(), property_identifier.category()} {
}


std::string xtop_typeless_property_identifier::full_name() const {
    // if (m_fullname.empty()) {
    //     m_fullname = category_character(m_category) + m_name;
    // }
    // return m_fullname;
    return m_name;
}

std::string const & xtop_typeless_property_identifier::name() const {
    return m_name;
}

xproperty_category_t xtop_typeless_property_identifier::category() const noexcept {
    return m_category;
}

bool xtop_typeless_property_identifier::operator==(xtop_typeless_property_identifier const & other) const {
    return m_name == other.m_name && m_category == other.m_category;
}

bool xtop_typeless_property_identifier::operator!=(xtop_typeless_property_identifier const & other) const {
    return !(*this == other);
}

xtop_property_identifier::xtop_property_identifier(std::string name, xproperty_type_t type, xproperty_category_t category) noexcept
    : xtypeless_property_identifier_t{ std::move(name), category }, m_type { type } {
    assert(type != xproperty_type_t::invalid);
}

xtop_property_identifier::xtop_property_identifier(xtypeless_property_identifier_t const & other, xproperty_type_t type)
    : xtop_property_identifier{ other.name(), type, other.category() } {
}

xproperty_type_t xtop_property_identifier::type() const noexcept {
    return m_type;
}

bool xtop_property_identifier::operator==(xtop_property_identifier const & other) const {
    auto const same = xtypeless_property_identifier_t::operator==(other);
    if (same && m_type != other.m_type) {
        assert(false);
        top::error::throw_error({ error::xerrc_t::invalid_property_type }, "property id: " + full_name() + " type mismatch (" + to_string(m_type) + " vs " + to_string(other.m_type) + ")");
    }

    return same;
}

bool xtop_property_identifier::operator!=(xtop_property_identifier const & other) const {
    return !(*this == other);
}

xtop_property_identifier::operator xtypeless_property_identifier_t() const noexcept {
    return *this;
}

bool system_property(xproperty_identifier_t const & property_id) noexcept {
    return property_id.category() == xproperty_category_t::system;
}

bool system_property(xtypeless_property_identifier_t const & property_id) noexcept {
    return property_id.category() == xproperty_category_t::system;
}

NS_END3

namespace std {

size_t hash<top::state_accessor::properties::xproperty_identifier_t>::operator()(top::state_accessor::properties::xproperty_identifier_t const & property_id) const noexcept {
    return std::hash<std::string>{}(property_id.full_name());
}

}
