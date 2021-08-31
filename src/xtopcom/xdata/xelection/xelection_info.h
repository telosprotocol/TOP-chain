// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xcommon/xversion.h"
#include "xdata/xdata_common.h"
#include "xdata/xelection/xstandby_node_info.h"

#include <cstdint>

NS_BEG3(top, data, election)

class xtop_election_info final {
public:
    // xstandby_node_info_t standby_info{};
    common::xelection_round_t joined_version{};     // at which version of the group the node is added in
    std::uint64_t stake{ 0 };
    std::uint64_t comprehensive_stake{ 0 };
    xpublic_key_t consensus_public_key{};    // public key for consensus business

    bool
    operator==(xtop_election_info const & other) const noexcept;

    bool
    operator!=(xtop_election_info const & other) const noexcept;

    void
    swap(xtop_election_info & other) noexcept;
};
using xelection_info_t = xtop_election_info;

NS_END3
