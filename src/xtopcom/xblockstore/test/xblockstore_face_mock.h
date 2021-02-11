#pragma once

#include "xbase/xvledger.h"

using namespace top;
using namespace top::base;

class xblockstore_face_mock_t : public base::xvblockstore_t
{
public:
    xblockstore_face_mock_t() {}
public:
    virtual ~xblockstore_face_mock_t() {}

public: //return raw ptr with added reference,caller respond to release it after that.
    //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
    virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const std::string & account) override {return nullptr;}//genesis block
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const std::string & account) override {return nullptr;}//highest view# for any status
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const std::string & account)    override {return nullptr;}//block with locked status
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const std::string & account) override {return nullptr;}//block with committed status
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const std::string & account)  override {return nullptr;}//block with executed status
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const std::string & account) override {return nullptr;}//block connected to genesis
    virtual xauto_ptr<xvblock_t>  get_latest_full_block(const std::string & account) override {return nullptr;}//block has full state,genesis is a full block
    //just load vblock object but not load header and body those need load seperately if need.
    virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const std::string & account,const uint64_t height,bool ask_full_load = true) override {return nullptr;}
    
    virtual bool                load_block_input(base::xvblock_t* block)  override {return true;}//load and assign input data into block
    virtual bool                load_block_output(base::xvblock_t* block) override {return true;}//load and assign output data into block
    
    virtual bool                store_block(base::xvblock_t* block)  override {return true;} //return false if fail to store
    virtual bool                delete_block(base::xvblock_t* block) override {return true;} //return false if fail to delete
    
public://better performance,and return raw ptr with added reference,caller respond to release it after that.
    //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
    
    virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const base::xvaccount_t & account) override {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const base::xvaccount_t & account)  override {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const base::xvaccount_t & account)   override {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const base::xvaccount_t & account)override {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const base::xvaccount_t & account) override {return nullptr;}
    virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const base::xvaccount_t & account)override {return nullptr;}
    virtual xauto_ptr<xvblock_t>  get_latest_full_block(const xvaccount_t & account) override {return nullptr;} //block has full state,genesis is a full block
	//ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
    virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const xvaccount_t & account,const uint64_t height,bool ask_full_load = true) override {return nullptr;}
    
    virtual bool                load_block_input(const base::xvaccount_t & account,base::xvblock_t* block) override {return true;}
    virtual bool                load_block_output(const base::xvaccount_t & account,base::xvblock_t* block) override {return true;}
    
    virtual bool                store_block(const base::xvaccount_t & account,base::xvblock_t* block) override {return true;}
    virtual bool                delete_block(const base::xvaccount_t & account,base::xvblock_t* block) override {return true;}
    
public://batch process api
    virtual base::xblock_mptrs  get_latest_blocks(const base::xvaccount_t & account) override {return {};}
    virtual bool                store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks) override {return true;}
    
protected:
    //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
    virtual std::string         get_store_path() const override {return "";}//each store may has own space at DB/disk

private:

    virtual int32_t             do_write(base::xstream_t & stream) override {return 0;}//write whole object to binary
    virtual int32_t             do_read(base::xstream_t & stream) override {return 0;} //read from binary and regeneate content
};