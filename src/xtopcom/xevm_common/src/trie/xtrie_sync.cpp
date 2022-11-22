// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_sync.h"

#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/xerror/xerror.h"
#include "xmetrics/xmetrics.h"

#include <assert.h>

NS_BEG3(top, evm_common, trie)

// maxFetchesPerDepth is the maximum number of pending trie nodes per depth. The
// role of this value is to limit the number of trie nodes that get expanded in
// memory if the node was configured with a significant number of peers.
static const std::size_t maxFetchesPerDepth = 16384;

SyncPath newSyncPath(xbytes_t const & path) {
    SyncPath res;
    if (path.size() < 64) {
        res.push_back(hexToCompact(path));
    } else {
        res.push_back(hexToKeybytes(xbytes_t{path.begin(), path.begin() + 64}));
        res.push_back(hexToCompact(xbytes_t{path.begin() + 64, path.end()}));
    }
    return res;
}

Sync::Sync(xhash256_t const & root, xkv_db_face_ptr_t _database, leaf_callback callback) : database{_database} {
    AddSubTrie(root, xbytes_t{}, xhash256_t{}, callback);
}

Sync::Sync(xkv_db_face_ptr_t _database) : database{_database} {
    XMETRICS_COUNTER_INCREMENT("trie_sync", 1);
}

Sync::~Sync() {
    XMETRICS_COUNTER_DECREMENT("trie_sync", 1);
}

std::shared_ptr<Sync> Sync::NewSync(xhash256_t const & root, xkv_db_face_ptr_t _database, leaf_callback callback) {
    return std::make_shared<Sync>(root, _database, callback);
}

std::shared_ptr<Sync> Sync::NewSync(xkv_db_face_ptr_t _database) {
    return std::make_shared<Sync>( _database);
}

void Sync::Init(xhash256_t const & root, leaf_callback callback) {
    syncRoot = root;
    AddSubTrie(root, xbytes_t{}, xhash256_t{}, callback);
}

void Sync::AddSubTrie(xhash256_t const & root, xbytes_t const & path, xhash256_t const & parent, leaf_callback callback) {
    if (root == empty_root) {
        xdbg("Sync::AddSubTrie hash root empty: %s", root.as_hex_str().c_str());
        return;
    }

    if (membatch.hasNode(root.to_bytes())) {
        xdbg("Sync::AddSubTrie already hash root: %s in membatch", root.as_hex_str().c_str());
        return;
    }

    assert(database != nullptr);
    if (HasTrieNode(database, root)) {
        xdbg("Sync::AddSubTrie already hash root: %s in db", root.as_hex_str().c_str());
        return;
    }

    auto req = std::make_shared<request>(path, root, callback);
    if (parent != xhash256_t{}) {
        auto ancestor = nodeReqs[parent];
        if (ancestor == nullptr) {
            xerror("sub-trie ancestor not found %s", parent.as_hex_str().c_str());
            return;
        }
        ancestor->deps++;
        req->parents.push_back(ancestor);
    }
    schedule(req);
}

void Sync::AddUnitEntry(xhash256_t const & hash, xbytes_t const & path, xbytes_t const & unit_sync_key, xbytes_t const & unit_store_key, xhash256_t const & parent) {
    if (membatch.hasUnit(unit_store_key)) {
        return;
    }
    assert(database != nullptr);
    if (HasUnitWithPrefix(database, unit_store_key)) {
        return;
    }

    auto req = std::make_shared<request>(path, hash, unit_sync_key, unit_store_key);
    if (parent != xhash256_t{}) {
        auto ancestor = nodeReqs[parent];
        if (ancestor == nullptr) {
            xerror("raw-entry ancestor not found %s", parent.as_hex_str().c_str());
        }
        ancestor->deps++;
        req->parents.push_back(ancestor);
    }
    schedule(req);
}

std::tuple<std::vector<xhash256_t>, std::vector<xhash256_t>, std::vector<xbytes_t>> Sync::Missing(std::size_t max) {
    std::vector<xhash256_t> nodeHashes;
    std::vector<xhash256_t> unitHashes;
    std::vector<xbytes_t> unitSyncKeys;

    while (!queue.empty() && (max == 0 || nodeHashes.size() + unitSyncKeys.size() < max)) {
        // xhash256_t hash;
        xbytes_t key;
        int64_t prio;
        std::tie(key, prio) = queue.top();

        // If we have too many already-pending tasks for this depth, throttle
        auto depth = (std::size_t)(prio >> 56);
        if (fetches[depth] > maxFetchesPerDepth) {
            break;
        }

        queue.pop();
        fetches[depth]++;

        if (unitKeys.count(key)) {
            if (unitReqs.find(unitKeys[key]) == unitReqs.end()) {
                xassert(false);
            }
            unitHashes.push_back(unitKeys[key]);
            unitSyncKeys.push_back(key);
        } else {
            xhash256_t hash(key);
            if (nodeReqs.find(hash) == nodeReqs.end()) {
                xassert(false);
            }
            nodeHashes.push_back(hash);
        }
    }

    return std::make_tuple(nodeHashes, unitHashes, unitSyncKeys);
}

void Sync::Process(SyncResult const & result, std::error_code & ec) {
    // If the item was not requested either for code or node, bail out
    if (nodeReqs.find(result.Hash) == nodeReqs.end()) {
        ec = error::xerrc_t::trie_sync_not_requested;
        return;
    }

    bool filled{false};
    // There is an pending node request for this data, fill it.
    auto req = nodeReqs.at(result.Hash);
    filled = true;
    // Decode the node data content and update the request
    auto node = xtrie_node_rlp::decode_node(result.Hash, result.Data, ec);
    if (ec) {
        return;
    }
    req->data = result.Data;

    // Create and schedule a request for all the children nodes

    auto const requests = children(req, node, ec);
    if (ec) {
        return;
    }

    if (requests.empty() && req->deps == 0) {
        commit(req, ec);  // todo commit parameter ec has no error occasion.
    } else {
        req->deps += requests.size();
        for (auto const & child : requests) {
            schedule(child);
        }
    }

    if (!filled) {
        ec = error::xerrc_t::trie_sync_already_processed;
        return;
    }
}

void Sync::ProcessUnit(SyncResult const & result, std::error_code & ec) {
    // If the item was not requested either for code or node, bail out
    if (unitReqs.find(result.Hash) == unitReqs.end()) {
        ec = error::xerrc_t::trie_sync_not_requested;
        return;
    }

    bool filled{false};
    // There is an pending code request for this data, commit directly
    auto req = unitReqs.at(result.Hash);
    if (req->data.empty()) {
        filled = true;
        req->data = result.Data;
        commit(req, ec);
    }

    if (!filled) {
        ec = error::xerrc_t::trie_sync_already_processed;
        return;
    }
}

void Sync::Commit(xkv_db_face_ptr_t const & db) {
    // make sure last write syncRoot, should call once
    assert(membatch.nodes.count(syncRoot.to_bytes()));

    WriteTrieNodeBatch(db, membatch.nodes);
    WriteUnitBatch(db, membatch.units);

    membatch.clear();
}

void Sync::CommitUnit(xkv_db_face_ptr_t db) {
    WriteUnitBatch(db, membatch.units);
    membatch.units.clear();
    return;
}

std::size_t Sync::Pending() const {
    return nodeReqs.size() + unitReqs.size();
}

void Sync::schedule(std::shared_ptr<request> req) {
    if (req->unit) {
        if (unitReqs.find(req->hash) != unitReqs.end()) {
            unitReqs[req->hash]->parents.insert(unitReqs[req->hash]->parents.begin(), req->parents.begin(), req->parents.end());
            return;
        }
        unitReqs.insert(std::make_pair(req->hash, req));
    } else {
        if (nodeReqs.find(req->hash) != nodeReqs.end()) {
            nodeReqs[req->hash]->parents.insert(nodeReqs[req->hash]->parents.begin(), req->parents.begin(), req->parents.end());
            return;
        }
        nodeReqs.insert(std::make_pair(req->hash, req));
    }

    // Schedule the request for future retrieval. This queue is shared
    // by both node requests and code requests. It can happen that there
    // is a trie node and code has same hash. In this case two elements
    // with same hash and same or different depth will be pushed. But it's
    // ok the worst case is the second response will be treated as duplicated.
    assert(req->path.size() < 128);  // depth >= 128 will never happen, storage leaves will be included in their parents
    int64_t prio = (int64_t)req->path.size() << 56;
    for (std::size_t i = 0; i < 14 && i < req->path.size(); ++i) {
        prio |= (int64_t)(15 - req->path[i]) << (52 - i * 4);  // 15-nibble => lexicographic order
    }

    if (req->unit) {
        unitKeys.emplace(std::make_pair(req->unit_sync_key, req->hash));
        queue.push(req->unit_sync_key, prio);
    } else {
        queue.push(req->hash.to_bytes(), prio);
    }
}

std::vector<std::shared_ptr<Sync::request>> Sync::children(std::shared_ptr<request> req, xtrie_node_face_ptr_t object, std::error_code & ec) {
    // Gather all the children of the node, irrelevant whether known or not

    // <path, node>
    std::vector<std::pair<xbytes_t, xtrie_node_face_ptr_t>> children;
    switch (object->type()) {
    case xtrie_node_type_t::shortnode: {
        auto node = std::dynamic_pointer_cast<xtrie_short_node_t>(object);
        assert(node != nullptr);

        auto key = node->key;
        if (hasTerm(key)) {
            key = xbytes_t{key.begin(), key.end() - 1};
        }
        xbytes_t combined_path;
        combined_path.insert(combined_path.end(), req->path.begin(), req->path.end());
        combined_path.insert(combined_path.end(), key.begin(), key.end());
        children.push_back(std::make_pair(combined_path, node->val));
        break;
    }
    case xtrie_node_type_t::fullnode: {
        auto node = std::dynamic_pointer_cast<xtrie_full_node_t>(object);
        assert(node != nullptr);

        for (std::size_t i = 0; i < 17; ++i) {
            if (node->children[i] != nullptr) {
                auto child = node->children[i];
                xbytes_t combined_path;
                combined_path.insert(combined_path.end(), req->path.begin(), req->path.end());
                combined_path.insert(combined_path.end(), xbyte_t(i));
                children.push_back(std::make_pair(combined_path, child));
            }
        }
        break;
    }
    default: {
        xerror("unknown node type :%zu", static_cast<std::size_t>(object->type()));
        break;
    }
    }

    // Iterate over the children, and request all unknown ones
    std::vector<std::shared_ptr<request>> requests;
    requests.reserve(children.size());
    for (auto const & child_p : children) {
        // Notify any external watcher of a new key/value node
        if (req->callback != nullptr) {
            if (child_p.second->type() == xtrie_node_type_t::valuenode) {
                auto node = std::dynamic_pointer_cast<xtrie_value_node_t>(child_p.second);
                assert(node != nullptr);

                std::vector<xbytes_t> paths;
                if (child_p.first.size() == 2 * 32) {
                    paths.push_back(hexToKeybytes(child_p.first));
                } else if (child_p.first.size() == 4 * 32) {
                    paths.push_back(hexToKeybytes(xbytes_t{child_p.first.begin(), child_p.first.begin() + 2 * 32}));
                    paths.push_back(hexToKeybytes(xbytes_t{child_p.first.begin() + 2 * 32, child_p.first.end()}));
                }
                req->callback(paths, child_p.first, node->data(), req->hash, ec);
                if (ec) {
                    return {};
                }
            }
        }

        // If the child references another node, resolve or schedule
        if (child_p.second->type() == xtrie_node_type_t::hashnode) {
            // Try to resolve the node from the local database
            auto node = std::dynamic_pointer_cast<xtrie_hash_node_t>(child_p.second);
            assert(node != nullptr);

            auto const & hash = node->data();
            if (membatch.hasNode(hash.to_bytes())) {
                continue;
            }

            auto new_req = std::make_shared<request>(child_p.first, hash, req->callback);
            new_req->parents.push_back(req);
            requests.push_back(new_req);
        }
    }

    return requests;
}

void Sync::commit(std::shared_ptr<request> req, std::error_code & ec) {
    // Write the node content to the membatch
    if (req->unit) {
        membatch.units[req->unit_store_key] = req->data;
        unitReqs.erase(req->hash);
        unitKeys.erase(req->unit_sync_key);
        fetches[req->path.size()]--;
    } else {
        membatch.nodes[req->hash.to_bytes()] = req->data;
        nodeReqs.erase(req->hash);
        fetches[req->path.size()]--;
    }

    // Check all parents for completion
    for (auto const & parent : req->parents) {
        parent->deps--;
        if (parent->deps == 0) {
            commit(parent, ec);
            if (ec) {
                return;
            }
        }
    }
    return;
}

void Sync::clear() {
    membatch.clear();
    nodeReqs.clear();
    unitReqs.clear();
    unitKeys.clear();
    fetches.clear();
    while (!queue.empty()) {
        queue.pop();
    }
}

NS_END3