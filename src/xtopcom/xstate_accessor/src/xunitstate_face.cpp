// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstate_accessor/xunitstate_face.h"

namespace top {
namespace state_accessor {

xtop_unit_state_face::xtop_unit_state_face(top::observer_ptr<xstate_accessor_t> const & state_accessor) noexcept : state_accessor_{ state_accessor } {
}

uint64_t xtop_unit_state_face::nonce() const noexcept {
    std::error_code ec;
    return state_accessor_->nonce({ "nonce", properties::xproperty_type_t::nonce, properties::xproperty_category_t::system }, ec);
}

xtoken_t xtop_unit_state_face::withdraw(std::string const & symbol, uint64_t const amount, std::error_code & ec) {
    assert(!ec);
    assert(state_accessor_ != nullptr);
    return state_accessor_->withdraw({ "balance", properties::xproperty_type_t::token, properties::xproperty_category_t::system }, symbol, amount, ec);
}

xtoken_t xtop_unit_state_face::withdraw(uint64_t const amount, std::error_code & ec) {
    return withdraw("TOP", amount, ec);
}

void xtop_unit_state_face::deposit(std::string const & symbol, xtoken_t & amount, std::error_code & ec) {
    assert(!ec);
    assert(state_accessor_ != nullptr);
    state_accessor_->deposit({ "balance", properties::xproperty_type_t::token, properties::xproperty_category_t::system }, symbol, amount, ec);
}

void xtop_unit_state_face::deposit(xtoken_t & amount, std::error_code & ec) {
    deposit("TOP", amount, ec);
}

void xtop_unit_state_face::create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(!ec);
    state_accessor_->create_property(property_id, ec);
}

void xtop_unit_state_face::clear_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) {
    assert(!ec);
    state_accessor_->clear_property(property_id, ec);
}

void xtop_unit_state_face::set_string(properties::xtypeless_property_identifier_t const & property_id, std::string const & value, std::error_code & ec) {
    assert(!ec);
    state_accessor_->set_property<properties::xproperty_type_t::string>(property_id, value, ec);
}

std::string xtop_unit_state_face::get_string(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    assert(!ec);
    return state_accessor_->get_property<properties::xproperty_type_t::string>(property_id, ec);
}

void xtop_unit_state_face::set_map_value(properties::xtypeless_property_identifier_t const & property_id, std::string const & key, xbyte_buffer_t const & value, std::error_code & ec) {
    state_accessor_->set_property_cell_value<properties::xproperty_type_t::map>(property_id, key, value, ec);
}

xbyte_buffer_t xtop_unit_state_face::get_map_value(properties::xtypeless_property_identifier_t const & property_id, std::string const & key, std::error_code & ec) const {
    return state_accessor_->get_property_cell_value<properties::xproperty_type_t::map>(property_id, key, ec);
}

std::map<std::string, xbyte_buffer_t> xtop_unit_state_face::get_map(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    return state_accessor_->get_property<properties::xproperty_type_t::map>(property_id, ec);
}

void xtop_unit_state_face::erase_map_key(properties::xtypeless_property_identifier_t const & property_id, std::string const & key, std::error_code & ec) {
    state_accessor_->remove_property_cell<properties::xproperty_type_t::map>(property_id, key, ec);
}

void xtop_unit_state_face::set_deque_value(properties::xtypeless_property_identifier_t const & property_id, std::size_t const key, xbyte_buffer_t const & value, std::error_code & ec) {
    state_accessor_->set_property_cell_value<properties::xproperty_type_t::deque>(property_id, key, value, ec);
}

xbyte_buffer_t xtop_unit_state_face::get_deque_value(properties::xtypeless_property_identifier_t const & property_id, std::size_t const pos, std::error_code & ec) const {
    return state_accessor_->get_property_cell_value<properties::xproperty_type_t::deque>(property_id, pos, ec);
}

std::deque<xbyte_buffer_t> xtop_unit_state_face::get_deque(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const {
    return state_accessor_->get_property<properties::xproperty_type_t::deque>(property_id, ec);
}

void xtop_unit_state_face::erase_deque_front(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) {
    state_accessor_->remove_property_cell<properties::xproperty_type_t::deque>(property_id, 0, ec);
}

void xtop_unit_state_face::erase_deque_back(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) {
    state_accessor_->remove_property_cell<properties::xproperty_type_t::deque>(property_id, SIZE_MAX, ec);
}

void xtop_unit_state_face::push_deque_front(properties::xtypeless_property_identifier_t const & property_id, xbyte_buffer_t const & value, std::error_code & ec) {
    // state_accessor_->set_property_cell_value<properties::xproperty_type_t::deque>(property_id, )
}

void xtop_unit_state_face::push_deque_back(properties::xtypeless_property_identifier_t const & property_id, xbyte_buffer_t const & value, std::error_code & ec) {

}

}
}
