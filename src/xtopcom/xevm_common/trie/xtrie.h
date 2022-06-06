// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "assert.h"
#include "xbase/xns_macro.h"
#include "xbasic/xhash.hpp"
#include "xbasic/xhex.h"
#include "xevm_common/trie/xtrie_db_face.h"
#include "xevm_common/trie/xtrie_node.h"
#include "xevm_common/trie/xtrie_node_coding.h"

#include <algorithm>
#include <tuple>
#include <type_traits>

NS_BEG3(top, evm_common, trie)

class xtop_trie {
private:
    xtrie_db_face_ptr_t m_db;
    xtrie_node_face_ptr_t m_root;

    std::size_t unhashed{0};

public:
    xtop_trie(xtrie_db_face_ptr_t db) : m_db{db} {
    }

public:
    static std::shared_ptr<xtop_trie> New(xhash256_t hash, xtrie_db_face_ptr_t db, std::error_code & ec);

public:
    // Reset drops the referenced root node and cleans all internal state.
    void Reset();

    // todo tmp solution...
    xbytes_t Encode() {
        return xtrie_node_rlp::EncodeToBytes(m_root);
    }

    // Hash returns the root hash of the trie. It does not write to the
    // database and can be used even if the trie doesn't have one.
    xhash256_t Hash();

    // Get returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    xbytes_t Get(xbytes_t const & key);

    // TryGet returns the value for key stored in the trie.
    // The value bytes must not be modified by the caller.
    // If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
    xbytes_t TryGet(xbytes_t const & key, std::error_code & ec);

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
    // If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
    void TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec);

    // Delete removes any existing value for key from the trie.
    void Delete(xbytes_t const & key);

    // TryDelete removes any existing value for key from the trie.
    // If a node was not found in the database, a MissingNodeError(trie_db_missing_node_error) is returned.
    void TryDelete(xbytes_t const & key, std::error_code & ec);

    // Commit writes all nodes to the trie's memory database, tracking the internal
    // and external (for account tries) references.
    std::pair<xhash256_t, int32_t> Commit(std::error_code & ec);

    // Prove constructs a merkle proof for key. The result contains all encoded nodes
    // on the path to the value at key. The value itself is also included in the last
    // node and can be retrieved by verifying the proof.
    //
    // If the trie does not contain a value for key, the returned proof contains all
    // nodes of the longest existing prefix of the key (at least the root node), ending
    // with the node that proves the absence of the key.
    bool Prove(xbytes_t const & key, uint32_t fromLevel, xkv_db_face_ptr_t proofDB, std::error_code & ec);

private:
    std::tuple<xbytes_t, xtrie_node_face_ptr_t, bool> tryGet(xtrie_node_face_ptr_t node, xbytes_t const & key, std::size_t const pos, std::error_code & ec);

private:
    std::pair<bool, xtrie_node_face_ptr_t> insert(xtrie_node_face_ptr_t node, xbytes_t prefix, xbytes_t key, xtrie_node_face_ptr_t value, std::error_code & ec);

    std::pair<bool, xtrie_node_face_ptr_t> erase(xtrie_node_face_ptr_t node, xbytes_t prefix, xbytes_t key, std::error_code & ec);

private:
    xtrie_node_face_ptr_t resolve(xtrie_node_face_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec);

    xtrie_node_face_ptr_t resolveHash(xtrie_hash_node_ptr_t n, /*xbytes_t prefix,*/ std::error_code & ec);

    // hashRoot calculates the root hash of the given trie
    std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> hashRoot();

private:
    bool EqualBytes(xbytes_t const & a, xbytes_t const & b) {
        return (a.size() == b.size()) && std::equal(a.begin(), a.end(), b.begin());
    }

    inline nodeFlag newFlag() {
        nodeFlag f;
        f.dirty = true;
        return f;
    }
};
using xtrie_t = xtop_trie;

NS_END3