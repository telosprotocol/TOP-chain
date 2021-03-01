// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "tests/xelection/xdummy_election_cache_data_accessor.h"

NS_BEG3(top, vnetwork, tests)

class xtop_dummy_election_data_accessor final : public top::tests::election::xdummy_election_cache_data_accessor_t {
    top::common::xnetwork_id_t m_network_id;

public:
    XDECLARE_DEFAULTED_COPY_AND_MOVE_SEMANTICS(xtop_dummy_election_data_accessor);
    XDECLARE_DEFAULTED_OVERRIDE_DESTRUCTOR(xtop_dummy_election_data_accessor);

    explicit
    xtop_dummy_election_data_accessor(top::common::xnetwork_id_t const & network_id);

    top::common::xnetwork_id_t
    network_id() const noexcept override;
};
using xdummy_election_data_accessor_t = xtop_dummy_election_data_accessor;

NS_END3
