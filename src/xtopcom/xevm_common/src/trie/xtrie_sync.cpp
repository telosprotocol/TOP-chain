// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_sync.h"

#include <assert.h>

NS_BEG3(top, evm_common, trie)

// emptyRoot is the known root hash of an empty trie.
static constexpr auto emptyRootBytes = ConstHexBytes<32>("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
static xhash256_t emptyRoot = xhash256_t{emptyRootBytes};

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

void Sync::AddSubTrie(xhash256_t root, xbytes_t path, xhash256_t parent /*callback*/) {
    if (root == emptyRoot) {
        return;
    }

    if (membatch.hasNode(root)) {
        return;
    }

    assert(database != nullptr);
    std::error_code ec;

    if (HasTrieNode(database, root)) {
        return;
    }

    auto req = new request(path, root);  // callback
    if (parent != xhash256_t{}) {
        auto ancestor = nodeReqs[parent];
        if (ancestor == nullptr) {
            xerror("sub-trie ancestor not found %s", parent.as_hex_str());
            return;
        }
        ancestor->deps++;
        req->parents.push_back(ancestor);
    }
    schedule(req);
}

void Sync::AddCodeEntry(xhash256_t hash, xbytes_t path, xhash256_t parent) {
    if (hash == emptyRoot) {
        return;
    }

    if (membatch.hasNode(hash)) {
        return;
    }
    assert(database != nullptr);
    std::error_code ec;
    if (HasCodeWithPrefix(database, hash)) {
        return;
    }

    auto req = new request(path, hash, true);
    if (parent != xhash256_t{}) {
        auto ancestor = nodeReqs[parent];
        if (ancestor == nullptr) {
            xerror("raw-entry ancestor not found %s", parent.as_hex_str());
        }
        ancestor->deps++;
        req->parents.push_back(ancestor);
    }
    schedule(req);
}

std::tuple<std::vector<xhash256_t>, SyncPath, std::vector<xhash256_t>> Sync::Missing(std::size_t max) {
    std::vector<xhash256_t> nodeHashes;
    SyncPath nodePaths;
    std::vector<xhash256_t> codeHashes;

    while (!queue.empty() && (max == 0 || nodeHashes.size() + codeHashes.size() < max)) {
        xhash256_t hash;
        int64_t prio;
        std::tie(hash, prio) = queue.top();

        // If we have too many already-pending tasks for this depth, throttle
        auto depth = (std::size_t)(prio >> 56);
        if (fetches[depth] > maxFetchesPerDepth) {
            break;
        }

        queue.pop();
        fetches[depth]++;

        if (nodeReqs.find(hash) != nodeReqs.end()) {
            nodeHashes.push_back(hash);
            auto new_path = newSyncPath(nodeReqs[hash]->path);
            nodePaths.insert(nodePaths.end(), new_path.begin(), new_path.end());
        } else {
            codeHashes.push_back(hash);
        }
    }

    return std::make_tuple(nodeHashes, nodePaths, codeHashes);
}

void Sync::schedule(request * req) {
    if (req->code) {
        if (codeReqs.find(req->hash) != codeReqs.end()) {
            codeReqs[req->hash]->parents.insert(codeReqs[req->hash]->parents.begin(), req->parents.begin(), req->parents.end());
            return;
        }
        codeReqs.insert(std::make_pair(req->hash, req));
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
    queue.push(req->hash, prio);
}

NS_END3