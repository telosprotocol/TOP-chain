// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <stdexcept>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <vector>

#include "xdb/xdb_face.h"

namespace top { namespace db {

class xdb_memdb_transaction_t;

class xdb_mem_t : public xdb_face_t {
 public:
    bool open() override { return true; }
    bool close() override { return true; }
    bool read(const std::string& key, std::string& value) const override;
    bool exists(const std::string& key) const override;

    bool write(const std::string& key, const std::string& value) override;
    bool write(const std::string& key, const char* data, size_t size) override;
    bool write(const std::map<std::string, std::string>& batches) override;

    bool erase(const std::string& key) override;
    bool erase(const std::vector<std::string>& keys) override;
    bool batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) override;

    //prefix must start from first char of key
    bool read_range(const std::string& prefix, std::vector<std::string>& values) override;
    //note:begin_key and end_key must has same style(first char of key)
    bool delete_range(const std::string& begin_key,const std::string& end_key) override;
    //key must be readonly(never update after PUT),otherwise the behavior is undefined
    bool single_delete(const std::string& key) override;
    
    //iterator each key of prefix.note: go throuh whole db if prefix is empty
    bool read_range(const std::string& prefix,xdb_iterator_callback callback,void * cookie) override;
    //compact whole DB if both begin_key and end_key are empty
    //note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
    bool compact_range(const std::string & begin_key,const std::string & end_key) override;
    bool get_estimate_num_keys(uint64_t & num) const override;
    
    xdb_meta_t  get_meta() override {return m_meta;}  // implement for test
 public:
    std::map<std::string, std::string> m_values;
    xdb_meta_t m_meta;
    mutable std::mutex m_lock;
};

class xdb_memdb_transaction_t : public xdb_transaction_t {
 public:
    xdb_memdb_transaction_t(xdb_mem_t* db) : m_db(db) { }
    ~xdb_memdb_transaction_t() { }

    bool rollback() override;
    bool commit() override;

    bool read(const std::string& key, std::string& value) const override;

    bool write(const std::string& key, const std::string& value) override;
    bool write(const std::string& key, const char* data, size_t size) override;

    bool erase(const std::string& key) override;

 private:
    xdb_mem_t* m_db;
    mutable std::map<std::string, std::string> m_read_values;
    std::map<std::string, std::string> m_write_values;
    std::set<std::string> m_erase_keys;
};

}  // namespace ledger
}  // namespace top
