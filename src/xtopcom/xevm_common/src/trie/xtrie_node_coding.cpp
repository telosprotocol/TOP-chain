// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_node_coding.h"

#include "xevm_common/rlp/xrlp_raw.h"
#include "xevm_common/trie/xtrie_encoding.h"
#include "xevm_common/xerror/xerror.h"

NS_BEG3(top, evm_common, trie)
xbytes_t xtop_trie_node_rlp::EncodeToBytes(xtrie_node_face_ptr_t node) {
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

xtrie_node_face_ptr_t xtop_trie_node_rlp::mustDecodeNode(xhash256_t const & hash_bytes, xbytes_t const & buf) {
    std::error_code ec;
    auto n = decodeNode(hash_bytes, buf, ec);
    if (ec) {
        xerror("decode error: %s", ec.message().c_str());
    }
    return n;
}

xtrie_node_face_ptr_t xtop_trie_node_rlp::decodeNode(xhash256_t const & hash_bytes, xbytes_t const & buf, std::error_code & ec) {
    return decodeNode(xtrie_hash_node_t{hash_bytes}, buf, ec);
}

xtrie_node_face_ptr_t xtop_trie_node_rlp::decodeNode(xtrie_hash_node_t hash, xbytes_t const & buf, std::error_code & ec) {
    if (buf.empty()) {
        ec = error::xerrc_t::not_enough_data;
        return nullptr;
    }
    auto elems = rlp::SplitList(buf, ec).first;
    xdbg("decodeNode: elems.size(): %zu", elems.size());
    if (ec) {
        xwarn("decode error: %s", ec.message().c_str());
        return nullptr;
    }
    auto c = rlp::CountValue(elems, ec);
    xdbg("decodeNode: elems.value.counts: %zu", c);
    if (ec) {
        xwarn("decode error: %s", ec.message().c_str());
        return nullptr;
    }

    if (c == 2) {
        auto n = decodeShort(hash, elems, ec);
        return n;
    } else if (c == 17) {
        auto n = decodeFull(hash, elems, ec);
        return n;
    } else {
        ec = error::xerrc_t::not_enough_data;
        xwarn("invalid list element number (%zu) for trie", c);
    }

    return nullptr;
}

xtrie_node_face_ptr_t xtop_trie_node_rlp::decodeShort(xtrie_hash_node_t hash, xbytes_t const & elems, std::error_code & ec) {
    xbytes_t kbuf, rest;
    std::tie(kbuf, rest) = rlp::SplitString(elems, ec);
    if (ec) {
        return nullptr;
    }
    auto flag = nodeFlag{hash};
    auto key = compactToHex(kbuf);
    if (hasTerm(key)) {
        // value node
        xbytes_t val, _;
        std::tie(val, _) = rlp::SplitString(rest, ec);
        if (ec) {
            xwarn("decode error: %s", ec.message().c_str());
            return nullptr;
        }
        return std::make_shared<xtrie_short_node_t>(key, std::make_shared<xtrie_value_node_t>(val), flag);
    }
    xtrie_node_face_ptr_t r;
    xbytes_t _;
    std::tie(r, _) = decodeRef(rest, ec);
    if (ec) {
        xwarn("decode error: %s", ec.message().c_str());
        return nullptr;
    }
    return std::make_shared<xtrie_short_node_t>(key, r, flag);

    return nullptr;
}
xtrie_node_face_ptr_t xtop_trie_node_rlp::decodeFull(xtrie_hash_node_t hash, xbytes_t const & elems, std::error_code & ec) {
    auto e = elems;
    xtrie_full_node_ptr_t n = std::make_shared<xtrie_full_node_t>(nodeFlag{hash});
    for (std::size_t i = 0; i < 16; ++i) {
        xtrie_node_face_ptr_t cld;
        xbytes_t rest;
        xdbg("decodeFull: do decodeChildren: %zu , elems:size():%zu", i, e.size());
        std::tie(cld, rest) = decodeRef(e, ec);
        if (ec) {
            xwarn("decode error: %s", ec.message().c_str());
            return n;
        }
        n->Children[i] = cld;
        e = rest;
    }
    xbytes_t val, _;
    std::tie(val, _) = rlp::SplitString(e, ec);
    if (ec) {
        xwarn("decode error: %s", ec.message().c_str());
        return n;
    }
    if (!val.empty()) {
        xdbg("decodeFull: get value: %s", to_string(val).c_str());
        n->Children[16] = std::make_shared<xtrie_value_node_t>(val);
    }
    return n;
}

std::pair<xtrie_node_face_ptr_t, xbytes_t> xtop_trie_node_rlp::decodeRef(xbytes_t const & buf, std::error_code & ec) {
    rlp::xrlp_elem_kind kind;
    xbytes_t val, rest;
    xdbg("decodeRef: buf.size():%zu", buf.size());
    std::tie(kind, val, rest) = rlp::Split(buf, ec);
    xdbg("decodeRef: split result: %zu %zu %zu", val.size(), rest.size(), buf.size());
    if (ec) {
        return std::make_pair(nullptr, buf);
    }
    if (kind == rlp::xrlp_elem_kind::List) {
        // 'embedded' node reference. The encoding must be smaller
        // than a hash in order to be valid.
        auto size = buf.size() - rest.size();
        if (size > 32) {  // hashLen
            ec = error::xerrc_t::rlp_oversized;
            return std::make_pair(nullptr, buf);
        }
        auto n = decodeNode(xbytes_t{}, buf, ec);
        return std::make_pair(n, rest);
    } else if (kind == rlp::xrlp_elem_kind::String && val.size() == 0) {
        // emtpy node
        return std::make_pair(nullptr, rest);
    } else if (kind == rlp::xrlp_elem_kind::String && val.size() == 32) {
        return std::make_pair(std::make_shared<xtrie_hash_node_t>(val), rest);
    } else {
        xwarn("decode error :invalid RLP string size %d (want 0 or 32)", val.size());
        return std::make_pair(nullptr, xbytes_t{});
    }

    __builtin_unreachable();
}
NS_END3