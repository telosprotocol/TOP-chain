// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xv2/xstandby_node_info.h"

#include "xbasic/xutility.h"

NS_BEG4(top, data, election, v2)

bool xtop_standby_node_info::operator==(xtop_standby_node_info const & other) const noexcept {
    return consensus_public_key == other.consensus_public_key &&
           stake_container      == other.stake_container      &&
           program_version      == other.program_version      &&
           miner_type           == other.miner_type           &&
           genesis              == other.genesis              &&
           raw_credit_scores    == other.raw_credit_scores;
}

bool xtop_standby_node_info::operator!=(xtop_standby_node_info const & other) const noexcept {
    return !(*this == other);
}

void xtop_standby_node_info::swap(xtop_standby_node_info & other) noexcept {
    consensus_public_key.swap(other.consensus_public_key);
    std::swap(stake_container, other.stake_container);
    std::swap(program_version, other.program_version);
    std::swap(genesis, other.genesis);
    std::swap(miner_type, other.miner_type);
    raw_credit_scores.swap(other.raw_credit_scores);
}

uint64_t xtop_standby_node_info::stake(common::xnode_type_t const & node_type) const noexcept {
    auto const it = stake_container.find(node_type);
    if (it == stake_container.end()) {
        assert(false);
        return 0;
    } else {
        return top::get<uint64_t>(*it);
    }
}

void xtop_standby_node_info::stake(common::xnode_type_t const & node_type, uint64_t new_stake) noexcept {
    stake_container[node_type] = new_stake;
}

uint64_t xtop_standby_node_info::raw_credit_score(common::xnode_type_t const node_type) const noexcept {
    auto const it = raw_credit_scores.find(node_type);
    if (it == std::end(raw_credit_scores)) {
        return 0;
    }

    return top::get<uint64_t>(*it);
}

void xtop_standby_node_info::raw_credit_score(common::xnode_type_t const node_type, uint64_t const value) noexcept {
    raw_credit_scores[node_type] = value;
}

v1::xstandby_node_info_t xtop_standby_node_info::v1() const {
    v1::xstandby_node_info_t r;
    r.stake_container = stake_container;
    r.consensus_public_key = consensus_public_key;
    r.program_version = program_version;
    r.genesis = genesis;

    return r;
}

NS_END4
