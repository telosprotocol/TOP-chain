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

enum xdb_kind_t {
    xdb_kind_kvdb           = 0x001,
    xdb_kind_mem            = 0x002,
    
    xdb_kind_readonly       = 0x010, //open for read-only
    xdb_kind_no_multi_cf    = 0x020, //open for multi cf
    //compression/decompression control
    xdb_kind_high_compress  = 0x100, //high compression for data while persisting
    xdb_kind_bottom_compress= 0x200, //only compress the bottom level
    xdb_kind_no_compress    = 0x400, //disable copmression completely,useful for consensus node
};

// XTODO for test
struct xdb_meta_t {
    size_t      m_db_key_size{0};
    size_t      m_db_value_size{0};
    size_t      m_key_count{0};
    size_t      m_write_count{0};
    mutable size_t      m_read_count{0};
    size_t      m_erase_count{0};
};

struct xdb_path_t {
    std::string path;
    uint64_t    target_size;  // Target size of total files under the path, in byte.
    
    xdb_path_t() : target_size(0) {}
    xdb_path_t(const std::string& p, uint64_t t) : path(p), target_size(t) {}
};

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

typedef bool (*xdb_iterator_callback)(const std::string& key, const std::string& value,void*cookie);

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
    virtual xdb_meta_t  get_meta() = 0;
    
    //batch mode for multiple keys with multiple ops
    virtual bool batch_change(const std::map<std::string, std::string>& objs, const std::vector<std::string>& delete_keys) = 0;
    
    //prefix must start from first char of key
    virtual bool read_range(const std::string& prefix, std::vector<std::string>& values) = 0;
    //note:begin_key and end_key must has same style(first char of key)
    virtual bool delete_range(const std::string& begin_key,const std::string& end_key) = 0;
    //key must be readonly(never update after PUT),otherwise the behavior is undefined
    virtual bool single_delete(const std::string& key) = 0;
    //iterator each key of prefix.note: go throuh whole db if prefix is empty
    virtual bool read_range(const std::string& prefix,xdb_iterator_callback callback_fuc,void * cookie) = 0;
    //compact whole DB if both begin_key and end_key are empty
    //note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
    virtual bool compact_range(const std::string & begin_key,const std::string & end_key) = 0;
    virtual bool get_estimate_num_keys(uint64_t & num) const = 0;
};

}  // namespace ledger
}  // namespace top
