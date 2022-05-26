// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_proof.h"

#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/trie/xtrie_node_coding.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG3(top, evm_common, trie)
xbytes_t VerifyProof(xhash256_t rootHash, xbytes_t const & _key, xkv_db_face_ptr_t proofDB, std::error_code & ec) {
    auto key = keybytesToHex(_key);
    xdbg("[VerifyProof]: roothash: %s", rootHash.as_hex_str());
    xhash256_t wantHash = rootHash;
    for (std::size_t index = 0;; index++) {
        auto buf = proofDB->Get(xbytes_t{wantHash.begin(), wantHash.end()}, ec);
        if (buf.empty()) {
            xwarn("proof node %zu (hash: %s) missing", index, wantHash.as_hex_str());
            ec = error::xerrc_t::trie_proof_missing;
            return {};
        }
        auto n = xtrie_node_rlp::decodeNode(wantHash, buf, ec);
        if (ec) {
            return {};
        }
        xbytes_t keyrest;
        xtrie_node_face_ptr_t cld;
        std::tie(keyrest, cld) = get(n, key, true, ec);
        if (cld == nullptr) {
            return {};
        }
        switch (cld->type()) {
        case xtrie_node_type_t::hashnode: {
            key = keyrest;
            auto cld_n = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(cld.get())));
            wantHash = xhash256_t{cld_n->data()};
            break;
        }
        case xtrie_node_type_t::valuenode: {
            auto cld_n = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(cld.get())));
            return cld_n->data();
        }
        default: {
            // pass;
        }
        }
    }
    __builtin_unreachable();
}

std::pair<xbytes_t, xtrie_node_face_ptr_t> get(xtrie_node_face_ptr_t tn, xbytes_t key, bool skipResolved, std::error_code & ec) {
    for (;;) {
        if (tn == nullptr) {
            return std::make_pair(key, nullptr);
        }
        switch (tn->type()) {
        case xtrie_node_type_t::shortnode: {
            auto n = std::make_shared<xtrie_short_node_t>(*(static_cast<xtrie_short_node_t *>(tn.get())));
            if (key.size() < n->Key.size() || !std::equal(n->Key.begin(), n->Key.end(), key.begin())) {
                return std::make_pair(xbytes_t{}, nullptr);
            }
            tn = n->Val;
            key = xbytes_t{key.begin() + n->Key.size(), key.end()};
            if (!skipResolved) {
                return std::make_pair(key, tn);
            }
            break;
        }
        case xtrie_node_type_t::fullnode: {
            auto n = std::make_shared<xtrie_full_node_t>(*(static_cast<xtrie_full_node_t *>(tn.get())));
            tn = n->Children[key[0]];
            key = xbytes_t{key.begin() + 1, key.end()};
            if (!skipResolved) {
                return std::make_pair(key, tn);
            }
            break;
        }
        case xtrie_node_type_t::hashnode: {
            auto n = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(tn.get())));
            return std::make_pair(key, n);
        }
        case xtrie_node_type_t::valuenode: {
            auto n = std::make_shared<xtrie_value_node_t>(*(static_cast<xtrie_value_node_t *>(tn.get())));
            return std::make_pair(xbytes_t{}, n);
        }
        case xtrie_node_type_t::invalid:{
            return std::make_pair(key, nullptr);
        }
        default: {
            xassert(false);
        }
        }
    }
    __builtin_unreachable();
}

NS_END3