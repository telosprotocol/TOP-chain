// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <vector>

#include "xdb/xdb_face.h"
#include "xvledger/xvdbstore.h"
#include "xbase/xobject.h"
#include "xdata/xblock.h"
#include "xdata/xunit_bstate.h"
#include "xbasic/xmemory.hpp"

using namespace top::data;

namespace top { namespace store {

using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;

class xstore_face_t : public base::xvdbstore_t {
 public:
    xstore_face_t() = default;
 protected:
    virtual ~xstore_face_t() {}
 public:
    virtual xaccount_ptr_t query_account(const std::string& address) const = 0;
    virtual bool           string_property_get(base::xvblock_t* block, const std::string& prop, std::string& value) const = 0;

    virtual uint64_t get_blockchain_height(const std::string& account) = 0;

    virtual int32_t get_map_property(const std::string& account, uint64_t height, const std::string& name, std::map<std::string, std::string>& value) = 0;
    virtual int32_t get_string_property(const std::string& account, uint64_t height, const std::string& name, std::string& value) = 0;

    // property operation api
    virtual int32_t string_get(const std::string& account, const std::string& key, std::string& value) const = 0;

    virtual int32_t list_get_all(const std::string& account, const std::string &key, std::vector<std::string> &values) = 0;
    virtual int32_t map_get(const std::string& account, const std::string & key, const std::string & field, std::string & value) = 0;
    virtual int32_t map_copy_get(const std::string& account, const std::string & key, std::map<std::string, std::string> & map) const = 0;

public:
    virtual bool delete_block_by_path(const std::string & store_path,const std::string & account, uint64_t height, bool has_input_output) = 0;
public:
    virtual bool  execute_block(base::xvblock_t* block) = 0;
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
