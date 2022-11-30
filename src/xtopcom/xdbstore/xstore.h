// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include <string>

#include "xbase/xobject.h"
#include "xdb/xdb_face.h"
#include "xdbstore/xstore_face.h"


namespace top { namespace store {

// wrapper for the block/tx storing in xdb
class xstore final: public xstore_face_t {
 public:
    explicit xstore(const std::shared_ptr<db::xdb_face_t>& db);

 protected:
    ~xstore() {}

 public://block object manage
    virtual bool             set_vblock(const std::string & store_path,base::xvblock_t* block) override;
    virtual bool             set_vblock_header(const std::string & store_path,base::xvblock_t* block) override;

    virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, const uint64_t height) const override;
    virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account,const uint64_t height) const override;
    virtual bool             get_vblock_input(const std::string & store_path,base::xvblock_t* for_block)  const override;
    virtual bool             get_vblock_output(const std::string & store_path,base::xvblock_t* for_block) const override;

 public://key-value manage
    virtual bool                set_value(const std::string & key, const std::string& value) override;
    virtual bool                delete_value(const std::string & key) override;
    virtual std::string   get_value(const std::string & key) const override;
    virtual bool                set_values(const std::map<std::string, std::string> & objs) override;
    virtual bool                delete_values(const std::vector<std::string> & to_deleted_keys) override;
    bool delete_values(std::vector<gsl::span<char const>> const & to_deleted_keys) override;

public:
    //prefix must start from first char of key
    virtual bool             read_range(const std::string& prefix, std::vector<std::string>& values) override;
    //note:begin_key and end_key must has same style(first char of key)
    virtual bool             delete_range(const std::string & begin_key,const std::string & end_key) override;
    //key must be readonly(never update after PUT),otherwise the behavior is undefined
    virtual bool             single_delete(const std::string & target_key) override;
    
    //compact whole DB if both begin_key and end_key are empty
    //note: begin_key and end_key must be at same CF while XDB configed by multiple CFs
    virtual bool             compact_range(const std::string & begin_key,const std::string & end_key) override;
    virtual void             GetDBMemStatus() const override;
    //iterator each key of prefix.note: go throuh whole db if prefix is empty
    bool                     read_range_callback(const std::string& prefix,db::xdb_iterator_callback callback,void * cookie);
 public:
    virtual std::string         get_store_path() const  override {return m_store_path;}
    virtual bool                open() const override;
    virtual bool                close() const override;

 private:
    std::string                     m_store_path;
    std::shared_ptr<db::xdb_face_t> m_db;
};

}  // namespace store
}  // namespace top
