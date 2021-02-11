// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <stdexcept>
#include <vector>
#include <memory>
#include <map>

namespace top { namespace db {

class xdb_transaction_t {
public:
    virtual bool rollback() { return true; }
    virtual bool commit() { return true; }
    virtual ~xdb_transaction_t() { }
    virtual bool read(const std::string& key, std::string& value) const = 0;
    virtual bool write(const std::string& key, const std::string& value) = 0;
    virtual bool write(const std::string& key, const char* data, size_t size) = 0;
    virtual bool erase(const std::string& key) = 0;
};

class xdb_face_t {
 public:
    virtual bool open() = 0;
    virtual bool close() = 0;
    virtual bool read(const std::string& key, std::string& value) const = 0;
    virtual bool exists(const std::string& key) const = 0;
    virtual bool write(const std::string& key, const std::string& value) = 0;
    virtual bool write(const std::string& key, const char* data, size_t size) = 0;
    virtual bool write(const std::map<std::string, std::string>& batches) = 0;
    virtual bool erase(const std::string& key) = 0;
    virtual bool erase(const std::vector<std::string>& keys) = 0;
    virtual bool batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) = 0;
    virtual xdb_transaction_t* begin_transaction() = 0;
};

}  // namespace ledger
}  // namespace top
