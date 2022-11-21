// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_hasher.h"

#include "xbasic/xhex.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xutility/xhash.h"

#include <cassert>

NS_BEG3(top, evm_common, trie)

std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> xtop_trie_hasher::hash(xtrie_node_face_ptr_t const & node, bool const force) {
    {
        auto const & cached = node->cache();
        if (cached.hash_node() != nullptr) {
            return std::make_pair(cached.hash_node(), node);
        }
    }

    switch (node->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::shortnode: {
        auto const n = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(n != nullptr);

        auto result = hashShortNodeChildren(n);
        auto hashed = shortnodeToHash(result.first, force);
        if (hashed->type() == xtrie_node_type_t::hashnode) {
            assert(dynamic_cast<xtrie_hash_node_t *>(hashed.get()) != nullptr);
            result.second->flags.hash_node(std::dynamic_pointer_cast<xtrie_hash_node_t>(hashed));
        } else {
            result.second->flags.hash_node(nullptr);
        }

        return std::make_pair(hashed, result.second);
    }

    case xtrie_node_type_t::fullnode: {
        auto const n = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(n != nullptr);

        auto result = hashFullNodeChildren(n);
        auto hashed = fullnodeToHash(result.first, force);
        if (hashed->type() == xtrie_node_type_t::hashnode) {
            assert(dynamic_cast<xtrie_hash_node_t *>(hashed.get()) != nullptr);
            result.second->flags.hash_node(std::dynamic_pointer_cast<xtrie_hash_node_t>(hashed));
        } else {
            result.second->flags.hash_node(nullptr);
        }
        return std::make_pair(hashed, result.second);
    }

    case xtrie_node_type_t::valuenode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::hashnode: {
        return {node, node};
    }

    default:
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        xerror("xtrie_hasher::hash reach unreachable code");
    }

    unreachable();
}

xtrie_hash_node_ptr_t xtop_trie_hasher::hashData(xbytes_t const & input) const {
    xdbg("hashData:(%zu) %s ", input.size(), top::to_hex(input).c_str());
    xbytes_t hashbuf;
    utl::xkeccak256_t hasher;
    // hasher.reset(); // make hasher class member , than need this.
    hasher.update(input.data(), input.size());
    hasher.get_hash(hashbuf);
    xdbg(" -> hashed data:(%zu) %s", hashbuf.size(), top::to_hex(hashbuf).c_str());
    return std::make_shared<xtrie_hash_node_t>(hashbuf);
}

std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> xtop_trie_hasher::proofHash(xtrie_node_face_ptr_t node) {
    switch (node->type()) {
    case xtrie_node_type_t::shortnode: {
        auto const n = std::dynamic_pointer_cast<xtrie_short_node_t>(node);
        assert(n != nullptr);

        // xtrie_short_node_ptr_t sn;
        // xtrie_short_node_ptr_t _;
        auto sn = hashShortNodeChildren(n).first;
        return std::make_pair(sn, shortnodeToHash(sn, false));
    }

    case xtrie_node_type_t::fullnode: {
        auto const n = std::dynamic_pointer_cast<xtrie_full_node_t>(node);
        assert(n != nullptr);

        // xtrie_full_node_ptr_t fn;
        // xtrie_full_node_ptr_t _;
        auto fn = hashFullNodeChildren(n).first;
        return std::make_pair(fn, fullnodeToHash(fn, false));
    }

    default:
        // Value and hash nodes don't have children so they're left as were
        return std::make_pair(node, node);
    }
}

std::pair<xtrie_short_node_ptr_t, xtrie_short_node_ptr_t> xtop_trie_hasher::hashShortNodeChildren(xtrie_short_node_ptr_t const & node) {
    auto collapsed = node->clone();
    auto cached = node->clone(); // must be cloned?

    collapsed->key = hexToCompact(node->key);

    if (node->val->type() == xtrie_node_type_t::shortnode || node->val->type() == xtrie_node_type_t::fullnode) {
        auto res = hash(node->val, false);
        collapsed->val = std::move(res.first);
        cached->val = std::move(res.second);
    }

    return std::make_pair(collapsed, cached);
}

std::pair<xtrie_full_node_ptr_t, xtrie_full_node_ptr_t> xtop_trie_hasher::hashFullNodeChildren(xtrie_full_node_ptr_t const & node) {
    auto collapsed = node->clone();
    auto cached = node->clone();    // must be cloned?

    // todo impl
    // if h.parallel{}
    // ...
    // else{
    for (std::size_t index = 0; index < 16; ++index) {
        auto child = node->children[index];
        if (child != nullptr) {
            auto res = hash(child, false);
            collapsed->children[index] = std::move(res.first);
            cached->children[index] = std::move(res.second);
        } else {
            collapsed->children[index] = std::make_shared<xtrie_value_node_t>(nilValueNode);
        }
    }
    // }
    return std::make_pair(collapsed, cached);
}

xtrie_node_face_ptr_t xtop_trie_hasher::shortnodeToHash(xtrie_short_node_ptr_t node, bool const force) {
    tmp.Reset();

    std::error_code ec;
    node->EncodeRLP(tmp.data(), ec);
    xdbg("[shortnodeToHash] %s  -> %s ", top::to_hex(node->key).c_str(), top::to_hex(tmp.data()).c_str());
    xassert(!ec);

    if (tmp.len() < 32 && !force) {
        return node;  // Nodes smaller than 32 bytes are stored inside their parent
    }
    return hashData(tmp.data());
}

xtrie_node_face_ptr_t xtop_trie_hasher::fullnodeToHash(xtrie_full_node_ptr_t node, bool const force) {
    tmp.Reset();

    std::error_code ec;
    node->EncodeRLP(tmp.data(), ec);
    xdbg("[fullnodeToHash] -> %s ", top::to_hex(tmp.data()).c_str());
    xassert(!ec);

    if (tmp.len() < 32 && !force) {
        return node;  // Nodes smaller than 32 bytes are stored inside their parent
    }
    return hashData(tmp.data());
}

NS_END3
