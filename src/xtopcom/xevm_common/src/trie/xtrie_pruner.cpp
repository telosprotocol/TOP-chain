// Copyright (c) 2022-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_pruner.h"

#include "xevm_common/trie/xtrie_node.h"
#include "xevm_common/trie/xtrie_db.h"
#include "xevm_common/trie/xtrie_hasher.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG3(top, evm_common, trie)

void xtop_trie_pruner::init(std::shared_ptr<xtrie_node_face_t> const & trie_root, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);
    load_trie_node(trie_root, trie_db, ec);
}

std::shared_ptr<xtrie_node_face_t> xtop_trie_pruner::load_trie_node(std::shared_ptr<xtrie_node_face_t> const & trie_node,
                                                                    std::shared_ptr<xtrie_db_t> const & trie_db,
                                                                    std::error_code & ec) {
    assert(!ec);

    if (trie_node == nullptr) {
        ec = error::xerrc_t::trie_node_unexpected;
        xwarn("empty trie node");
        return nullptr;
    }

    switch (trie_node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::hashnode: {
        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(trie_node);
        assert(hash_node != nullptr);

        return load_hash_node(hash_node, trie_db, ec);
    }

    case xtrie_node_type_t::valuenode: {
        return trie_node;
    }

    case xtrie_node_type_t::shortnode: {
        auto const short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(trie_node);
        assert(short_node != nullptr);

        return load_short_node(short_node, trie_db, ec);
    }

    case xtrie_node_type_t::fullnode: {
        auto const full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(trie_node);
        assert(full_node != nullptr);

        return load_full_node(full_node, trie_db, ec);
    }

    default: {
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        ec = error::xerrc_t::trie_node_unexpected;
        return nullptr;
    }
    }
}

std::shared_ptr<xtrie_short_node_t> xtop_trie_pruner::load_short_node(std::shared_ptr<xtrie_short_node_t> const & short_node,
                                                                      std::shared_ptr<xtrie_db_t> const & trie_db,
                                                                      std::error_code & ec) {
    assert(!ec);

    auto new_short_node = short_node;

    if (short_node->cache().hash_node() == nullptr) {
        auto hasher = xtrie_hasher_t::newHasher(false);

        auto const hash_result = hasher.hash(short_node, true);
        new_short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(hash_result.second);
        assert(new_short_node != nullptr);
    }

    auto const & hash = new_short_node->cache().hash_node()->data();
    assert(!hash.empty());

    trie_node_hashes_.insert(hash);

    new_short_node->val = load_trie_node(new_short_node->val, trie_db, ec);
    return new_short_node;
}

std::shared_ptr<xtrie_full_node_t> xtop_trie_pruner::load_full_node(std::shared_ptr<xtrie_full_node_t> const & full_node,
                                                                    std::shared_ptr<xtrie_db_t> const & trie_db,
                                                                    std::error_code & ec) {
    assert(!ec);

    auto new_full_node = full_node;
    if (full_node->cache().hash_node() == nullptr) {
        auto hasher = xtrie_hasher_t::newHasher(false);

        auto const hash_result = hasher.hash(full_node, true);
        new_full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(hash_result.second);
        assert(new_full_node != nullptr);
    }
    auto const & hash = new_full_node->cache().hash_node()->data();
    assert(!hash.empty());

    trie_node_hashes_.insert(hash);

    load_full_node_children(new_full_node, trie_db, ec);
    return new_full_node;
}

std::shared_ptr<xtrie_node_face_t> xtop_trie_pruner::load_hash_node(std::shared_ptr<xtrie_hash_node_t> const & hash_node,
                                                                    std::shared_ptr<xtrie_db_t> const & trie_db,
                                                                    std::error_code & ec) {
    assert(!ec);

    auto const & hash = hash_node->data();
    assert(!hash.empty());
    assert(hash_node->data().size() == 32);
    trie_node_hashes_.insert(hash);

    auto const node = trie_db->node(hash);
    if (!node) {
        ec = error::xerrc_t::trie_db_missing_node_error;
        xwarn("trie node (%s) not found", hash.hex().c_str());
        return nullptr;
    }

    return load_trie_node(node, trie_db, ec);
}

void xtop_trie_pruner::load_full_node_children(std::shared_ptr<xtrie_full_node_t> const & full_node, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);

    for (auto & child : full_node->children) {
        if (child != nullptr) {
            child = load_trie_node(child, trie_db, ec);
        }
    }
}

void xtop_trie_pruner::prune(xh256_t const & old_trie_root_hash, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);

    auto const trie_root = std::make_shared<xtrie_hash_node_t>(old_trie_root_hash);
    try_prune_trie_node(trie_root, trie_db, ec);
}

void xtop_trie_pruner::try_prune_trie_node(std::shared_ptr<xtrie_node_face_t> const & trie_node, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);

    if (trie_node == nullptr) {
        ec = error::xerrc_t::trie_node_unexpected;
        xwarn("empty trie node");
        return;
    }

    switch (trie_node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::hashnode: {
        auto const hash_node = std::dynamic_pointer_cast<xtrie_hash_node_t>(trie_node);
        assert(hash_node != nullptr);

        try_prune_hash_node(hash_node, trie_db, ec);

        break;
    }

    case xtrie_node_type_t::valuenode: {
        break;
    }

    case xtrie_node_type_t::shortnode: {
        auto const short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(trie_node);
        assert(short_node != nullptr);

        try_prune_short_node(short_node, trie_db, ec);

        break;
    }

    case xtrie_node_type_t::fullnode: {
        auto const full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(trie_node);
        assert(full_node != nullptr);

        try_prune_full_node(full_node, trie_db, ec);

        break;
    }

    default: {
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        ec = error::xerrc_t::trie_node_unexpected;
        xerror("unknown type(%d) of trie node", static_cast<int>(trie_node->type()));
        break;
    }
    }
}

void xtop_trie_pruner::try_prune_hash_node(std::shared_ptr<xtrie_hash_node_t> const & hash_node, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);

    auto const & hash = hash_node->data();
    assert(hash_node->data().size() == 32);

    if (trie_node_hashes_.find(hash) == std::end(trie_node_hashes_)) {
        auto const node = trie_db->node(hash);
        if (node != nullptr) {
            try_prune_trie_node(node, trie_db, ec);
        }

        trie_db->prune(hash, ec);
    }
}

void xtop_trie_pruner::try_prune_short_node(std::shared_ptr<xtrie_short_node_t> const & short_node, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);

    auto new_short_node = short_node;

    if (short_node->cache().hash_node() == nullptr) {
        auto hasher = xtrie_hasher_t::newHasher(false);

        auto const hash_result = hasher.hash(short_node, true);
        new_short_node = std::dynamic_pointer_cast<xtrie_short_node_t>(hash_result.second);
        assert(new_short_node != nullptr);
    }

    auto const & hash = new_short_node->cache().hash_node()->data();
    assert(!hash.empty());

    if (trie_node_hashes_.find(hash) == std::end(trie_node_hashes_)) {
        try_prune_trie_node(new_short_node->val, trie_db, ec);

        trie_db->prune(hash, ec);
    }
}

void xtop_trie_pruner::try_prune_full_node(std::shared_ptr<xtrie_full_node_t> const & full_node, std::shared_ptr<xtrie_db_t> const & trie_db, std::error_code & ec) {
    assert(!ec);
    auto new_full_node = full_node;

    if (full_node->cache().hash_node() == nullptr) {
        auto hasher = xtrie_hasher_t::newHasher(false);

        auto const hash_result = hasher.hash(full_node, true);
        new_full_node = std::dynamic_pointer_cast<xtrie_full_node_t>(hash_result.second);
        assert(new_full_node != nullptr);
    }

    auto const & hash = full_node->cache().hash_node()->data();
    assert(!hash.empty());

    if (trie_node_hashes_.find(hash) == std::end(trie_node_hashes_)) {
        for (auto const & child : new_full_node->children) {
            if (child != nullptr) {
                try_prune_trie_node(child, trie_db, ec);
            }
        }

        trie_db->prune(hash, ec);
    }
}

NS_END3
