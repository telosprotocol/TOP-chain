// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_sync.h"

#include <assert.h>

NS_BEG3(top, evm_common, trie)

// emptyRoot is the known root hash of an empty trie.
static constexpr auto emptyRootBytes = ConstHexBytes<32>("56e81f171bcc55a6ff8345e692c0f86e5b48e01b996cadc001622fb5e363b421");
static xhash256_t emptyRoot = xhash256_t{emptyRootBytes};

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
    // schedule(req);
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
    // todo;
    // sechedule(req);
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

    // todo priority queue
}

NS_END3