// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.#pragma once

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <map>

#include "xbase/xobject.h"
#include "xdata/xchain_param.h"
#include "xdata/xfull_tableblock.h"
#include "xdb/xdb_face.h"
#include "xmbus/xmessage_bus.h"

#include "xbasic/xversion.h"
#include "xbasic/xserialize_face.h"

#include "xstore/xstore_face.h"

namespace top { namespace store {

using base::xdataobj_t;
using data::xblockchain2_t;

class xstore_key_t {
 public:
    xstore_key_t(int32_t _type, int32_t _block_component, const std::string & _owner, const std::string & _subowner, const std::string & _id)
    : type(_type), block_component(_block_component), owner(_owner), subowner(_subowner), id(_id) {}

    // xstore_key_t(int32_t _type, int32_t _block_component, const std::string & _owner, const std::string & _subowner, const uint256_t & hash)
    // : type(_type), block_component(_block_component), owner(_owner), subowner(_subowner), id(std::string((const char*)hash.data(), hash.size())) {}

    xstore_key_t(int32_t _type, int32_t _block_component, const std::string & _owner, const std::string & _subowner)
    : type(_type), block_component(_block_component), owner(_owner), subowner(_subowner) {}

    std::string to_db_key() const;
    std::string printable_key() const;

 public:
    int32_t         type{0};            // data type
    int32_t         block_component{0};  // block sub type
    std::string     owner{};           // data owner, eg. acount address
    std::string     subowner{};        // data name, eg. property's name
    std::string     id{};              // data id, eg. transaction's hash block's hash
    static const size_t KEY_BUFFER_SIZE = 512;
private:
    bool is_numeric(const std::string& str) const;
};

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
    virtual bool             get_vblock_offdata(const std::string & store_path,base::xvblock_t* for_block) const override;
    virtual bool             set_vblock_offdata(const std::string & store_path,base::xvblock_t* for_block) override;

 public://key-value manage
    virtual bool                set_value(const std::string & key, const std::string& value) override;
    virtual bool                delete_value(const std::string & key) override;
    virtual const std::string   get_value(const std::string & key) const override;
    virtual bool                find_values(const std::string & key,std::vector<std::string> & values) override;//support wild search

 public:
    virtual std::string         get_store_path() const  override {return m_store_path;}

 public://other old api

    xaccount_ptr_t query_account(const std::string& address) override;

    uint64_t get_blockchain_height(const std::string& account) override;

    data::xblock_t* get_block_by_height(const std::string& account, uint64_t height) const override;

    int32_t get_map_property(const std::string& account, uint64_t height, const std::string& name, std::map<std::string, std::string>& value) override;
    int32_t  get_list_property(const std::string& account, uint64_t height, const std::string& name, std::vector<std::string>& value) override;
    int32_t get_string_property(const std::string& account, uint64_t height, const std::string& name, std::string& value) override;
    int32_t get_property(const std::string& account, uint64_t height, const std::string& name, xdataobj_ptr_t& obj) override;
    xblockchain2_t* clone_account(const std::string& account) const override;
    xobject_ptr_t<base::xdataobj_t> clone_property(const std::string& account, const std::string& property_name) override;

    xtransaction_store_ptr_t query_transaction_store(const uint256_t &hash) override;

    // property operation api
    int32_t string_get(const std::string& account, const std::string& key, std::string& value) const override;
    int32_t string_empty(const std::string& account, const std::string& key, bool& empty) override;
    int32_t string_size(const std::string& account, const std::string& key, int32_t& size) override;

    int32_t list_get_back(const std::string& account, const std::string& key, std::string & value) override;
    int32_t list_get_front(const std::string& account, const std::string& key, std::string & value) override;
    int32_t list_get(const std::string& account, const std::string& key, const uint32_t index, std::string & value) override;
    int32_t list_empty(const std::string& account, const std::string& key, bool& empty) override;
    int32_t list_size(const std::string& account, const std::string& key, int32_t& size) override;
    int32_t list_get_range(const std::string& account, const std::string &key, int32_t start, int32_t stop, std::vector<std::string> &values) override;
    int32_t list_get_all(const std::string& account, const std::string &key, std::vector<std::string> &values) override;
    void    list_clear(const std::string& account, const std::string &key) override;
    int32_t map_get(const std::string& account, const std::string & key, const std::string & field, std::string & value) override;
    int32_t map_empty(const std::string& account, const std::string & key, bool& empty) override;
    int32_t map_size(const std::string& account, const std::string & key, int32_t& size) override;
    int32_t map_copy_get(const std::string& account, const std::string & key, std::map<std::string, std::string> & map) const override;

public:
    virtual bool get_vblock_offstate(const std::string & store_path,base::xvblock_t* for_block) const override;
    virtual bool delete_block_by_path(const std::string & store_path,const std::string & account, uint64_t height, bool has_input_output) override;
    virtual bool  execute_block(base::xvblock_t* block) override;
    virtual std::string get_full_offstate(const std::string & account, uint64_t height) override;
    virtual base::xdataunit_t* get_full_block_offstate(const std::string & account, uint64_t height) const override;
    virtual bool set_full_block_offstate(const std::string & account, uint64_t height, base::xdataunit_t* offstate) override;

 private:
    // for rocksdb transaction
    class xstore_transaction_t {
    public:
        xstore_transaction_t(std::shared_ptr<db::xdb_face_t> db);
        ~xstore_transaction_t();
        bool do_read(const std::string& key, std::string& value);
        bool do_get(const xstore_key_t &key, base::xdataobj_t** obj) const;

        bool do_write(const std::string& key, const std::string& value);
        bool do_set(const xstore_key_t &key, base::xdataobj_t *obj);
        bool do_set(const std::map<std::string, std::string>& write_pairs);

        bool do_delete(const std::string& key);

        bool commit();
        bool rollback();
    private:
        db::xdb_transaction_t* m_db_txn {nullptr};
        std::shared_ptr<db::xdb_face_t> m_db;
    };

    std::pair<std::string, std::string> generate_db_object(const xstore_key_t &key, base::xdataobj_t *object);
    std::map<std::string, std::string> generate_block_object(xblock_t *block);

    xdataobj_ptr_t get_property_object(const std::string & account, const std::string & prop_name, uint64_t height);

    bool execute_fullunit(xblockchain2_t* account, const xblock_t* block, std::map<std::string, std::string> & property_pairs);
    bool execute_lightunit(xblockchain2_t* account, const xblock_t* block, std::map<std::string, std::string> & property_pairs);
    bool execute_tableblock_light(xblockchain2_t* account, const xblock_t *block);
    bool execute_tableblock_full(xblockchain2_t* account, xfull_tableblock_t *block, std::map<std::string, std::string> & kv_pairs);

    xdataobj_ptr_t get_property(const std::string & account, const std::string & prop_name, int32_t type);
    bool set_transaction_hash(xstore_transaction_t& txn, const uint64_t unit_height, const std::string &txhash, enum_transaction_subtype txtype, xtransaction_t* tx);

    bool update_blockchain_by_block(xblockchain2_t* blockchain, const data::xblock_t* block, uint64_t now) const;

    bool save_block(const std::string & store_path, data::xblock_t* block);

    bool set_object(const xstore_key_t & key, const std::string& value, const std::string & detail_info = {});
    bool set_object(const xstore_key_t & key, base::xdataobj_t* value, const std::string & detail_info = {});
    base::xdataobj_t* get_object(const xstore_key_t & key) const;
    std::string get_value(const xstore_key_t & key) const;

    base::xdataobj_t* get(const xstore_key_t & key) const;
    bool set(const std::string& key, base::xdataobj_t* object);

 private:
    std::string                     m_store_path;
    std::shared_ptr<db::xdb_face_t> m_db;
};

}  // namespace store
}  // namespace top
