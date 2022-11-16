// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_db.h"

#include "xbasic/xhex.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_node.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/xerror/xerror.h"
#include "xmetrics/xmetrics.h"

#include <cassert>

NS_BEG3(top, evm_common, trie)

constexpr uint32_t IdealBatchSize = 1024;

constexpr auto PreimagePrefix = ConstBytes<11>("secure-key-");

std::shared_ptr<xtop_trie_db> xtop_trie_db::NewDatabase(xkv_db_face_ptr_t diskdb) {
    return NewDatabaseWithConfig(std::move(diskdb), nullptr);
}

std::shared_ptr<xtop_trie_db> xtop_trie_db::NewDatabaseWithConfig(xkv_db_face_ptr_t diskdb, xtrie_db_config_ptr_t /*config*/) {
    //if (config != nullptr && config->Cache_size > 0) {
    //    if (config->Journal.empty()) {
    //        // todo
    //    } else {
    //        // todo
    //    }
    //}

    return std::make_shared<xtop_trie_db>(std::move(diskdb));
}

void xtop_trie_db::insert(xhash256_t hash, int32_t const size, xtrie_node_face_ptr_t const & node) {
    // If the node's already cached, skip
    if (dirties_.find(hash) != dirties_.end()) {
        return;
    }

    // todo mark size.

    auto entry = xtrie_cache_node_t{simplify_node(node), static_cast<uint16_t>(size), newest_};
    entry.forChilds([&](xhash256_t const & child) {
        if (this->dirties_.find(child) != this->dirties_.end()) {
            this->dirties_.at(child).parents_++;
        }
    });
    xdbg("xtop_trie_db::insert %s size:%d", hash.as_hex_str().c_str(), size);
    dirties_.insert({hash, entry});

    if (oldest_ == xhash256_t{}) {
        oldest_ = hash;
    } else {
        dirties_.at(newest_).flush_next_ = hash;
    }
    newest_ = hash;

    // todo dirties size;
}

void xtop_trie_db::insertPreimage(xhash256_t hash, xbytes_t const & preimage) {
    preimages_.insert({hash, preimage});
    // todo cal preimage size metrics.
}

xtrie_node_face_ptr_t xtop_trie_db::node(xhash256_t hash) {
    // todo:
    xbytes_t value;
    if (cleans_.get(hash, value)) {
        // todo clean mark hit
        return xtrie_node_rlp::must_decode_node(hash, value);
    }
    if (dirties_.find(hash) != dirties_.end()) {
        // todo dirty mark hit
        return dirties_.at(hash).obj(hash);
    }
    // todo mark miss hit

    // retrieve from disk db
    auto enc = ReadTrieNode(diskdb_, hash);
    if (enc.empty()) {
        return nullptr;
    }
    // put into clean cache
    cleans_.insert({hash, enc});
    return xtrie_node_rlp::must_decode_node(hash, enc);
}

xbytes_t xtop_trie_db::Node(xhash256_t hash, std::error_code & ec) {
    // It doesn't make sense to retrieve the metaroot
    if (hash.empty()) {
        ec = error::xerrc_t::trie_db_not_found;
        return xbytes_t{};
    }

    // Retrieve the node from the clean cache if available
    xbytes_t value;
    if (cleans_.get(hash, value)) {
        return value;
    }

    // Retrieve the node from the dirty cache if available
    if (dirties_.find(hash) != dirties_.end()) {
        return dirties_.at(hash).rlp();
    }

    // Content unavailable in memory, attempt to retrieve from disk
    auto enc = ReadTrieNode(diskdb_, hash);
    if (enc.empty()) {
        ec = error::xerrc_t::trie_db_not_found;
        return xbytes_t{};
    }
    // put into clean cache
    cleans_.insert({hash, enc});
    return enc;
}

xbytes_t xtop_trie_db::preimage(xhash256_t hash) const {
    if (preimages_.find(hash) != preimages_.end()) {
        return preimages_.at(hash);
    }
    // could put this diskdb->Get into trie_kv_db_face :: ReadPreimage
    std::error_code ec;
    auto result = diskdb_->Get(preimage_key(hash), ec);
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
    for (auto const & preimage : preimages_) {
        diskdb_->Put(preimage_key(preimage.first), preimage.second, ec);
        if (ec) {
            xwarn("xtrie_db Commit diskdb error at preimage %s, err: %s", preimage.first.as_hex_str().c_str(), ec.message().c_str());
            // return;
        }
        // todo could add some metrics here.
    }

    std::map<xbytes_t, xbytes_t> batch;
    commit(hash, batch, cb, ec);
    if (ec) {
        xerror("xtop_trie_db::Commit error: %s, %s", ec.category().name(), ec.message().c_str());
        return;
    }
    WriteTrieNodeBatch(diskdb_, batch);
    batch.clear();

    // todos: noted to remove size of dirty cache.
    // todos: change link-list of flush-list Next/Prev Node ptr
    // for now : we can simplily erase it:
    newest_ = {};
    oldest_ = {};

    // clean preimages:
    preimages_.clear();
}

void xtop_trie_db::commit(xhash256_t hash, std::map<xbytes_t, xbytes_t> & data, AfterCommitCallback cb, std::error_code & ec) {
    // If the node does not exist, it's a previously committed node
    if (dirties_.find(hash) == dirties_.end()) {
        return;
    }

    auto node = dirties_.at(hash);
    node.forChilds([&](xhash256_t child) {
        if (!ec) {
            this->commit(child, data, cb, ec);
        }
    });
    if (ec) {
        return;
    }

    // put it into batch
    auto enc = node.rlp();
    auto const hash_bytes = to_bytes(hash);
    xdbg("xtop_trie_db::Commit write node %s, size %zu", top::to_hex(hash_bytes).c_str(), enc.size());
    data.emplace(std::make_pair(hash.to_bytes(), enc));

    if (cb) {
        cb(hash);
    }
    if (data.size() >= IdealBatchSize) {
        WriteTrieNodeBatch(diskdb_, data);
        data.clear();
    }

    // clean dirties:
    dirties_.erase(hash);

    // and move it to cleans:
    cleans_.insert({hash, enc});
    // todo mark size everywhere with cleans/dirties' insert/erase/...
}

void xtop_trie_db::prune(xhash256_t const & hash, std::error_code & ec) {
    if (pruned_hashes_.find(hash) != std::end(pruned_hashes_)) {
        return;
    }

    cleans_.erase(hash);

    assert(dirties_.find(hash) == dirties_.end());

    pruned_hashes_.insert(hash);
    // xinfo("hash %s added to be pruned later", hash.as_hex_str().c_str());
    XMETRICS_GAUGE(metrics::mpt_cached_pruned_trie_node_cnt, 1);
}

void xtop_trie_db::commit_pruned(std::error_code & ec) {
    assert(!ec);

    std::vector<xbytes_t> pruned_keys;
    pruned_keys.reserve(pruned_hashes_.size());
    std::transform(std::begin(pruned_hashes_), std::end(pruned_hashes_), std::back_inserter(pruned_keys), [](xhash256_t const & hash) { return hash.to_bytes(); });
    diskdb_->DeleteBatch(pruned_keys, ec);
    if (ec) {
        xwarn("pruning MPT nodes failed. %s", ec.message().c_str());
        return;
    }

    xkinfo("%zu keys pruned from disk", pruned_hashes_.size());
    XMETRICS_GAUGE(metrics::mpt_total_pruned_trie_node_cnt, pruned_hashes_.size());
    XMETRICS_GAUGE(metrics::mpt_cached_pruned_trie_node_cnt, -static_cast<int32_t>(pruned_hashes_.size()));

    pruned_hashes_.clear();
}

xbytes_t xtop_trie_db::preimage_key(xhash256_t const & hash_key) const {
    xbytes_t res;
    res.insert(res.begin(), PreimagePrefix.begin(), PreimagePrefix.end());
    res.insert(res.end(), hash_key.begin(), hash_key.end());
    return res;
}

// ============
// trie cache
// ============
xbytes_t xtrie_cache_node_t::rlp() {
    if (node_ == nullptr) {
        return xbytes_t{};
    }
    // if (node->type() == xtrie_node_type_t::rawnode) {
    //     auto n = std::dynamic_pointer_cast<xtrie_raw_node_t>(node);
    //     return n->data();
    // }
    return xtrie_node_rlp::EncodeToBytes(node_);
}

xtrie_node_face_ptr_t xtrie_cache_node_t::obj(xhash256_t hash) {
    if (node_ == nullptr) {
        return node_;
    }
    if (node_->type() == xtrie_node_type_t::rawnode) {
        auto n = std::dynamic_pointer_cast<xtrie_raw_node_t>(node_);
        assert(n != nullptr);

        return xtrie_node_rlp::must_decode_node(hash, n->data());
    }
    return expandNode(std::make_shared<xtrie_hash_node_t>(hash), node_);
}

void xtrie_cache_node_t::forChilds(onChildFunc const & f) {
    for (auto const & c : children_) {
        f(c.first);
    }
    if (node_ && node_->type() != xtrie_node_type_t::rawnode) {
        forGatherChildren(node_, f);
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
xtrie_node_face_ptr_t simplify_node(xtrie_node_face_ptr_t const & n) {
    switch (n->type()) {  // NOLINT(clang-diagnostic-switch-enum)
    case xtrie_node_type_t::shortnode: {
        auto const node = std::dynamic_pointer_cast<xtrie_short_node_t>(n);
        assert(node != nullptr);

        return std::make_shared<xtrie_raw_short_node_t>(node->key, simplify_node(node->val));
    }
    case xtrie_node_type_t::fullnode: {
        auto const node = std::dynamic_pointer_cast<xtrie_full_node_t>(n);
        assert(node != nullptr);

        auto raw_fullnode_ptr = std::make_shared<xtrie_raw_full_node_t>(node->Children);
        for (std::size_t i = 0; i < raw_fullnode_ptr->Children.size(); ++i) {
            if (raw_fullnode_ptr->Children[i] != nullptr) {
                raw_fullnode_ptr->Children[i] = simplify_node(raw_fullnode_ptr->Children[i]);
            }
        }
        return raw_fullnode_ptr;
    }

    case xtrie_node_type_t::valuenode:  // NOLINT(bugprone-branch-clone)
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::hashnode:
        XATTRIBUTE_FALLTHROUGH;
    case xtrie_node_type_t::rawnode:
        return n;

    default: {
        xerror("unknown node type: %d", n->type());
        assert(false);  // NOLINT(clang-diagnostic-disabled-macro-expansion)
        return nullptr;
    }
    }
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
