// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xmemory.hpp"
#include "xstate_accessor/xstate_accessor.h"
#include "xstate_accessor/xstate_face.h"

#include <system_error>

namespace top {
namespace state_accessor {

class xtop_unit_state_face : public xstate_face_t {
protected:
    observer_ptr<xstate_accessor_t> state_accessor_;

public:
    xtop_unit_state_face() = default;
    xtop_unit_state_face(xtop_unit_state_face const &) = delete;
    xtop_unit_state_face & operator=(xtop_unit_state_face const &) = delete;
    xtop_unit_state_face(xtop_unit_state_face &&) = default;
    xtop_unit_state_face & operator=(xtop_unit_state_face &&) = default;
    virtual ~xtop_unit_state_face() = default;

protected:
    explicit xtop_unit_state_face(top::observer_ptr<xstate_accessor_t> const & state_accessor) noexcept;

public:

    virtual uint64_t nonce() const noexcept;
    virtual xtoken_t withdraw(std::string const & symbol, uint64_t const amount, std::error_code & ec);
    virtual xtoken_t withdraw(uint64_t const amount, std::error_code & ec);
    virtual void deposit(std::string const & symbol, xtoken_t & amount, std::error_code & ec);
    virtual void deposit(xtoken_t & amount, std::error_code & ec);

    virtual void create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);
    virtual void clear_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec);

    virtual void set_string(properties::xtypeless_property_identifier_t const & property_id, std::string const & value, std::error_code & ec);
    virtual std::string get_string(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const;

    virtual void set_map_value(properties::xtypeless_property_identifier_t const & property_id, std::string const & key, xbyte_buffer_t const & value, std::error_code & ec);
    virtual xbyte_buffer_t get_map_value(properties::xtypeless_property_identifier_t const & property_id, std::string const & key, std::error_code & ec) const;
    virtual std::map<std::string, xbyte_buffer_t> get_map(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const;
    virtual void erase_map_key(properties::xtypeless_property_identifier_t const & property_id, std::string const & key, std::error_code & ec);

    virtual void set_deque_value(properties::xtypeless_property_identifier_t const & property_id, std::size_t const key, xbyte_buffer_t const & value, std::error_code & ec);
    virtual xbyte_buffer_t get_deque_value(properties::xtypeless_property_identifier_t const & property_id, std::size_t const pos, std::error_code & ec) const;
    virtual std::deque<xbyte_buffer_t> get_deque(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const;
    virtual void erase_deque_front(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec);
    virtual void erase_deque_back(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec);
    virtual void push_deque_front(properties::xtypeless_property_identifier_t const & property_id, xbyte_buffer_t const & value, std::error_code & ec);
    virtual void push_deque_back(properties::xtypeless_property_identifier_t const & property_id, xbyte_buffer_t const & value, std::error_code & ec);

    virtual void set_uint64(properties::xtypeless_property_identifier_t const & property_id, uint64_t const value, std::error_code & ec);
    virtual uint64_t get_uint64(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const;
    virtual void set_int64(properties::xtypeless_property_identifier_t const & property_id, int64_t const value, std::error_code & ec);
    virtual int64_t get_int64(properties::xtypeless_property_identifier_t const & property_id, std::error_code & ec) const;
};
using xunit_state_face_t = xtop_unit_state_face;

}
}
