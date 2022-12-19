// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelection/xcache/xdata_accessor.h"
#include "tests/xelection/xdummy_chain_timer.h"

#include <gtest/gtest.h>

TEST(xtest_network_id, _) {
    top::common::xnetwork_id_t network_id{ top::common::xmax_network_id_value };
    top::election::cache::xdata_accessor_t data_accessor1{ network_id, top::make_observer(top::tests::election::xdummy_chain_timer) };
    ASSERT_TRUE(network_id == data_accessor1.network_id());

    top::election::cache::xdata_accessor_t data_accessor2{ top::common::xtestnet_id, top::make_observer(top::tests::election::xdummy_chain_timer) };
    ASSERT_EQ(top::common::xtestnet_id.value() & top::common::xbroadcast_network_id_value, data_accessor2.network_id().value());
}
