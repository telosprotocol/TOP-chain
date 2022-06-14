// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xstrategy.hpp"

#include <gtest/gtest.h>

using namespace top::common;

using top::common::define_bool_strategy;
using top::common::xdefault_strategy_t;
using top::common::xnode_type_strategy_t;
using top::common::xnode_type_t;
using top::common::xuser_strategy_t;

using top::common::xstrategy_priority_enum_t;
using top::common::xstrategy_value_enum_t;

NS_BEG3(top, common, test)

TEST(xcommon, default_strategy) {
    auto strategy = define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::lowest});
    EXPECT_FALSE(strategy.allow(xnode_type_t::rec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::zec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_archive));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_exchange));
    EXPECT_FALSE(strategy.allow(xnode_type_t::edge));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_auditor));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_validator));
    EXPECT_FALSE(strategy.allow(xnode_type_t::frozen));
    EXPECT_FALSE(strategy.allow(xnode_type_t::invalid));
}

TEST(xcommon, node_type_strategy) {
    auto strategy = define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
                                         xnode_type_strategy_t{xnode_type_t::consensus_auditor, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal});

    EXPECT_FALSE(strategy.allow(xnode_type_t::rec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::zec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_archive));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_exchange));
    EXPECT_FALSE(strategy.allow(xnode_type_t::edge));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::consensus_validator));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_validator));
    EXPECT_FALSE(strategy.allow(xnode_type_t::frozen));
    EXPECT_FALSE(strategy.allow(xnode_type_t::invalid));
}

TEST(xcommon, node_type_strategy2) {
    auto strategy = define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
                                         xnode_type_strategy_t{xnode_type_t::consensus, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal},
                                         xnode_type_strategy_t{xnode_type_t::storage, xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::highest});

    EXPECT_FALSE(strategy.allow(xnode_type_t::rec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::zec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_archive));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_exchange));
    EXPECT_FALSE(strategy.allow(xnode_type_t::edge));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::consensus_validator));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_validator));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::storage_exchange));
    // EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::archive));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::storage_archive));
    EXPECT_FALSE(strategy.allow(xnode_type_t::frozen));
    EXPECT_FALSE(strategy.allow(xnode_type_t::invalid));
}

TEST(xcommon, user_strategy) {
    auto strategy = define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
                                         //  xuser_strategy_t{nullptr,xstrategy_priority_enum_t::highest}, // todo need to add uts after user_option finished
                                         xnode_type_strategy_t{xnode_type_t::consensus_auditor, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal});

    EXPECT_FALSE(strategy.allow(xnode_type_t::rec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::zec));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_archive));
    EXPECT_FALSE(strategy.allow(xnode_type_t::storage_exchange));
    EXPECT_FALSE(strategy.allow(xnode_type_t::edge));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_validator));
    EXPECT_FALSE(strategy.allow(xnode_type_t::frozen));
    EXPECT_FALSE(strategy.allow(xnode_type_t::invalid));
}

TEST(xcommon, test_unordered_strategy) {
    // ordered strategy.
    // auto strategy = define_bool_strategy(xdefault_strategy_t{xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::lowest},
    //                                      xnode_type_strategy_t{xnode_type_t::rec, xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low},
    //                                      xnode_type_strategy_t{xnode_type_t::zec, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal},
    //                                      xnode_type_strategy_t{xnode_type_t::consensus_auditor, xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::high},
    //                                      xnode_type_strategy_t{xnode_type_t::consensus_validator, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::highest});

    // make it unordered:
    auto strategy = define_bool_strategy(xnode_type_strategy_t{xnode_type_t::consensus_validator, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::highest},
                                         xnode_type_strategy_t{xnode_type_t::zec, xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::normal},
                                         xdefault_strategy_t{xstrategy_value_enum_t::enable, xstrategy_priority_enum_t::lowest},
                                         xnode_type_strategy_t{xnode_type_t::consensus_auditor, xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::high},
                                         xnode_type_strategy_t{xnode_type_t::rec, xstrategy_value_enum_t::disable, xstrategy_priority_enum_t::low});

    // defaulty: enable
    EXPECT_TRUE(strategy.allow(xnode_type_t::storage_archive));
    EXPECT_TRUE(strategy.allow(xnode_type_t::storage_exchange));
    EXPECT_TRUE(strategy.allow(xnode_type_t::frozen));
    EXPECT_TRUE(strategy.allow(xnode_type_t::invalid));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus | xnode_type_t::storage_exchange | xnode_type_t::storage_archive));

    // rec: disable
    EXPECT_FALSE(strategy.allow(xnode_type_t::rec));

    // zec: enable
    EXPECT_TRUE(strategy.allow(xnode_type_t::zec));
    EXPECT_TRUE(strategy.allow(xnode_type_t::zec | xnode_type_t::rec));

    // auditor: disable
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_auditor));
    EXPECT_FALSE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::zec | xnode_type_t::rec));

    // validator: enable
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_validator));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_validator | xnode_type_t::zec | xnode_type_t::rec));
    EXPECT_TRUE(strategy.allow(xnode_type_t::consensus_auditor | xnode_type_t::consensus_validator | xnode_type_t::zec | xnode_type_t::rec));
}

NS_END3