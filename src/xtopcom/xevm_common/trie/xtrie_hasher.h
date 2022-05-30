// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_encoding.h"  // todo move to .cpp
#include "xevm_common/trie/xtrie_node.h"
#include "xutility/xhash.h"  // todo move to .cpp

NS_BEG3(top, evm_common, trie)

class sliceBuffer {
private:
    xbytes_t m_data;

public:
    std::size_t Write(xbytes_t const & data) {
        m_data.insert(m_data.end(), data.begin(), data.end());
        return m_data.size();
    }

    void Reset() {
        m_data.clear();
    }

    std::size_t len() const {
        return m_data.size();
    }

    xbytes_t & data() {
        return m_data;
    }
};

class xtop_trie_hasher {
private:
    bool m_parallel{false};  // whether to use paralallel threads when hashing
    sliceBuffer tmp;

public:
    xtop_trie_hasher(bool parallel) : m_parallel{parallel} {
    }

public:
    static xtop_trie_hasher newHasher(bool parallel) {
        return xtop_trie_hasher{parallel};
    }

public:
    /**
     * @brief hash collapses a node down into a hash node, also returning a copy of the
     * original node initialized with the computed hash to replace the original one.
     *
     * @param node original node
     * @param force bool
     * @return (hashed, cached)
     */
    std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> hash(xtrie_node_face_ptr_t node, bool force) {
        {
            auto _cached = node->cache();
            if (!_cached.first.is_null()) {
                return std::make_pair(std::make_shared<xtrie_hash_node_t>(_cached.first), node);
            }
        }
        switch (node->type()) {
        case xtrie_node_type_t::shortnode: {
            auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
            auto result = hashShortNodeChildren(n);
            auto hashed = shortnodeToHash(result.first, force);
            if (hashed->type() == xtrie_node_type_t::hashnode) {
                result.second->flags.hash = static_cast<xtrie_hash_node_t *>(hashed.get())->data();
            } else {
                result.second->flags.hash = {};
            }

            return std::make_pair(hashed, result.second);
        }
        case xtrie_node_type_t::fullnode: {
            auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
            auto result = hashFullNodeChildren(n);
            auto hashed = fullnodeToHash(result.first, force);
            if (hashed->type() == xtrie_node_type_t::hashnode) {
                result.second->flags.hash = static_cast<xtrie_hash_node_t *>(hashed.get())->data();
            } else {
                result.second->flags.hash = {};
            }
            return std::make_pair(hashed, result.second);
        }
        case xtrie_node_type_t::valuenode:
            XATTRIBUTE_FALLTHROUGH;
        case xtrie_node_type_t::hashnode: {
            return {node, node};
            break;
        }
        default:
            xassert(false);
            __builtin_unreachable();
        }
    }

private:
    std::pair<xtrie_short_node_ptr_t, xtrie_short_node_ptr_t> hashShortNodeChildren(xtrie_short_node_ptr_t node) {
        auto collapsed = node->copy();
        auto cached = node->copy();

        collapsed->Key = hexToCompact(node->Key);

        if (node->Val->type() == xtrie_node_type_t::shortnode || node->Val->type() == xtrie_node_type_t::fullnode) {
            auto res = hash(node->Val, false);
            collapsed->Val = res.first;
            cached->Val = res.second;
        }

        return std::make_pair(collapsed, cached);
    }

    std::pair<xtrie_full_node_ptr_t, xtrie_full_node_ptr_t> hashFullNodeChildren(xtrie_full_node_ptr_t node) {
        auto collapsed = node->copy();
        auto cached = node->copy();

        // todo impl
        // if h.parallel{}
        // ...
        // else{
        for (std::size_t index = 0; index < 16; ++index) {
            auto child = node->Children[index];
            if (child != nullptr) {
                auto res = hash(child, false);
                collapsed->Children[index] = res.first;
                cached->Children[index] = res.second;
            } else {
                collapsed->Children[index] = std::make_shared<xtrie_value_node_t>(nilValueNode);
            }
        }
        // }
        return std::make_pair(collapsed, cached);
    }

    xtrie_node_face_ptr_t shortnodeToHash(xtrie_short_node_ptr_t node, bool force) {
        tmp.Reset();

        std::error_code ec;
        node->EncodeRLP(tmp.data(), ec);
        printf("[shortnodeToHash] %s \n -> %s \n", top::to_hex(node->Key).c_str(), top::to_hex(tmp.data()).c_str());
        xassert(!ec);

        if (tmp.len() < 32 && !force) {
            return node;  // Nodes smaller than 32 bytes are stored inside their parent
        }
        return hashData(tmp.data());
    }

    xtrie_node_face_ptr_t fullnodeToHash(xtrie_full_node_ptr_t node, bool force) {
        tmp.Reset();

        std::error_code ec;
        node->EncodeRLP(tmp.data(), ec);
        printf("[fullnodeToHash] -> %s \n", top::to_hex(tmp.data()).c_str());
        xassert(!ec);

        if (tmp.len() < 32 && !force) {
            return node;  // Nodes smaller than 32 bytes are stored inside their parent
        }
        return hashData(tmp.data());
    }

    xtrie_hash_node_ptr_t hashData(xbytes_t input) {
        printf("hashData:(%zu) %s\n", input.size(), top::to_hex(input).c_str());
        xbytes_t hashbuf;
        utl::xkeccak256_t hasher;
        // hasher.reset(); // make hasher class member , than need this.
        hasher.update(input.data(), input.size());
        hasher.get_hash(hashbuf);
        printf(" -> hashed data:(%zu) %s\n", hashbuf.size(), top::to_hex(hashbuf).c_str());
        printf(" ----------------- \n");
        return std::make_shared<xtrie_hash_node_t>(hashbuf);
    }
};
using xtrie_hasher_t = xtop_trie_hasher;

NS_END3