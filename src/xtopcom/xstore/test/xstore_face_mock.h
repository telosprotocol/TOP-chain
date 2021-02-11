#pragma once

#include "xstore/xstore_face.h"

using namespace top;

class xstore_face_mock_t : public top::store::xstore_face_t {
    public:
    virtual mbus::xmessage_bus_face_t* get_mbus() override {return nullptr;}

    virtual xaccount_ptr_t query_account(const std::string& address)  override { return nullptr; }

    virtual uint64_t get_blockchain_height(const std::string& account) override { return 0; }

    virtual data::xblock_t* get_block_by_height(const std::string& account, uint64_t height) const override { return nullptr; }

    virtual int32_t get_map_property(const std::string& account, uint64_t height, const std::string& name, std::map<std::string, std::string>& value) override { return 0; }
    virtual int32_t  get_list_property(const std::string& account, uint64_t height, const std::string& name, std::vector<std::string>& value) override { return 0; }
    virtual int32_t get_string_property(const std::string& account, uint64_t height, const std::string& name, std::string& value) override { return 0; }
    virtual int32_t get_property(const std::string& account, uint64_t height, const std::string& name, store::xdataobj_ptr_t& obj) override { return 0; }
    virtual xblockchain2_t* clone_account(const std::string& account) const  override { return nullptr; }
    virtual xobject_ptr_t<base::xdataobj_t> clone_property(const std::string& account, const std::string& property_name)  override { return nullptr; }

    virtual xtransaction_store_ptr_t query_transaction_store(const uint256_t &hash)  override { return nullptr; }

    // property operation api
    virtual int32_t string_get(const std::string& account, const std::string& key, std::string& value) const override { return 0; }
    virtual int32_t string_empty(const std::string& account, const std::string& key, bool& empty) override { return 0; }
    virtual int32_t string_size(const std::string& account, const std::string& key, int32_t& size) override { return 0; }

    virtual int32_t list_get_back(const std::string& account, const std::string& key, std::string & value) override { return 0; }
    virtual int32_t list_get_front(const std::string& account, const std::string& key, std::string & value) override { return 0; }
    virtual int32_t list_get(const std::string& account, const std::string& key, const uint32_t index, std::string & value) override { return 0; }
    virtual int32_t list_empty(const std::string& account, const std::string& key, bool& empty) override { return 0; }
    virtual int32_t list_size(const std::string& account, const std::string& key, int32_t& size) override { return 0; }
    virtual int32_t list_get_range(const std::string& account, const std::string &key, int32_t start, int32_t stop, std::vector<std::string> &values) override { return 0; }
    virtual int32_t list_get_all(const std::string& account, const std::string &key, std::vector<std::string> &values) override { return 0; }
    virtual void    list_clear(const std::string& account, const std::string &key) override { return; }
    virtual int32_t map_get(const std::string& account, const std::string & key, const std::string & field, std::string & value) override { return 0; }
    virtual int32_t map_empty(const std::string& account, const std::string & key, bool& empty) override { return 0; }
    virtual int32_t map_size(const std::string& account, const std::string & key, int32_t& size) override { return 0; }
    virtual int32_t map_copy_get(const std::string& account, const std::string & key, std::map<std::string, std::string> & map) const override { return 0; }

    virtual bool set_vblock(const std::string & store_path,base::xvblock_t* block) override { return true; }
    virtual bool set_vblock_header(const std::string & store_path,base::xvblock_t* block) override { return true; }

    virtual base::xvblock_t* get_vblock(const std::string & store_path,const std::string & account, uint64_t height) const override { return nullptr; }
    virtual base::xvblock_t* get_vblock_header(const std::string & store_path,const std::string & account, uint64_t height) const override { return nullptr; }
    virtual bool get_vblock_input(const std::string & store_path,base::xvblock_t* for_block) const override { return true; }
    virtual bool get_vblock_output(const std::string & store_path,base::xvblock_t* for_block) const override { return true; }

    virtual bool delete_block_by_path(const std::string & store_path,const std::string & account, uint64_t height, bool has_input_output) override { return true; }

    virtual bool  set_value(const std::string & key, const std::string& value) override { return true; }
    virtual bool  delete_value(const std::string & key) override { return true; }
    virtual const std::string get_value(const std::string & key) const override { return ""; }

};
