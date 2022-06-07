// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_db.h"

#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_node_coding.h"
NS_BEG3(top, evm_common, trie)

std::shared_ptr<xtop_trie_db> xtop_trie_db::NewDatabase(xkv_db_face_ptr_t diskdb) {
    return NewDatabaseWithConfig(diskdb, nullptr);
}

std::shared_ptr<xtop_trie_db> xtop_trie_db::NewDatabaseWithConfig(xkv_db_face_ptr_t diskdb, xtrie_db_config_ptr_t config) {
    if (config != nullptr && config->Cache_size > 0) {
        if (config->Journal == "") {
            // todo
        } else {
            // todo
        }
    }

    // todo:
    auto db = xtrie_db_t{};
    return std::make_shared<xtop_trie_db>(db);
}

xtrie_node_face_ptr_t xtop_trie_db::node(xhash256_t hash) {
    // todo:
    return nullptr;
}

void xtop_trie_db::insert(xhash256_t hash, int32_t size, xtrie_node_face_ptr_t node) {
    // todo:

    auto entry = xtrie_cache_node_t{};
    entry.forChilds([](xhash256_t child) {
        // for example:
        xdbg("%s", child.hex_string().c_str());
    });
}

// ============
// trie cache
// ============
xbytes_t xtrie_cache_node_t::rlp() {
    if (node == nullptr) {
        return xbytes_t{};
    }
    if (node->type() == xtrie_node_type_t::rawnode) {
        auto n = std::make_shared<xtrie_raw_node_t>(*(static_cast<xtrie_raw_node_t *>(node.get())));
        return n->data();
    }
    return xtrie_node_rlp::EncodeToBytes(node);
}

xtrie_node_face_ptr_t xtrie_cache_node_t::obj(xhash256_t hash) {
    if (node == nullptr) {
        return node;
    }
    if (node->type() == xtrie_node_type_t::rawnode) {
        auto n = std::make_shared<xtrie_raw_node_t>(*(static_cast<xtrie_raw_node_t *>(node.get())));
        return xtrie_node_rlp::mustDecodeNode(hash, n->data());
    }
    return expandNode(xtrie_hash_node_t{hash}, node);
}

void xtrie_cache_node_t::forChilds(onChildFunc f) {
    for (auto const & c : children) {
        f(c.first);
    }
    if (node && node->type() != xtrie_node_type_t::rawnode) {
        forGatherChildren(node, f);
    }
}

void xtrie_cache_node_t::forGatherChildren(xtrie_node_face_ptr_t n, onChildFunc f) {
    if (n == nullptr) {
        return;
    }
    switch (n->type()) {
    case xtrie_node_type_t::rawshortnode: {
        auto nn = std::make_shared<xtrie_raw_short_node_t>(*(static_cast<xtrie_raw_short_node_t *>(n.get())));
        forGatherChildren(nn->Val, f);
        break;
    }
    case xtrie_node_type_t::rawfullnode: {
        auto nn = std::make_shared<xtrie_raw_full_node_t>(*(static_cast<xtrie_raw_full_node_t *>(n.get())));
        for (std::size_t index = 0; index < 16; ++index) {
            forGatherChildren(nn->Children[index], f);
        }
        break;
    }
    case xtrie_node_type_t::hashnode: {
        auto nn = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(n.get())));
        f(xhash256_t{nn->data()});
        break;
    }
    case xtrie_node_type_t::valuenode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::rawnode:
        // pass;
        break;
    default: {
        xerror("unknown node type: %d", n->type());
        xassert(false);
    }
    }
    __builtin_unreachable();
}

// static methods:
xtrie_node_face_ptr_t simplifyNode(xtrie_node_face_ptr_t n) {
    switch (n->type()) {
    case xtrie_node_type_t::shortnode: {
        auto node = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(n.get())));
        return std::make_shared<xtrie_raw_short_node_t>(node->Key, simplifyNode(node->Val));
    }
    case xtrie_node_type_t::fullnode: {
        auto node = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(n.get())));
        auto raw_fullnode_ptr = std::make_shared<xtrie_raw_full_node_t>(node->Children);
        for (std::size_t i = 0; i < raw_fullnode_ptr->Children.size(); ++i) {
            if (raw_fullnode_ptr->Children[i] != nullptr) {
                raw_fullnode_ptr->Children[i] = simplifyNode(raw_fullnode_ptr->Children[i]);
            }
        }
        return raw_fullnode_ptr;
    }
    case xtrie_node_type_t::valuenode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::hashnode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::rawnode:
        return n;
    default: {
        xerror("unknown node type: %d", n->type());
        xassert(false);
    }
    }
    __builtin_unreachable();
}

xtrie_node_face_ptr_t expandNode(xtrie_hash_node_t hash, xtrie_node_face_ptr_t n) {
    switch (n->type()) {
    case xtrie_node_type_t::rawshortnode: {
        auto node = std::make_shared<xtrie_raw_short_node_t>(*(static_cast<xtrie_raw_short_node_t *>(n.get())));
        return std::make_shared<xtrie_short_node_t>(compactToHex(node->Key), expandNode(xtrie_hash_node_t{}, node->Val), nodeFlag{hash});
    }
    case xtrie_node_type_t::rawfullnode: {
        auto node = std::make_shared<xtrie_raw_full_node_t>(*(static_cast<xtrie_raw_full_node_t *>(n.get())));
        auto fullnode_ptr = std::make_shared<xtrie_full_node_t>(nodeFlag{hash});
        for (std::size_t i = 0; i < fullnode_ptr->Children.size(); ++i) {
            if (node->Children[i] != nullptr) {
                fullnode_ptr->Children[i] = expandNode(xtrie_hash_node_t{}, node->Children[i]);
            }
        }
        return fullnode_ptr;
    }
    case xtrie_node_type_t::valuenode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::hashnode:
        return n;
    default: {
        xerror("unknown node type: %d", n->type());
        xassert(false);
    }
    }
    __builtin_unreachable();
}

NS_END3