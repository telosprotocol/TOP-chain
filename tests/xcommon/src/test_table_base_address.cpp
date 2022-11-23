// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xerror/xerror.h"
#include "xcommon/xerror/xerror.h"
#include "xcommon/xtable_base_address.h"

#include <gtest/gtest.h>

NS_BEG3(top, tests, common)

TEST(table_base_address, con) {
    std::error_code ec;
    top::common::xtable_base_address_t const con_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_consensus_index, ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ("Ta0000", con_base_address.to_string());
    ASSERT_EQ(base::enum_vaccount_addr_type_block_contract, con_base_address.type());
    ASSERT_EQ(base::enum_chain_zone_consensus_index, con_base_address.zone_index());
    ASSERT_EQ(base::enum_chain_zone_consensus_index, con_base_address.zone_id().value());
    ASSERT_FALSE(con_base_address.empty());
    ASSERT_EQ(top::common::xtable_base_address_t::table_base_address_length, con_base_address.to_string().length());  // table base address length is fixed, 6.
}

TEST(table_base_address, rec) {
    std::error_code ec;
    top::common::xtable_base_address_t const rec_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_beacon_index, ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ("Ta0001", rec_base_address.to_string());
    ASSERT_EQ(base::enum_vaccount_addr_type_block_contract, rec_base_address.type());
    ASSERT_EQ(base::enum_chain_zone_beacon_index, rec_base_address.zone_index());
    ASSERT_EQ(base::enum_chain_zone_beacon_index, rec_base_address.zone_id().value());
    ASSERT_FALSE(rec_base_address.empty());
    ASSERT_EQ(top::common::xtable_base_address_t::table_base_address_length, rec_base_address.to_string().length());  // table base address length is fixed, 6.
}

TEST(table_base_address, zec) {
    std::error_code ec;
    top::common::xtable_base_address_t const zec_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_zec_index, ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ("Ta0002", zec_base_address.to_string());
    ASSERT_EQ(base::enum_vaccount_addr_type_block_contract, zec_base_address.type());
    ASSERT_EQ(base::enum_chain_zone_zec_index, zec_base_address.zone_index());
    ASSERT_EQ(base::enum_chain_zone_zec_index, zec_base_address.zone_id().value());
    ASSERT_FALSE(zec_base_address.empty());
    ASSERT_EQ(top::common::xtable_base_address_t::table_base_address_length, zec_base_address.to_string().length());  // table base address length is fixed, 6.
}

TEST(table_base_address, evm) {
    std::error_code ec;
    top::common::xtable_base_address_t const evm_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_evm_index, ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ("Ta0004", evm_base_address.to_string());
    ASSERT_EQ(base::enum_vaccount_addr_type_block_contract, evm_base_address.type());
    ASSERT_EQ(base::enum_chain_zone_evm_index, evm_base_address.zone_index());
    ASSERT_EQ(base::enum_chain_zone_evm_index, evm_base_address.zone_id().value());
    ASSERT_FALSE(evm_base_address.empty());
    ASSERT_EQ(top::common::xtable_base_address_t::table_base_address_length, evm_base_address.to_string().length());  // table base address length is fixed, 6.
}

TEST(table_base_address, relay) {
    std::error_code ec;
    top::common::xtable_base_address_t const relay_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_relay_index, ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ("Ta0005", relay_base_address.to_string());
    ASSERT_EQ(base::enum_vaccount_addr_type_block_contract, relay_base_address.type());
    ASSERT_EQ(base::enum_chain_zone_relay_index, relay_base_address.zone_index());
    ASSERT_EQ(base::enum_chain_zone_relay_index, relay_base_address.zone_id().value());
    ASSERT_FALSE(relay_base_address.empty());
    ASSERT_EQ(top::common::xtable_base_address_t::table_base_address_length, relay_base_address.to_string().length());  // table base address length is fixed, 6.
}

TEST(table_base_address, xchain) {
    std::error_code ec;
    top::common::xtable_base_address_t const xchain_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_relay_block, base::enum_chain_zone_relay_index, ec);

    ASSERT_TRUE(!ec);
    ASSERT_EQ("Tb0005", xchain_base_address.to_string());
    ASSERT_EQ(base::enum_vaccount_addr_type_relay_block, xchain_base_address.type());
    ASSERT_EQ(base::enum_chain_zone_relay_index, xchain_base_address.zone_index());
    ASSERT_EQ(base::enum_chain_zone_relay_index, xchain_base_address.zone_id().value());
    ASSERT_FALSE(xchain_base_address.empty());
    ASSERT_EQ(top::common::xtable_base_address_t::table_base_address_length, xchain_base_address.to_string().length());  // table base address length is fixed, 6.
}

TEST(table_base_address, invalid_table_type) {
    std::error_code ec;
    top::common::xtable_base_address_t const table_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_secp256k1_eth_user_account, base::enum_chain_zone_relay_index, ec);

    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::invalid_table_type), ec.value());
    ASSERT_TRUE(table_base_address.empty());
    ASSERT_THROW(table_base_address.to_string(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_index(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_id(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.type(), top::error::xtop_error_t);

    ec.clear();
    ASSERT_TRUE(table_base_address.to_string(ec).empty());
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());

    ec.clear();
    auto const zidx = table_base_address.zone_index(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zidx));

    ec.clear();
    auto const zid = table_base_address.zone_id(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zid.value()));
}

TEST(table_base_address, invalid_zone_index1) {
    std::error_code ec;
    top::common::xtable_base_address_t const table_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_relay_block, base::enum_chain_zone_frozen_index, ec);

    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::invalid_zone_index), ec.value());
    ASSERT_TRUE(table_base_address.empty());
    ASSERT_THROW(table_base_address.to_string(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_index(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_id(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.type(), top::error::xtop_error_t);

    ec.clear();
    ASSERT_TRUE(table_base_address.to_string(ec).empty());
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());

    ec.clear();
    auto const zidx = table_base_address.zone_index(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zidx));

    ec.clear();
    auto const zid = table_base_address.zone_id(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zid.value()));
}

TEST(table_base_address, invalid_zone_index2) {
    std::error_code ec;
    top::common::xtable_base_address_t const table_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_relay_block, base::enum_chain_zone_fullnode_index, ec);

    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::invalid_zone_index), ec.value());
    ASSERT_TRUE(table_base_address.empty());
    ASSERT_THROW(table_base_address.to_string(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_index(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_id(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.type(), top::error::xtop_error_t);

    ec.clear();
    ASSERT_TRUE(table_base_address.to_string(ec).empty());
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());

    ec.clear();
    auto const zidx = table_base_address.zone_index(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zidx));

    ec.clear();
    auto const zid = table_base_address.zone_id(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zid.value()));
}

TEST(table_base_address, invalid_zone_index3) {
    std::error_code ec;
    top::common::xtable_base_address_t const table_base_address =
        top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_relay_block, static_cast<base::enum_xchain_zone_index>(17), ec);

    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::invalid_zone_index), ec.value());
    ASSERT_TRUE(table_base_address.empty());
    ASSERT_THROW(table_base_address.to_string(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_index(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.zone_id(), top::error::xtop_error_t);
    ASSERT_THROW(table_base_address.type(), top::error::xtop_error_t);

    ec.clear();
    ASSERT_TRUE(table_base_address.to_string(ec).empty());
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());

    ec.clear();
    auto const zidx = table_base_address.zone_index(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zidx));

    ec.clear();
    auto const zid = table_base_address.zone_id(ec);
    ASSERT_FALSE(!ec);
    ASSERT_EQ(static_cast<int>(top::common::error::xerrc_t::table_base_address_is_empty), ec.value());
    ASSERT_EQ(0, static_cast<int>(zid.value()));
}

TEST(table_base_address, to_string) {
    {
        top::common::xtable_base_address_t const con_base_address =
            top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_consensus_index);

        auto const & str1 = con_base_address.to_string();
        auto const & str2 = con_base_address.to_string();

        ASSERT_EQ(std::addressof(str1), std::addressof(str2));
    }
    {
        top::common::xtable_base_address_t const rec_base_address =
            top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_beacon_index);

        auto const & str1 = rec_base_address.to_string();
        auto const & str2 = rec_base_address.to_string();

        ASSERT_EQ(std::addressof(str1), std::addressof(str2));
    }
    {
        top::common::xtable_base_address_t const zec_base_address =
            top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_zec_index);

        auto const & str1 = zec_base_address.to_string();
        auto const & str2 = zec_base_address.to_string();

        ASSERT_EQ(std::addressof(str1), std::addressof(str2));
    }
    {
        top::common::xtable_base_address_t const eth_base_address =
            top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_evm_index);

        auto const & str1 = eth_base_address.to_string();
        auto const & str2 = eth_base_address.to_string();

        ASSERT_EQ(std::addressof(str1), std::addressof(str2));
    }
    {
        top::common::xtable_base_address_t const relay_base_address =
            top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_block_contract, base::enum_chain_zone_relay_index);

        auto const & str1 = relay_base_address.to_string();
        auto const & str2 = relay_base_address.to_string();

        ASSERT_EQ(std::addressof(str1), std::addressof(str2));
    }
    {
        top::common::xtable_base_address_t const cross_base_address =
            top::common::xtable_base_address_t::build_from(base::enum_vaccount_addr_type_relay_block, base::enum_chain_zone_relay_index);

        auto const & str1 = cross_base_address.to_string();
        auto const & str2 = cross_base_address.to_string();

        ASSERT_EQ(std::addressof(str1), std::addressof(str2));
    }
}

NS_END3
