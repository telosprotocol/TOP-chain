// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xevm_common/trie/xtrie_node.h"

#include "xevm_common/rlp.h"

#include <cassert>

NS_BEG3(top, evm_common, trie)

xtop_trie_hash_node::xtop_trie_hash_node(xbytes_t data)
    : m_data{std::move(data)} {
}

xtop_trie_hash_node::xtop_trie_hash_node(xhash256_t const & hash) : m_data{hash.begin(), hash.end()} {
}

xbytes_t const & xtop_trie_hash_node::data() const noexcept {
    return m_data;
}

bool xtop_trie_hash_node::is_null() const noexcept {
    return m_data.empty();
}

std::string xtop_trie_hash_node::fstring(std::string const & ind) {
    return {};
}

std::pair<xtrie_hash_node_t, bool> xtop_trie_hash_node::cache() {
    return {{}, true};
}

xtrie_node_type_t xtop_trie_hash_node::type() const noexcept {
    return xtrie_node_type_t::hashnode;
}

xtop_trie_value_node::xtop_trie_value_node(xbytes_t data)
    : m_data{std::move(data)} {
}

xbytes_t const & xtop_trie_value_node::data() const noexcept {
    return m_data;
}

std::string xtop_trie_value_node::fstring(std::string const & ind) {
    return {};
}

std::pair<xtrie_hash_node_t, bool> xtop_trie_value_node::cache() {
    return {{}, true};
}

xtrie_node_type_t xtop_trie_value_node::type() const noexcept {
    return xtrie_node_type_t::valuenode;
}

xtop_node_flag::xtop_node_flag(xtrie_hash_node_t _hash)
    : hash{std::move(_hash)} {
}

xtop_trie_short_node::xtop_trie_short_node(xbytes_t _key, xtrie_node_face_ptr_t _val, xnode_flag_t flag)
    : key{std::move(_key)}, val{std::move(_val)}, flags{std::move(flag)} {
}

std::shared_ptr<xtop_trie_short_node> xtop_trie_short_node::clone() const {
    return std::make_shared<xtop_trie_short_node>(*this);
}

std::string xtop_trie_short_node::fstring(std::string const & ind) {
    return {};
}

std::pair<xtrie_hash_node_t, bool> xtop_trie_short_node::cache() {
    return {flags.hash, flags.dirty};
}

xtrie_node_type_t xtop_trie_short_node::type() const noexcept {
    return xtrie_node_type_t::shortnode;
}

void xtop_trie_short_node::EncodeRLP(xbytes_t & buf, std::error_code & ec) {
    xbytes_t encoded;
    append(encoded, RLP::encode(key));

    switch (val->type()) {
    case xtrie_node_type_t::hashnode: {
        assert(dynamic_cast<xtrie_hash_node_t *>(val.get()) != nullptr);
        append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_hash_node_t>(val)->data()));

        break;
    }

    case xtrie_node_type_t::valuenode: {
        assert(dynamic_cast<xtrie_value_node_t *>(val.get()) != nullptr);
        append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_value_node_t>(val)->data()));

        break;
    }

    case xtrie_node_type_t::fullnode: {
        assert(dynamic_cast<xtrie_full_node_t *>(val.get()) != nullptr);
        std::dynamic_pointer_cast<xtrie_full_node_t>(val)->EncodeRLP(encoded, ec);

        break;
    }

    default:
        assert(false);
        xwarn("!!!! shortnode not encode type: %d", static_cast<uint8_t>(val->type()));

        break;
    }

    append(buf, RLP::encodeList(encoded));
}

void xtop_trie_full_node::EncodeRLP(xbytes_t & buf, std::error_code & ec) {
    xbytes_t encoded;
    for (auto const & child : Children) {
        if (child == nullptr) {
            append(encoded, RLP::encode(nilValueNode.data()));  // 0x80 for empty bytes.
            continue;
        }

        switch (child->type()) {
        case xtrie_node_type_t::hashnode: {
            assert(dynamic_cast<xtrie_hash_node_t *>(child.get()) != nullptr);
            append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_hash_node_t>(child)->data()));

            break;
        }

        case xtrie_node_type_t::valuenode: {
            assert(dynamic_cast<xtrie_value_node_t *>(child.get()) != nullptr);
            append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_value_node_t>(child)->data()));

            break;
        }

        case xtrie_node_type_t::fullnode: {
            assert(dynamic_cast<xtrie_full_node_t *>(child.get()) != nullptr);
            std::dynamic_pointer_cast<xtrie_full_node_t>(child)->EncodeRLP(encoded, ec);

            break;
        }

        case xtrie_node_type_t::shortnode: {
            assert(dynamic_cast<xtrie_short_node_t *>(child.get()) != nullptr);
            std::dynamic_pointer_cast<xtrie_short_node_t>(child)->EncodeRLP(encoded, ec);

            break;
        }

        default:
            assert(false);
            xwarn("!!! full node not encode child type: %d", static_cast<uint8_t>(child->type()));

            break;
        }
    }
    append(buf, RLP::encodeList(encoded));
}

void xtop_trie_raw_full_node::EncodeRLP(xbytes_t & buf, std::error_code & ec) {
    xbytes_t encoded;
    ;
    for (auto const & child : Children) {
        if (child == nullptr) {
            append(encoded, RLP::encode(nilValueNode.data()));  // 0x80 for empty bytes.
            continue;
        }

        switch (child->type()) {
        case xtrie_node_type_t::hashnode: {
            assert(dynamic_cast<xtrie_hash_node_t *>(child.get()) != nullptr);
            append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_hash_node_t>(child)->data()));

            break;
        }

        case xtrie_node_type_t::valuenode: {
            assert(dynamic_cast<xtrie_value_node_t *>(child.get()) != nullptr);
            append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_value_node_t>(child)->data()));

            break;
        }

        case xtrie_node_type_t::fullnode: {
            assert(dynamic_cast<xtrie_full_node_t *>(child.get()) != nullptr);
            std::dynamic_pointer_cast<xtrie_full_node_t>(child)->EncodeRLP(encoded, ec);

            break;
        }

        case xtrie_node_type_t::shortnode: {
            assert(dynamic_cast<xtrie_short_node_t *>(child.get()) != nullptr);
            std::dynamic_pointer_cast<xtrie_short_node_t>(child)->EncodeRLP(encoded, ec);

            break;
        }

        case xtrie_node_type_t::rawshortnode: {
            assert(dynamic_cast<xtrie_raw_short_node_t *>(child.get()) != nullptr);
            std::dynamic_pointer_cast<xtrie_raw_short_node_t>(child)->EncodeRLP(encoded, ec);

            break;
        }

        default: {
            assert(false);
            xwarn("!!! raw full node not encode child type: %d", static_cast<uint8_t>(child->type()));

            break;
        }
        }
    }
    append(buf, RLP::encodeList(encoded));
}

void xtop_trie_raw_short_node::EncodeRLP(xbytes_t & buf, std::error_code & ec) {
    xbytes_t encoded;
    append(encoded, RLP::encode(Key));

    switch (Val->type()) {
    case xtrie_node_type_t::hashnode: {
        assert(dynamic_cast<xtrie_hash_node_t *>(Val.get()) != nullptr);
        append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_hash_node_t>(Val)->data()));

        break;
    }

    case xtrie_node_type_t::valuenode: {
        assert(dynamic_cast<xtrie_value_node_t *>(Val.get()) != nullptr);
        append(encoded, RLP::encode(std::dynamic_pointer_cast<xtrie_value_node_t>(Val)->data()));

        break;
    }

    case xtrie_node_type_t::fullnode: {
        assert(dynamic_cast<xtrie_full_node_t *>(Val.get()) != nullptr);
        std::dynamic_pointer_cast<xtrie_full_node_t>(Val)->EncodeRLP(encoded, ec);

        break;
    }

    case xtrie_node_type_t::rawfullnode: {
        assert(dynamic_cast<xtrie_raw_full_node_t *>(Val.get()) != nullptr);
        std::dynamic_pointer_cast<xtrie_raw_full_node_t>(Val)->EncodeRLP(encoded, ec);

        break;
    }

    default: {
        assert(false);
        xwarn("!!!! raw short node not encode type: %d", static_cast<uint8_t>(Val->type()));
        break;
    }
    }

    append(buf, RLP::encodeList(encoded));
}

NS_END3