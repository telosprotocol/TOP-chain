// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate/xproperties/xproperty_identifier.h"

#include <cassert>

NS_BEG3(top, state, properties)

xtop_property_identifier::xtop_property_identifier(std::string name, xproperty_type_t type, xproperty_category_t category) noexcept
    : m_name{ std::move(name) }, m_type{ type }, m_category{ category } {
    assert(type != xproperty_type_t::invalid);
    assert(category != xproperty_category_t::invalid);
}

std::string xtop_property_identifier::full_name() const {
    return category_character(m_category) + m_name;
}

std::string xtop_property_identifier::name() const {
    return m_name;
}

xproperty_type_t xtop_property_identifier::type() const noexcept {
    return m_type;
}

xproperty_category_t xtop_property_identifier::category() const noexcept {
    return m_category;
}

bool xtop_property_identifier::system_property(xproperty_identifier_t const & property_id) noexcept {
    return property_id.category() == xproperty_category_t::system;
}

bool xtop_property_identifier::user_property(xproperty_identifier_t const & property_id) noexcept {
    return property_id.category() == xproperty_category_t::user;
}

NS_END3

namespace std {

size_t hash<top::state::properties::xproperty_identifier_t>::operator()(top::state::properties::xproperty_identifier_t const & property_id) const noexcept {
    return std::hash<std::string>{}(property_id.full_name());
}

}
