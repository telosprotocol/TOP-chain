// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>

#include "xbase/xobject.h"
#include "xbasic/xmemory.hpp"
#include "xdb/xdb_face.h"
#include "xvledger/xvdbstore.h"

namespace top { namespace store {

using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;

class xstore_face_t : public base::xvdbstore_t {
 public:
    xstore_face_t() = default;
 protected:
    virtual ~xstore_face_t() {}
 public:
    virtual bool open() const = 0;
    virtual bool close() const = 0;
};

using xstore_face_ptr_t = xobject_ptr_t<xstore_face_t>;

class xstore_factory {
 public:
    static xobject_ptr_t<xstore_face_t> create_store_with_memdb();
    static xobject_ptr_t<xstore_face_t> create_store_with_kvdb(const std::string & db_path);
    static xobject_ptr_t<xstore_face_t> create_store_with_static_kvdb(const std::string & db_path);
    static xobject_ptr_t<xstore_face_t> create_store_with_static_kvdb(std::shared_ptr<db::xdb_face_t>& db);
};

}  // namespace store
}  // namespace top
