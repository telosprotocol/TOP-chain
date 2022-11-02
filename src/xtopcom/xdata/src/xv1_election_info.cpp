// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xv1/xelection_info.h"

NS_BEG4(top, data, election, v1)

bool
xtop_election_info::operator==(xtop_election_info const & other) const noexcept {
    return v0_ == other.v0_ && miner_type_ == other.miner_type_ && genesis_ == other.genesis_;
}

bool
xtop_election_info::operator!=(xtop_election_info const & other) const noexcept {
    return !(*this == other);
}

void
xtop_election_info::swap(xtop_election_info & other) noexcept {
    v0_.swap(other.v0_);
    std::swap(miner_type_, other.miner_type_);
    std::swap(genesis_, other.genesis_);
}

v0::xelection_info_t const & xtop_election_info::v0() const noexcept {
    return v0_;
}

v0::xelection_info_t & xtop_election_info::v0() noexcept {
    return v0_;
}

common::xelection_epoch_t const & xtop_election_info::joined_epoch() const noexcept {
    return v0().joined_epoch();
}

void xtop_election_info::joined_epoch(common::xelection_epoch_t joined_epoch) {
    v0().joined_epoch(std::move(joined_epoch));
}

uint64_t xtop_election_info::stake() const noexcept {
    return v0().stake();
}

void xtop_election_info::stake(uint64_t const stake) noexcept {
    v0().stake(stake);
}

uint64_t xtop_election_info::comprehensive_stake() const noexcept {
    return v0().comprehensive_stake();
}

void xtop_election_info::comprehensive_stake(uint64_t const stake) noexcept {
    v0().comprehensive_stake(stake);
}

xpublic_key_t const & xtop_election_info::public_key() const noexcept {
    return v0().public_key();
}

void xtop_election_info::public_key(xpublic_key_t pubkey) noexcept {
    v0().public_key(std::move(pubkey));
}

common::xminer_type_t xtop_election_info::miner_type() const noexcept {
    return miner_type_;
}

void xtop_election_info::miner_type(common::xminer_type_t const type) noexcept {
    miner_type_ = type;
}

bool xtop_election_info::genesis() const noexcept {
    return genesis_;
}
void xtop_election_info::genesis(bool const v) noexcept {
    genesis_ = v;
}

NS_END4
