// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xtop_action.h"

#include <cassert>

NS_BEG2(top, data)

xtop_basic_top_action::xtop_basic_top_action(xobject_ptr_t<base::xobject_t> action_src, common::xlogic_time_t expiration_time) noexcept
    : m_action_src{ std::move(action_src) }, m_expiration_time{ expiration_time } {
}

xtop_action_type_t xtop_basic_top_action::type() const noexcept {
    return xtop_action_type_t::invalid;
}

NS_END2
