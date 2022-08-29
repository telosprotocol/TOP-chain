// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xdata/xelection/xv2/xelection_info.h"

NS_BEG4(top, data, election, v2)

bool xtop_election_info::operator==(xtop_election_info const & other) const noexcept {
    return v1_ == other.v1_ && raw_credit_score_ == other.raw_credit_score_;
}

bool xtop_election_info::operator!=(xtop_election_info const & other) const noexcept {
    return !(*this == other);
}

void xtop_election_info::swap(xtop_election_info & other) noexcept {
    v1_.swap(other.v1_);
    std::swap(raw_credit_score_, other.raw_credit_score_);
}

v1::xelection_info_t const & xtop_election_info::v1() const {
    return v1_;
}

v1::xelection_info_t & xtop_election_info::v1() {
    return v1_;
}

common::xelection_epoch_t const & xtop_election_info::joined_epoch() const noexcept {
    return v1().joined_epoch();
}

void xtop_election_info::joined_epoch(common::xelection_epoch_t joined_epoch) {
    v1().joined_epoch(std::move(joined_epoch));
}

uint64_t xtop_election_info::stake() const noexcept {
    return v1().stake();
}

void xtop_election_info::stake(uint64_t const stake) noexcept {
    v1().stake(stake);
}

uint64_t xtop_election_info::comprehensive_stake() const noexcept {
    return v1().comprehensive_stake();
}

void xtop_election_info::comprehensive_stake(uint64_t const stake) noexcept {
    v1().comprehensive_stake(stake);
}

xpublic_key_t const & xtop_election_info::public_key() const noexcept {
    return v1().public_key();
}

void xtop_election_info::public_key(xpublic_key_t pubkey) noexcept {
    v1().public_key(std::move(pubkey));
}

common::xminer_type_t xtop_election_info::miner_type() const noexcept {
    return v1().miner_type();
}

void xtop_election_info::miner_type(common::xminer_type_t const type) noexcept {
    v1().miner_type(type);
}

bool xtop_election_info::genesis() const noexcept {
    return v1().genesis();
}

void xtop_election_info::genesis(bool const v) noexcept {
    v1().genesis(v);
}

uint64_t xtop_election_info::raw_credit_score() const noexcept {
    return raw_credit_score_;
}

void xtop_election_info::raw_credit_score(uint64_t const score) noexcept {
    raw_credit_score_ = score;
}

NS_END4
