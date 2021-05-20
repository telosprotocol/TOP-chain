// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xerror/xthrow_error.h"
#include "xstate/xerror/xerror.h"
#include "xstate/xproperties/xproperty_category.h"
#include "xstate/xproperties/xproperty_identifier_fwd.h"
#include "xstate/xproperties/xproperty_type.h"

#include <string>
#include <type_traits>

NS_BEG3(top, state, properties)

template <xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV != xproperty_type_t::invalid>::type *>
class xtop_typed_property_identifier {
private:
    std::string m_name;
    xproperty_category_t m_category{ xproperty_category_t::invalid };

public:
    xtop_typed_property_identifier() = default;
    xtop_typed_property_identifier(xtop_typed_property_identifier const &) = default;
    xtop_typed_property_identifier & operator=(xtop_typed_property_identifier const &) = default;
    xtop_typed_property_identifier(xtop_typed_property_identifier &&) = default;
    xtop_typed_property_identifier & operator=(xtop_typed_property_identifier &&) = default;
    ~xtop_typed_property_identifier() = default;

    xtop_typed_property_identifier(std::string name, xproperty_category_t category) noexcept;
    xtop_typed_property_identifier(xproperty_identifier_t const & other);

    /// @brief Get property full name. Full name contains prefix.
    /// @return Property full name.
    std::string full_name() const {
        return category_character(m_category) + m_name;
    }

    /// @brief Get property name. Doesn't contain prefix.
    /// @return Property name.
    std::string name() const {
        return m_name;
    }

    /// @brief Get property type (int8 / int16 / int32 / int64 / uint8 / uint16 ...)
    /// @return Property data type.
    constexpr xproperty_type_t type() const noexcept {
        return PropertyTypeV;
    }

    /// @brief Get Property category (system property / user property ...)
    /// @return Property category.
    xproperty_category_t category() const noexcept {
        return m_category;
    }
};

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

    template <xproperty_type_t PropertyTypeV>
    xtop_property_identifier(xtyped_property_identifier_t<PropertyTypeV> const & other) : m_name{ other.name() }, m_type{ other.type() }, m_category{ other.category() } {
    }

    template <xproperty_type_t PropertyTypeV>
    xtop_property_identifier & operator=(xtyped_property_identifier_t<PropertyTypeV> const & other) {
        xtop_property_identifier t{ other };
        *this = t;
        return *this;
    }

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

    template <xproperty_type_t PropertyTypeV>
    operator xtyped_property_identifier_t<PropertyTypeV>() const {
        if (PropertyTypeV != type()) {
            std::error_code ec;
            ec = top::state::error::xerrc_t::property_id_conversion_invalid;
            top::error::throw_error(ec);
        } else {
            xtyped_property_identifier_t<PropertyTypeV>{ name(), category() };
        }
    }
};

template <xproperty_type_t PropertyTypeV, typename std::enable_if<PropertyTypeV != xproperty_type_t::invalid>::type * PlaceholderPtr>
xtop_typed_property_identifier<PropertyTypeV, PlaceholderPtr>::xtop_typed_property_identifier(xproperty_identifier_t const & other)
    : m_name{ other.name() }, m_category{ other.category() } {
    if (PropertyTypeV != other.type()) {
        top::error::throw_error({ top::state::error::xerrc_t::property_id_conversion_invalid });
    }
}

/// @brief Determine if the property specified the ID object is a system property.
/// @param property_id ID object to be checked.
/// @return true if property is a system property. false otherwise.
bool system_property(xproperty_identifier_t const & property_id) noexcept;

/// @brief Determine if the property specified the ID object is a user property.
/// @param property_id ID object to be checked.
/// @return true if property is a user property. false otherwise.
bool user_property(xproperty_identifier_t const & property_id) noexcept;

template <xproperty_type_t PropertyTypeV>
bool system_property(xtyped_property_identifier_t<PropertyTypeV> const & property_id) noexcept {
    return property_id.category() == xproperty_category_t::system;
}

template <xproperty_type_t PropertyTypeV>
bool user_property(xtyped_property_identifier_t<PropertyTypeV> const & property_id) noexcept {
    return property_id.category() == xproperty_category_t::user;
}

NS_END3

namespace std {

template <>
struct hash<top::state::properties::xproperty_identifier_t> {
    size_t operator()(top::state::properties::xproperty_identifier_t const & property_id) const noexcept;
};


}
