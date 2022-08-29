// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xv0/xelection_info.h"

NS_BEG4(top, data, election, v0)

bool xtop_election_info::operator==(xtop_election_info const & other) const noexcept {
    return joined_version_ == other.joined_version_ && stake_ == other.stake_ && comprehensive_stake_ == other.comprehensive_stake_ &&
           consensus_public_key_ == other.consensus_public_key_;
}

bool xtop_election_info::operator!=(xtop_election_info const & other) const noexcept {
    return !(*this == other);
}

void xtop_election_info::swap(xtop_election_info & other) noexcept {
    joined_version_.swap(other.joined_version_);
    std::swap(stake_, other.stake_);
    std::swap(comprehensive_stake_, other.comprehensive_stake_);
    consensus_public_key_.swap(other.consensus_public_key_);
}

common::xelection_epoch_t const & xtop_election_info::joined_epoch() const noexcept {
    return joined_version_;
}

void xtop_election_info::joined_epoch(common::xelection_epoch_t joined_epoch) {
    joined_version_ = std::move(joined_epoch);
}

uint64_t xtop_election_info::stake() const noexcept {
    return stake_;
}

void xtop_election_info::stake(uint64_t const stake) noexcept {
    stake_ = stake;
}

uint64_t xtop_election_info::comprehensive_stake() const noexcept {
    return comprehensive_stake_;
}

void xtop_election_info::comprehensive_stake(uint64_t const stake) noexcept {
    comprehensive_stake_ = stake;
}

xpublic_key_t const & xtop_election_info::public_key() const noexcept {
    return consensus_public_key_;
}

void xtop_election_info::public_key(xpublic_key_t pubkey) noexcept {
    consensus_public_key_ = std::move(pubkey);
}

NS_END4
