// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xcommon/xversion.h"
#include "xdata/xelection/xv1/xelection_info.h"

#include <cstdint>

NS_BEG4(top, data, election, v2)

class xtop_election_info final {
private:
    v1::xelection_info_t v1_{};
    uint64_t raw_credit_score_{0};

public:
    bool operator==(xtop_election_info const & other) const noexcept;
    bool  operator!=(xtop_election_info const & other) const noexcept;

    void swap(xtop_election_info & other) noexcept;

    v1::xelection_info_t const & v1() const;

    common::xelection_epoch_t const & joined_epoch() const noexcept;
    void joined_epoch(common::xelection_epoch_t joined_epoch);

    uint64_t stake() const noexcept;
    void stake(uint64_t stake) noexcept;

    uint64_t comprehensive_stake() const noexcept;
    void comprehensive_stake(uint64_t stake) noexcept;

    xpublic_key_t const & public_key() const noexcept;
    void public_key(xpublic_key_t pubkey) noexcept;

    common::xminer_type_t miner_type() const noexcept;
    void miner_type(common::xminer_type_t type) noexcept;

    bool genesis() const noexcept;
    void genesis(bool v) noexcept;

    uint64_t raw_credit_score() const noexcept;
    void raw_credit_score(uint64_t score) noexcept;

private:
    v1::xelection_info_t & v1();
};
using xelection_info_t = xtop_election_info;

NS_END4
