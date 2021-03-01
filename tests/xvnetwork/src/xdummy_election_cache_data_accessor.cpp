// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "tests/xvnetwork/xdummy_election_cache_data_accessor.h"

NS_BEG3(top, vnetwork, tests)

xtop_dummy_election_data_accessor::xtop_dummy_election_data_accessor(top::common::xnetwork_id_t const & network_id)
    : m_network_id{ network_id } {
}

top::common::xnetwork_id_t
xtop_dummy_election_data_accessor::network_id() const noexcept {
    return m_network_id;
}

NS_END3
