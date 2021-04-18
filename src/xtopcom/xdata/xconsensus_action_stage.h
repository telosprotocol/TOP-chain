// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

#include <cstdint>

NS_BEG2(top, data)

enum class xenum_consensus_action_stage : uint8_t {
    invalid,
    send,
    recv,
    confirm,
    self
};
using xconsensus_action_stage_t = xenum_consensus_action_stage;

NS_END2
