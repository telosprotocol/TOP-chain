// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xns_macro.h"

#include <cassert>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

NS_BEG1(top)

/**
 * @brief Generic binary tree node. It holds a value with @relates ValueT
 *
 * @tparam ValueT Tree node value type.
 */
template <typename ValueT>
class xtop_tree_node {
public:
    using value_type = ValueT;

private:
    ValueT m_value{};
    std::shared_ptr<xtop_tree_node> m_left{};
    std::shared_ptr<xtop_tree_node> m_right{};
    std::weak_ptr<xtop_tree_node> m_parent{};

public:
    xtop_tree_node()                                   = default;
    xtop_tree_node(xtop_tree_node const &)             = default;
    xtop_tree_node & operator=(xtop_tree_node const &) = default;
    xtop_tree_node(xtop_tree_node &&)                  = default;
    xtop_tree_node & operator=(xtop_tree_node &&)      = default;
    virtual ~xtop_tree_node()                          = default;

    explicit
    xtop_tree_node(ValueT value) noexcept(std::is_nothrow_move_constructible<ValueT>::value)
        : m_value{ std::move(value) } {
    }

    xtop_tree_node(std::shared_ptr<xtop_tree_node<ValueT>> const & parent, ValueT value) noexcept(std::is_nothrow_move_constructible<ValueT>::value)
        : m_value{ std::move(value) }, m_parent{ parent } {
    }

    xtop_tree_node(std::shared_ptr<xtop_tree_node<ValueT>> left,
                   std::shared_ptr<xtop_tree_node<ValueT>> right,
                   ValueT value) noexcept(std::is_nothrow_move_constructible<ValueT>::value)
        : m_value{ std::move(value) }, m_left{ std::move(left) }, m_right{ std::move(right) } {
    }

    xtop_tree_node(std::shared_ptr<xtop_tree_node<ValueT>> const & parent,
                   std::shared_ptr<xtop_tree_node<ValueT>> left,
                   std::shared_ptr<xtop_tree_node<ValueT>> right,
                   ValueT value) noexcept(std::is_nothrow_move_constructible<ValueT>::value)
        : m_value{ std::move(value) }, m_left{ std::move(left) }
        , m_right{ std::move(right) }, m_parent{ parent } {
    }

    bool
    is_leaf() const noexcept {
        return m_left == nullptr && m_right == nullptr;
    }

    ValueT const &
    value() const noexcept {
        return m_value;
    }

    std::shared_ptr<xtop_tree_node> const &
    left() const noexcept {
        return m_left;
    }

    std::shared_ptr<xtop_tree_node> const &
    right() const noexcept {
        return m_right;
    }

    std::shared_ptr<xtop_tree_node>
    parent() const noexcept {
        return m_parent.lock();
    }

    void
    parent(std::shared_ptr<xtop_tree_node> const & p) noexcept {
        m_parent = p;
    }
};
template <typename ValueT>
using xtree_node_t = xtop_tree_node<ValueT>;

/**
 * @brief Generic tree type. Must be used as a base or be derived by @relates DerivedTreeT
 *
 * @tparam NodeT        Defines tree node type.
 * @tparam DerivedTreeT Defines the derived tree type.
 */
template <typename NodeT, typename DerivedTreeT>
class xtop_basic_tree {
public:
    using node_type = NodeT;
    using value_type = typename node_type::value_type;

protected:
    std::shared_ptr<node_type> m_root;

public:
    xtop_basic_tree()                                    = default;
    xtop_basic_tree(xtop_basic_tree const &)             = delete;
    xtop_basic_tree & operator=(xtop_basic_tree const &) = delete;
    xtop_basic_tree(xtop_basic_tree &&)                  = default;
    xtop_basic_tree & operator=(xtop_basic_tree &&)      = default;
    virtual ~xtop_basic_tree()                           = default;

    xtop_basic_tree(std::shared_ptr<node_type> root) noexcept : m_root{ std::move(root) } {
    }

    DerivedTreeT
    left_tree() const noexcept {
        assert(m_root);
        return DerivedTreeT{ m_root->left() };
    }

    DerivedTreeT
    right_tree() const noexcept {
        assert(m_root);
        return DerivedTreeT{ m_root->right() };
    }

    std::shared_ptr<node_type> const &
    root() const noexcept {
        assert(m_root);
        return m_root;
    }

    bool
    empty() const noexcept {
        return m_root == nullptr;
    }
};

template <typename TreeNodeT, typename DerivedTreeT>
using xbasic_tree_t = xtop_basic_tree<TreeNodeT, DerivedTreeT>;

NS_END1
