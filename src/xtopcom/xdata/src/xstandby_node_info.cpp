// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xstandby_node_info.h"

#include <utility>

#include "xbase/xbase.h"

NS_BEG3(top, data, election)

bool xtop_standby_node_info::operator==(xtop_standby_node_info const & other) const noexcept {
#if defined XENABLE_MOCK_ZEC_STAKE
    return consensus_public_key == other.consensus_public_key && stake_container == other.stake_container && program_version == other.program_version &&
           user_request_role == other.user_request_role && is_genesis_node == other.is_genesis_node;
#else
    return consensus_public_key == other.consensus_public_key && stake_container == other.stake_container && program_version == other.program_version &&
           is_genesis_node == other.is_genesis_node;
#endif
}

bool xtop_standby_node_info::operator!=(xtop_standby_node_info const & other) const noexcept {
    return !(*this == other);
}

void xtop_standby_node_info::swap(xtop_standby_node_info & other) noexcept {
    consensus_public_key.swap(other.consensus_public_key);
    std::swap(stake_container, other.stake_container);
#if defined XENABLE_MOCK_ZEC_STAKE
    std::swap(user_request_role, other.user_request_role);
#endif
    std::swap(program_version, other.program_version);
    std::swap(is_genesis_node, other.is_genesis_node);
}

uint64_t xtop_standby_node_info::stake(common::xnode_type_t const & node_type) const noexcept {
    if (stake_container.find(node_type) == stake_container.end()) {
        xassert(false);
        return 0;
    } else {
        return stake_container.at(node_type);
    }
}

void xtop_standby_node_info::stake(common::xnode_type_t const & node_type, uint64_t new_stake) noexcept {
    stake_container[node_type] = new_stake;
}

NS_END3
