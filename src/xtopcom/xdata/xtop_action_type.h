// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG2(top, data)

enum class xenum_top_action_type : uint8_t {
    invalid,
    system,
    user,
    kernel,// TODO just a placeholder, may remove later.
    event
};
using xtop_action_type_t = xenum_top_action_type;

NS_END2
