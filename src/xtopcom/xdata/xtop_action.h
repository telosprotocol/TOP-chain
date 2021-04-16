// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbase/xobject.h"
#include "xcommon/xlogic_time.h"

#include <cstdint>

NS_BEG2(top, data)

enum class xenum_top_action_type : uint8_t{
    invalid,
    system_contract,
    user_contract,
    kernel_action,
    event
};
using xtop_action_type_t = xenum_top_action_type;

struct xtop_top_action {
protected:
    xobject_ptr_t<base::xobject_t> m_action_src{ nullptr };
    // uint64_t m_max_gas_amount{ 0 };
    common::xlogic_time_t m_expiration_time{ 0 };
    // uint64_t m_nonce{ 0 };
    xtop_action_type_t m_action_type{ xtop_action_type_t::invalid };

public:
    xtop_top_action(xtop_top_action const &) = default;
    xtop_top_action & operator=(xtop_top_action const &) = default;
    xtop_top_action(xtop_top_action &&) = default;
    xtop_top_action & operator=(xtop_top_action &&) = default;
    virtual ~xtop_top_action() = default;

    xtop_top_action(xobject_ptr_t<base::xobject_t> action_src, common::xlogic_time_t expiration_time, xtop_action_type_t type) noexcept;

    xtop_action_type_t type() const noexcept;
};
using xtop_action_t = xtop_top_action;

NS_END2
