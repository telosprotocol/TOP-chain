// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xv2/xelection_info.h"

NS_BEG4(top, data, election, v2)

bool
xtop_election_info::operator==(xtop_election_info const & other) const noexcept {
    return joined_version       == other.joined_version       &&
           stake                == other.stake                &&
           comprehensive_stake  == other.comprehensive_stake  &&
           consensus_public_key == other.consensus_public_key &&
           miner_type           == other.miner_type           &&
           genesis              == other.genesis              &&
           raw_credit_score     == other.raw_credit_score;
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
    std::swap(miner_type, other.miner_type);
    std::swap(genesis, other.genesis);
    std::swap(raw_credit_score, other.raw_credit_score);
}

v1::xelection_info_t xtop_election_info::v1() const {
    v1::xelection_info_t r;
    r.joined_version = joined_version;
    r.stake = stake;
    r.comprehensive_stake = comprehensive_stake;
    r.consensus_public_key = consensus_public_key;
    r.miner_type = miner_type;
    r.genesis = genesis;

    return r;
}

NS_END4
