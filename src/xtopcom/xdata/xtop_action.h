// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbase/xobject.h"
#include "xcommon/xlogic_time.h"
#include "xdata/xtop_action_fwd.h"
#include "xdata/xtop_action_type.h"

#include <cstdint>

NS_BEG2(top, data)

class xtop_basic_top_action {
protected:
    xobject_ptr_t<base::xobject_t> m_action_src{ nullptr };
    common::xlogic_time_t m_expiration_time{ 0 };

public:
    xtop_basic_top_action() = default;
    xtop_basic_top_action(xtop_basic_top_action const &) = default;
    xtop_basic_top_action & operator=(xtop_basic_top_action const &) = default;
    xtop_basic_top_action(xtop_basic_top_action &&) = default;
    xtop_basic_top_action & operator=(xtop_basic_top_action &&) = default;
    virtual ~xtop_basic_top_action() = default;

protected:
    xtop_basic_top_action(xobject_ptr_t<base::xobject_t> action_src, common::xlogic_time_t expiration_time) noexcept;

public:
    virtual xtop_action_type_t type() const noexcept;
};
using xbasic_top_action_t = xtop_basic_top_action;

template <xtop_action_type_t ActionTypeV>
struct xtop_top_action : public xbasic_top_action_t {
public:
    xtop_top_action(xtop_top_action const &) = default;
    xtop_top_action & operator=(xtop_top_action const &) = default;
    xtop_top_action(xtop_top_action &&) = default;
    xtop_top_action & operator=(xtop_top_action &&) = default;
    ~xtop_top_action() override = default;

    xtop_top_action(xobject_ptr_t<base::xobject_t> action_src, common::xlogic_time_t expiration_time) noexcept : xbasic_top_action_t{ std::move(action_src), expiration_time } {
    }

    xtop_action_type_t type() const noexcept override final {
        return ActionTypeV;
    }
};

template <xtop_action_type_t ActionTypeV>
using xtop_action_t = xtop_top_action<ActionTypeV>;

NS_END2
