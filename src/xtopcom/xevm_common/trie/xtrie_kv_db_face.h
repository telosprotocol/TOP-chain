// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/trie/xtrie_node.h"

#include <gsl/span>

#include <map>
#include <memory>
#include <system_error>

NS_BEG3(top, evm_common, trie)

class xtop_kv_writer_face {
public:
    virtual void Put(gsl::span<xbyte_t const> key, xbytes_t const & value, std::error_code & ec) = 0;
    virtual void PutBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) = 0;

    virtual void PutDirect(xbytes_t const & key, xbytes_t const & value, std::error_code & ec) = 0;
    virtual void PutDirectBatch(std::map<xbytes_t, xbytes_t> const & batch, std::error_code & ec) = 0;

    virtual void Delete(xbytes_t const & key, std::error_code & ec) = 0;
    virtual void DeleteBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) = 0;

    virtual void DeleteDirect(xbytes_t const & key, std::error_code & ec) = 0;
    virtual void DeleteDirectBatch(std::vector<xbytes_t> const & batch, std::error_code & ec) = 0;
};
using xkv_writer_face_t = xtop_kv_writer_face;

class xtop_kv_reader_face {
public:
    virtual bool Has(xbytes_t const & key, std::error_code & ec) = 0;
    virtual bool HasDirect(xbytes_t const & key, std::error_code & ec) = 0;
    virtual xbytes_t Get(xbytes_t const & key, std::error_code & ec) = 0;
    virtual xbytes_t GetDirect(xbytes_t const & key, std::error_code & ec) = 0;
};
using xkv_reader_face_t = xtop_kv_reader_face;

class xtop_kv_db_face
  : public xkv_reader_face_t
  , public xkv_writer_face_t {};
using xkv_db_face_t = xtop_kv_db_face;
using xkv_db_face_ptr_t = std::shared_ptr<xkv_db_face_t>;

/// unit operation

inline xbytes_t ReadUnitWithPrefix(xkv_db_face_ptr_t db, xbytes_t const & unit_key) {
    std::error_code _;
    return db->GetDirect(unit_key, _);
}

inline bool HasUnitWithPrefix(xkv_db_face_ptr_t db, xbytes_t const & unit_key) {
    std::error_code _;
    return db->HasDirect(unit_key, _);
}

inline void WriteUnit(xkv_db_face_ptr_t db, xbytes_t const & unit_key, xbytes_t const & code) {
    std::error_code ec;
    db->PutDirect(unit_key, code, ec);
    if (ec) {
        xwarn("Failed to store unit: %s", ec.message().c_str());
    }
}

inline void WriteUnitBatch(xkv_db_face_ptr_t db, std::map<xbytes_t, xbytes_t> const & batch) {
    std::error_code ec;
    db->PutDirectBatch(batch, ec);
    if (ec) {
        xwarn("Failed to store trie node: %s", ec.message().c_str());
    }
}

inline void DeleteUnit(xkv_db_face_ptr_t db, xbytes_t const & unit_key) {
    std::error_code ec;
    db->DeleteDirect(unit_key, ec);
    if (ec) {
        xwarn("Failed to delete unit: %s", ec.message().c_str());
    }
}

inline void DeleteUnitBatch(xkv_db_face_ptr_t db, std::vector<xbytes_t> const & batch) {
    std::error_code ec;
    db->DeleteDirectBatch(batch, ec);
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

inline void WriteTrieNodeBatch(xkv_db_face_ptr_t db, std::map<xbytes_t, xbytes_t> const & batch) {
    std::error_code ec;
    db->PutBatch(batch, ec);
    if (ec) {
        xwarn("Failed to store trie node: %s", ec.message().c_str());
    }
}

inline void DeleteTrieNode(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code ec;
    db->Delete(hash.to_bytes(), ec);
    if (ec) {
        xwarn("Failed to delete trie node: %s", ec.message().c_str());
    }
}

inline void DeleteTrieBatch(xkv_db_face_ptr_t db, std::vector<xbytes_t> const & batch) {
    std::error_code ec;
    db->DeleteBatch(batch, ec);
    if (ec) {
        xwarn("Failed to delete trie node: %s", ec.message().c_str());
    }
}

NS_END3