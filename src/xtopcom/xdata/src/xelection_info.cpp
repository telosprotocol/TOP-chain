// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xelection_info.h"

NS_BEG3(top, data, election)

bool
xtop_election_info::operator==(xtop_election_info const & other) const noexcept {
    return joined_version == other.joined_version           &&
           stake == other.stake                             &&
           comprehensive_stake == other.comprehensive_stake &&
           consensus_public_key == other.consensus_public_key;
}

bool
xtop_election_info::operator!=(xtop_election_info const & other) const noexcept {
    return !(*this == other);
}

void
xtop_election_info::swap(xtop_election_info & other) noexcept {
    joined_version.swap(other.joined_version);
    std::swap(stake, other.stake);
    std::swap(comprehensive_stake, other.comprehensive_stake);
    consensus_public_key.swap(other.consensus_public_key);
}

NS_END3
