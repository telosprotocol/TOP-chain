// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xhash.hpp"
#include "xevm_common/trie/xtrie_node.h"

#include <memory>
#include <system_error>

NS_BEG3(top, evm_common, trie)

class xtop_trie_db_face {
public:
    virtual xtrie_node_face_ptr_t node(xhash256_t hash) = 0;

    virtual void insert(xhash256_t hash, int32_t size, xtrie_node_face_ptr_t node) = 0;
};
using xtrie_db_face_t = xtop_trie_db_face;
using xtrie_db_face_ptr_t = std::shared_ptr<xtrie_db_face_t>;

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

NS_END3