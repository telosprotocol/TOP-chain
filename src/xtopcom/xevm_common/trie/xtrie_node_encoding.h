// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xevm_common/trie/xtrie_node.h"

NS_BEG3(top, evm_common, trie)

static xbytes_t EncodeToBytes(xtrie_node_face_ptr_t node) {
    assert(node);
    // printf("type: %d\n", node->type());
    switch (node->type()) {
    case xtrie_node_type_t::fullnode: {
        auto fn = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(node.get())));
        xbytes_t buf;
        std::error_code ec;
        fn->EncodeRLP(buf, ec);
        // printf("buf: %s", top::to_hex(buf).c_str());
        return buf;
    }
    case xtrie_node_type_t::shortnode: {
        auto sn = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(node.get())));
        xbytes_t buf;
        std::error_code ec;
        sn->EncodeRLP(buf, ec);
        // printf("buf: %s", top::to_hex(buf).c_str());
        return buf;
    }
    case xtrie_node_type_t::hashnode: {
        auto hn = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(node.get())));
        return hn->data();
    }
    case xtrie_node_type_t::valuenode: {
        auto vn = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(node.get())));
        return vn->data();
    }
    default:
        xassert(false);
        return {};
    }
}

NS_END3