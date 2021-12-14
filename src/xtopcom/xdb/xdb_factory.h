// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xdb/xdb_face.h"

namespace top { namespace db {

class xdb_factory_t {
 public:
    xdb_factory_t() = delete;
    ~xdb_factory_t() = delete;
    static std::shared_ptr<xdb_face_t> create(int db_kinds, const std::string& db_root_dir,std::vector<xdb_path_t> db_data_paths = std::vector<xdb_path_t>());
    static std::shared_ptr<xdb_face_t> instance(const std::string& db_root_dir,std::vector<xdb_path_t> db_data_paths = std::vector<xdb_path_t>());
    static std::shared_ptr<xdb_face_t> create_kvdb(const std::string& db_root_dir) {
        return create(xdb_kind_kvdb, db_root_dir);
    }
    static std::shared_ptr<xdb_face_t> create_memdb() {
        return create(xdb_kind_mem, {});
    }
    static void destroy(xdb_kind_t kind, const std::string& path);
};


}  // namespace db
}  // namespace top
