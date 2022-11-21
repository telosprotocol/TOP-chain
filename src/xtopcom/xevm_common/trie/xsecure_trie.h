// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie.h"
#include "xevm_common/trie/xtrie_face.h"

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
class xtop_secure_trie : public xtrie_face_t {
private:
    std::shared_ptr<xtrie_t> m_trie;
    xbytes_t hashKeyBuf;
    std::shared_ptr<std::map<std::string, xbytes_t>> secKeyCache;
    xtop_secure_trie * secKeyCacheOwner{nullptr};  // Pointer to self, replace the key cache on mismatch

protected:
    xtop_secure_trie(std::shared_ptr<xtrie_t> trie) : m_trie{std::move(trie)} {
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
    static std::shared_ptr<xtop_secure_trie> build_from(xh256_t const & root, xtrie_db_ptr_t db, std::error_code & ec);

    // Get returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    xbytes_t get(xbytes_t const & key) const;

    // TryGet returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    // If a node was not found in the database, a MissingNodeError is returned.
    xbytes_t try_get(xbytes_t const & key, std::error_code & ec) const override;

    // TryGetNode attempts to retrieve a trie node by compact-encoded path. It is not
    // possible to use keybyte-encoding as the path might contain odd nibbles.
    std::pair<xbytes_t, std::size_t> try_get_node(xbytes_t const & path, std::error_code & ec);

    // Update associates key with value in the trie. Subsequent calls to
    // Get will return value. If value has length zero, any existing value
    // is deleted from the trie and calls to Get will return nil.
    //
    // The value bytes must not be modified by the caller while they are
    // stored in the trie.
    void update(xbytes_t const & key, xbytes_t const & value);

    // TryUpdate associates key with value in the trie. Subsequent calls to
    // Get will return value. If value has length zero, any existing value
    // is deleted from the trie and calls to Get will return nil.
    //
    // The value bytes must not be modified by the caller while they are
    // stored in the trie.
    //
    // If a node was not found in the database, a MissingNodeError is returned.
    void try_update(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) override;

    // Delete removes any existing value for key from the trie.
    void Delete(xbytes_t const & key);

    // TryDelete removes any existing value for key from the trie.
    // If a node was not found in the database, a MissingNodeError is returned.
    void try_delete(xbytes_t const & key, std::error_code & ec) override;

    // GetKey returns the sha3 preimage of a hashed key that was
    // previously used to store a value.
    xbytes_t get_key(xbytes_t const & shaKey);

    // Commit writes all nodes and the secure hash pre-images to the trie's database.
    // Nodes are stored with their sha3 hash as the key.
    //
    // Committing flushes nodes from memory. Subsequent Get calls will load nodes
    // from the database.
    std::pair<xh256_t, int32_t> commit(std::error_code & ec) override;

    // Hash returns the root hash of SecureTrie. It does not write to the
    // database and can be used even if the trie doesn't have one.
    xh256_t hash() override;

    std::shared_ptr<xtop_secure_trie> copy() {
        return std::make_shared<xtop_secure_trie>(*this);
    }

    // Prove constructs a merkle proof for key. The result contains all encoded nodes
    // on the path to the value at key. The value itself is also included in the last
    // node and can be retrieved by verifying the proof.
    //
    // If the trie does not contain a value for key, the returned proof contains all
    // nodes of the longest existing prefix of the key (at least the root node), ending
    // with the node that proves the absence of the key.
    bool prove(xbytes_t const & key, uint32_t fromLevel, xkv_db_face_ptr_t proofDB, std::error_code & ec) override {
        assert(m_trie != nullptr);
        return m_trie->prove(key, fromLevel, proofDB, ec);
    }

    void prune(xh256_t const & old_trie_root_hash, std::error_code & ec) override;

    void commit_pruned(std::error_code & ec) override;

private:
    // hashKey returns the hash of key as an ephemeral buffer.
    // The caller must not hold onto the return value because it will become
    // invalid on the next call to hashKey or secKey.
    xbytes_t hash_key(xbytes_t const & key) const;

    // getSecKeyCache returns the current secure key cache, creating a new one if
    // ownership changed (i.e. the current secure trie is a copy of another owning
    // the actual cache).
    std::shared_ptr<std::map<std::string, xbytes_t>> get_sec_key_cache() {
        if (secKeyCacheOwner == nullptr || this != secKeyCacheOwner) {
            xdbg("getSecKeyCache new, old is %p", secKeyCacheOwner);
            secKeyCacheOwner = this;
            secKeyCache = std::make_shared<std::map<std::string, xbytes_t>>();
            assert(secKeyCacheOwner == this);
        }
        xdbg("getSecKeyCache at %p", secKeyCacheOwner);
        return secKeyCache;
    }
};
using xsecure_trie_t = xtop_secure_trie;
using xsecure_trie_ptr_t = std::shared_ptr<xsecure_trie_t>;

NS_END3