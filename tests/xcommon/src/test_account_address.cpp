// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xerror.h"
#include "xcommon/xaccount_address.h"
#include "xdata/xnative_contract_address.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, common)

TEST(account_address, valid_construction_1) {
    {
        EXPECT_NO_THROW(top::common::xaccount_address_t account_address{"Ta0000"});

        top::common::xaccount_address_t account_address{"Ta0000"};
        ASSERT_EQ(account_address.to_string(), "Ta0000");
        ASSERT_TRUE(account_address.table_id().empty());
    }

    {
        EXPECT_NO_THROW(top::common::xaccount_address_t account_address{"Ta0001"});

        top::common::xaccount_address_t account_address{"Ta0001"};
        ASSERT_EQ(account_address.to_string(), "Ta0001");
        ASSERT_TRUE(account_address.table_id().empty());
    }

    {
        EXPECT_NO_THROW(top::common::xaccount_address_t account_address{"Ta0002"});

        top::common::xaccount_address_t account_address{"Ta0002"};
        ASSERT_EQ(account_address.to_string(), "Ta0002");
        ASSERT_TRUE(account_address.table_id().empty());
    }
}

TEST(account_address, valid_construction_2) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"});

    top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"};
    ASSERT_EQ(account_address.to_string(), "T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7");
    ASSERT_EQ(account_address.table_id().value(), 21);
}

TEST(account_address, valid_construction_3) {
    {
        EXPECT_NO_THROW(top::common::xaccount_address_t account_address{"T00000LMwyeixuLgjnDysRiStjffyzZtLmDoyBwV"});

        top::common::xaccount_address_t account_address{"T00000LMwyeixuLgjnDysRiStjffyzZtLmDoyBwV"};
        ASSERT_EQ(account_address.to_string(), "T00000LMwyeixuLgjnDysRiStjffyzZtLmDoyBwV");
        ASSERT_EQ(account_address.table_id().value(), 63);
    }
    {
        top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@32"};

        ASSERT_FALSE(account_address.empty());
        EXPECT_EQ(account_address.table_id().value(), 32);
        EXPECT_EQ(account_address.base_address().default_table_id().value(), 21);
        EXPECT_EQ(account_address.type(), base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_eth_user_account);
    }
}

TEST(account_address, valid_construction_4) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{black_hole_addr});
    top::common::xaccount_address_t account_address{black_hole_addr};

    ASSERT_EQ(account_address.to_string(), black_hole_addr);
    ASSERT_TRUE(account_address.table_id().empty());
}

TEST(account_address, valid_construction_5) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{genesis_root_addr_main_chain});
    top::common::xaccount_address_t account_address{genesis_root_addr_main_chain};

    ASSERT_EQ(account_address.to_string(), genesis_root_addr_main_chain);
    ASSERT_TRUE(account_address.table_id().empty());
}

TEST(account_address, valid_construction_6) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{sys_contract_beacon_timer_addr});
    top::common::xaccount_address_t account_address{sys_contract_beacon_timer_addr};

    ASSERT_EQ(account_address.to_string(), sys_contract_beacon_timer_addr);
    ASSERT_TRUE(account_address.table_id().empty());
}

TEST(account_address, valid_construction_7) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{sys_drand_addr});
    top::common::xaccount_address_t account_address{sys_drand_addr};

    ASSERT_EQ(account_address.to_string(), sys_drand_addr);
    ASSERT_TRUE(account_address.table_id().empty());
}

TEST(account_address, invalid_construction_1) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{});
    top::common::xaccount_address_t account_address{};

    ASSERT_TRUE(account_address.empty());
    EXPECT_THROW(account_address.table_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_address.ledger_id().value(), top::error::xtop_error_t);
    EXPECT_THROW(account_address.type(), top::error::xtop_error_t);

    EXPECT_TRUE(account_address.table_id().empty());
    EXPECT_TRUE(account_address.ledger_id().empty());
}

TEST(account_address, invalid_construction_2) {
    EXPECT_THROW(top::common::xaccount_address_t account_address{"TXXXTXT"}, top::error::xtop_error_t);
    EXPECT_THROW(top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@"}, top::error::xtop_error_t);
    EXPECT_THROW(top::common::xaccount_address_t account_address{"@T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"}, top::error::xtop_error_t);
    EXPECT_THROW(top::common::xaccount_address_t account_address{"T80000f1d@16965a3f485af048ebcec8fd700dc92d54fa7"}, top::error::xtop_error_t);
    EXPECT_THROW(top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@64"}, top::error::xtop_error_t);
    EXPECT_THROW(top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7@-1"}, top::error::xtop_error_t);
    EXPECT_THROW(top::common::xaccount_address_t account_address{"T80000f1d16965a3f485af048ebcec8@fd700dc92d54fa7"}, top::error::xtop_error_t);
}

NS_END3
