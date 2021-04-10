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
#include "xdata/xblockchain.h"
#include "xbasic/xmemory.hpp"
#include "xmbus/xmessage_bus.h"

using namespace top::data;

namespace top { namespace store {

using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;

class xstore_face_t : public base::xvdbstore_t {
 public:
    virtual mbus::xmessage_bus_face_t* get_mbus() = 0;

    virtual xaccount_ptr_t query_account(const std::string& address) = 0;

    virtual uint64_t get_blockchain_height(const std::string& account) = 0;

    virtual data::xblock_t* get_block_by_height(const std::string& account, uint64_t height) const = 0;

    virtual int32_t get_map_property(const std::string& account, uint64_t height, const std::string& name, std::map<std::string, std::string>& value) = 0;
    virtual int32_t  get_list_property(const std::string& account, uint64_t height, const std::string& name, std::vector<std::string>& value) = 0;
    virtual int32_t get_string_property(const std::string& account, uint64_t height, const std::string& name, std::string& value) = 0;
    virtual int32_t get_property(const std::string& account, uint64_t height, const std::string& name, xdataobj_ptr_t& obj) = 0;
    virtual xblockchain2_t* clone_account(const std::string& account) const = 0;
    virtual xobject_ptr_t<base::xdataobj_t> clone_property(const std::string& account, const std::string& property_name) = 0;

    virtual xtransaction_store_ptr_t query_transaction_store(const uint256_t &hash) = 0;

    // property operation api
    virtual int32_t string_get(const std::string& account, const std::string& key, std::string& value) const = 0;
    virtual int32_t string_empty(const std::string& account, const std::string& key, bool& empty) = 0;
    virtual int32_t string_size(const std::string& account, const std::string& key, int32_t& size) = 0;

    virtual int32_t list_get_back(const std::string& account, const std::string& key, std::string & value) = 0;
    virtual int32_t list_get_front(const std::string& account, const std::string& key, std::string & value) = 0;
    virtual int32_t list_get(const std::string& account, const std::string& key, const uint32_t index, std::string & value) = 0;
    virtual int32_t list_empty(const std::string& account, const std::string& key, bool& empty) = 0;
    virtual int32_t list_size(const std::string& account, const std::string& key, int32_t& size) = 0;
    virtual int32_t list_get_range(const std::string& account, const std::string &key, int32_t start, int32_t stop, std::vector<std::string> &values) = 0;
    virtual int32_t list_get_all(const std::string& account, const std::string &key, std::vector<std::string> &values) = 0;
    virtual void    list_clear(const std::string& account, const std::string &key) = 0;
    virtual int32_t map_get(const std::string& account, const std::string & key, const std::string & field, std::string & value) = 0;
    virtual int32_t map_empty(const std::string& account, const std::string & key, bool& empty) = 0;
    virtual int32_t map_size(const std::string& account, const std::string & key, int32_t& size) = 0;
    virtual int32_t map_copy_get(const std::string& account, const std::string & key, std::map<std::string, std::string> & map) const = 0;

public:
    virtual bool set_vblock(const std::string & store_path,base::xvblock_t* block) = 0; //fullly store header/cert/input/output of block
    bool set_vblock(base::xvblock_t* block) {return set_vblock(std::string(), block);}
    virtual bool set_vblock_header(const std::string & store_path,base::xvblock_t* block) = 0;//just store header and cert only

    virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, uint64_t height) const = 0;//load full
    base::xvblock_t* get_vblock(const std::string & account, uint64_t height) const {return get_vblock(std::string(), account, height);}
    virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account, uint64_t height) const = 0;//header and cert only
    virtual bool get_vblock_input(const std::string & store_path,base::xvblock_t* for_block) const = 0;//just load input
    virtual bool get_vblock_output(const std::string & store_path,base::xvblock_t* for_block) const = 0;//just load output
    virtual bool get_vblock_offstate(const std::string & store_path,base::xvblock_t* for_block) const = 0;//just load offstate

    virtual bool delete_block_by_path(const std::string & store_path,const std::string & account, uint64_t height, bool has_input_output) = 0;

    virtual bool  set_value(const std::string & key, const std::string& value) = 0;
    virtual bool  delete_value(const std::string & key) = 0;
    virtual const std::string get_value(const std::string & key) const = 0;

public:
    virtual bool  execute_block(base::xvblock_t* block) = 0;
    virtual std::string get_full_offstate(const std::string & account, uint64_t height) = 0;

    virtual base::xdataunit_t* get_full_block_offstate(const std::string & account, uint64_t height) const = 0;
    virtual bool set_full_block_offstate(const std::string & account, uint64_t height, base::xdataunit_t* offstate) = 0;
};

using xstore_face_ptr_t = xobject_ptr_t<xstore_face_t>;

class xstore_factory {
 public:
    static xobject_ptr_t<xstore_face_t> create_store_with_memdb();
    static xobject_ptr_t<xstore_face_t> create_store_with_memdb(observer_ptr<mbus::xmessage_bus_face_t> const & bus);
    static xobject_ptr_t<xstore_face_t> create_store_with_kvdb(const std::string & db_path);
    static xobject_ptr_t<xstore_face_t> create_store_with_static_kvdb(const std::string & db_path, observer_ptr<mbus::xmessage_bus_face_t> const & bus);
    static xobject_ptr_t<xstore_face_t> create_store_with_static_kvdb(std::shared_ptr<db::xdb_face_t>& db, observer_ptr<mbus::xmessage_bus_face_t> const &bus);
};

}  // namespace store
}  // namespace top
