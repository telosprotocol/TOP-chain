// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xnode_id.h"

#include <string>
#include <vector>

NS_BEG3(top, data, parachain)

class xtop_parachain_chain_info {
public:
    std::vector<common::xnode_id_t> genesis_node_info;
    std::string chain_name;
    uint32_t chain_id;  // actually uint8_t is enough...
};

using xparachain_chain_info_t = xtop_parachain_chain_info;

NS_END3