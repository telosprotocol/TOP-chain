// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_committer.h"

#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/xerror/xerror.h"

#include <cassert>

NS_BEG3(top, evm_common, trie)

std::pair<xtrie_hash_node_ptr_t, int32_t> xtop_trie_committer::Commit(xtrie_node_face_ptr_t const & n, xtrie_db_ptr_t db, std::error_code & ec) {
    if (db == nullptr) {
        ec = error::xerrc_t::trie_db_not_provided;
        return std::make_pair(nullptr, 0);
    }

    xtrie_node_face_ptr_t h;
    int32_t committed;
    std::tie(h, committed) = commit(n, db, ec);
    if (ec) {
        return std::make_pair(nullptr, 0);
    }
    xassert(h->type() == xtrie_node_type_t::hashnode);
    auto hashnode = std::dynamic_pointer_cast<xtrie_hash_node_t>(h);
    assert(hashnode != nullptr);

    return std::make_pair(hashnode, committed);
}

std::pair<xtrie_node_face_ptr_t, int32_t> xtop_trie_committer::commit(xtrie_node_face_ptr_t const & n, xtrie_db_ptr_t db, std::error_code & ec) {
    // if this path is clean, use available cached data
    auto const cached_data = n->cache();
    if (cached_data.hash_node() != nullptr && !cached_data.dirty()) {
        // std::printf("commit: hash %s\n", xhash256_t{cached_data.hash_node()->data()}.as_hex_str().c_str());
        return std::make_pair(cached_data.hash_node(), 0);
    }

    // Commit children, then parent, and remove remove the dirty flag.
    switch (n->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::shortnode: {
        auto cn = std::dynamic_pointer_cast<xtrie_short_node_t>(n);
        assert(cn != nullptr);

        // Commit child
        auto collapsed = cn->clone();
        // If the child is fullNode, recursively commit,
        // otherwise it can only be hashNode or valueNode.
        int32_t childCommitted{0};
        if (cn->val->type() == xtrie_node_type_t::fullnode) {
            xtrie_node_face_ptr_t childV;
            int32_t committed;
            std::tie(childV, committed) = commit(cn->val, db, ec);
            if (ec) {
                return std::make_pair(nullptr, 0);
            }
            collapsed->val = childV;
            childCommitted = committed;
        }
        // The key needs to be copied, since we're delivering it to database
        collapsed->key = hexToCompact(cn->key);
        auto const hashed_node = store(collapsed, db);
        assert(hashed_node != nullptr);
        if (hashed_node->type() == xtrie_node_type_t::hashnode) {
            auto hn = std::dynamic_pointer_cast<xtrie_hash_node_t>(hashed_node);
            assert(hn != nullptr);

            // std::printf("commit: hash %s\n", xhash256_t{hn->data()}.as_hex_str().c_str());

            return std::make_pair(hn, childCommitted + 1);
        }
        return std::make_pair(collapsed, childCommitted);
    }
    case xtrie_node_type_t::fullnode: {
        auto cn = std::dynamic_pointer_cast<xtrie_full_node_t>(n);
        assert(cn != nullptr);

        std::array<xtrie_node_face_ptr_t, 17> hashedKids;
        int32_t childCommitted{0};
        std::tie(hashedKids, childCommitted) = commitChildren(cn, db, ec);
        if (ec) {
            return std::make_pair(nullptr, 0);
        }

        auto collapsed = cn->clone();
        collapsed->children = hashedKids;

        auto const hashed_node = store(collapsed, db);
        assert(hashed_node != nullptr);
        if (hashed_node->type() == xtrie_node_type_t::hashnode) {
            auto hn = std::dynamic_pointer_cast<xtrie_hash_node_t>(hashed_node);
            assert(hn != nullptr);

            // std::printf("commit: hash %s\n", xhash256_t{hn->data()}.as_hex_str().c_str());

            return std::make_pair(hn, childCommitted + 1);
        }
        return std::make_pair(collapsed, childCommitted);
    }
    case xtrie_node_type_t::hashnode: {
        auto hn = std::dynamic_pointer_cast<xtrie_hash_node_t>(n);
        assert(hn != nullptr);

        return std::make_pair(hn, 0);
    }
    default:
        // nil, valuenode shouldn't be committed
        xassert(false);
    }
    __builtin_unreachable();
}

std::pair<std::array<xtrie_node_face_ptr_t, 17>, int32_t> xtop_trie_committer::commitChildren(xtrie_full_node_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec) {
    std::array<xtrie_node_face_ptr_t, 17> children;
    int32_t committed{0};

    for (std::size_t index = 0; index < 16; ++index) {
        auto child = n->children[index];
        if (child == nullptr)
            continue;

        // If it's the hashed child, save the hash value directly.
        // Note: it's impossible that the child in range [0, 15]
        // is a valueNode.
        if (child->type() == xtrie_node_type_t::hashnode) {
            auto hn = std::dynamic_pointer_cast<xtrie_hash_node_t>(child);
            assert(hn != nullptr);

            children[index] = hn;
            continue;
        }

        // Commit the child recursively and store the "hashed" value.
        // Note the returned node can be some embedded nodes, so it's
        // possible the type is not hashNode.
        xtrie_node_face_ptr_t hashed;
        int32_t childCommitted;
        std::tie(hashed, childCommitted) = commit(child, db, ec);
        if (ec) {
            return std::make_pair(children, 0);
        }
        children[index] = hashed;
        committed += childCommitted;
    }

    // For the 17th child, it's possible the type is valuenode.
    if (n->children[16] != nullptr) {
        children[16] = n->children[16];
    }

    return std::make_pair(children, committed);
}

// store hashes the node n and if we have a storage layer specified, it writes
// the key/value pair to it and tracks any node->child references as well as any
// node->external trie references.
xtrie_node_face_ptr_t xtop_trie_committer::store(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db) {
    auto hash = n->cache().hash_node();
    int32_t size{0};

    if (hash == nullptr) {
        // This was not generated - must be a small node stored in the parent.
        // In theory, we should apply the leafCall here if it's not nil(embedded
        // node usually contains value). But small value(less than 32bytes) is
        // not our target.
        return n;
    }

    // We have the hash already, estimate the RLP encoding-size of the node.
    // The size is used for mem tracking, does not need to be exact

    size = estimateSize(n);

    if (false) {
        // todo leafCh
    } else {
        db->insert(hash->data(), size, n);
    }
    return hash;
}

int32_t xtop_trie_committer::estimateSize(xtrie_node_face_ptr_t n) {
    switch (n->type()) {
    case xtrie_node_type_t::shortnode: {
        auto shortnode = std::dynamic_pointer_cast<xtrie_short_node_t>(n);
        assert(shortnode != nullptr);

        return 3 + shortnode->key.size() + estimateSize(shortnode->val);
    }
    case xtrie_node_type_t::fullnode: {
        auto fullnode = std::dynamic_pointer_cast<xtrie_full_node_t>(n);
        assert(fullnode != nullptr);

        int32_t s = 3;
        for (std::size_t index = 0; index < 16; ++index) {
            auto child = fullnode->children[index];
            if (child != nullptr) {
                s += estimateSize(child);
            } else {
                s++;
            }
        }
        return s;
    }
    case xtrie_node_type_t::valuenode: {
        auto valuenode = std::dynamic_pointer_cast<xtrie_value_node_t>(n);
        assert(valuenode != nullptr);

        return 1 + valuenode->data().size();
    }
    case xtrie_node_type_t::hashnode: {
        auto hashnode = std::dynamic_pointer_cast<xtrie_hash_node_t>(n);
        assert(hashnode != nullptr);

        return 1 + hashnode->data().size();
    }
    default:
        xassert(false);
    }
    __builtin_unreachable();
}

NS_END3
