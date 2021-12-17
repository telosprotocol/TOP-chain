// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <string>

#include "xbase/xlog.h"
#include "xdb/xdb.h"
#include "xdb/xdb_mem.h"
#include "xdb/xdb_face.h"
#include "xdb/xdb_factory.h"


namespace top { namespace db {

std::shared_ptr<xdb_face_t> xdb_factory_t::create(int db_kinds, const std::string& db_root_dir,std::vector<xdb_path_t> db_data_paths) {
    const xdb_kind_t kind = (xdb_kind_t)(db_kinds & 0x0F);
    switch (kind) {
        case xdb_kind_kvdb:
        {
            return std::make_shared<xdb>(db_kinds,db_root_dir,db_data_paths);
        }
        case xdb_kind_mem:
        {
            xwarn("xdb_factory_t::create a memory-db");
            return std::make_shared<xdb_mem_t>();
        }
        default:
        {
            assert(0);
            return nullptr;
        }
    }
}

std::shared_ptr<xdb_face_t> xdb_factory_t::instance(const std::string& db_root_dir,std::vector<xdb_path_t> db_data_paths) {
    static std::shared_ptr<xdb_face_t> db = nullptr;
    if (db == nullptr) {
        db = std::make_shared<xdb>(xdb_kind_kvdb,db_root_dir,db_data_paths);
    }
    return db;
}

void xdb_factory_t::destroy(db::xdb_kind_t kind, const std::string& path) {
    switch (kind) {
        case xdb_kind_kvdb:
        {
            xdb::destroy(path);
            return;
        }
        case xdb_kind_mem:
        {
            return;
        }
        default:
        {
            assert(0);
        }
    }
}

}  // namespace db
}  // namespace top
