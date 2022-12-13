// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xtree.hpp"
#include "xbasic/xutility.h"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <random>
#include <utility>

NS_BEG2(top, common)

using xstake_t = std::uint64_t;
template <typename AssociatedValueT>
class xtop_fts_tree_node final : public top::xtree_node_t<std::pair<xstake_t, AssociatedValueT>> {
    using base_t = top::xtree_node_t<std::pair<xstake_t, AssociatedValueT>>;

public:
    using value_type = typename base_t::value_type;

    xtop_fts_tree_node(xtop_fts_tree_node const &) = default;
    xtop_fts_tree_node & operator=(xtop_fts_tree_node const &) = default;
    xtop_fts_tree_node(xtop_fts_tree_node &&) = default;
    xtop_fts_tree_node & operator=(xtop_fts_tree_node &&) = default;
    ~xtop_fts_tree_node() override = default;

    explicit xtop_fts_tree_node(value_type value) noexcept : base_t{std::move(value)} {}

    xtop_fts_tree_node(std::shared_ptr<xtop_fts_tree_node> left, std::shared_ptr<xtop_fts_tree_node> right, value_type value) noexcept
      : base_t{std::move(left), std::move(right), std::move(value)} {}

    xstake_t stake() const noexcept { return top::get<xstake_t>(this->value()); }

    AssociatedValueT const & associated_value() const noexcept { return top::get<AssociatedValueT>(this->value()); }
};

template <typename AssociatedValueT>
using xfts_tree_node_t = xtop_fts_tree_node<AssociatedValueT>;

template <typename AssociatedValueT>
class xtop_fts_merkle_tree final : public xbasic_tree_t<xfts_tree_node_t<AssociatedValueT>, xtop_fts_merkle_tree<AssociatedValueT>> {
    using base_t = xbasic_tree_t<xfts_tree_node_t<AssociatedValueT>, xtop_fts_merkle_tree<AssociatedValueT>>;

public:
    using value_type = typename base_t::value_type;
    using node_type = typename base_t::node_type;

    xtop_fts_merkle_tree() = default;
    xtop_fts_merkle_tree(xtop_fts_merkle_tree const &) = delete;
    xtop_fts_merkle_tree & operator=(xtop_fts_merkle_tree const &) = delete;
    xtop_fts_merkle_tree(xtop_fts_merkle_tree &&) = default;
    xtop_fts_merkle_tree & operator=(xtop_fts_merkle_tree &&) = default;
    ~xtop_fts_merkle_tree() override = default;

    explicit xtop_fts_merkle_tree(std::shared_ptr<node_type> root) noexcept : base_t{std::move(root)} {}

    std::shared_ptr<node_type> select(std::uint64_t const seed) const {
        std::mt19937_64 prng{seed};
        return select(prng);
    }

    template <typename RNG>
    std::shared_ptr<node_type> select(RNG & prng) const {
        auto node = this->root();
        while (!node->is_leaf()) {
            auto left = std::dynamic_pointer_cast<node_type>(node->left());
            auto right = std::dynamic_pointer_cast<node_type>(node->right());
            assert(left != nullptr);

            auto const r = std::uniform_int_distribution<std::uint64_t>{0, node->stake()}(prng);
            node = (r <= left->stake()) ? left : right;
            assert(node != nullptr);
        }

        assert(node->is_leaf());
        return node;
    }

    static xtop_fts_merkle_tree construct(std::vector<value_type> const & data) {
        std::vector<std::shared_ptr<node_type>> nodes;
        nodes.reserve(data.size());
        for (auto const & datum : data) {
            nodes.push_back(std::make_shared<node_type>(datum));
        }

        if (nodes.empty()) {
            return {};
        }

        if (nodes.size() == 1) {
            return xtop_fts_merkle_tree{nodes.at(0)};
        }

        while (!nodes.empty()) {
            if (nodes.size() == 1) {
                break;
            }

            nodes = construct_impl(nodes);
        }

        assert(nodes.size() == 1);

        return xtop_fts_merkle_tree{nodes.at(0)};
    }

private:
    static value_type merge_stake(std::shared_ptr<node_type> const & lhs, std::shared_ptr<node_type> const & rhs) noexcept {
        if (lhs && rhs) {
            return {lhs->stake() + rhs->stake(), AssociatedValueT{}};
        }

        if (lhs) {
            return {lhs->stake(), AssociatedValueT{}};
        }

        if (rhs) {
            return {rhs->stake(), AssociatedValueT{}};
        }

        return {0, AssociatedValueT{}};
    }

    static std::vector<std::shared_ptr<node_type>> construct_impl(std::vector<std::shared_ptr<node_type>> const & nodes) {
        assert(!nodes.empty());

        std::vector<std::shared_ptr<node_type>> result;
        result.reserve(nodes.size() / 2 + 1);

        for (auto i = 0u; i <= nodes.size() / 2; ++i) {
            std::shared_ptr<node_type> parent_node{};

            auto const idx1 = 2 * i;
            auto const idx2 = idx1 + 1;

            if (idx2 < nodes.size()) {
                parent_node = std::make_shared<node_type>(nodes.at(idx1), nodes.at(idx2), merge_stake(nodes.at(idx1), nodes.at(idx2)));
            } else if (idx1 < nodes.size()) {
                parent_node = std::make_shared<node_type>(nodes.at(idx1), nullptr, merge_stake(nodes.at(idx1), nullptr));
            }

            if (parent_node != nullptr) {
                result.push_back(parent_node);
            }
        }

        return result;
    }
};

template<typename AssociatedValueT>
using xfts_merkle_tree_t = xtop_fts_merkle_tree<AssociatedValueT>;

template <typename AssociatedValueT, template <typename, typename...> class STLContainerT, typename... U>
std::vector<typename xfts_merkle_tree_t<AssociatedValueT>::value_type> select(STLContainerT<typename xfts_merkle_tree_t<AssociatedValueT>::value_type, U...> const & data, std::uint64_t const seed, std::size_t const count) {
    if (data.empty()) {
        return {};
    }

    std::vector<typename xfts_merkle_tree_t<AssociatedValueT>::value_type> tmp;
    tmp.reserve(data.size());

    for (auto const & datum : data) {
        tmp.push_back(datum);
    }

    if (data.size() <= count) {
        return tmp;
    }

    std::vector<typename xfts_merkle_tree_t<AssociatedValueT>::value_type> result;
    result.reserve(count);

    std::mt19937_64 prng{seed};
    // auto fts_merkel_tree = xfts_merkle_tree_t::construct(tmp);
    while (result.size() < count) {
        auto choosen_node = xfts_merkle_tree_t<AssociatedValueT>::construct(tmp).select(prng);
        // auto choosen_node = fts_merkel_tree.select(prng);
        assert(choosen_node != nullptr && choosen_node->is_leaf());

        result.push_back(choosen_node->value());
        tmp.erase(std::remove(std::begin(tmp), std::end(tmp), choosen_node->value()));

        // auto const it = std::find(std::begin(result), std::end(result), choosen_node->value());
        // if (it == std::end(result)) {
        //     result.push_back(choosen_node->value());
        // }
    }

    return result;
}

NS_END2
