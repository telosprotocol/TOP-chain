// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG3(top, contract_runtime, messages)

class xtop_cross_shard_message {
public:
    xtop_cross_shard_message(xtop_cross_shard_message const &) = delete;
    xtop_cross_shard_message & operator=(xtop_cross_shard_message const &) = delete;
    xtop_cross_shard_message(xtop_cross_shard_message &&) = default;
    xtop_cross_shard_message & operator=(xtop_cross_shard_message &&) = delete;
    ~xtop_cross_shard_message() = default;
};
using xcross_shard_message_t = xtop_cross_shard_message;

NS_END3
