// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtop_action.h"

NS_BEG2(top, data)

xtop_top_action::xtop_top_action(xobject_ptr_t<base::xobject_t> action_src, common::xlogic_time_t expiration_time, xtop_action_type_t type) noexcept
    : m_action_src{ std::move(action_src) }, m_expiration_time{ expiration_time }, m_action_type{ type } {
    assert(type != xtop_action_type_t::invalid);
}

xtop_action_type_t xtop_top_action::type() const noexcept {
    return m_action_type;
}

NS_END2
