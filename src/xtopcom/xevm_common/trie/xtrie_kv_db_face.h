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

/// Code operations

inline xbytes_t ReadCodeWithPrefix(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    return db->Get(schema::codeKey(hash), _);
}
// inline xbytes_t ReadCode(xkv_db_face_ptr_t db, xhash256_t hash) {
//     std::error_code _;
//     auto data = ReadCodeWithPrefix(db, hash);
//     if (data.empty()) {
//         // #[legacy?]
//         data = db->Get(hash.to_bytes(), _);
//     }
//     return data;
// }

inline bool HasCodeWithPrefix(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code _;
    return db->Has(schema::codeKey(hash), _);
}

inline void WriteCode(xkv_db_face_ptr_t db, xhash256_t const & hash, xbytes_t const & code) {
    std::error_code ec;
    db->Put(schema::codeKey(hash), code, ec);
    if (ec) {
        xwarn("Failed to store code: %s", ec.message().c_str());
    }
}

inline void DeleteCode(xkv_db_face_ptr_t db, xhash256_t const & hash) {
    std::error_code ec;
    db->Delete(schema::codeKey(hash), ec);
    if (ec) {
        xwarn("Failed to delete code: %s", ec.message().c_str());
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

NS_END3