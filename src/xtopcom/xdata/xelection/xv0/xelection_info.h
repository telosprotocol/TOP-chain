// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xcommon/xversion.h"

#include <cstdint>

NS_BEG4(top, data, election, v0)

class xtop_election_info final {
    common::xelection_epoch_t joined_version_{};  // at which version of the group the node is added in
    std::uint64_t stake_{ 0 };
    std::uint64_t comprehensive_stake_{ 0 };
    xpublic_key_t consensus_public_key_{};    // public key for consensus business

public:
    bool operator==(xtop_election_info const & other) const noexcept;
    bool operator!=(xtop_election_info const & other) const noexcept;

    void swap(xtop_election_info & other) noexcept;

    common::xelection_epoch_t const & joined_epoch() const noexcept;
    void joined_epoch(common::xelection_epoch_t joined_epoch);

    uint64_t stake() const noexcept;
    void stake(uint64_t stake) noexcept;

    uint64_t comprehensive_stake() const noexcept;
    void comprehensive_stake(uint64_t stake) noexcept;

    xpublic_key_t const & public_key() const noexcept;
    void public_key(xpublic_key_t pubkey) noexcept;
};
using xelection_info_t = xtop_election_info;

NS_END4
