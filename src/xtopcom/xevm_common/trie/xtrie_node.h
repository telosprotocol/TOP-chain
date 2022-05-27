// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/rlp/xrlp_encodable.h"

#include <array>
#include <map>
#include <memory>
#include <string>

NS_BEG3(top, evm_common, trie)

// fwd:
class xtop_trie_hash_node;
using xtrie_hash_node_t = xtop_trie_hash_node;

// using hashNode = xbytes_t;
// using valueNode = xbytes_t;
// static valueNode nullValueNode{};

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

// for branch node
// actual trie node data to encode/decode, so it need to impl encodeELP
class xtop_trie_full_node
  : public xtrie_node_face_t
  , public rlp::xrlp_encodable_t<xtop_trie_full_node> {
public:
    std::array<xtrie_node_face_ptr_t, 17> Childern;
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
    }
};
using xtrie_full_node_t = xtop_trie_full_node;
using xtrie_full_node_ptr_t = std::shared_ptr<xtrie_full_node_t>;

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
    }
};
using xtrie_short_node_t = xtop_trie_short_node;
using xtrie_short_node_ptr_t = std::shared_ptr<xtrie_short_node_t>;

NS_END3