// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_db.h"

#include "xbasic/xhex.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_node.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/xerror/xerror.h"

#include <cassert>

NS_BEG3(top, evm_common, trie)

constexpr auto PreimagePrefix = ConstBytes<11>("secure-key-");

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

    return std::make_shared<xtop_trie_db>(diskdb);
}

void xtop_trie_db::insert(xhash256_t hash, int32_t size, xtrie_node_face_ptr_t node) {
    // If the node's already cached, skip
    if (dirties.find(hash) != dirties.end()) {
        return;
    }

    // todo mark size.

    auto entry = xtrie_cache_node_t{simplifyNode(node), static_cast<uint16_t>(size), newest};
    entry.forChilds([&](xhash256_t child) {
        if (this->dirties.find(child) != this->dirties.end()) {
            this->dirties.at(child).parents++;
        }
    });
    xdbg("xtop_trie_db::insert %s size:%d", hash.as_hex_str().c_str(), size);
    dirties.insert({hash, entry});

    if (oldest == xhash256_t{}) {
        oldest = hash;

    } else {
        dirties.at(newest).flushNext = hash;
    }
    newest = hash;

    // todo dirties size;
}

void xtop_trie_db::insertPreimage(xhash256_t hash, xbytes_t const & preimage) {
    preimages.insert({hash, preimage});
    // todo cal preimage size metrics.
}

xtrie_node_face_ptr_t xtop_trie_db::node(xhash256_t hash) {
    // todo:
    if (cleans.find(hash) != cleans.end()) {
        // todo clean mark hit
        return xtrie_node_rlp::mustDecodeNode(hash, cleans.at(hash));
    }
    if (dirties.find(hash) != dirties.end()) {
        // todo dirty mark hit
        return dirties.at(hash).obj(hash);
    }
    // todo mark miss hit

    // retrieve from disk db
    auto enc = ReadTrieNode(diskdb, hash);
    if (enc.empty()) {
        return nullptr;
    }
    // put into clean cache
    cleans.insert({hash, enc});
    return xtrie_node_rlp::mustDecodeNode(hash, enc);
}

xbytes_t xtop_trie_db::Node(xhash256_t hash, std::error_code & ec) {
    // It doesn't make sense to retrieve the metaroot
    if (hash == xhash256_t{}) {
        ec = error::xerrc_t::trie_db_not_found;
        return xbytes_t{};
    }

    // Retrieve the node from the clean cache if available
    if (cleans.find(hash) != cleans.end()) {
        return cleans.at(hash);
    }

    // Retrieve the node from the dirty cache if available
    if (dirties.find(hash) != dirties.end()) {
        return dirties.at(hash).rlp();
    }

    // Content unavailable in memory, attempt to retrieve from disk
    auto enc = ReadTrieNode(diskdb, hash);
    if (enc.empty()) {
        ec = error::xerrc_t::trie_db_not_found;
        return xbytes_t{};
    }
    // put into clean cache
    cleans.insert({hash, enc});
    return enc;
}

xbytes_t xtop_trie_db::preimage(xhash256_t hash) const {
    if (preimages.find(hash) != preimages.end()) {
        return preimages.at(hash);
    }
    // could put this diskdb->Get into trie_kv_db_face :: ReadPreimage
    std::error_code ec;
    auto result = diskdb->Get(preimageKey(hash), ec);
    if (ec) {
        xwarn("xtrie_db Get preimage %s failed: %s", hash.as_hex_str().c_str(), ec.message().c_str());
    }
    return result;
}

void xtop_trie_db::Commit(xhash256_t hash, AfterCommitCallback cb, std::error_code & ec) {
    // what we can optimize here:
    // 1. use batch writer to sum all <k,v> into db with once writeDB operation
    // 2. db.preimages for secure trie
    // 3. use async cleaner to clean dirties value after commit.

    // 0. first, Move all of the accumulated preimages in.
    for (auto const & preimage : preimages) {
        diskdb->Put(preimageKey(preimage.first), preimage.second, ec);
        if (ec) {
            xwarn("xtrie_db Commit diskdb error at preimage %s, err: %s", preimage.first.as_hex_str().c_str(), ec.message().c_str());
            // return;
        }
        // todo could add some metrics here.
    }

    // If the node does not exist, it's a previously committed node
    if (dirties.find(hash) == dirties.end()) {
        return;
    }

    auto node = dirties.at(hash);
    node.forChilds([&](xhash256_t child) {
        if (!ec) {
            this->Commit(child, cb, ec);
        }
    });
    if (ec) {
        return;
    }

    // put it into diskDB
    auto enc = node.rlp();
    auto hash_bytes = xbytes_t{hash.begin(), hash.end()};
    xdbg("xtop_trie_db::Commit write node %s, size %zu", top::to_hex(hash_bytes).c_str(), enc.size());
    WriteTrieNode(diskdb, hash, enc);
    if (cb) {
        cb(hash);
    }

    // clean dirties:
    // todos: noted to remove size of dirty cache.
    // todos: change link-list of flush-list Next/Prev Node ptr
    // for now : we can simplily erase it:
    dirties.erase(hash);

    // and move it to cleans:
    cleans.insert({hash, enc});
    // todo mark size everywhere with cleans/dirties' insert/erase/...

    // clean preimages:
    preimages.clear();
}

// void xtop_trie_db::commit(xhash256_t hash, AfterCommitCallback cb, std::error_code & ec) {
// }

xbytes_t xtop_trie_db::preimageKey(xhash256_t hash_key) const {
    xbytes_t res;
    res.insert(res.begin(), PreimagePrefix.begin(), PreimagePrefix.end());
    res.insert(res.end(), hash_key.begin(), hash_key.end());
    return res;
}

// ============
// trie cache
// ============
xbytes_t xtrie_cache_node_t::rlp() {
    if (node == nullptr) {
        return xbytes_t{};
    }
    // if (node->type() == xtrie_node_type_t::rawnode) {
    //     auto n = std::dynamic_pointer_cast<xtrie_raw_node_t>(node);
    //     return n->data();
    // }
    return xtrie_node_rlp::EncodeToBytes(node);
}

xtrie_node_face_ptr_t xtrie_cache_node_t::obj(xhash256_t hash) {
    if (node == nullptr) {
        return node;
    }
    if (node->type() == xtrie_node_type_t::rawnode) {
        auto n = std::dynamic_pointer_cast<xtrie_raw_node_t>(node);
        assert(n != nullptr);

        return xtrie_node_rlp::mustDecodeNode(hash, n->data());
    }
    return expandNode(std::make_shared<xtrie_hash_node_t>(hash), node);
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
        auto nn = std::dynamic_pointer_cast<xtrie_raw_short_node_t>(n);
        assert(nn != nullptr);

        forGatherChildren(nn->Val, f);
        return;
    }
    case xtrie_node_type_t::rawfullnode: {
        auto nn = std::dynamic_pointer_cast<xtrie_raw_full_node_t>(n);
        assert(nn != nullptr);

        for (std::size_t index = 0; index < 16; ++index) {
            forGatherChildren(nn->Children[index], f);
        }
        return;
    }
    case xtrie_node_type_t::hashnode: {
        auto nn = std::dynamic_pointer_cast<xtrie_hash_node_t>(n);
        assert(nn != nullptr);

        f(xhash256_t{nn->data()});
        return;
    }
    case xtrie_node_type_t::valuenode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::rawnode:
        // pass;
        return;
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
        auto node = std::dynamic_pointer_cast<xtrie_short_node_t>(n);
        assert(node != nullptr);

        return std::make_shared<xtrie_raw_short_node_t>(node->key, simplifyNode(node->val));
    }
    case xtrie_node_type_t::fullnode: {
        auto node = std::dynamic_pointer_cast<xtrie_full_node_t>(n);
        assert(node != nullptr);

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

xtrie_node_face_ptr_t expandNode(std::shared_ptr<xtrie_hash_node_t> hash, xtrie_node_face_ptr_t n) {
    switch (n->type()) {
    case xtrie_node_type_t::rawshortnode: {
        auto node = std::dynamic_pointer_cast<xtrie_raw_short_node_t>(n);
        assert(node != nullptr);

        return std::make_shared<xtrie_short_node_t>(compactToHex(node->Key), expandNode(nullptr, node->Val), xnode_flag_t{hash});
    }
    case xtrie_node_type_t::rawfullnode: {
        auto node = std::dynamic_pointer_cast<xtrie_raw_full_node_t>(n);
        assert(node != nullptr);

        auto fullnode_ptr = std::make_shared<xtrie_full_node_t>(xnode_flag_t{hash});
        for (std::size_t i = 0; i < fullnode_ptr->Children.size(); ++i) {
            if (node->Children[i] != nullptr) {
                fullnode_ptr->Children[i] = expandNode(nullptr, node->Children[i]);
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