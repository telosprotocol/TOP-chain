// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xbasic/xsimple_map_result.h"
#include "xcommon/xnode_type.h"

NS_BEG3(top, data, standby)

class xtop_rec_standby_node_info {
public:
    uint64_t ec_stake;
    xpublic_key_t public_key;
    bool is_genesis_node;
    std::string program_version;

    bool operator==(xtop_rec_standby_node_info const & other) const noexcept;

    bool operator!=(xtop_rec_standby_node_info const & other) const noexcept;
};
using xrec_standby_node_info_t = xtop_rec_standby_node_info;

using xzec_standby_stake_container = top::xsimple_map_result_t<common::xnode_type_t, uint64_t>;
class xtop_zec_standby_node_info {
public:
    xzec_standby_stake_container stake_container;
    xpublic_key_t public_key;
    bool is_genesis_node;
    std::string program_version;
    bool operator==(xtop_zec_standby_node_info const & other) const noexcept;

    bool operator!=(xtop_zec_standby_node_info const & other) const noexcept;
};
using xzec_standby_node_info_t = xtop_zec_standby_node_info;

NS_END3