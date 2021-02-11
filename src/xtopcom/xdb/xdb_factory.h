// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xdb/xdb_face.h"

namespace top { namespace db {

enum xdb_kind_t {
    xdb_kind_kvdb,
    xdb_kind_mem
};

class xdb_factory_t {
 public:
    xdb_factory_t() = delete;
    ~xdb_factory_t() = delete;
    static std::shared_ptr<xdb_face_t> create(xdb_kind_t kind, const std::string& path);
    static std::shared_ptr<xdb_face_t> instance(const std::string& path);
    static std::shared_ptr<xdb_face_t> create_kvdb(const std::string& path) {
        return create(xdb_kind_kvdb, path);
    }
    static std::shared_ptr<xdb_face_t> create_memdb() {
        return create(xdb_kind_mem, {});
    }
    static void destroy(xdb_kind_t kind, const std::string& path);
};


}  // namespace db
}  // namespace top
