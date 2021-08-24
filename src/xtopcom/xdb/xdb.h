// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <stdexcept>
#include <memory>
#include <vector>

#include "xdb/xdb_face.h"

namespace top { namespace db {

class xdb_error : public std::runtime_error {
 public:
    explicit xdb_error(const std::string& msg) : std::runtime_error(msg) {}
};

class xdb : public xdb_face_t {
 public:
    explicit xdb(const std::string& name);
    ~xdb() noexcept;
    bool open() override;
    bool close() override;
    bool read(const std::string& key, std::string& value) const override;
    bool exists(const std::string& key) const override;
    bool write(const std::string& key, const std::string& value) override;
    bool write(const std::string& key, const char* data, size_t size) override;
    bool write(const std::map<std::string, std::string>& batches) override;
    bool erase(const std::string& key) override;
    bool erase(const std::vector<std::string>& keys) override;
    bool batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) override;
    static void destroy(const std::string& m_db_name);
    bool read_range(const std::string& prefix, std::vector<std::string>& values) const;
    xdb_transaction_t* begin_transaction() override;
    xdb_meta_t  get_meta() override {return xdb_meta_t();}  // XTODO no need implement

 private:
    class xdb_impl;
    std::unique_ptr<xdb_impl> m_db_impl;
};

}  // namespace ledger
}  // namespace top
