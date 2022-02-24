// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcontract_common/xproperties/xproperty_bytes.h"

NS_BEG3(top, contract_common, properties)

xtop_bytes_property::xtop_bytes_property(std::string const & name, xcontract_face_t * contract)
  : xbasic_property_t{name, state_accessor::properties::xproperty_type_t::bytes, make_observer(contract)} {
}

xtop_bytes_property::xtop_bytes_property(std::string const & name, std::unique_ptr<xcontract_state_t> state_owned)
  : xbasic_property_t{name, state_accessor::properties::xproperty_type_t::bytes, std::move(state_owned)} {
}

void xtop_bytes_property::set(xbytes_t const & value) {
    assert(associated_state() != nullptr);
    associated_state()->xcontract_state_t::set_property<state_accessor::properties::xproperty_type_t::bytes>(typeless_id(), value);
}

void xtop_bytes_property::clear() {
    assert(associated_state() != nullptr);
    associated_state()->clear_property(id());
}

xbytes_t xtop_bytes_property::value() const {
    assert(associated_state() != nullptr);
    return associated_state()->xcontract_state_t::get_property<state_accessor::properties::xproperty_type_t::bytes>(typeless_id());
}

size_t xtop_bytes_property::size() const {
    assert(associated_state() != nullptr);
    return associated_state()->property_size(id());
}

bool xtop_bytes_property::empty() const {
    return size() == 0;
}

NS_END3
