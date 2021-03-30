// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <iostream>
#include <vector>

#include "leveldb/options.h"
#include "leveldb/slice.h"
#include "leveldb/status.h"
#include "leveldb/write_batch.h"
#include "leveldb/cache.h"
#include "leveldb/filter_policy.h"
#include "leveldb/db.h"

#include "xbase/xlog.h"
#include "xdb/xdb.h"

using std::string;

namespace top { namespace db {

class xdb::xdb_impl final{
 public:
    explicit xdb_impl(const std::string& name);
    ~xdb_impl();
    void open();
    void close();
    bool read(const std::string& key, std::string& value) const;
    bool exists(const std::string& key) const;
    void write(const std::string& key, const std::string& value) const;
    void write(const std::string& key, const char* data, size_t size) const;
    void erase(const std::string& key) const;
    bool read_range(const std::string& prefix, std::vector<std::string>& values) const;
    static void destroy(const std::string& m_db_name);

 private:
    void handle_error(const leveldb::Status& status) const;
    std::string m_db_name{};
    leveldb::DB* m_db{nullptr};
    leveldb::Options m_options{};
    leveldb::WriteBatch m_batch{};
};

void xdb::xdb_impl::open() {
    if (m_db == nullptr) {
        leveldb::Status s = leveldb::DB::Open(m_options, m_db_name, &m_db);
        handle_error(s);
    }
}
void xdb::xdb_impl::close() {
    if (m_db) {
        delete m_db;
        m_db = nullptr;
    }
}

xdb::xdb_impl::xdb_impl(const std::string& name) {
    m_db_name = name;
    m_options.create_if_missing = true;
    m_options.filter_policy = leveldb::NewBloomFilterPolicy(10);
    open();
}

xdb::xdb_impl::~xdb_impl() {
    close();
}

void xdb::xdb_impl::handle_error(const leveldb::Status& status) const {
    if (status.ok())
        return;
    const string errmsg = "[xdb] levelDB error: " + status.ToString() + " ,db name " + m_db_name;
    xerror("%s", errmsg.c_str());
    throw xdb_error(errmsg);
}

bool xdb::xdb_impl::read(const std::string& key, std::string& value) const {
    leveldb::Status s = m_db->Get(leveldb::ReadOptions(), leveldb::Slice(key), &value);
    if (!s.ok()) {
        if (s.IsNotFound()) {
            return false;
        }
        handle_error(s);
    }

    return true;
}

bool xdb::xdb_impl::exists(const std::string& key) const {
    std::string value;
    return read(key, value);
}

void xdb::xdb_impl::write(const std::string& key, const std::string& value) const {
    leveldb::Status s = m_db->Put(leveldb::WriteOptions(), leveldb::Slice(key), leveldb::Slice(value));
    handle_error(s);
}

void xdb::xdb_impl::write(const std::string& key, const char* data, size_t size) const {
    leveldb::Status s = m_db->Put(leveldb::WriteOptions(), leveldb::Slice(key), leveldb::Slice(data, size));
    handle_error(s);
}

void xdb::xdb_impl::erase(const std::string& key) const {
    leveldb::Status s = m_db->Delete(leveldb::WriteOptions(), leveldb::Slice(key));
    if (!s.ok()) {
        if (!s.IsNotFound()) {
            handle_error(s);
        }
    }
}

void xdb::xdb_impl::destroy(const std::string& m_db_name) {
    leveldb::DestroyDB(m_db_name, leveldb::Options());
}

bool xdb::xdb_impl::read_range(const std::string& prefix, std::vector<std::string>& values) const {
    bool ret = false;
    auto iter = m_db->NewIterator(leveldb::ReadOptions());

    for (iter->Seek(prefix); iter->Valid() && iter->key().starts_with(prefix); iter->Next()) {
        values.push_back(iter->value().ToString());
        ret = true;
    }
    return ret;
}

xdb::xdb(const std::string& name)
: m_db_impl(new xdb_impl(name)) {
}

xdb::~xdb() noexcept = default;

void xdb::open() {
    return m_db_impl->open();
}
void xdb::close() {
    return m_db_impl->close();
}
bool xdb::read(const std::string& key, std::string& value) const {
    return m_db_impl->read(key, value);
}

bool xdb::exists(const std::string& key) const {
    return m_db_impl->exists(key);
}

void xdb::write(const std::string& key, const std::string& value) {
    return m_db_impl->write(key, value);
}

void xdb::write(const std::string& key, const char* data, size_t size) {
    return m_db_impl->write(key, data, size);
}

void xdb::erase(const std::string& key) {
    return m_db_impl->erase(key);
}

void xdb::destroy(const std::string& m_db_name) {
    return xdb::xdb_impl::destroy(m_db_name);
}

bool xdb::read_range(const std::string& prefix, std::vector<std::string>& values) const {
    return m_db_impl->read_range(prefix, values);
}

}  // namespace ledger
}  // namespace top
