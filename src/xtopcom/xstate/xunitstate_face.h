// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xstate/xstate_face.h"

#include <system_error>

namespace top {
namespace state {

class xtop_unit_state_face : public xstate_face_t {
public:
    xtop_unit_state_face() = default;
    xtop_unit_state_face(xtop_unit_state_face const &) = delete;
    xtop_unit_state_face & operator=(xtop_unit_state_face const &) = delete;
    xtop_unit_state_face(xtop_unit_state_face &&) = default;
    xtop_unit_state_face & operator=(xtop_unit_state_face &&) = default;
    virtual ~xtop_unit_state_face() = default;

    virtual uint64_t nonce() const noexcept = 0;
    virtual xtoken_t withdraw(std::string const & symbol, uint64_t const amount, std::error_code & ec) = 0;
    virtual xtoken_t withdraw(uint64_t const amount, std::error_code & ec) = 0;

    virtual void create_property(properties::xproperty_identifier_t const & property_id, std::error_code & ec) = 0;
    /// @brief Update property by 
    /// @param property_id 
    /// @param string_value 
    /// @param ec 
    virtual void set_property(properties::xproperty_identifier_t const & property_id, std::string const & string_value, std::error_code & ec) = 0;
    virtual void set_property(properties::xproperty_identifier_t const & property_id, std::string const & key, xbyte_buffer_t const & value, std::error_code & ec);

};
using xunit_state_face_t = xtop_unit_state_face;

}
}
