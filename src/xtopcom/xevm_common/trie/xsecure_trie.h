// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie.h"

NS_BEG3(top, evm_common, trie)

// SecureTrie wraps a trie with key hashing. In a secure trie, all
// access operations hash the key using keccak256. This prevents
// calling code from creating long chains of nodes that
// increase the access time.
//
// Contrary to a regular trie, a SecureTrie can only be created with
// New and must have an attached database. The database also stores
// the preimage of each key.
//
// SecureTrie is not safe for concurrent use.
class xtop_secure_trie {
private:
    xtrie_t m_trie;
    xbytes_t hashKeyBuf;
    std::shared_ptr<std::map<std::string, xbytes_t>> secKeyCache;
    std::shared_ptr<xtop_secure_trie> secKeyCacheOwner;  // Pointer to self, replace the key cache on mismatch

public:
    xtop_secure_trie(xtrie_t _trie) : m_trie{_trie} {
    }

public:
    // NewSecure creates a trie with an existing root node from a backing database
    // and optional intermediate in-memory node pool.
    //
    // If root is the zero hash or the sha3 hash of an empty string, the
    // trie is initially empty. Otherwise, New will panic if db is nil
    // and returns MissingNodeError if the root node cannot be found.
    //
    // Accessing the trie loads nodes from the database or node pool on demand.
    // Loaded nodes are kept around until their 'cache generation' expires.
    // A new cache generation is created by each call to Commit.
    // cachelimit sets the number of past cache generations to keep.
    static std::shared_ptr<xtop_secure_trie> NewSecure(xhash256_t root, xtrie_db_ptr_t db, std::error_code & ec);

    // Get returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    xbytes_t Get(xbytes_t const & key) const;

    // TryGet returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    // If a node was not found in the database, a MissingNodeError is returned.
    xbytes_t TryGet(xbytes_t const & key, std::error_code & ec) const;

    // todo TryGetNode

    // todo TryUpdateAccount

    // Update associates key with value in the trie. Subsequent calls to
    // Get will return value. If value has length zero, any existing value
    // is deleted from the trie and calls to Get will return nil.
    //
    // The value bytes must not be modified by the caller while they are
    // stored in the trie.
    void Update(xbytes_t const & key, xbytes_t const & value);

    // TryUpdate associates key with value in the trie. Subsequent calls to
    // Get will return value. If value has length zero, any existing value
    // is deleted from the trie and calls to Get will return nil.
    //
    // The value bytes must not be modified by the caller while they are
    // stored in the trie.
    //
    // If a node was not found in the database, a MissingNodeError is returned.
    void TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec);

    // Delete removes any existing value for key from the trie.
    void Delete(xbytes_t const & key);

    // TryDelete removes any existing value for key from the trie.
    // If a node was not found in the database, a MissingNodeError is returned.
    void TryDelete(xbytes_t const & key, std::error_code & ec);

    // GetKey returns the sha3 preimage of a hashed key that was
    // previously used to store a value.
    xbytes_t GetKey(xbytes_t const & shaKey);

    // Commit writes all nodes and the secure hash pre-images to the trie's database.
    // Nodes are stored with their sha3 hash as the key.
    //
    // Committing flushes nodes from memory. Subsequent Get calls will load nodes
    // from the database.
    std::pair<xhash256_t, int32_t> Commit(std::error_code & ec);

    // Hash returns the root hash of SecureTrie. It does not write to the
    // database and can be used even if the trie doesn't have one.
    xhash256_t Hash();

    std::shared_ptr<xtop_secure_trie> copy() {
        return std::make_shared<xtop_secure_trie>(*this);
    }

    // todo NodeIterator

private:
    // hashKey returns the hash of key as an ephemeral buffer.
    // The caller must not hold onto the return value because it will become
    // invalid on the next call to hashKey or secKey.
    xbytes_t hashKey(xbytes_t const & key) const;

    // getSecKeyCache returns the current secure key cache, creating a new one if
    // ownership changed (i.e. the current secure trie is a copy of another owning
    // the actual cache).
    std::shared_ptr<std::map<std::string, xbytes_t>> getSecKeyCache() {
        if (this != secKeyCacheOwner.get()) {
            secKeyCacheOwner = std::make_shared<xtop_secure_trie>(*this);
            secKeyCache = std::make_shared<std::map<std::string, xbytes_t>>();
        }
        return secKeyCache;
    }
};
using xsecure_trie_t = xtop_secure_trie;
using xsecure_trie_ptr_t = std::shared_ptr<xsecure_trie_t>;

NS_END3