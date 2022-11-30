// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <assert.h>
#include <inttypes.h>

#include "xbase/xcontext.h"
#include "xbase/xlog.h"
#include "xbase/xutl.h"
#include "xbasic/xmodule_type.h"
#include "xbase/xobject_ptr.h"
#include "xdb/xdb_factory.h"

#include "xmetrics/xmetrics.h"
#include "xdbstore/xstore.h"
#include "xdbstore/xstore_face.h"

using namespace top::base;

namespace top {
namespace store {

xstore::xstore(const std::shared_ptr<db::xdb_face_t> &db)
    : m_db(db) {}

bool xstore::open() const {
    if (m_db == nullptr) {
        return false;
    }
    return m_db->open();
}
bool xstore::close() const {
    if (m_db == nullptr) {
        return false;
    }
    return m_db->close();
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_memdb() {
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_memdb();
    auto                            store = top::make_object_ptr<xstore>(db);
    return store;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_kvdb(const std::string &db_path) {
    xinfo("store init with one db, db path %s", db_path.c_str());
    std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_kvdb(db_path);
    auto                            store = top::make_object_ptr<xstore>(db);
    return store;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_static_kvdb(const std::string &db_path) {
    xinfo("static store init with one db, db path %s", db_path.c_str());
    static std::shared_ptr<db::xdb_face_t> db = db::xdb_factory_t::create_kvdb(db_path);
    auto                                   store = top::make_object_ptr<xstore>(db);
    return store;
}

xobject_ptr_t<xstore_face_t> xstore_factory::create_store_with_static_kvdb(std::shared_ptr<db::xdb_face_t>& db) {
    auto                                   store = top::make_object_ptr<xstore>(db);
    return store;
}

bool xstore::set_vblock(const std::string & store_path, base::xvblock_t* vblock) {
    xassert(false);
    return false;
}

bool xstore::set_vblock_header(const std::string & store_path, base::xvblock_t* vblock) {
    xassert(false);
    return false;
}

base::xvblock_t *xstore::get_vblock(const std::string & store_path, const std::string &account, uint64_t height) const {
    xassert(false);
    return nullptr;
}

base::xvblock_t * xstore::get_vblock_header(const std::string & store_path, const std::string &account, uint64_t height) const {
    return nullptr;
}

bool xstore::get_vblock_input(const std::string & store_path, base::xvblock_t* block) const {
    xassert(false);
    return false;
}

bool xstore::get_vblock_output(const std::string &account, base::xvblock_t* block) const {
    xassert(false);
    return false;
}

bool xstore::set_value(const std::string &key, const std::string &value) {
    return m_db->write(key, value);
}

bool xstore::set_values(const std::map<std::string, std::string> & objs) {
    std::vector<std::string> empty_delete_keys;
    return m_db->batch_change(objs, empty_delete_keys);
}

bool xstore::delete_value(const std::string &key) {
    return m_db->erase(key);
}

std::string xstore::get_value(const std::string &key) const {
    std::string value;

    bool success = m_db->read(key, value);
    if (!success) {
        return std::string();
    }
    return value;
}

bool  xstore::delete_values(const std::vector<std::string> & to_deleted_keys)
{
    std::map<std::string, std::string> empty_put;
    return m_db->batch_change(empty_put, to_deleted_keys);
}

bool xstore::delete_values(std::vector<gsl::span<char const>> const & to_deleted_keys) {
    std::map<std::string, std::string> empty_put;
    return m_db->batch_change(empty_put, to_deleted_keys);
}

//prefix must start from first char of key
bool   xstore::read_range(const std::string& prefix, std::vector<std::string>& values)
{
    return m_db->read_range(prefix,values);
}
 
//note:begin_key and end_key must has same style(first char of key)
bool   xstore::delete_range(const std::string & begin_key,const std::string & end_key)
{
    return m_db->delete_range(begin_key,end_key);
}

//compact whole DB if both begin_key and end_key are empty
//note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
bool  xstore::compact_range(const std::string & begin_key,const std::string & end_key)
{
    return m_db->compact_range(begin_key,end_key);
}

//key must be readonly(never update after PUT),otherwise the behavior is undefined
bool   xstore::single_delete(const std::string & target_key)//key must be readonly(never update after PUT),otherwise the behavior is undefined
{
    return m_db->single_delete(target_key);
}

void xstore::GetDBMemStatus() const
{
    return m_db->GetDBMemStatus();
}

bool   xstore::read_range_callback(const std::string& prefix,db::xdb_iterator_callback callback,void * cookie)
{
    return m_db->read_range(prefix, callback, cookie);
}

} // namespace store
} // namespace top
