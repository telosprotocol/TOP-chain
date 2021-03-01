// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbasic/xutility.h"
#include "xdata/xelection/xelection_group_result.h"
#include "xdata/xelection/xelection_info_bundle.h"
#include "xdata/xelection/xelection_info.h"

#include <gtest/gtest.h>

using namespace top::data::election;
using namespace top::common;

TEST(election_group_result, insert_and_reset_all) {
    xelection_group_result_t result;

    constexpr std::size_t result_size{ 100 };
    for (auto i = 0u; i < result_size; ++i) {
        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(xnode_id_t{ std::to_string(i) });
        election_info_bundle.election_info(xelection_info_t{});

        result.insert(std::move(election_info_bundle));
    }

    for (auto i = 0u; i < result_size; ++i) {
        auto const it = std::next(std::begin(result), i);
        auto const & slot_it = top::get<xslot_id_t const>(*it);

        EXPECT_EQ(xslot_id_t{ i }, slot_it);
    }

    for (auto i = 0u; i < result_size; i += 2) {
        result.reset(xnode_id_t{ std::to_string(i) });

        auto const it = std::next(std::begin(result), i);
        auto const & slot_it = top::get<xslot_id_t const>(*it);
        EXPECT_EQ(xslot_id_t{ i }, slot_it);

        auto const & bundle = top::get<xelection_info_bundle_t>(*it);
        EXPECT_TRUE(bundle.empty());
    }

    for (auto i = 0u; i < result_size; ++i) {
        auto const & r = result.find(xnode_id_t{ std::to_string(i) });

        auto const it = std::next(std::begin(result), i);
        auto const & slot_it = top::get<xslot_id_t const>(*it);
        EXPECT_EQ(xslot_id_t{ i }, slot_it);

        if (i % 2 == 0) {
            auto const & bundle = top::get<xelection_info_bundle_t>(*it);
            EXPECT_TRUE(bundle.empty());

            EXPECT_EQ(xslot_id_t{ 0 }, top::get<xslot_id_t>(r));
        } else {
            auto const & bundle = top::get<xelection_info_bundle_t>(*it);
            EXPECT_FALSE(bundle.empty());
        }
    }

    for (auto i = 0u; i < result_size; i += 2) {
        xelection_info_bundle_t election_info_bundle{};
        election_info_bundle.node_id(xnode_id_t{ std::to_string(i) });
        election_info_bundle.election_info(xelection_info_t{});

        auto r = result.insert(std::move(election_info_bundle));
        EXPECT_TRUE(top::get<bool>(r));

        auto const & slot_it = top::get<xslot_id_t const>(*top::get<xelection_group_result_t::iterator>(r));
        EXPECT_EQ(xslot_id_t{ i }, slot_it);
    }
}
