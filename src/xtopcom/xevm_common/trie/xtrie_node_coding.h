// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/rlp.h"
#include "xevm_common/trie/xtrie_node.h"

NS_BEG3(top, evm_common, trie)

class xtop_trie_node_rlp {
public:
    static xbytes_t EncodeToBytes(xtrie_node_face_ptr_t node);

    static xtrie_node_face_ptr_t decodeNode(xtrie_hash_node_t hash, xbytes_t const & buf, std::error_code & ec);

private:
    static xtrie_node_face_ptr_t decodeShort(xtrie_hash_node_t hash, xbytes_t elems, std::error_code & ec);
    static xtrie_node_face_ptr_t decodeFull(xtrie_hash_node_t hash, xbytes_t elems, std::error_code & ec);
    static std::pair<xtrie_node_face_ptr_t, xbytes_t> decodeRef(xbytes_t const & buf, std::error_code & ec);
};
using xtrie_node_rlp = xtop_trie_node_rlp;
NS_END3