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
    xh256_t Hash;  // Hash of the originally unknown trie node
    xbytes_t Data;    // Data content of the retrieved node
};

// Sync is the main state trie synchronisation scheduler, which provides yet
// unknown trie hashes to retrieve, accepts node data associated with said hashes
// and reconstructs the trie step by step until all is done.
class Sync {
private:
    // request represents a scheduled or already in-flight state retrieval request.
    struct request {
        xbytes_t path;    // Merkle path leading to this node for prioritization
        xh256_t hash;  // Hash of the node data content to retrieve
        xbytes_t data;    // Data content of the node, cached until all subtrees complete

        xbytes_t unit_sync_key;   // Unit key is different from data hash
        xbytes_t unit_store_key;  // Unit key is different from data hash
        bool unit{false};         // Whether this is a unit entry

        std::vector<std::shared_ptr<request>> parents;  // Parent state nodes referencing this entry (notify all upon completion)
        std::size_t deps{0};                            // Number of dependencies before allowed to commit this node

        leaf_callback callback{nullptr};  // Callback to invoke if a leaf node it reached on this branch

        request(xbytes_t const & _path, xh256_t const & _hash, leaf_callback _callback) : path{_path}, hash{_hash}, callback{_callback} {
        }
        request(xbytes_t const & _path, xh256_t const & _hash, xbytes_t const & _unit_sync_key, xbytes_t const & _unit_store_key)
          : path{_path}, hash{_hash}, unit_sync_key(_unit_sync_key), unit_store_key(_unit_store_key), unit{true} {
        }
    };

    // syncMemBatch is an in-memory buffer of successfully downloaded but not yet
    // persisted data items.
    class syncMemBatch {
    public:
        std::map<xbytes_t, xbytes_t> nodes;  // In-memory membatch of recently completed nodes
        std::map<xbytes_t, xbytes_t> units;  // In-memory membatch of recently completed codes

    public:
        inline bool hasNode(xbytes_t const & hash) const {
            return nodes.find(hash) != nodes.end();
        }
        inline bool hasUnit(xbytes_t const & hash) const {
            return units.find(hash) != units.end();
        }

        inline void clear() {
            nodes.clear();
            units.clear();
        }
    };

private:
    xh256_t syncRoot;
    xkv_db_face_ptr_t database{nullptr};                      // Persistent database to check for existing entries
    syncMemBatch membatch;                                    // Memory buffer to avoid frequent database writes
    std::map<xh256_t, std::shared_ptr<request>> nodeReqs;  // Pending requests pertaining to a trie node hash
    std::map<xh256_t, std::shared_ptr<request>> unitReqs;  // Pending requests pertaining to a code hash
    std::map<xbytes_t, xh256_t> unitKeys;
    top::threading::xthreadsafe_priority_queue<xbytes_t, int64_t> queue;  // Priority queue with the pending requests
    std::map<std::size_t, std::size_t> fetches;                           // Number of active fetches per trie node depth

public:
    Sync(xh256_t const & root, xkv_db_face_ptr_t _database, leaf_callback callback);
    Sync(xkv_db_face_ptr_t _database);

    static std::shared_ptr<Sync> NewSync(xh256_t const & root, xkv_db_face_ptr_t _database, leaf_callback callback);
    static std::shared_ptr<Sync> NewSync(xkv_db_face_ptr_t _database);

    Sync(Sync const &) = delete;
    Sync & operator=(Sync const &) = delete;
    Sync(Sync &&) = default;
    Sync & operator=(Sync &&) = default;
    ~Sync();

public:
    // Init
    void Init(xh256_t const & root, leaf_callback callback);

    // AddSubTrie registers a new trie to the sync code, rooted at the designated parent.
    void AddSubTrie(xh256_t const & root, xbytes_t const & path, xh256_t const & parent, leaf_callback callback);

    // AddUnitTrie registers unit index.
    void AddUnitEntry(xh256_t const & hash, xbytes_t const & path, xbytes_t const & unit_sync_key, xbytes_t const & unit_store_key, xh256_t const & parent);

    // Missing retrieves the known missing nodes from the trie for retrieval. To aid
    // both eth/6x style fast sync and snap/1x style state sync, the paths of trie
    // nodes are returned too, as well as separate hash list for codes.
    std::tuple<std::vector<xh256_t>, std::vector<xh256_t>, std::vector<xbytes_t>> Missing(std::size_t max);

    // Process injects the received data for requested item. Note it can
    // happpen that the single response commits two pending requests(e.g.
    // there are two requests one for code and one for node but the hash
    // is same). In this case the second response for the same hash will
    // be treated as "non-requested" item or "already-processed" item but
    // there is no downside.
    void Process(SyncResult const & result, std::error_code & ec);

    // ProcessUnit for unit
    void ProcessUnit(SyncResult const & result, std::error_code & ec);

    // Commit flushes the data stored in the internal membatch out to persistent
    // storage, returning any occurred error.
    void Commit(const xkv_db_face_ptr_t & db);

    void CommitUnit(xkv_db_face_ptr_t db);

    // Pending returns the number of state entries currently pending for download.
    std::size_t Pending() const;

    void clear();

private:
    // schedule inserts a new state retrieval request into the fetch queue. If there
    // is already a pending request for this node, the new request will be discarded
    // and only a parent reference added to the old one.
    void schedule(std::shared_ptr<request> req);

    // children retrieves all the missing children of a state trie entry for future
    // retrieval scheduling.
    std::vector<std::shared_ptr<request>> children(std::shared_ptr<request>, xtrie_node_face_ptr_t object, std::error_code & ec);

    // commit finalizes a retrieval request and stores it into the membatch. If any
    // of the referencing parent requests complete due to this commit, they are also
    // committed themselves.
    void commit(std::shared_ptr<request>, std::error_code & ec);
};

NS_END3