// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbasic/xbyte_buffer.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/rlp/xrlp_encodable.h"
#include "xevm_common/trie/xtrie_node_fwd.h"

#include <array>
#include <atomic>
#include <map>
#include <memory>
#include <string>

NS_BEG3(top, evm_common, trie)

enum class xtop_trie_node_type : uint8_t {
    invalid = 0,
    fullnode = 1,
    shortnode = 2,
    hashnode = 3,
    valuenode = 4,

    // rawNode is a simple binary blob used to differentiate between collapsed trie
    // nodes and already encoded RLP binary blobs (while at the same time store them
    // in the same cache fields).
    rawnode = 5,

    // rawFullNode represents only the useful data content of a full node, with the
    // caches and flags stripped out to minimize its data storage. This type honors
    // the same RLP encoding as the original parent.
    rawfullnode = 6,

    // rawShortNode represents only the useful data content of a short node, with the
    // caches and flags stripped out to minimize its data storage. This type honors
    // the same RLP encoding as the original parent.
    rawshortnode = 7,
};
using xtrie_node_type_t = xtop_trie_node_type;

class xtop_trie_node_face {
private:
    // std::atomic<int> referenced_cnt_by_parent_{0};

public:
    xtop_trie_node_face() = default;
    xtop_trie_node_face(xtop_trie_node_face const &) = default;
    xtop_trie_node_face & operator=(xtop_trie_node_face const &) = default;
    xtop_trie_node_face(xtop_trie_node_face &&) = default;
    xtop_trie_node_face & operator=(xtop_trie_node_face &&) = default;
    virtual ~xtop_trie_node_face() = default;

    virtual std::string fstring(std::string const & ind) = 0;
    virtual std::pair<xtrie_hash_node_t, bool> cache() = 0;
    virtual xtrie_node_type_t type() const noexcept = 0;

    
};
using xtrie_node_face_t = xtop_trie_node_face;
using xtrie_node_face_ptr_t = std::shared_ptr<xtrie_node_face_t>;

// value&&hash node
class xtop_trie_hash_node : public xtrie_node_face_t {
private:
    xbytes_t m_data{};

public:
    xtop_trie_hash_node() = default;
    xtop_trie_hash_node(xtop_trie_hash_node const &) = default;
    xtop_trie_hash_node & operator=(xtop_trie_hash_node const &) = default;
    xtop_trie_hash_node(xtop_trie_hash_node &&) = default;
    xtop_trie_hash_node & operator=(xtop_trie_hash_node &&) = default;
    ~xtop_trie_hash_node() override = default;

    explicit xtop_trie_hash_node(xbytes_t data);
    explicit xtop_trie_hash_node(xhash256_t const & hash);

public:
    xbytes_t const & data() const noexcept;
    bool is_null() const noexcept;

public:
    std::string fstring(std::string const & ind) override;
    std::pair<xtrie_hash_node_t, bool> cache() override;
    xtrie_node_type_t type() const noexcept override;
};
using xtrie_hash_node_t = xtop_trie_hash_node;
using xtrie_hash_node_ptr_t = std::shared_ptr<xtrie_hash_node_t>;

class xtop_trie_value_node : public xtrie_node_face_t {
private:
    xbytes_t m_data;

public:
    xtop_trie_value_node() = default;
    xtop_trie_value_node(xtop_trie_value_node const &) = default;
    xtop_trie_value_node & operator=(xtop_trie_value_node const &) = default;
    xtop_trie_value_node(xtop_trie_value_node &&) = default;
    xtop_trie_value_node & operator=(xtop_trie_value_node &&) = default;
    ~xtop_trie_value_node() override = default;

    explicit xtop_trie_value_node(xbytes_t data);

public:
    xbytes_t const & data() const noexcept;

public:
    std::string fstring(std::string const & ind) override;
    std::pair<xtrie_hash_node_t, bool> cache() override;
    xtrie_node_type_t type() const noexcept override;
};
using xtrie_value_node_t = xtop_trie_value_node;
using xtrie_value_node_ptr_t = std::shared_ptr<xtrie_value_node_t>;

// nilValueNode is used when collapsing internal trie nodes for hashing, since
// unset children need to serialize correctly.
static const xtrie_value_node_t nilValueNode{};

struct xtop_node_flag {
    xtrie_hash_node_t hash{};
    bool dirty{false};

    xtop_node_flag() = default;
    xtop_node_flag(xtop_node_flag const &) = default;
    xtop_node_flag & operator=(xtop_node_flag const &) = default;
    xtop_node_flag(xtop_node_flag &&) = default;
    xtop_node_flag & operator=(xtop_node_flag &&) = default;
    ~xtop_node_flag() = default;

    explicit xtop_node_flag(xtrie_hash_node_t hash);
};
using xnode_flag_t = xtop_node_flag;

// for leaf node && extension node
class xtop_trie_short_node : public xtrie_node_face_t, public rlp::xrlp_encodable_t<xtop_trie_short_node> {
public:
    xbytes_t key{};
    xtrie_node_face_ptr_t val{};
    xnode_flag_t flags{};

public:
    xtop_trie_short_node() = default;
    xtop_trie_short_node(xtop_trie_short_node const &) = default;
    xtop_trie_short_node & operator=(xtop_trie_short_node const &) = default;
    xtop_trie_short_node(xtop_trie_short_node &&) = default;
    xtop_trie_short_node & operator=(xtop_trie_short_node &&) = default;
    ~xtop_trie_short_node() override = default;

    xtop_trie_short_node(xbytes_t key, xtrie_node_face_ptr_t val, xnode_flag_t flag);

public:
    std::shared_ptr<xtop_trie_short_node> clone() const;

public:
    std::string fstring(std::string const & ind) override;
    std::pair<xtrie_hash_node_t, bool> cache() override;
    xtrie_node_type_t type() const noexcept override;

public:
    void EncodeRLP(xbytes_t & buf, std::error_code & ec) override;
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
    xnode_flag_t flags;

public:
    xtop_trie_full_node() {
    }
    xtop_trie_full_node(xnode_flag_t const & f) {
        flags = f;
    }

private:
public:
    std::shared_ptr<xtop_trie_full_node> clone() {
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
    xtrie_node_type_t type() const noexcept override {
        return xtrie_node_type_t::fullnode;
    }

public:
    void EncodeRLP(xbytes_t & buf, std::error_code & ec) override;
};
using xtrie_full_node_t = xtop_trie_full_node;
using xtrie_full_node_ptr_t = std::shared_ptr<xtrie_full_node_t>;

/// ? only used by database? xtrie_db.cpp
class xtop_trie_raw_node : public xtrie_node_face_t {
private:
    xbytes_t m_data;

public:
    xtop_trie_raw_node() {
    }
    xtop_trie_raw_node(xbytes_t const & data) : m_data{data} {
    }

public:
    xbytes_t data() const {
        return m_data;
    }

public:
    std::string fstring(std::string const & ind) override {
        xassert(false);
        // should not used
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        xassert(false);
        // should not used
        return {{}, true};
    }
    xtrie_node_type_t type() const noexcept override {
        return xtrie_node_type_t::rawnode;
    }
};
using xtrie_raw_node_t = xtop_trie_raw_node;
using xtrie_raw_node_ptr_t = std::shared_ptr<xtrie_raw_node_t>;

class xtop_trie_raw_full_node
  : public xtrie_node_face_t
  , public rlp::xrlp_encodable_t<xtop_trie_raw_full_node> {
public:
    std::array<xtrie_node_face_ptr_t, 17> Children;

public:
    xtop_trie_raw_full_node(std::array<xtrie_node_face_ptr_t, 17> const & child) {
        Children = child;
    }

public:
    std::string fstring(std::string const & ind) override {
        xassert(false);
        // should not used
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        xassert(false);
        // should not used
        return {{}, true};
    }
    xtrie_node_type_t type() const noexcept override {
        return xtrie_node_type_t::rawfullnode;
    }

public:
    void EncodeRLP(xbytes_t & buf, std::error_code & ec);
};
using xtrie_raw_full_node_t = xtop_trie_raw_full_node;
using xtrie_raw_full_node_ptr_t = std::shared_ptr<xtrie_raw_full_node_t>;

class xtop_trie_raw_short_node
  : public xtrie_node_face_t
  , public rlp::xrlp_encodable_t<xtop_trie_raw_full_node> {
public:
    xbytes_t Key;
    xtrie_node_face_ptr_t Val;

public:
    xtop_trie_raw_short_node(xbytes_t const & key, xtrie_node_face_ptr_t val) : Key{key}, Val{val} {
    }

public:
    std::string fstring(std::string const & ind) override {
        xassert(false);
        // should not used
        return "";
    }
    std::pair<xtrie_hash_node_t, bool> cache() override {
        xassert(false);
        // should not used
        return {{}, true};
    }
    xtrie_node_type_t type() const noexcept override {
        return xtrie_node_type_t::rawshortnode;
    }

public:
    void EncodeRLP(xbytes_t & buf, std::error_code & ec);
};
using xtrie_raw_short_node_t = xtop_trie_raw_short_node;
using xtrie_raw_short_node_ptr_t = std::shared_ptr<xtrie_raw_short_node_t>;

NS_END3