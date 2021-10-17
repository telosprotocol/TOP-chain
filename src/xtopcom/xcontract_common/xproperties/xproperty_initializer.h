// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xproperties/xbasic_property.h"

#include <vector>

NS_BEG3(top, contract_common, properties)

class xtop_property_initializer {
    std::vector<observer_ptr<xbasic_property_t>> m_properties;

public:
    xtop_property_initializer() = default;
    xtop_property_initializer(xtop_property_initializer const &) = delete;
    xtop_property_initializer & operator=(xtop_property_initializer const &) = delete;
    xtop_property_initializer(xtop_property_initializer &&) = default;
    xtop_property_initializer & operator=(xtop_property_initializer &&) = default;
    ~xtop_property_initializer() = default;

    void register_property(observer_ptr<xbasic_property_t> const & property);
    void initialize() const;
};
using xproperty_initializer_t = xtop_property_initializer;

NS_END3
