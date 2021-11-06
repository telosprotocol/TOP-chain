// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xerror.h"
#include "xcommon/xaccount_base_address.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, common)

TEST(account_base_address, valid_construction_1) {
    std::error_code ec;
    top::common::xaccount_base_address_t account_base = top::common::xaccount_base_address_t::build_from("Ta0000", ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), "Ta0000");
}

TEST(account_base_address, valid_construction_2) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from("T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7", ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), "T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7");
    ASSERT_EQ(account_base.default_table_id().value(), 21);
}

TEST(account_base_address, valid_construction_3) {
    std::error_code ec;
    auto account_base = top::common::xaccount_base_address_t::build_from("T00000LMwyeixuLgjnDysRiStjffyzZtLmDoyBwV", ec);
    ASSERT_TRUE(!ec);
    ASSERT_EQ(account_base.to_string(), "T00000LMwyeixuLgjnDysRiStjffyzZtLmDoyBwV");
    ASSERT_EQ(account_base.default_table_id().value(), 63);
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
    auto account_base = top::common::xaccount_base_address_t::build_from("", ec);
    ASSERT_TRUE(ec);
    ASSERT_TRUE(account_base.empty());
    EXPECT_THROW(account_base.default_table_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_base.ledger_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_base.type(), top::error::xtop_error_t);
}

NS_END3
