// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xstandby/xstandby_node_info2.h"

NS_BEG3(top, data, standby)

bool xtop_simple_standby_node_info::operator==(xtop_simple_standby_node_info const & other) const noexcept {
    return public_key == other.public_key && stake == other.stake && is_genesis_node == other.is_genesis_node && program_version == other.program_version;
}

bool xtop_simple_standby_node_info::operator!=(xtop_simple_standby_node_info const & other) const noexcept {
    return !(*this == other);
}

bool xtop_zec_standby_node_info::operator==(xtop_zec_standby_node_info const & other) const noexcept {
    return public_key == other.public_key && stake_container == other.stake_container && is_genesis_node == other.is_genesis_node && program_version == other.program_version;
}

bool xtop_zec_standby_node_info::operator!=(xtop_zec_standby_node_info const & other) const noexcept {
    return !(*this == other);
}
NS_END3