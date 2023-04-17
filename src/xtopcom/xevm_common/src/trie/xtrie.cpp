// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie.h"

#include "xbasic/xmemory.hpp"
#include "xevm_common/trie/xtrie_committer.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_hasher.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/trie/xtrie_pruner.h"
#include "xevm_common/xerror/xerror.h"
#include "xmetrics/xmetrics.h"

#include <cstddef>

NS_BEG3(top, evm_common, trie)

xh256_t const empty_root{empty_root_bytes};

xtop_trie::~xtop_trie() = default;

xtop_trie::xtop_trie(observer_ptr<xtrie_db_t> const db, xh256_t const & root_hash) : trie_db_{db}, original_root_hash_{root_hash} {
}

observer_ptr<xtrie_db_t> xtop_trie::trie_db() const noexcept {
    return trie_db_;
}

std::unique_ptr<xtop_trie> xtop_trie::build_from(xh256_t const & hash, observer_ptr<xtrie_db_t> const db, std::error_code & ec) {
    assert(!ec);

    if (db == nullptr) {
        xerror("build trie from null db");
        return nullptr;
    }

    auto trie = std::unique_ptr<xtop_trie>{new xtop_trie{db, hash}};
    if (hash != empty_root && !hash.empty()) {
        // resolve Hash
        auto root = trie->resolve_hash(hash, ec);
        if (ec) {
            xwarn("xtrie_t::build_from failed due to error %d msg %s", ec.value(), ec.message().c_str());
            return nullptr;
        }

        trie->trie_root_ = std::move(root);
    }

    xdbg("trie original root hash %s", hash.hex().c_str());
    return trie;
}

// Reset drops the referenced root node and cleans all internal state.
void xtop_trie::reset() {
    trie_root_ = nullptr;
    unhashed_ = 0;
}

// hash returns the root hash of the trie. It does not write to the
// database and can be used even if the trie doesn't have one.
xh256_t xtop_trie::hash() {
    auto result = hash_root();
    if (trie_root_.owner_before(result.second) || result.second.owner_before(trie_root_)) {
        trie_root_ = std::move(result.second);
    }

    if (result.first->type() == xtrie_node_type_t::hashnode) {
        assert(dynamic_cast<xtrie_hash_node_t *>(result.first.get()) != nullptr);
        return std::dynamic_pointer_cast<xtrie_hash_node_t>(result.first)->data();
    }

    unreachable();
}

// Get returns the value for key stored in the trie.
// The value bytes must not be modified by the caller.
xbytes_t xtop_trie::get(xbytes_t const & key) const {
    std::error_code ec;
    auto result = try_get(key, ec);
    if (ec) {
        xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
    }
    return result;
}

// TryGet returns the value for key stored in the trie.
// The value bytes must not be modified by the caller.
// If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
xbytes_t xtop_trie::try_get(xbytes_t const & key, std::error_code & ec) const {
    xassert(!ec);
    xbytes_t value;
    xtrie_node_face_ptr_t newroot;
    bool did_resolve;
    std::tie(value, newroot, did_resolve) = try_get(trie_root_, key_bytes_to_hex(key), 0, ec);

    return value;
}

// disable this API for now
//std::pair<xbytes_t, std::size_t> xtop_trie::try_get_node(xbytes_t const & path, std::error_code & ec) {
//    xassert(!ec);
//    xbytes_t item;
//    xtrie_node_face_ptr_t newroot;
//    std::size_t resolved;
//    std::tie(item, newroot, resolved) = try_get_node(trie_root_, compact_to_hex(path), 0, ec);
//    if (ec) {
//        return std::make_pair(xbytes_t{}, resolved);
//    }
//    if (resolved > 0) {
//        if (trie_root_.owner_before(newroot) || newroot.owner_before(trie_root_)) {
//            trie_root_ = newroot;
//        }
//    }
//    if (item.empty()) {
//        return std::make_pair(xbytes_t{}, resolved);
//    }
//    return std::make_pair(item, resolved);
//}

// Update associates key with value in the trie. Subsequent calls to
// Get will return value. If value has length zero, any existing value
// is deleted from the trie and calls to Get will return nil.
//
// The value bytes must not be modified by the caller while they are
// stored in the trie.
void xtop_trie::update(xbytes_t const & key, xbytes_t const & value) {
    // printf("update: %s %s\n", top::to_hex(key).c_str(), top::to_hex(value).c_str());
    std::error_code ec;
    try_update(key, value, ec);
    if (ec) {
        xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
    }
    return;
}

// TryUpdate associates key with value in the trie. Subsequent calls to
// Get will return value. If value has length zero, any existing value
// is deleted from the trie and calls to Get will return nil.
//
// The value bytes must not be modified by the caller while they are
// stored in the trie.
//
// If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
void xtop_trie::try_update(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) {
    xassert(!ec);
    unhashed_++;
    auto const k = key_bytes_to_hex(key);
    if (!value.empty()) {
        auto result = insert(trie_root_, {}, k, std::make_shared<xtrie_value_node_t>(value), ec);
        if (ec) {
            return;
        }

        if (trie_root_.owner_before(result.node) || result.node.owner_before(trie_root_)) {
            trie_root_ = std::move(result.node);
        }
    } else {
        auto result = erase(trie_root_, {}, k, ec);
        if (ec) {
            return;
        }

        if (trie_root_.owner_before(result.node) || result.node.owner_before(trie_root_)) {
            trie_root_ = std::move(result.node);
        }
    }
    return;
}

// Delete removes any existing value for key from the trie.
void xtop_trie::Delete(xbytes_t const & key) {
    std::error_code ec;
    try_delete(key, ec);
    if (ec) {
        xerror("trie error: %s %s", ec.category().name(), ec.message().c_str());
    }
    return;
}

// TryDelete removes any existing value for key from the trie.
// If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
void xtop_trie::try_delete(xbytes_t const & key, std::error_code & ec) {
    xassert(!ec);
    unhashed_++;
    auto const k = key_bytes_to_hex(key);
    auto result = erase(trie_root_, {}, k, ec);
    if (ec) {
        return;
    }

    if (trie_root_.owner_before(result.node) || result.node.owner_before(trie_root_)) {
        trie_root_ = std::move(result.node);
    }
}

// Commit writes all nodes to the trie's memory database, tracking the internal
// and external (for account tries) references.
std::pair<xh256_t, int32_t> xtop_trie::commit(std::error_code & ec) {
    assert(!ec);
    // todo leaf_callback
    if (trie_db_ == nullptr) {
        xerror("commit called on trie without database");
    }
    if (trie_root_ == nullptr) {
        return std::make_pair(empty_root, 0);
    }

    auto root_hash = hash();
    if (!trie_root_->cache().dirty()) {
        return std::make_pair(root_hash, 0);
    }

    xtrie_committer_t h;

    xtrie_hash_node_ptr_t new_root;
    int32_t committed;
    std::tie(new_root, committed) = h.Commit(trie_root_, trie_db_, ec);
    if (ec) {
        return std::make_pair(xh256_t{}, 0);
    }

    if (trie_root_.owner_before(new_root) || new_root.owner_before(trie_root_)) {
        trie_root_ = new_root;
    }

    return std::make_pair(root_hash, committed);
}

bool xtop_trie::prove(xbytes_t const & key, uint32_t from_level, xkv_db_face_ptr_t const & proof_db, std::error_code & ec) const {
    xassert(!ec);
    // Collect all nodes on the path to key.
    auto key_path = key_bytes_to_hex(key);
    std::vector<xtrie_node_face_ptr_t> nodes;
    for (auto tn = trie_root_; !key_path.empty() && tn != nullptr;) {
        xdbg("key: %s", top::to_hex(key_path).c_str());
        switch (tn->type()) {  // NOLINT(clang-diagnostic-switch-enum)
        case xtrie_node_type_t::shortnode: {
            auto n = std::dynamic_pointer_cast<xtrie_short_node_t>(tn);
            assert(n != nullptr);

            if (key_path.size() < n->key.size() || !std::equal(key_path.begin(), std::next(key_path.begin(), static_cast<std::ptrdiff_t>(n->key.size())), n->key.begin())) {
                // The trie doesn't contain the key.
                tn = nullptr;
            } else {
                tn = n->val;
                key_path = {key_path.begin() + static_cast<std::ptrdiff_t>(n->key.size()), key_path.end()};
            }
            nodes.push_back(n);
            xdbg("append shortnode %s", top::to_hex(key_path).c_str());
            break;
        }
        case xtrie_node_type_t::fullnode: {
            auto n = std::dynamic_pointer_cast<xtrie_full_node_t>(tn);
            assert(n != nullptr);
            tn = n->children[key_path[0]];
            key_path.erase(key_path.begin());
            nodes.push_back(n);
            xdbg("append fullnode %s", top::to_hex(key_path).c_str());
            break;
        }
        case xtrie_node_type_t::hashnode: {
            auto n = std::dynamic_pointer_cast<xtrie_hash_node_t>(tn);
            assert(n != nullptr);
            tn = resolve_hash(n, ec);
            if (ec) {
                xerror("unhandled trie error %s", ec.message().c_str());
                return false;
            }
            break;
        }
        default: {
            xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        }
        }
    }

    auto hasher = xtrie_hasher_t::newHasher(false);
    xdbg("nodes.size():%zu", nodes.size());
    for (std::size_t index = 0; index < nodes.size(); ++index) {
        if (from_level > 0) {
            from_level--;
            continue;
        }
        auto n = nodes[index];
        xtrie_node_face_ptr_t hn;
        std::tie(n, hn) = hasher.proofHash(n);
        if (hn->type() == xtrie_node_type_t::hashnode || index == 0) {
            // If the node's database encoding is a hash (or is the
            // root node), it becomes a proof element.
            auto enc = xtrie_node_rlp::EncodeToBytes(n);
            xtrie_hash_node_ptr_t hash;
            if (hn->type() != xtrie_node_type_t::hashnode) {
                hash = hasher.hashData(enc);
            } else {
                hash = std::dynamic_pointer_cast<xtrie_hash_node_t>(hn);
                assert(hash != nullptr);
            }
            xdbg("[Prove]: put into db <%s> %s", top::to_hex(hash->data()).c_str(), top::to_hex(enc).c_str());
            proof_db->Put(hash->data(), enc, ec);
        }
    }

    return true;
}

std::tuple<xbytes_t, xtrie_node_face_ptr_t, bool> xtop_trie::try_get(xtrie_node_face_ptr_t const & node, xbytes_t const & key, std::size_t const pos, std::error_code & ec) const {
    xdbg("tryGet key: %s ,pos: %zu", top::to_hex(key).c_str(), pos);
    if (node == nullptr) {
        return std::make_tuple(xbytes_t{}, nullptr, false);
    }
    assert(node != nullptr);
    switch (node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::invalid: {
        return std::make_tuple(xbytes_t{}, nullptr, false);
    }
    case xtrie_node_type_t::valuenode: {
        auto n = std::dynamic_pointer_cast<xtrie_value_node_t>(node);
        assert(n != nullptr);
        return std::make_tuple(n->data(), n, false);
    }
    case xtrie_node_type_t::shortnode: {
        auto n = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(n != nullptr);
        if ((key.size() - pos < n->key.size()) || (!std::equal(n->key.begin(), n->key.end(), std::next(key.begin(), static_cast<std::ptrdiff_t>(pos))))) {
            // key not found in trie
            return std::make_tuple(xbytes_t{}, n, false);
        }
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool did_resolve;
        std::tie(value, newnode, did_resolve) = try_get(n->val, key, pos + n->key.size(), ec);
        if (!ec && did_resolve) {
            n = n->clone();
            n->val = newnode;
        }
        return std::make_tuple(value, n, did_resolve);
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(n != nullptr);
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool did_resolve;
        std::tie(value, newnode, did_resolve) = try_get(n->children[key[pos]], key, pos + 1, ec);
        if (!ec && did_resolve) {
            n = n->clone();
            n->children[key[pos]] = newnode;
        }
        return std::make_tuple(value, n, did_resolve);
    }
    case xtrie_node_type_t::hashnode: {
        auto n = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
        assert(n != nullptr);
        auto const child = resolve_hash(n, ec);
        if (ec) {
            xwarn("resolve hash error: at key: %s pos:%zu", top::to_hex(key).c_str(), pos);
            return std::make_tuple(xbytes_t{}, n, true);
        }
        xbytes_t value;
        xtrie_node_face_ptr_t newnode;
        bool did_resolve;
        std::tie(value, newnode, did_resolve) = try_get(child, key, pos, ec);
        return std::make_tuple(value, newnode, true);
    }
    default: {
        xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        __builtin_unreachable();
    }
    }
}

std::tuple<xbytes_t, xtrie_node_face_ptr_t, std::size_t> xtop_trie::try_get_node(xtrie_node_face_ptr_t const & orig_node,
                                                                                 xbytes_t const & path,
                                                                                 std::size_t const pos,
                                                                                 std::error_code & ec) const {
    xdbg("tryGetNode path: %s, pos: %zu", top::to_hex(path).c_str(), pos);
    // If non-existent path requested, abort
    if (orig_node == nullptr) {
        return std::make_tuple(xbytes_t{}, nullptr, 0);
    }
    // If we reached the requested path, return the current node
    if (pos >= path.size()) {
        // Although we most probably have the original node expanded, encoding
        // that into consensus form can be nasty (needs to cascade down) and
        // time consuming. Instead, just pull the hash up from disk directly.

        std::shared_ptr<xtrie_hash_node_t> hash;
        if (orig_node->type() == xtrie_node_type_t::hashnode) {
            assert(dynamic_cast<xtrie_hash_node_t *>(orig_node.get()) != nullptr);
            hash = std::dynamic_pointer_cast<xtrie_hash_node_t>(orig_node);
        } else {
            hash = orig_node->cache().hash_node();
        }
        if (hash == nullptr) {
            ec = error::xerrc_t::trie_node_unexpected;
            return std::make_tuple(xbytes_t{}, orig_node, 0);
        }
        auto blob = trie_db_->Node(hash->data(), ec);
        return std::make_tuple(blob, orig_node, 1);
    }
    // Path still needs to be traversed, descend into children
    assert(orig_node != nullptr);
    switch (orig_node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::valuenode: {
        // Path prematurely ended, abort
        return std::make_tuple(xbytes_t{}, nullptr, 0);
    }
    case xtrie_node_type_t::shortnode: {
        auto n = std::dynamic_pointer_cast<xtrie_short_node_t>(orig_node);
        assert(n != nullptr);

        if ((path.size() - pos < n->key.size()) || (!std::equal(n->key.begin(), n->key.end(), std::next(path.begin(), static_cast<std::ptrdiff_t>(pos))))) {
            // Path branches off from short node
            return std::make_tuple(xbytes_t{}, n, 0);
        }
        xbytes_t item;
        xtrie_node_face_ptr_t newnode;
        std::size_t resolved;
        std::tie(item, newnode, resolved) = try_get_node(n->val, path, pos + n->key.size(), ec);
        if (!ec && resolved > 0) {
            n = n->clone();
            n->val = newnode;
        }
        return std::make_tuple(item, n, resolved);
    }
    case xtrie_node_type_t::fullnode: {
        auto n = std::dynamic_pointer_cast<xtrie_full_node_t>(orig_node);
        assert(n != nullptr);

        xbytes_t item;
        xtrie_node_face_ptr_t newnode;
        std::size_t resolved;
        std::tie(item, newnode, resolved) = try_get_node(n->children[path[pos]], path, pos + 1, ec);
        if (!ec && resolved > 0) {
            n = n->clone();
            n->children[path[pos]] = newnode;
        }
        return std::make_tuple(item, n, resolved);
    }
    case xtrie_node_type_t::hashnode: {
        auto n = std::dynamic_pointer_cast<xtrie_hash_node_t>(orig_node);
        assert(n != nullptr);

        auto const child = resolve_hash(n, ec);
        if (ec) {
            return std::make_tuple(xbytes_t{}, n, 1);
        }

        xbytes_t item;
        xtrie_node_face_ptr_t newnode;
        std::size_t resolved;
        std::tie(item, newnode, resolved) = try_get_node(child, path, pos, ec);
        return std::make_tuple(item, newnode, resolved + 1);
    }
    default: {
        xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        __builtin_unreachable();
    }
    }
}

xtop_trie::update_result::update_result(bool const updated, std::shared_ptr<xtrie_node_face_t> n) noexcept : node{std::move(n)}, dirty{updated} {
}

xtop_trie::update_result xtop_trie::insert(xtrie_node_face_ptr_t const & node, xbytes_t prefix, xbytes_t key, xtrie_node_face_ptr_t const & value, std::error_code & ec) {
    if (key.empty()) {
        if (node && node->type() == xtrie_node_type_t::valuenode) {
            assert(std::dynamic_pointer_cast<xtrie_value_node_t>(node) != nullptr);
            assert(std::dynamic_pointer_cast<xtrie_value_node_t>(value) != nullptr);

            return {std::dynamic_pointer_cast<xtrie_value_node_t>(node)->data() != std::dynamic_pointer_cast<xtrie_value_node_t>(value)->data(), value};
        }
        return {true, value};
    }

    if (node == nullptr) {
        return {true, std::make_shared<xtrie_short_node_t>(key, value, node_dirty())};
    }

    assert(node != nullptr);
    switch (node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::shortnode: {
        auto short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(short_node != nullptr);
        auto const matchlen = prefix_len(key, short_node->key);
        // If the whole key matches, keep this short node as is
        // and only update the value.
        if (matchlen == short_node->key.size()) {
            auto key_break_pos = std::next(key.begin(), static_cast<std::ptrdiff_t>(matchlen));

            prefix.insert(prefix.end(), key.begin(), key_break_pos);
            key = {key_break_pos, key.end()};

            auto result = insert(short_node->val, prefix, key, value, ec);
            if (!result.dirty || ec) {
                return {false, short_node};
            }
            return {true, std::make_shared<xtrie_short_node_t>(short_node->key, std::move(result.node), node_dirty())};
        }

        // Otherwise branch out at the index where they differ.
        auto branch = std::make_shared<xtrie_full_node_t>(node_dirty());
        {
            auto const short_node_key_break_pos = std::next(short_node->key.begin(), static_cast<std::ptrdiff_t>(matchlen) + 1);

            auto sprefix = prefix;
            sprefix.insert(sprefix.end(), short_node->key.begin(), short_node_key_break_pos);
            auto skey = xbytes_t{short_node_key_break_pos, short_node->key.end()};

            auto result = insert(nullptr, sprefix, skey, short_node->val, ec);
            branch->children[short_node->key[matchlen]] = std::move(result.node);
            if (ec) {
                return {false, nullptr};
            }
        }
        {
            auto const key_break_pos = std::next(key.begin(), static_cast<std::ptrdiff_t>(matchlen) + 1);

            auto sprefix = prefix;
            sprefix.insert(sprefix.end(), key.begin(), key_break_pos);
            auto const skey = xbytes_t{key_break_pos, key.end()};
            auto result = insert(nullptr, sprefix, skey, value, ec);
            branch->children[key[matchlen]] = std::move(result.node);
            if (ec) {
                return {false, nullptr};
            }
        }
        // Replace this shortNode with the branch if it occurs at index 0.
        if (matchlen == 0) {
            return {true, branch};
        }
        // Otherwise, replace it with a short node leading up to the branch.
        xbytes_t short_key = {key.begin(), std::next(key.begin(), static_cast<std::ptrdiff_t>(matchlen))};
        return {true, std::make_shared<xtrie_short_node_t>(short_key, branch, node_dirty())};
    }
    case xtrie_node_type_t::fullnode: {
        auto full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(full_node != nullptr);

        xbyte_t tkey = key[0];
        prefix.insert(prefix.end(), tkey);
        key.erase(key.begin());
        auto result = insert(full_node->children[tkey], prefix, key, value, ec);
        if (!result.dirty || ec) {
            return {false, full_node};
        }

        full_node = full_node->clone();
        full_node->flags = node_dirty();
        full_node->children[tkey] = std::move(result.node);

        return {true, full_node};
    }
    case xtrie_node_type_t::invalid: {
        return {true, std::make_shared<xtrie_short_node_t>(key, value, node_dirty())};
    }
    case xtrie_node_type_t::hashnode: {
        // We've hit a part of the trie that isn't loaded yet. Load
        // the node and insert into it. This leaves all child nodes on
        // the path to the value in the trie.
        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
        assert(hash_node != nullptr);
        auto real_node = resolve_hash(hash_node, ec);
        if (ec) {
            assert(real_node == nullptr);
            return {false, real_node};
        }
        auto result = insert(real_node, prefix, key, value, ec);
        if (!result.dirty || ec) {
            if (ec) {
                xwarn("xtrie_t::insert() failed. code %d msg %s prefix %s key %s",
                      ec.value(),
                      ec.message().c_str(),
                      std::string{std::begin(prefix), std::end(prefix)}.c_str(),
                      std::string{std::begin(key), std::end(key)}.c_str());
                assert(result.node == nullptr);
            }
            return {false, real_node};
        }

        pending_to_be_pruned_.emplace_back(hash_node->data());
        xdbg("insert: prunne hash %s", hash_node->data().hex().c_str());

        return {true, std::move(result.node)};
    }
    default: {
        xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        __builtin_unreachable();
    }
    }
}

xtop_trie::update_result xtop_trie::erase(xtrie_node_face_ptr_t const & node, xbytes_t const & prefix, xbytes_t key, std::error_code & ec) {
    // printf("erase: prefix: %s key: %s\n", top::to_hex(prefix).c_str(), top::to_hex(key).c_str());
    if (node == nullptr) {
        return {false, nullptr};
    }
    assert(node != nullptr);

    switch (node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::shortnode: {
        // printf("erase node_type short\n");
        auto short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(short_node != nullptr);
        auto const matchlen = prefix_len(key, short_node->key);
        if (matchlen < short_node->key.size()) {
            return {false, short_node};  // don't replace n on mismatch
        }
        if (matchlen == key.size()) {
            return {true, nullptr};  // remove n entirely for whole matches
        }
        // The key is longer than n.Key. Remove the remaining suffix
        // from the subtrie. Child can never be nil here since the
        // subtrie must contain at least two other values with keys
        // longer than n.Key.
        xbytes_t child_prefix = prefix;
        auto const key_break_pos = std::next(key.begin(), static_cast<std::ptrdiff_t>(short_node->key.size()));
        child_prefix.insert(child_prefix.end(), key.begin(), key_break_pos);
        xbytes_t const child_key = {key_break_pos, key.end()};
        auto const result = erase(short_node->val, child_prefix, child_key, ec);
        if (!result.dirty || ec) {
            return {false, short_node};
        }

        switch (result.node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
        case xtrie_node_type_t::shortnode: {
            // Deleting from the subtrie reduced it to another
            // short node. Merge the nodes to avoid creating a
            // shortNode{..., shortNode{...}}. Use concat (which
            // always creates a new slice) instead of append to
            // avoid modifying n.Key since it might be shared with
            // other nodes.
            auto const child_node = std::dynamic_pointer_cast<xtrie_short_node_t>(result.node);
            assert(child_node != nullptr);

            xbytes_t cchild_key = short_node->key;
            cchild_key.insert(cchild_key.end(), child_node->key.begin(), child_node->key.end());
            return {true, std::make_shared<xtrie_short_node_t>(cchild_key, child_node->val, node_dirty())};
        }
        default: {
            return {true, std::make_shared<xtrie_short_node_t>(short_node->key, result.node, node_dirty())};
        }
        }
    }

    case xtrie_node_type_t::fullnode: {
        // printf("erase node_type full\n");
        auto full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(full_node != nullptr);

        xbyte_t const tkey = key[0];
        xbytes_t nprefix = prefix;
        nprefix.insert(nprefix.end(), tkey);
        key.erase(key.begin());
        auto const result = erase(full_node->children[tkey], nprefix, key, ec);
        if (!result.dirty || ec) {
            return {false, full_node};
        }

        full_node = full_node->clone();
        full_node->flags = node_dirty();
        full_node->children[tkey] = result.node;

        // Because n is a full node, it must've contained at least two children
        // before the delete operation. If the new child value is non-nil, n still
        // has at least two children after the deletion, and cannot be reduced to
        // a short node.
        if (result.node != nullptr) {
            assert(std::count_if(std::begin(full_node->children), std::end(full_node->children), [](std::shared_ptr<xtrie_node_face_t> const & child) { return child != nullptr; }) >= 2);
            return {true, full_node};
        }

        // Reduction:
        // Check how many non-nil entries are left after deleting and
        // reduce the full node to a short node if only one entry is
        // left. Since n must've contained at least two children
        // before deletion (otherwise it would not be a full node) n
        // can never be reduced to nil.
        //
        // When the loop is done, pos contains the index of the single
        // value that is left in n or -2 if n contains at least two
        // values.
        int32_t pos = -1;
        int32_t ind = 0;
        for (auto const & cld : full_node->children) {
            if (cld != nullptr) {
                if (pos == -1) {
                    pos = ind;
                } else {
                    pos = -2;
                    break;
                }
            }
            ind++;
        }
        if (pos >= 0) {
            if (pos != 16) {
                auto const cnode = resolve(full_node->children[pos], /*prefix,*/ ec);
                if (ec) {
                    return {false, nullptr};
                }
                if (cnode->type() == xtrie_node_type_t::shortnode) {
                    auto const short_child_node = std::dynamic_pointer_cast<xtrie_short_node_t>(cnode);
                    xbytes_t k = short_child_node->key;
                    k.insert(k.begin(), xbyte_t{static_cast<xbyte_t>(pos)});
                    return {true, std::make_shared<xtrie_short_node_t>(k, short_child_node->val, node_dirty())};
                }
            }

            // Otherwise, n is replaced by a one-nibble short node
            // containing the child.
            xbytes_t k{static_cast<xbyte_t>(pos)};
            return {true, std::make_shared<xtrie_short_node_t>(k, full_node->children[pos], node_dirty())};
        }
        // n still contains at least two values and cannot be reduced.
        return {true, full_node};
    }
    case xtrie_node_type_t::valuenode: {
        // printf("erase node_type value\n");
        return {true, nullptr};
    }
    case xtrie_node_type_t::invalid: {
        // printf("erase node_type invalid\n");
        return {false, nullptr};
    }
    case xtrie_node_type_t::hashnode: {
        // printf("erase node_type hash\n");
        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(node);
        assert(hash_node != nullptr);

        auto real_node = resolve_hash(hash_node, /*prefix,*/ ec);
        if (ec) {
            return {false, nullptr};
        }

        auto result = erase(real_node, prefix, key, ec);
        if (!result.dirty || ec) {
            return {false, real_node};
        }
        return {true, std::move(result.node)};
    }
    default:
        xassert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        __builtin_unreachable();
    }
}

xtrie_node_face_ptr_t xtop_trie::resolve(xtrie_node_face_ptr_t const & n, /*xbytes_t prefix,*/ std::error_code & ec) const {
    if (n->type() == xtrie_node_type_t::hashnode) {
        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(n);
        assert(hash_node != nullptr);

        return resolve_hash(hash_node /*, prefix*/, ec);
    }
    return n;
}

xtrie_node_face_ptr_t xtop_trie::resolve_hash(xtrie_hash_node_ptr_t const & n, /*xbytes_t prefix,*/ std::error_code & ec) const {
    assert(!ec);

    return resolve_hash(n->data(), ec);
}

xtrie_node_face_ptr_t xtop_trie::resolve_hash(xh256_t const & hash, std::error_code & ec) const {
    assert(!ec);

    auto node = trie_db_->node(hash);
    if (!node) {
        ec = error::xerrc_t::trie_db_missing_node_error;
        xwarn("xtrie_t::resolve_hash failed %s", hash.hex().c_str());
        return nullptr;
    }
    return node;
}

xbytes_t xtop_trie::resolve_blob(std::shared_ptr<xtrie_hash_node_t> const & n, std::error_code & ec) const {
    auto const & hash = n->data();
    return trie_db_->Node(hash, ec);
}

// hashRoot calculates the root hash of the given trie
std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> xtop_trie::hash_root() {
    if (trie_root_ == nullptr) {
        return std::make_pair(std::make_shared<xtrie_hash_node_t>(empty_root), nullptr);
    }

    auto hasher = xtrie_hasher_t::newHasher(unhashed_ >= 100u);

    auto hash_result = hasher.hash(trie_root_, true);
    unhashed_ = 0;
    return hash_result;
}

xnode_flag_t xtop_trie::node_dirty() {
    return xnode_flag_t{true};
}

void xtop_trie::prune(xh256_t const & old_trie_root_hash, std::unordered_set<xh256_t> & pruned_hashes, std::error_code & ec) {
    assert(!ec);

    if (pruner_ == nullptr) {
        pruner_ = top::make_unique<xtrie_pruner_t>();
        pruner_->init(trie_root_, trie_db_, ec);
    }

    pruner_->prune(old_trie_root_hash, trie_db_, pruned_hashes, ec);
}

void xtop_trie::commit_pruned(std::unordered_set<xh256_t> const & pruned_hashes, std::error_code & ec) {
    assert(!ec);
    assert(pruner_);

    trie_db_->commit_pruned(pruned_hashes, ec);
    if (ec) {
        xwarn("commit prune failed");
        return;
    }

    pruner_.reset();
}

void xtop_trie::prune(std::error_code & ec) {
    assert(!ec);

    if (pending_to_be_pruned_.empty() || pending_to_be_pruned_.back() != original_root_hash_) {
        pending_to_be_pruned_.emplace_back(original_root_hash_);
        xdbg("prune original root hash %s", original_root_hash_.hex().c_str());
    }
    assert(pending_to_be_pruned_.back() == original_root_hash_);

    XMETRICS_GAUGE_SET_VALUE(metrics::mpt_trie_prune_to_trie_db, pending_to_be_pruned_.size());

    trie_db_->prune(hash(), std::move(pending_to_be_pruned_), ec);
    if (ec) {
        xwarn("xtrie_t::prune move pending keys into trie db failed. errc %d msg %s", ec.value(), ec.message().c_str());
        return;
    }
    assert(pending_to_be_pruned_.empty());
}

void xtop_trie::commit_pruned(std::vector<xh256_t> pruned_root_hashes, std::error_code & ec) {
    assert(!ec);
    assert(trie_db_);

    trie_db_->commit_pruned(std::move(pruned_root_hashes), ec);
    if (ec) {
        xwarn("xtrie_t::commit_pruned failed. category %s errc %d msg %s", ec.category().name(), ec.value(), ec.message().c_str());
    }
}

void xtop_trie::clear_pruned(xh256_t const & root_hash, std::error_code & ec) {
    assert(!ec);
    assert(trie_db_);
    assert(pending_to_be_pruned_.empty());

    trie_db_->clear_pruned(root_hash, ec);
    if (ec) {
        xwarn("xtrie_t::clear_pruned failed on pruning root %s. category %s errc %d msg %s", root_hash.hex().c_str(), ec.category().name(), ec.value(), ec.message().c_str());
    }
}

void xtop_trie::clear_pruned(std::error_code & ec) {
    assert(!ec);
    assert(trie_db_);
    assert(pending_to_be_pruned_.empty());
    trie_db_->clear_pruned(ec);
    if (ec) {
        xwarn("xtrie_t::clear_pruned all failed");
    }
}

std::string xtop_trie::to_string() const {
    if (trie_root_ != nullptr) {
        return trie_root_->fstring("");
    }

    return {};
}

std::shared_ptr<xtrie_node_face_t> xtop_trie::root() const noexcept {
    return trie_root_;
}

std::size_t xtop_trie::pending_pruned_size() const noexcept {
    return pending_to_be_pruned_.size();
}

xh256_t const & xtop_trie::original_root_hash() const noexcept {
    return original_root_hash_;
}

NS_END3
