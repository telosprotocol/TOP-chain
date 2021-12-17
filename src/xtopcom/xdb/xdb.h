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
    //db_kinds refer to xdb_kind_t
    explicit xdb(const int db_kinds,const std::string& db_root_dir,std::vector<xdb_path_t> & db_paths);
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
    static void destroy(const std::string& m_db_name);
    
    //batch mode for multiple keys with multiple ops
    bool batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) override;
    
    //prefix must start from first char of key
    bool read_range(const std::string& prefix, std::vector<std::string>& values) override;
    //note:begin_key and end_key must has same style(first char of key)
    bool delete_range(const std::string& begin_key,const std::string& end_key) override;
    //key must be readonly(never update after PUT),otherwise the behavior is undefined
    bool single_delete(const std::string& key) override;
    
    //iterator each key of prefix.note: go throuh whole db if prefix is empty
    bool read_range(const std::string& prefix,xdb_iterator_callback callback,void * cookie) override;
    bool get_estimate_num_keys(uint64_t & num) const override;
    
    //compact whole DB if both begin_key and end_key are empty
    //note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
    virtual bool compact_range(const std::string & begin_key,const std::string & end_key) override;
    
    xdb_meta_t  get_meta() override {return xdb_meta_t();}  // XTODO no need implement

 private:
    class xdb_impl;
    std::unique_ptr<xdb_impl> m_db_impl;
};

}  // namespace ledger
}  // namespace top
