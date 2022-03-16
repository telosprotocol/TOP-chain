// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xcrypto_key.h"
#include "xcommon/xversion.h"
#include "xdata/xdata_common.h"
#include "xdata/xelection/xv1/xelection_info.h"
#include "xdata/xelection/xv1/xstandby_node_info.h"
#include "xdata/xelection/xv2/xstandby_node_info.h"

#include <cstdint>

NS_BEG4(top, data, election, v2)

class xtop_election_info final {
public:
    // xstandby_node_info_t standby_info{};
    common::xelection_round_t joined_version{};     // at which version of the group the node is added in
    std::uint64_t stake{ 0 };
    std::uint64_t comprehensive_stake{ 0 };
    xpublic_key_t consensus_public_key{};    // public key for consensus business
    common::xminer_type_t miner_type{common::xminer_type_t::invalid};
    bool genesis{false};

    bool
    operator==(xtop_election_info const & other) const noexcept;

    bool
    operator!=(xtop_election_info const & other) const noexcept;

    void
    swap(xtop_election_info & other) noexcept;

    v1::xelection_info_t v1() const;
};
using xelection_info_t = xtop_election_info;

NS_END4
