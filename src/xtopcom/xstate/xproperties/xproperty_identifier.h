// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate/xproperties/xproperty_category.h"
#include "xstate/xproperties/xproperty_type.h"

#include <string>

NS_BEG3(top, state, properties)

class xtop_property_identifier {
private:
    std::string m_name;
    xproperty_type_t m_type{ xproperty_type_t::invalid };
    xproperty_category_t m_category{ xproperty_category_t::invalid };

public:
    xtop_property_identifier() = default;
    xtop_property_identifier(xtop_property_identifier const &) = default;
    xtop_property_identifier & operator=(xtop_property_identifier const &) = default;
    xtop_property_identifier(xtop_property_identifier &&) = default;
    xtop_property_identifier & operator=(xtop_property_identifier &&) = default;
    ~xtop_property_identifier() = default;

    xtop_property_identifier(std::string name, xproperty_type_t type, xproperty_category_t category) noexcept;

    /// @brief Get property full name. Full name contains prefix.
    /// @return Property full name.
    std::string full_name() const;

    /// @brief Get property name. Doesn't contain prefix.
    /// @return Property name.
    std::string name() const;

    /// @brief Get property type (int8 / int16 / int32 / int64 / uint8 / uint16 ...)
    /// @return Property data type.
    xproperty_type_t type() const noexcept;

    /// @brief Get Property category (system property / user property ...)
    /// @return Property category.
    xproperty_category_t category() const noexcept;

    /// @brief Determine if the property specified the ID object is a system property.
    /// @param property_id ID object to be checked.
    /// @return true if property is a system property. false otherwise.
    static bool system_property(xtop_property_identifier const & property_id) noexcept;

    /// @brief Determine if the property specified the ID object is a user property.
    /// @param property_id ID object to be checked.
    /// @return true if property is a user property. false otherwise.
    static bool user_property(xtop_property_identifier const & property_id) noexcept;
};
using xproperty_identifier_t = xtop_property_identifier;

NS_END3

namespace std {

template <>
struct hash<top::state::properties::xproperty_identifier_t> {
    size_t operator()(top::state::properties::xproperty_identifier_t const & property_id) const noexcept;
};


}
