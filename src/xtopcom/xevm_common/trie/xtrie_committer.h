// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_db.h"
#include "xevm_common/trie/xtrie_node.h"

NS_BEG3(top, evm_common, trie)

class xtop_trie_committer {
public:
    // Commit collapses a node down into a hash node and inserts it into the database
    std::pair<xtrie_hash_node_ptr_t, int32_t> Commit(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec);

private:
    // commit collapses a node down into a hash node and inserts it into the database
    std::pair<xtrie_node_face_ptr_t, int32_t> commit(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec);

    // commitChildren commits the children of the given fullnode
    std::pair<std::array<xtrie_node_face_ptr_t, 17>, int32_t> commitChildren(xtrie_full_node_ptr_t n, xtrie_db_ptr_t db, std::error_code & ec);

    // store hashes the node n and if we have a storage layer specified, it writes
    // the key/value pair to it and tracks any node->child references as well as any
    // node->external trie references.
    xtrie_node_face_ptr_t store(xtrie_node_face_ptr_t n, xtrie_db_ptr_t db);

private:
    // estimateSize estimates the size of an rlp-encoded node, without actually
    // rlp-encoding it (zero allocs). This method has been experimentally tried, and with a trie
    // with 1000 leafs, the only errors above 1% are on small shortnodes, where this
    // method overestimates by 2 or 3 bytes (e.g. 37 instead of 35)
    int32_t estimateSize(xtrie_node_face_ptr_t n);
};
using xtrie_committer_t = xtop_trie_committer;

NS_END3