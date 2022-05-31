// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_node.h"

#include "xevm_common/rlp.h"

NS_BEG3(top, evm_common, trie)

void xtop_trie_short_node::EncodeRLP(xbytes_t & buf, std::error_code & ec) {
    // todo . how to recursive encode a trie.
    xbytes_t encoded;
    append(encoded, RLP::encode(Key));
    if (Val->type() == xtrie_node_type_t::hashnode) {
        auto child = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(Val.get())));
        append(encoded, RLP::encode(child->data()));
    } else if (Val->type() == xtrie_node_type_t::valuenode) {
        auto child = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(Val.get())));
        append(encoded, RLP::encode(child->data()));
    } else if (Val->type() == xtrie_node_type_t::fullnode) {
        auto child_node = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(Val.get())));
        child_node->EncodeRLP(encoded, ec);
    } else {
        printf("!!!! shortnode not encode type: %d\n", static_cast<uint8_t>(Val->type()));
    }
    append(buf, RLP::encodeList(encoded));
}

void xtop_trie_full_node::EncodeRLP(xbytes_t & buf, std::error_code & ec) {
    // todo . how to recursive encode a trie.
    xbytes_t encoded;
    for (auto const & child : Children) {
        if (child == nullptr) {
            // printf("encode nullptr value\n");
            append(encoded, RLP::encode(nilValueNode.data()));
            continue;
        }
        if (child->type() == xtrie_node_type_t::hashnode) {
            auto child_node = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(child.get())));
            // printf("encode hash child node\n");
            append(encoded, RLP::encode(child_node->data()));
        } else if (child->type() == xtrie_node_type_t::shortnode) {
            auto child_node = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(child.get())));
            // printf("encode short child node\n");
            child_node->EncodeRLP(encoded, ec);
        } else if (child->type() == xtrie_node_type_t::valuenode) {
            auto child_node = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(child.get())));
            // printf("encode value child node\n");
            append(encoded, RLP::encode(child_node->data()));
        } else {
            printf("!!! full node not encode child type: %d\n", static_cast<uint8_t>(child->type()));
        }
    }
    append(buf, RLP::encodeList(encoded));
}

NS_END3