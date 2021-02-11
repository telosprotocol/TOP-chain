// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xaddress.h"
#include "xdata/xelection/xelection_info.h"

#include <cstdint>

NS_BEG2(top, data)

struct xtop_node_info final {
    election::xelection_info_t election_info{};
    common::xnode_address_t address{};
};
using xnode_info_t = xtop_node_info;

NS_END2
