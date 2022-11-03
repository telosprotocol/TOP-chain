// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xfts.h"
#include "xcommon/xnode_id.h"

#include <gtest/gtest.h>

#include <chrono>
#include <ctime>
#include <iostream>
#include <random>
#include <string>

constexpr std::size_t multiplying_factor{1};

template <std::size_t N>
static std::vector<top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::value_type> generate() {
    std::vector<top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::value_type> result;
    result.reserve(N);

    for (auto i = 0u; i < N; ++i) {
        top::common::xstake_t stake{i * multiplying_factor};
        top::common::xnode_id_t node_id{std::to_string(i)};

        result.push_back({stake, std::move(node_id)});
    }

    return result;
}

template <std::size_t N>
static std::vector<top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::value_type> generate_zero_stake() {
    std::vector<top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::value_type> result;
    result.reserve(N);

    for (auto i = 0u; i < N; ++i) {
        top::common::xnode_id_t node_id{std::to_string(i)};

        result.push_back({0, std::move(node_id)});
    }

    return result;
}

TEST(fts, small_merkle_tree) {
    constexpr std::size_t tree_leaf_count{10};
    auto                  data = generate<tree_leaf_count>();

    top::common::xstake_t total_stake{0};
    for (auto i = 0u; i < tree_leaf_count; ++i) {
        total_stake += static_cast<top::common::xstake_t>(i * multiplying_factor);
    }

    auto fts_merkel_tree = top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::construct(data);
    EXPECT_EQ(total_stake, fts_merkel_tree.root()->stake());
}

TEST(fts, hugh_merkle_tree) {
    constexpr std::size_t tree_leaf_count{10000};
    auto                  data = generate<tree_leaf_count>();

    top::common::xstake_t total_stake{0};
    for (auto i = 0u; i < tree_leaf_count; ++i) {
        total_stake += static_cast<top::common::xstake_t>(i * multiplying_factor);
    }

    auto fts_merkel_tree = top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::construct(data);
    EXPECT_EQ(total_stake, fts_merkel_tree.root()->stake());
}

TEST(fts, zero_merkle_tree) {
    constexpr std::size_t tree_leaf_count{0};

    auto fts_merkel_tree = top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::construct(generate<tree_leaf_count>());
    EXPECT_TRUE(fts_merkel_tree.empty());
}

TEST(fts, merkle_tree_select_single_leaf) {
    constexpr std::size_t   tree_leaf_count{1};
    auto                    data = generate<tree_leaf_count>();
    top::common::xstake_t total_stake{0};
    for (auto i = 0u; i < tree_leaf_count; ++i) {
        total_stake += static_cast<top::common::xstake_t>(i * multiplying_factor);
    }

    auto fts_merkle_tree = top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::construct(data);
    auto selected_node = fts_merkle_tree.select(0);

    EXPECT_EQ(total_stake, selected_node->stake());
    EXPECT_EQ(top::common::xnode_id_t{std::to_string(total_stake)}, selected_node->associated_value());
}

TEST(fts, select_zero) {
    constexpr std::size_t tree_leaf_count{10};
    auto                  data = generate<tree_leaf_count>();

    auto selected_result = top::common::select<top::common::xnode_id_t>(data, 0, 0);

    EXPECT_EQ(0, selected_result.size());
}

TEST(fts, select_all) {
    constexpr std::size_t tree_leaf_count{10};
    auto                  data = generate<tree_leaf_count>();

    auto selected_result = top::common::select<top::common::xnode_id_t>(data, 0, tree_leaf_count);

    EXPECT_EQ(data.size(), selected_result.size());

    std::sort(std::begin(selected_result),
              std::end(selected_result),
              [](top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::value_type const & lhs, top::common::xfts_merkle_tree_t<top::common::xnode_id_t>::value_type const & rhs) {
                  return top::get<top::common::xstake_t>(lhs) < top::get<top::common::xstake_t>(rhs);
              });

    EXPECT_EQ(data, selected_result);
}

TEST(fts, zero_stakes_select_half) {
    constexpr std::size_t tree_leaf_count{10};
    auto                  data = generate_zero_stake<tree_leaf_count>();

    auto selected_result = top::common::select<top::common::xnode_id_t>(data, 0, tree_leaf_count / 2);

    EXPECT_EQ(tree_leaf_count / 2, selected_result.size());
}

TEST(fts, select_possibility) {
    constexpr std::size_t round{1440};
    constexpr std::size_t select_count{21};
    constexpr std::size_t tree_leaf_count{512};
    auto                  data = generate<tree_leaf_count>();

    std::map<top::common::xnode_id_t, std::size_t> statistic_result;
    for (auto i = 0u; i < round; ++i) {
        auto selected_result = top::common::select<top::common::xnode_id_t>(
            data, static_cast<std::uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count()), select_count);

        EXPECT_EQ(select_count, selected_result.size());

        for (auto j = 0u; j < select_count; ++j) {
            ++statistic_result[top::get<top::common::xnode_id_t>(selected_result[j])];
        }
    }

    std::vector<std::pair<top::common::xnode_id_t, std::size_t>> results;
    for (auto const & result : statistic_result) {
        results.push_back(result);
    }

    std::sort(std::begin(results), std::end(results), [](std::pair<top::common::xnode_id_t, std::size_t> const & lhs, std::pair<top::common::xnode_id_t, std::size_t> const & rhs) {
        auto const lhs_count = top::get<std::size_t>(lhs);
        auto const rhs_count = top::get<std::size_t>(rhs);
        if (lhs_count != rhs_count) {
            return lhs_count > rhs_count;
        }

        auto const & lhs_node_id = top::get<top::common::xnode_id_t>(lhs);
        auto const & rhs_node_id = top::get<top::common::xnode_id_t>(rhs);

        if (lhs_node_id.to_string().length() != rhs_node_id.to_string().length()) {
            return lhs_node_id.value().length() > rhs_node_id.value().length();
        }

        return lhs_node_id > rhs_node_id;
    });

    std::size_t header_part_selected_count{0};
    for (auto i = tree_leaf_count; i > 0; --i) {
        auto const node_id_value = std::to_string(i - 1);
        auto const node_id = top::common::xnode_id_t{node_id_value};

        header_part_selected_count += statistic_result[node_id];

        std::cout << node_id.value() << u8"\tcount = " << statistic_result[node_id] << u8"\tpossibility = " << statistic_result[node_id] / static_cast<double>(round) * 100.0
                  << std::endl;
    }

    std::cout << std::endl
              << u8"header part possibility = " << std::setprecision(2) << std::fixed
              << (static_cast<double>(header_part_selected_count) / static_cast<double>(round * select_count)) * 100 << std::endl;
}
