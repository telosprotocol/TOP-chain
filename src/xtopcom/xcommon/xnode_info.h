// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xcommon/xaddress.h"
#include "xcommon/xversion.h"

#include <cstdint>

NS_BEG2(top, common)

struct xtop_node_info {
    xpublic_key_t public_key;
    std::uint64_t staking;
    common::xelection_round_t joined_version;
    common::xnode_address_t node_address;
};

using xnode_info_t = xtop_node_info;

NS_END2
