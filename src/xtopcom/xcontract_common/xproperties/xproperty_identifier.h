// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xproperty_category.h"
#include "xcontract_common/xproperties/xproperty_type.h"

#include <string>

NS_BEG3(top, contract_common, properties)

class xtop_property_identifier {
public:
    xtop_property_identifier() = default;
    xtop_property_identifier(xtop_property_identifier const &) = default;
    xtop_property_identifier & operator=(xtop_property_identifier const &) = default;
    xtop_property_identifier(xtop_property_identifier &&) = default;
    xtop_property_identifier & operator=(xtop_property_identifier &&) = default;
    ~xtop_property_identifier() = default;

    xtop_property_identifier(std::string n, xproperty_type_t t, xproperty_category_t c) noexcept;

    std::string full_name() const;
    std::string name() const;
    xproperty_type_t type() const noexcept;
    xproperty_category_t category() const noexcept;

private:
    std::string m_name;
    xproperty_type_t m_type{xproperty_type_t::invalid};
    xproperty_category_t m_category{xproperty_category_t::invalid};

};
using xproperty_identifier_t = xtop_property_identifier;

NS_END3
