// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xevm_common/trie/xtrie_kv_db_face.h"

#include <algorithm>
#include <array>
#include <memory>

NS_BEG3(top, evm_common, trie)

enum class xtop_stack_trie_node_type : uint8_t {
    emptyNode = 0,
    branchNode = 1,
    extNode = 2,
    leafNode = 3,
    hashedNode = 4,
};
using xstack_trie_node_type_t = xtop_stack_trie_node_type;

// class xtop_stack_trie;

class xtop_stack_trie {
private:
    xstack_trie_node_type_t m_type{xstack_trie_node_type_t::emptyNode};
    xbytes_t m_val;
    xbytes_t m_key;
    std::array<std::shared_ptr<xtop_stack_trie>, 16> m_children;
    xkv_db_face_ptr_t m_db;

public:
    xtop_stack_trie(xkv_db_face_ptr_t _db) : m_db{_db} {
    }

public:
    static std::shared_ptr<xtop_stack_trie> NewStackTrie(xkv_db_face_ptr_t db);

public:
    // TryUpdate inserts a (key, value) pair into the stack trie
    void Update(xbytes_t const & key, xbytes_t const & value);

    void TryUpdate(xbytes_t const & key, xbytes_t const & value, std::error_code & ec);

    // hash() hashes the node 'st' and converts it into 'hashedNode', if possible.
    // Possible outcomes:
    // 1. The rlp-encoded value was >= 32 bytes:
    //  - Then the 32-byte `hash` will be accessible in `st.val`.
    //  - And the 'st.type' will be 'hashedNode'
    // 2. The rlp-encoded value was < 32 bytes
    //  - Then the <32 byte rlp-encoded value will be accessible in 'st.val'.
    //  - And the 'st.type' will be 'hashedNode' AGAIN
    //
    // This method will also:
    // set 'st.type' to hashedNode
    // clear 'st.key'
    void hash();

    // Hash returns the hash of the current node
    xh256_t Hash();

    // Commit will firstly hash the entrie trie if it's still not hashed
    // and then commit all nodes to the associated database. Actually most
    // of the trie nodes MAY have been committed already. The main purpose
    // here is to commit the root node.
    //
    // The associated database is expected, otherwise the whole commit
    // functionality should be disabled.
    xh256_t Commit(std::error_code & ec);

public:
    xstack_trie_node_type_t type() {
        return m_type;
    }

private:
    // Helper function to that inserts a (key, value) pair into the trie.
    void insert(xbytes_t const & key, xbytes_t const & value);

    // Helper function that, given a full key, determines the index
    // at which the chunk pointed by st.keyOffset is different from
    // the same chunk in the full key.
    std::size_t getDiffIndex(xbytes_t const & key) {
        assert(key.size() >= m_key.size());
        for (std::size_t index = 0; index < std::min(m_key.size(), key.size()); ++index) {
            auto nibble = m_key[index];
            if (nibble != key[index]) {
                return index;
            }
        }
        return m_key.size();
    }

private:
    std::shared_ptr<xtop_stack_trie> newLeaf(xbytes_t const & key, xbytes_t const & val);
    std::shared_ptr<xtop_stack_trie> newExt(xbytes_t const & key, std::shared_ptr<xtop_stack_trie> child);
};

using xstack_trie_t = xtop_stack_trie;

NS_END3