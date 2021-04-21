// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xdata/xtop_action_type.h"

NS_BEG2(top, data)

class xtop_basic_top_action;
using xbasic_top_action_t = xtop_basic_top_action;

template <xtop_action_type_t ActionTypeV>
struct xtop_top_action;

template <xtop_action_type_t ActionTypeV>
using xtop_action_t = xtop_top_action<ActionTypeV>;

NS_END2
