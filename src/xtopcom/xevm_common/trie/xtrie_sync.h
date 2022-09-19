// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xthreading/xthreadsafe_priority_queue.hpp"
#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_kv_db_face.h"

#include <map>
#include <memory>
#include <vector>

NS_BEG3(top, evm_common, trie)

// SyncPath is a path tuple identifying a particular trie node either in a single
// trie (account) or a layered trie (account -> storage).
//
// Content wise the tuple either has 1 element if it addresses a node in a single
// trie or 2 elements if it addresses a node in a stacked trie.
//
// To support aiming arbitrary trie nodes, the path needs to support odd nibble
// lengths. To avoid transferring expanded hex form over the network, the last
// part of the tuple (which needs to index into the middle of a trie) is compact
// encoded. In case of a 2-tuple, the first item is always 32 bytes so that is
// simple binary encoded.
//
// Examples:
//   - Path 0x9  -> {0x19}
//   - Path 0x99 -> {0x0099}
//   - Path 0x01234567890123456789012345678901012345678901234567890123456789019  -> {0x0123456789012345678901234567890101234567890123456789012345678901, 0x19}
//   - Path 0x012345678901234567890123456789010123456789012345678901234567890199 -> {0x0123456789012345678901234567890101234567890123456789012345678901, 0x0099}
using SyncPath = std::vector<xbytes_t>;

SyncPath newSyncPath(xbytes_t const & path);

// SyncResult is a response with requested data along with it's hash.
struct SyncResult {
    xhash256_t Hash;  // Hash of the originally unknown trie node
    xbytes_t Data;    // Data content of the retrieved node
};

// Sync is the main state trie synchronisation scheduler, which provides yet
// unknown trie hashes to retrieve, accepts node data associated with said hashes
// and reconstructs the trie step by step until all is done.
class Sync {
private:
    // request represents a scheduled or already in-flight state retrieval request.
    struct request {
        xbytes_t path;     // Merkle path leading to this node for prioritization
        xhash256_t hash;   // Hash of the node data content to retrieve
        xbytes_t data;     // Data content of the node, cached until all subtrees complete
        bool code{false};  // Whether this is a code entry

        std::vector<request *> parents;  // Parent state nodes referencing this entry (notify all upon completion)
        std::size_t deps{0};                // Number of dependencies before allowed to commit this node

        LeafCallback callback{nullptr};  // Callback to invoke if a leaf node it reached on this branch

        request(xbytes_t const & _path, xhash256_t const & _hash, LeafCallback _callback) : path{_path}, hash{_hash}, callback{_callback} {
        }
        request(xbytes_t const & _path, xhash256_t const & _hash, bool is_code) : path{_path}, hash{_hash}, code{is_code} {
        }
    };

    // syncMemBatch is an in-memory buffer of successfully downloaded but not yet
    // persisted data items.
    class syncMemBatch {
    public:
        std::map<xhash256_t, xbytes_t> nodes;  // In-memory membatch of recently completed nodes
        std::map<xhash256_t, xbytes_t> codes;  // In-memory membatch of recently completed codes

    public:
        inline bool hasNode(xhash256_t const & hash) const {
            return nodes.find(hash) != nodes.end();
        }
        inline bool hasCode(xhash256_t const & hash) const {
            return codes.find(hash) != codes.end();
        }

        inline void clear() {
            nodes.clear();
            codes.clear();
        }
    };

private:
    xkv_db_face_ptr_t database{nullptr};                                    // Persistent database to check for existing entries
    syncMemBatch membatch;                                                  // Memory buffer to avoid frequent database writes
    std::map<xhash256_t, request *> nodeReqs;                               // Pending requests pertaining to a trie node hash
    std::map<xhash256_t, request *> codeReqs;                               // Pending requests pertaining to a code hash
    top::threading::xthreadsafe_priority_queue<xhash256_t, int64_t> queue;  // Priority queue with the pending requests
    std::map<std::size_t, std::size_t> fetches;                             // Number of active fetches per trie node depth

public:
    Sync(xhash256_t const & root, xkv_db_face_ptr_t _database, LeafCallback callback);
    static std::shared_ptr<Sync> NewSync(xhash256_t const & root, xkv_db_face_ptr_t _database, LeafCallback callback);

    Sync(Sync const &) = delete;
    Sync & operator=(Sync const &) = delete;
    Sync(Sync &&) = default;
    Sync & operator=(Sync &&) = default;
    ~Sync();

public:
    // AddSubTrie registers a new trie to the sync code, rooted at the designated parent.
    void AddSubTrie(xhash256_t const & root, xbytes_t const & path, xhash256_t const & parent, LeafCallback callback);

    // AddCodeEntry schedules the direct retrieval of a contract code that should not
    // be interpreted as a trie node, but rather accepted and stored into the database
    // as is.
    void AddCodeEntry(xhash256_t const & hash, xbytes_t const & path, xhash256_t const & parent);

    // Missing retrieves the known missing nodes from the trie for retrieval. To aid
    // both eth/6x style fast sync and snap/1x style state sync, the paths of trie
    // nodes are returned too, as well as separate hash list for codes.
    // return type: <nodes, SyncPath, codes>
    std::tuple<std::vector<xhash256_t>, std::vector<SyncPath>, std::vector<xhash256_t>> Missing(std::size_t max);

    // Process injects the received data for requested item. Note it can
    // happpen that the single response commits two pending requests(e.g.
    // there are two requests one for code and one for node but the hash
    // is same). In this case the second response for the same hash will
    // be treated as "non-requested" item or "already-processed" item but
    // there is no downside.
    void Process(SyncResult const & result, std::error_code & ec);

    // Commit flushes the data stored in the internal membatch out to persistent
    // storage, returning any occurred error.
    void Commit(xkv_db_face_ptr_t db);

    // Pending returns the number of state entries currently pending for download.
    std::size_t Pending() const;

private:
    // schedule inserts a new state retrieval request into the fetch queue. If there
    // is already a pending request for this node, the new request will be discarded
    // and only a parent reference added to the old one.
    void schedule(request * req);

    // children retrieves all the missing children of a state trie entry for future
    // retrieval scheduling.
    std::vector<request *> children(request * req, xtrie_node_face_ptr_t object, std::error_code & ec);

    // commit finalizes a retrieval request and stores it into the membatch. If any
    // of the referencing parent requests complete due to this commit, they are also
    // committed themselves.
    void commit(request * req, std::error_code & ec);
};

NS_END3