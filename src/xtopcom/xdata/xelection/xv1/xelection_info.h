// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcommon/xrole_type.h"
#include "xdata/xelection/xv0/xelection_info.h"


NS_BEG4(top, data, election, v1)

class xtop_election_info final {
    v0::xelection_info_t v0_{};

    common::xminer_type_t miner_type_{common::xminer_type_t::invalid};
    bool genesis_{false};

public:
    bool operator==(xtop_election_info const & other) const noexcept;
    bool operator!=(xtop_election_info const & other) const noexcept;

    void swap(xtop_election_info & other) noexcept;

    v0::xelection_info_t const & v0() const noexcept;

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

private:
    v0::xelection_info_t & v0() noexcept;
};
using xelection_info_t = xtop_election_info;

NS_END4
