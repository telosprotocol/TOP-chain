// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"

NS_BEG2(top, sync)

enum enum_chain_sync_policy {
    enum_chain_sync_policy_fast,
    enum_chain_sync_policy_full,
    enum_chain_sync_policy_checkpoint,
    enum_chain_sync_policy_max,
};

NS_END2
