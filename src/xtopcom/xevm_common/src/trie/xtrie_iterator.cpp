// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_iterator.h"

#include "xbasic/xmemory.hpp"
#include "xevm_common/trie/xtrie_db.h"
#include "xevm_common/trie/xtrie_encoding.h"

#include <utility>

NS_BEG3(top, evm_common, trie)

std::vector<xbytes_t> xtop_trie_simple_iterator::trie_leafs(xhash256_t const & trie_root_hash, observer_ptr<xtrie_db_t> const & trie_db) {
    std::vector<xbytes_t> leafs;

    get_trie_leafs(std::make_shared<xtrie_hash_node_t>(trie_root_hash), trie_db, leafs);

    return leafs;
}

void xtop_trie_simple_iterator::get_trie_leafs(std::shared_ptr<xtrie_node_face_t> const & node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs) {
    if (node == nullptr) {
        return;
    }

    switch (node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::hashnode: {
        get_hash_node_leafs(std::dynamic_pointer_cast<xtrie_hash_node_t>(node), trie_db, leafs);
        break;
    }

    case xtrie_node_type_t::shortnode: {
        get_short_node_leafs(std::dynamic_pointer_cast<xtrie_short_node_t>(node), trie_db, leafs);
        break;
    }

    case xtrie_node_type_t::fullnode: {
        get_full_node_leafs(std::dynamic_pointer_cast<xtrie_full_node_t>(node), trie_db, leafs);
        break;
    }

    case xtop_trie_node_type::valuenode: {
        get_value_node_leaf(std::dynamic_pointer_cast<xtrie_value_node_t>(node), trie_db, leafs);
        break;
    }

    default: {
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        unreachable();
    }
    }
}

void xtop_trie_simple_iterator::get_hash_node_leafs(std::shared_ptr<xtrie_hash_node_t> const & hash_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs) {
    auto const trie_node = trie_db->node(xhash256_t{hash_node->data()});
    if (trie_node == nullptr) {
        return;
    }

    get_trie_leafs(trie_node, trie_db, leafs);
}

void xtop_trie_simple_iterator::get_short_node_leafs(std::shared_ptr<xtrie_short_node_t> const & short_node,
                                                     observer_ptr<xtrie_db_t> const & trie_db,
                                                     std::vector<xbytes_t> & leafs) {
    assert(short_node);

    get_trie_leafs(short_node->val, trie_db, leafs);
}

void xtop_trie_simple_iterator::get_value_node_leaf(std::shared_ptr<xtrie_value_node_t> const & value_node,
                                                    observer_ptr<xtrie_db_t> const & trie_db,
                                                    std::vector<xbytes_t> & leafs) {
    assert(value_node);

    leafs.push_back(value_node->data());
}

void xtop_trie_simple_iterator::get_full_node_leafs(std::shared_ptr<xtrie_full_node_t> const & full_node, observer_ptr<xtrie_db_t> const & trie_db, std::vector<xbytes_t> & leafs) {
    for (auto const & child : full_node->children) {
        if (child != nullptr) {
            get_trie_leafs(child, trie_db, leafs);
        }
    }
}

//
//xtop_trie_iterator::xtop_trie_iterator(std::unique_ptr<xtrie_node_iterator_face_t> iter) : node_iterator_{std::move(iter)} {
//}
//
//bool xtop_trie_iterator::next() {
//    while (node_iterator_->next(true)) {
//        if (node_iterator_->leaf()) {
//            key_ = node_iterator_->leaf_key();
//            value_ = node_iterator_->leaf_blob();
//
//            return true;
//        }
//    }
//    key_.clear();
//    value_.clear();
//    return false;
//}
//
//xbytes_t xtop_trie_iterator::key() const {
//    return key_;
//}
//
//xbytes_t xtop_trie_iterator::value() const {
//    return value_;
//}
//
//std::error_code xtop_trie_iterator::error_code() const {
//    return ec_;
//}
//
//xtop_trie_node_iterator::xtop_trie_node_iterator(std::shared_ptr<xtrie_t> trie) : trie_{std::move(trie)} {
//    
//}
//
//xhash256_t xtop_trie_node_iterator::hash() const {
//    if (stack_.empty()) {
//        return xhash256_t{};
//    }
//
//    return stack_.back()->hash;
//}
//
//xhash256_t xtop_trie_node_iterator::parent_hash() const {
//    if (stack_.empty()) {
//        return xhash256_t{};
//    }
//
//    return stack_.back()->parent_hash;
//}
//
//bool xtop_trie_node_iterator::leaf() const noexcept {
//    return hasTerm(path_);
//}
//
//xbytes_t xtop_trie_node_iterator::leaf_key() const {
//    assert(!stack_.empty());
//
//    if (stack_.back()->node->type() == xtrie_node_type_t::valuenode) {
//        return hexToKeybytes(path_);
//    }
//
//    unreachable();
//}
//
//xbytes_t xtop_trie_node_iterator::leaf_blob() const {
//    assert(!stack_.empty());
//    assert(stack_.back()->node->type() == xtrie_node_type_t::valuenode);
//
//    return std::dynamic_pointer_cast<xtrie_value_node_t>(stack_.back()->node)->data();
//}
//
//std::vector<xbytes_t> xtop_trie_node_iterator::leaf_proof() const {
//    unreachable();
//}
//
//xbytes_t xtop_trie_node_iterator::path() const {
//    return path_;
//}
//
//xbytes_t xtop_trie_node_iterator::node_blob() const {
//    if (hash().empty()) {
//        return {};
//    }
//
//    unreachable();
//}
//
//bool xtop_trie_node_iterator::error() const {
//    unreachable();
//}
//
//bool xtop_trie_node_iterator::next(bool const descend) const {
//    return false;
//}
//
//void xtop_trie_node_iterator::seek(xbytes_t const & prefix, std::error_code & ec) {
//    auto key = keybytesToHex(prefix);
//    key = {std::begin(key), std::next(std::end(key), -1)};
//
//    // for (...)
//}
//
//std::tuple<std::unique_ptr<xtrie_node_iterator_state_t>, std::unique_ptr<int>, xbytes_t> xtop_trie_node_iterator::peek_seek(xbytes_t const & seek_key, std::error_code & ec) {
//    if (stack_.empty()) {
//        auto state = init(ec);
//        if (ec) {
//            return std::make_tuple<std::unique_ptr<xtrie_node_iterator_state_t>, std::unique_ptr<int>, xbytes_t>(nullptr, nullptr, {});
//        }
//    }
//
//    if (!xbytes_helper_t{seek_key}.start_with(path_)) {
//        pop();
//    }
//
//    while (!stack_.empty()) {
//        auto const & parent = stack_.back();
//        auto ancestor_hash = parent->hash;
//        if (ancestor_hash.empty()) {
//            ancestor_hash = parent->hash;
//        }
//
//        auto const & result = next_child_at(parent, ancestor_hash, seek_key, ec);
//    }
//}
//
//std::shared_ptr<xtrie_node_face_t> xtop_trie_node_iterator::resolve_hash(xhash256_t const & hash, std::error_code & ec) const {
//    return trie_->resolve_hash(hash, ec);
//}
//
//
//std::unique_ptr<xtrie_node_iterator_state_t> xtop_trie_node_iterator::init(std::error_code & ec) {
//    assert(!ec);
//
//    auto const & trie_root_hash = trie_->hash();
//
//    auto state = top::make_unique<xtrie_node_iterator_state_t>();
//    state->node = trie_->root();
//    state->index = static_cast<size_t>(-1);
//    if (trie_root_hash != empty_root) {
//        state->hash = trie_root_hash;
//    }
//
//    state->resolve(make_observer(this), ec);
//    if (ec) {
//        return nullptr;
//    }
//
//    return state;
//}
//
//void xtop_trie_node_iterator::pop() {
//    auto const & last = stack_.back();
//    path_ = {std::begin(path_), std::next(std::begin(path_), static_cast<ptrdiff_t>(last->path_length))};
//    stack_.pop_back();
//}
//
//
//void xtop_trie_node_iterator_state::resolve(observer_ptr<xtrie_node_iterator_t> const trie_node_iterator, std::error_code & ec) {
//    if (node->type() == xtrie_node_type_t::hashnode) {
//        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
//        assert(hash_node != nullptr);
//        auto const h = xhash256_t{hash_node->data()};
//
//        auto real_node = trie_node_iterator->resolve_hash(h, ec);
//        if (ec) {
//            return;
//        }
//
//        node = std::move(real_node);
//        hash = h;
//    }
//}


NS_END3
