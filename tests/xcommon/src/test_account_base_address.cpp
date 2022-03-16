// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xerror.h"
#include "xcommon/xaccount_base_address.h"
#include "xdata/xnative_contract_address.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, common)

TEST(account_base_address, valid_construction_1) {
    {
        std::error_code ec;
        top::common::xaccount_base_address_t account_base = top::common::xaccount_base_address_t::build_from("Ta0000", ec);
        ASSERT_TRUE(!ec);
        ASSERT_EQ(account_base.to_string(), "Ta0000");
        ASSERT_TRUE(account_base.default_table_id().empty());
        ASSERT_EQ(account_base.ledger_id().value(), 0);
    }

    {
        std::error_code ec;
        top::common::xaccount_base_address_t account_base = top::common::xaccount_base_address_t::build_from("Ta0001", ec);
        ASSERT_TRUE(!ec);
        ASSERT_EQ(account_base.to_string(), "Ta0001");
        ASSERT_TRUE(account_base.default_table_id().empty());
        ASSERT_EQ(account_base.ledger_id().value(), 1);
    }

    {
        std::error_code ec;
        top::common::xaccount_base_address_t account_base = top::common::xaccount_base_address_t::build_from("Ta0002", ec);
        ASSERT_TRUE(!ec);
        ASSERT_EQ(account_base.to_string(), "Ta0002");
        ASSERT_TRUE(account_base.default_table_id().empty());
        ASSERT_EQ(account_base.ledger_id().value(), 2);
    }
}

TEST(account_base_address, valid_construction_2) {
    std::string const account_string{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"};
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from(account_string, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), account_string);
    ASSERT_EQ(account_base.default_table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
    ASSERT_EQ(account_base.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_string));
}

TEST(account_base_address, valid_construction_3) {
    std::string const account_string{"T00000LMwyeixuLgjnDysRiStjffyzZtLmDoyBwV"};
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from(account_string, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), account_string);
    ASSERT_EQ(account_base.default_table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
    ASSERT_EQ(account_base.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_string));
}

TEST(account_base_address, valid_construction_4) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from(black_hole_addr, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), black_hole_addr);
    ASSERT_TRUE(account_base.default_table_id().empty());
    ASSERT_EQ(account_base.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(black_hole_addr));
}

TEST(account_base_address, valid_construction_5) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from(genesis_root_addr_main_chain, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), genesis_root_addr_main_chain);
    ASSERT_TRUE(account_base.default_table_id().empty());
    ASSERT_EQ(account_base.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(genesis_root_addr_main_chain));
}

TEST(account_base_address, valid_construction_6) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from(sys_contract_beacon_timer_addr, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), sys_contract_beacon_timer_addr);
    ASSERT_TRUE(account_base.default_table_id().empty());
    ASSERT_EQ(account_base.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(sys_contract_beacon_timer_addr));
}

TEST(account_base_address, valid_construction_7) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from(sys_drand_addr, ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), sys_drand_addr);
    ASSERT_TRUE(account_base.default_table_id().empty());
    ASSERT_EQ(account_base.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(sys_drand_addr));
}

TEST(account_base_address, invalid_construction_1) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from("", ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(account_base.empty());
    EXPECT_THROW(account_base.default_table_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_base.ledger_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_base.type(), top::error::xtop_error_t);

    EXPECT_THROW(top::common::xaccount_base_address_t::build_from(""), top::error::xtop_error_t);
}

TEST(account_base_address, invalid_construction_2) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from("TTTTTTTT", ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(account_base.empty());
    EXPECT_THROW(account_base.default_table_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_base.ledger_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_base.type(), top::error::xtop_error_t);

    EXPECT_THROW(top::common::xaccount_base_address_t::build_from("TTTTTTTT"), top::error::xtop_error_t);
}

TEST(account_base_address, invalid_construction_3) {
    {
        std::error_code ec;
        auto account_base = top::common::xaccount_base_address_t::build_from("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@21", ec);
        ASSERT_TRUE(ec);
        ASSERT_TRUE(account_base.empty());
        EXPECT_THROW(account_base.default_table_id().value(), top::error::xtop_error_t);
        EXPECT_THROW(account_base.ledger_id().value(), top::error::xtop_error_t);
        EXPECT_THROW(account_base.type(), top::error::xtop_error_t);

        EXPECT_THROW(top::common::xaccount_base_address_t::build_from("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@21"), top::error::xtop_error_t);
    }

    {
        std::error_code ec;
        auto account_base = top::common::xaccount_base_address_t::build_from("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@", ec);
        ASSERT_TRUE(ec);
        ASSERT_TRUE(account_base.empty());
        EXPECT_THROW(account_base.default_table_id().value(), top::error::xtop_error_t);
        EXPECT_THROW(account_base.ledger_id().value(), top::error::xtop_error_t);
        EXPECT_THROW(account_base.type(), top::error::xtop_error_t);

        EXPECT_THROW(top::common::xaccount_base_address_t::build_from("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@"), top::error::xtop_error_t);
    }
}

NS_END3
