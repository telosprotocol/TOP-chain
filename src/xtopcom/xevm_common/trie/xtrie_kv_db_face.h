// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/trie/xtrie_kv_db_schema.h"
#include "xevm_common/trie/xtrie_node.h"

#include <memory>
#include <system_error>

NS_BEG3(top, evm_common, trie)

class xtop_kv_writer_face {
public:
    virtual void Put(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) = 0;
    virtual void Delete(xbytes_t const & key, std::error_code & ec) = 0;
};
using xkv_writer_face_t = xtop_kv_writer_face;

class xtop_kv_reader_face {
public:
    virtual bool Has(xbytes_t const & key, std::error_code & ec) = 0;
    virtual xbytes_t Get(xbytes_t const & key, std::error_code & ec) = 0;
};
using xkv_reader_face_t = xtop_kv_reader_face;

class xtop_kv_db_face
  : public xkv_reader_face_t
  , public xkv_writer_face_t {};
using xkv_db_face_t = xtop_kv_db_face;
using xkv_db_face_ptr_t = std::shared_ptr<xkv_db_face_t>;

/// unit operation

inline xbytes_t ReadUnitWithPrefix(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    return db->Get(schema::unitKey(hash), _);
}

inline bool HasUnitWithPrefix(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    return db->Has(schema::unitKey(hash), _);
}

inline void WriteUnit(xkv_db_face_ptr_t db, xhash256_t const & hash, xbytes_t const & code) {
    std::error_code ec;
    db->Put(schema::unitKey(hash), code, ec);
    if (ec) {
        xwarn("Failed to store unit: %s", ec.message().c_str());
    }
}

inline void DeleteUnit(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code ec;
    db->Delete(schema::unitKey(hash), ec);
    if (ec) {
        xwarn("Failed to delete unit: %s", ec.message().c_str());
    }
}

/// Trie Node operations

inline xbytes_t ReadTrieNode(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    return db->Get(hash.to_bytes(), _);
}

inline bool HasTrieNode(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    return db->Has(hash.to_bytes(), _);
}

inline void WriteTrieNode(xkv_db_face_ptr_t db, xhash256_t const & hash, xbytes_t const & node) {
    std::error_code ec;
    db->Put(hash.to_bytes(), node, ec);
    if (ec) {
        xwarn("Failed to store trie node: %s", ec.message().c_str());
    }
}

// #[maybe unused]
inline void DeleteTrieNode(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code ec;
    db->Delete(hash.to_bytes(), ec);
    if (ec) {
        xwarn("Failed to delete trie node: %s", ec.message().c_str());
    }
}

inline bool ReadTrieSyncFlag(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    auto k = std::string{schema::TrieSyncKey} + hash.as_hex_str();
    return db->Has({k.begin(), k.end()}, _);
}

inline void WriteTrieSyncFlag(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code ec;
    auto k = std::string{schema::TrieSyncKey} + hash.as_hex_str();
    auto v = std::string{"1"};
    db->Put({k.begin(), k.end()}, {v.begin(), v.end()}, ec);
    if (ec) {
        xwarn("WriteTrieSyncFlag error: %s, %s", ec.category().name(), ec.message().c_str());
    }
}

inline void DeleteTrieSyncFlag(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code ec;
    auto k = std::string{schema::TrieSyncKey} + hash.as_hex_str();
    db->Delete({k.begin(), k.end()}, ec);
    if (ec) {
        xwarn("DeleteTrieSyncFlag error: %s, %s", ec.category().name(), ec.message().c_str());
    }
}

NS_END3