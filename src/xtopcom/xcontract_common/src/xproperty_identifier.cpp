// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_identifier.h"

NS_BEG3(top, contract_common, properties)

xtop_property_identifier::xtop_property_identifier(std::string n, xproperty_type_t t, xproperty_category_t c) noexcept
  : m_name{std::move(n)}, m_type{t}, m_category{c} {
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

NS_END3
