// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/rlp.h"  // todo move it to .cpp
#include "xevm_common/rlp/xrlp_encodable.h"

#include <array>
#include <map>
#include <memory>
#include <string>

NS_BEG3(top, evm_common, trie)

// fwd:
class xtop_trie_hash_node;
using xtrie_hash_node_t = xtop_trie_hash_node;

enum class xtop_trie_node_type : uint8_t {
    invalid = 0,
    fullnode = 1,
    shortnode = 2,
    hashnode = 3,
    valuenode = 4,
};
using xtrie_node_type_t = xtop_trie_node_type;

class xtop_trie_node_face {
public:
    virtual std::string fstring(std::string const & ind) = 0;
    virtual std::pair<xtrie_hash_node_t, bool> cache() = 0;
    virtual xtrie_node_type_t type() = 0;
};
using xtrie_node_face_t = xtop_trie_node_face;
using xtrie_node_face_ptr_t = std::shared_ptr<xtrie_node_face_t>;

// value&&hash node
class xtop_trie_hash_node : public xtrie_node_face_t {
private:
    xbytes_t m_data;

public:
    xtop_trie_hash_node() {
    }
    xtop_trie_hash_node(xbytes_t const & data) : m_data{data} {
    }
    xtop_trie_hash_node(xhash256_t const & hash) : m_data{hash.begin(), hash.end()} {
    }

public:
    xbytes_t data() const {
        return m_data;
    }

    bool is_null() const {
        return m_data.empty();
    }

public:
    std::string fstring(std::string const & ind) override {
        // todo;
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        return {{}, true};
    }
    xtrie_node_type_t type() override {
        return xtrie_node_type_t::hashnode;
    }
};
using xtrie_hash_node_t = xtop_trie_hash_node;
using xtrie_hash_node_ptr_t = std::shared_ptr<xtrie_hash_node_t>;

class xtop_trie_value_node : public xtrie_node_face_t {
private:
    xbytes_t m_data;

public:
    xbytes_t data() const {
        return m_data;
    }

public:
    std::string fstring(std::string const & ind) override {
        // todo;
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        return {{}, true};
    }
    xtrie_node_type_t type() override {
        return xtrie_node_type_t::valuenode;
    }
};
using xtrie_value_node_t = xtop_trie_value_node;
using xtrie_value_node_ptr_t = std::shared_ptr<xtrie_value_node_t>;

static const xtrie_value_node_t nilValueNode{};

struct nodeFlag {
    xtrie_hash_node_t hash;
    bool dirty;
};

// for leaf node && extension node
class xtop_trie_short_node
  : public xtrie_node_face_t
  , public rlp::xrlp_encodable_t<xtop_trie_short_node> {
public:
    xbytes_t Key;
    xtrie_node_face_ptr_t Val;
    nodeFlag flags;

public:
    xtop_trie_short_node(xbytes_t const & key, xtrie_node_face_ptr_t val, nodeFlag const & flag) : Key{key}, Val{val} {
        flags = flag;
    }

private:
public:
    std::shared_ptr<xtop_trie_short_node> copy() {
        return std::make_shared<xtop_trie_short_node>(*this);
    }

public:
    std::string fstring(std::string const & ind) override {
        // todo;
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        return {flags.hash, flags.dirty};
    }
    xtrie_node_type_t type() override {
        return xtrie_node_type_t::shortnode;
    }

public:
    void EncodeRLP(xbytes_t & buf, std::error_code & ec) override {
        // todo . how to recursive encode a trie.
        xbytes_t encoded;
        append(encoded, RLP::encode(Key));
        if (Val->type() == xtrie_node_type_t::hashnode) {
            auto child = std::make_shared<xtrie_hash_node_t>(*(static_cast<xtrie_hash_node_t *>(Val.get())));
            append(encoded, RLP::encode(child->data()));
        } else {
            printf("Value type: %d\n", static_cast<uint8_t>(Val->type()));
        }
        append(buf, RLP::encodeList(encoded));
    }
};
using xtrie_short_node_t = xtop_trie_short_node;
using xtrie_short_node_ptr_t = std::shared_ptr<xtrie_short_node_t>;

// for branch node
// actual trie node data to encode/decode, so it need to impl encodeELP
class xtop_trie_full_node
  : public xtrie_node_face_t
  , public rlp::xrlp_encodable_t<xtop_trie_full_node> {
public:
    std::array<xtrie_node_face_ptr_t, 17> Children;
    nodeFlag flags;

public:
    xtop_trie_full_node() {
    }
    xtop_trie_full_node(nodeFlag const & f) {
        flags = f;
    }

private:
public:
    std::shared_ptr<xtop_trie_full_node> copy() {
        return std::make_shared<xtop_trie_full_node>(*this);
    }

public:
    std::string fstring(std::string const & ind) override {
        // todo;
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        return {flags.hash, flags.dirty};
    }
    xtrie_node_type_t type() override {
        return xtrie_node_type_t::fullnode;
    }

public:
    void EncodeRLP(xbytes_t & buf, std::error_code & ec) override {
        // todo . how to recursive encode a trie.
        xbytes_t encoded;
        for (auto const & child : Children) {
            if (child == nullptr){
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
                printf("child type: %d\n", static_cast<uint8_t>(child->type()));
            }
        }
        append(buf, RLP::encodeList(encoded));
    }
};
using xtrie_full_node_t = xtop_trie_full_node;
using xtrie_full_node_ptr_t = std::shared_ptr<xtrie_full_node_t>;

NS_END3