// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xerror/xerror.h"
#include "xstate_accessor/xerror/xerror.h"
#include "xstate_accessor/xproperties/xproperty_category.h"
#include "xstate_accessor/xproperties/xproperty_identifier_fwd.h"
#include "xstate_accessor/xproperties/xproperty_type.h"

#include <string>
#include <type_traits>

NS_BEG3(top, state_accessor, properties)

class xtop_property_identifier;

class xtop_typeless_property_identifier {
private:
    std::string m_name;
    xproperty_category_t m_category{ xproperty_category_t::invalid };
    mutable std::string m_fullname;

public:
    xtop_typeless_property_identifier() = default;
    xtop_typeless_property_identifier(xtop_typeless_property_identifier const &) = default;
    xtop_typeless_property_identifier & operator=(xtop_typeless_property_identifier const &) = default;
    xtop_typeless_property_identifier(xtop_typeless_property_identifier &&) = default;
    xtop_typeless_property_identifier & operator=(xtop_typeless_property_identifier &&) = default;
    ~xtop_typeless_property_identifier() = default;

    explicit xtop_typeless_property_identifier(std::string name, xproperty_category_t category);
    explicit xtop_typeless_property_identifier(std::string name);
    xtop_typeless_property_identifier(xtop_property_identifier const & property_identifier);

    /// @brief Get property full name. Full name contains prefix.
    /// @return Property full name.
    std::string full_name() const;

    /// @brief Get property name. Doesn't contain prefix.
    /// @return Property name.
    std::string const & name() const;

    /// @brief Get Property category (system property / user property ...)
    /// @return Property category.
    xproperty_category_t category() const noexcept;

    /// @brief Compare two property id object.
    bool operator==(xtop_typeless_property_identifier const & other) const;

    /// @brief Compare two property id object.
    bool operator!=(xtop_typeless_property_identifier const & other) const;
};

class xtop_property_identifier : private xtypeless_property_identifier_t {
private:
    xproperty_type_t m_type{ xproperty_type_t::invalid };

public:
    xtop_property_identifier() = default;
    xtop_property_identifier(xtop_property_identifier const &) = default;
    xtop_property_identifier & operator=(xtop_property_identifier const &) = default;
    xtop_property_identifier(xtop_property_identifier &&) = default;
    xtop_property_identifier & operator=(xtop_property_identifier &&) = default;
    ~xtop_property_identifier() = default;

    xtop_property_identifier(std::string name, xproperty_type_t type, xproperty_category_t category) noexcept;
    xtop_property_identifier(xtypeless_property_identifier_t const & other, xproperty_type_t type);

    using xtypeless_property_identifier_t::name;
    using xtypeless_property_identifier_t::full_name;
    using xtypeless_property_identifier_t::category;

    /// @brief Get property type (int8 / int16 / int32 / int64 / uint8 / uint16 ...)
    /// @return Property data type.
    xproperty_type_t type() const noexcept;

    operator xtypeless_property_identifier_t() const noexcept;

    /// @brief Compare two property id object. Exception will throw if property's m_name & m_category are the same but m_type is different.
    bool operator==(xtop_property_identifier const & other) const;

    /// @brief Compare two property id object.
    bool operator!=(xtop_property_identifier const & other) const;
};

/// @brief Determine if the property specified the ID object is a system property.
/// @param property_id ID object to be checked.
/// @return true if property is a system property. false otherwise.
bool system_property(xproperty_identifier_t const & property_id) noexcept;
bool system_property(xtypeless_property_identifier_t const & property_id) noexcept;

NS_END3

namespace std {

template <>
struct hash<top::state_accessor::properties::xproperty_identifier_t> {
    size_t operator()(top::state_accessor::properties::xproperty_identifier_t const & property_id) const noexcept;
};


}
