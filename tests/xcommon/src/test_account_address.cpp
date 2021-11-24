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
    std::string const account_string{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"};
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{account_string});

    top::common::xaccount_address_t account_address{account_string};
    ASSERT_EQ(account_address.to_string(), account_string);
    ASSERT_EQ(account_address.table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
}

TEST(account_address, valid_construction_3) {
    {
        std::string const account_string{"T00000LVgLn3yVd11d2izvJg6znmxddxg8JEShoJ"};

        EXPECT_NO_THROW(top::common::xaccount_address_t account_address{account_string});

        top::common::xaccount_address_t account_address{account_string};
        ASSERT_EQ(account_address.to_string(), account_string);
        ASSERT_EQ(account_address.table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
        ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
    }
    {
        std::string const account_base_string{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"};
        std::string const account_string{account_base_string + "@32"};
        std::uint16_t table_id_value{32};

        top::common::xaccount_address_t account_address{account_string};

        ASSERT_FALSE(account_address.empty());
        EXPECT_EQ(account_address.table_id().value(), table_id_value);
        EXPECT_EQ(account_address.table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
        ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
        EXPECT_EQ(account_address.base_address().default_table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_base_string));
        EXPECT_EQ(account_address.type(), base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_eth_user_account);
    }

    {
        std::string const account_string{"T80000f1d16965a3f485af048ebcec8fd700dc92d54fa7"};
        std::uint16_t table_id_value{32};

        top::common::xaccount_address_t account_address{top::common::xaccount_base_address_t::build_from(account_string), table_id_value};

        ASSERT_FALSE(account_address.empty());
        EXPECT_EQ(account_address.table_id().value(), table_id_value);
        EXPECT_EQ(account_address.table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_address.value()));
        ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
        EXPECT_EQ(account_address.base_address().default_table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
        EXPECT_EQ(account_address.type(), base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_eth_user_account);
    }

    {
        std::string const account_string{"T00000LXWe8Z1CRrrMB54dVBH9mKn4AJukpEGi9j"};
        std::uint16_t table_id_value{3};

        top::common::xaccount_address_t account_address{top::common::xaccount_base_address_t::build_from(account_string), table_id_value};

        ASSERT_FALSE(account_address.empty());
        EXPECT_EQ(account_address.table_id().value(), table_id_value);
        EXPECT_EQ(account_address.table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_address.value()));
        ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
        EXPECT_EQ(account_address.base_address().default_table_id().value(), top::base::xvaccount_t::get_ledgersubaddr_from_account(account_string));
        EXPECT_EQ(account_address.type(), base::enum_vaccount_addr_type::enum_vaccount_addr_type_secp256k1_user_account);
    }
}

TEST(account_address, valid_construction_4) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{black_hole_addr});
    top::common::xaccount_address_t account_address{black_hole_addr};

    ASSERT_EQ(account_address.to_string(), black_hole_addr);
    ASSERT_TRUE(account_address.table_id().empty());
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(black_hole_addr));
}

TEST(account_address, valid_construction_5) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{genesis_root_addr_main_chain});
    top::common::xaccount_address_t account_address{genesis_root_addr_main_chain};

    ASSERT_EQ(account_address.to_string(), genesis_root_addr_main_chain);
    ASSERT_TRUE(account_address.table_id().empty());
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(genesis_root_addr_main_chain));
}

TEST(account_address, valid_construction_6) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{sys_contract_beacon_timer_addr});
    top::common::xaccount_address_t account_address{sys_contract_beacon_timer_addr};

    ASSERT_EQ(account_address.to_string(), sys_contract_beacon_timer_addr);
    ASSERT_TRUE(account_address.table_id().empty());
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(sys_contract_beacon_timer_addr));
}

TEST(account_address, valid_construction_7) {
    EXPECT_NO_THROW(top::common::xaccount_address_t account_address{sys_drand_addr});
    top::common::xaccount_address_t account_address{sys_drand_addr};

    ASSERT_EQ(account_address.to_string(), sys_drand_addr);
    ASSERT_TRUE(account_address.table_id().empty());
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(account_address.value()));
    ASSERT_EQ(account_address.ledger_id().value(), top::base::xvaccount_t::get_ledgerid_from_account(sys_drand_addr));
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
