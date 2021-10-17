// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_initializer.h"

NS_BEG3(top, contract_common, properties)

void xtop_property_initializer::register_property(observer_ptr<xbasic_property_t> const & property) {
    m_properties.push_back(property);
}

void xtop_property_initializer::initialize() const {
    for (auto & property : m_properties) {
        property->initialize();
    }
}


NS_END3
