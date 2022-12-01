// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_node.h"

NS_BEG3(top, evm_common, trie)

class xtop_trie_hasher {
    class sliceBuffer {
    private:
        xbytes_t m_data;

    public:
        std::size_t Write(xbytes_t const & data) {
            m_data.insert(m_data.end(), data.begin(), data.end());
            return m_data.size();
        }

        void Reset() {
            m_data.clear();
        }

        std::size_t len() const {
            return m_data.size();
        }

        xbytes_t & data() {
            return m_data;
        }
    };

private:
    bool m_parallel{false};  // whether to use paralallel threads when hashing
    sliceBuffer tmp;

public:
    xtop_trie_hasher(bool parallel) : m_parallel{parallel} {
    }

public:
    static xtop_trie_hasher newHasher(bool parallel) {
        return xtop_trie_hasher{parallel};
    }

public:
    /**
     * @brief hash collapses a node down into a hash node, also returning a copy of the
     * original node initialized with the computed hash to replace the original one.
     *
     * @param node original node
     * @param force bool
     * @return (hashed, cached)
     */
    std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> hash(xtrie_node_face_ptr_t const & node, bool force);

    // hashData hashes the provided data
    xtrie_hash_node_ptr_t hashData(xbytes_t const & input) const;

public:
    // proofHash is used to construct trie proofs, and returns the 'collapsed'
    // node (for later RLP encoding) aswell as the hashed node -- unless the
    // node is smaller than 32 bytes, in which case it will be returned as is.
    // This method does not do anything on value- or hash-nodes.
    std::pair<xtrie_node_face_ptr_t, xtrie_node_face_ptr_t> proofHash(xtrie_node_face_ptr_t node);

private:
    std::pair<xtrie_short_node_ptr_t, xtrie_short_node_ptr_t> hashShortNodeChildren(xtrie_short_node_ptr_t const & node);

    std::pair<xtrie_full_node_ptr_t, xtrie_full_node_ptr_t> hashFullNodeChildren(xtrie_full_node_ptr_t const & node);

    xtrie_node_face_ptr_t shortnodeToHash(xtrie_short_node_ptr_t node, bool force);

    xtrie_node_face_ptr_t fullnodeToHash(xtrie_full_node_ptr_t node, bool force);
};
using xtrie_hasher_t = xtop_trie_hasher;

NS_END3