// Copyright (c) 2017-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xunitblock.hpp"
#include "xbase/xcontext.h"
#include "xbase/xmem.h"
#include "xbase/xhash.h"
#include "xbase/xobject.h"

#ifdef DEBUG
    #define __test_cache_for_block_store__
#endif

namespace top
{
    namespace test
    {
        xunitheader_t::xunitheader_t(const std::string & account,uint64_t height,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,const std::string & body_input)
        {
            const uint16_t ledger_id = base::xvaccount_t::get_ledgerid_from_account(account);
            set_chainid(base::xvaccount_t::get_chainid_from_ledgerid(ledger_id));
            set_account(account);
            set_height(height);
            if(0 == height) //init for genesis block
            {
                set_block_type(base::enum_xvblock_type_genesis); //must be enum_xvblock_type_genesis
                set_block_class(base::enum_xvblock_class_nil);   //must be nil
                set_block_level(base::enum_xvblock_level_unit);  //must be nil
                
                set_last_block_hash(std::string());              //must be nil
                set_last_full_block(std::string(),0);            //must be nil and 0
            }
            else
            {
                set_block_type(base::enum_xvblock_type_txs); //just test txs
                set_last_block_hash(last_block_hash);
                set_last_full_block(last_full_block_hash,last_full_block_height);
                set_block_level(base::enum_xvblock_level_unit);     //just test unit block
                
                if(body_input.empty())
                    set_block_class(base::enum_xvblock_class_nil);
                else
                    set_block_class(base::enum_xvblock_class_light); //test
            }
        }
        
        xunitheader_t::~xunitheader_t()
        {
        }
        
        xunitcert_t::xunitcert_t()
            :base::xvqcert_t(std::string()) //init as empty
        {
            set_consensus_type(base::enum_xconsensus_type_xhbft);             //test pBFT
            set_consensus_threshold(base::enum_xconsensus_threshold_2_of_3);  // >= 2/3 vote
            
            set_crypto_key_type(base::enum_xvchain_key_curve_secp256k1); //default one
            set_crypto_sign_type(base::enum_xvchain_threshold_sign_scheme_schnorr); //test schnorr scheme
            set_crypto_hash_type(enum_xhash_type_sha2_256);  //basic sha2_256
        }
        
        xunitcert_t::~xunitcert_t()
        {
            
        }
       
        xclockblock_t*  xclockblock_t::create_clockblock(const std::string & account,uint64_t height,uint64_t clock,uint64_t viewid,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,const std::string & body_input,const std::string & body_output) //link xunitheader_t and xunitcert_t into xunitblock_t
        {
            //setp#1 create header object first,must initialize everyting here
            xunitheader_t* _header_obj = new xunitheader_t(account,height,last_block_hash,last_full_block_hash,last_full_block_height,body_input);
            
            //step#2: initialize certifaction object
            xunitcert_t  * _cert_obj   = new xunitcert_t();
            _cert_obj->set_viewid(viewid);
            _cert_obj->set_clock(clock);
            
            //step#3: link header and certification into block
            base::xvinput_t* input_ptr = NULL;
            base::xvoutput_t* output_ptr = NULL;
 
            if (_header_obj->get_block_class() != base::enum_xvblock_class_nil)
            {
                base::xvbinentity_t * input_entity = new base::xvbinentity_t(body_input);
                std::vector<base::xventity_t*> input_entity_vector;
                input_entity_vector.push_back(input_entity);
                
                base::xvbinentity_t * output_entity = new base::xvbinentity_t(body_output);
                std::vector<base::xventity_t*> output_entity_vector;
                output_entity_vector.push_back(output_entity);
                
                base::xstrmap_t* input_resource = new base::xstrmap_t();
                input_resource->set("body", body_input);
                
                base::xstrmap_t* output_resource = new base::xstrmap_t();
                output_resource->set("body", body_output);
                
                input_ptr = new base::xvinput_t(input_entity_vector,*input_resource);
                output_ptr = new base::xvoutput_t(output_entity_vector,*output_resource);
                
                input_resource->release_ref();
                output_resource->release_ref();
                
                input_entity->release_ref();
                output_entity->release_ref();
            }
            
            //*****************NOT allow change _header_obj anymore after this line
            xclockblock_t * _block_obj  = new xclockblock_t(*_header_obj,*_cert_obj,input_ptr,output_ptr);
            //*****************Allow contiusely modify xunitcert_t object
            
            //then clean
            _header_obj->release_ref();//safe to release now
            _cert_obj->release_ref();  //safe to release now
            
            if(input_ptr != NULL)
                input_ptr->release_ref();
            
            if(output_ptr != NULL)
                output_ptr->release_ref();
            
            return _block_obj;
        }
        
        xclockblock_t::xclockblock_t(xunitheader_t & header,xunitcert_t & cert,base::xvinput_t* input,base::xvoutput_t* output)
        : base::xvblock_t(header,cert,input,output)
        {
        }
        
        xclockblock_t:: ~xclockblock_t()
        {
            xdbg("xclockblock_t::destroy,dump=%s",dump().c_str());
        }
        
        xunitblock_t*  xunitblock_t::create_unitblock(const std::string & account,uint64_t height,uint64_t clock,uint64_t viewid,const std::string & last_block_hash,const std::string & last_full_block_hash,const uint64_t last_full_block_height,const std::string & body_input,const std::string & body_output) //link xunitheader_t and xunitcert_t into xunitblock_t
        {
            if(0 == height)
                return create_genesis_block(account);
            
            //setp#1 create header object first,must initialize everyting here
            xunitheader_t* _header_obj = new xunitheader_t(account,height,last_block_hash,last_full_block_hash,last_full_block_height,body_input);

            //step#2: initialize certifaction object
            xunitcert_t  * _cert_obj   = new xunitcert_t();
            _cert_obj->set_viewid(viewid);
            _cert_obj->set_clock(clock);
 
            //step#3: link header and certification into block
            
            base::xvinput_t* input_ptr = NULL;
            base::xvoutput_t* output_ptr = NULL;
            if (_header_obj->get_block_class() != base::enum_xvblock_class_nil)
            {
                base::xvbinentity_t * input_entity = new base::xvbinentity_t(body_input);
                std::vector<base::xventity_t*> input_entity_vector;
                input_entity_vector.push_back(input_entity);
                
                base::xvbinentity_t * output_entity = new base::xvbinentity_t(body_output);
                std::vector<base::xventity_t*> output_entity_vector;
                output_entity_vector.push_back(output_entity);
                
                base::xstrmap_t* input_resource = new base::xstrmap_t();
                input_resource->set("body", body_input);
                
                base::xstrmap_t* output_resource = new base::xstrmap_t();
                output_resource->set("body", body_output);
                
                input_ptr = new base::xvinput_t(input_entity_vector,*input_resource);
                output_ptr = new base::xvoutput_t(output_entity_vector,*output_resource);
                
                input_resource->release_ref();
                output_resource->release_ref();
                
                input_entity->release_ref();
                output_entity->release_ref();
            }
            
            //*****************NOT allow change _header_obj anymore after this line
            xunitblock_t * _block_obj  = new xunitblock_t(*_header_obj,*_cert_obj,input_ptr,output_ptr);
            
            //*****************Allow contiusely modify xunitcert_t object
            
            //then clean
            _header_obj->release_ref();//safe to release now
            _cert_obj->release_ref();  //safe to release now
            if(input_ptr != NULL)
                input_ptr->release_ref();
            
            if(output_ptr != NULL)
                output_ptr->release_ref();
            return _block_obj;
        }
        
        xunitblock_t*   xunitblock_t::create_genesis_block(const std::string & account)
        {
            std::string last_block_hash;
            std::string last_full_block_hash;
            std::string body_input;
            std::string body_output;
            xunitblock_t * genesis_block_ptr =  xunitblock_t::create_unitblock(account,0,0,0,last_block_hash,last_full_block_hash,0,body_input,body_output);
            
            genesis_block_ptr->get_cert()->set_viewtoken(-1); //for test only
            xvip2_t _wildaddr{(xvip_t)(-1),(uint64_t)-1};
            genesis_block_ptr->get_cert()->set_validator(_wildaddr);//genesis block not
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_locked);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_committed);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_executed);
            genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_connected);
            genesis_block_ptr->reset_modified_count();
            
            return genesis_block_ptr;
        }
        
        xunitblock_t::xunitblock_t(xunitheader_t & header,xunitcert_t & cert,base::xvinput_t* input,base::xvoutput_t* output)
          : base::xvblock_t(header,cert,input,output)
        {
        }
 
        xunitblock_t:: ~xunitblock_t()
        {
            xdbg("xunitblock_t::destroy,dump=%s",dump().c_str());
        }
        
#ifdef DEBUG //tracking memory of proposal block
        int32_t   xunitblock_t::add_ref()
        {
            //xdump_stack2(enum_xlog_level_debug,3,"%d reference_to_add,objectid=%lld,height=%llu,view=%llu,this=%llx",get_refcount(),get_obj_id(),get_height(),get_viewid(),(int64_t)this);
            return base::xvblock_t::add_ref();
        }
        int32_t   xunitblock_t::release_ref()
        {
            //xdump_stack2(enum_xlog_level_debug,3,"%d reference_to_release,objectid=%lld,height=%llu,view=%llu,this=%llx",get_refcount(),get_obj_id(),get_height(),get_viewid(),(int64_t)this);
            xdbgassert(get_refcount() > 0);
            return base::xvblock_t::release_ref();
        }
#endif
        
        xunitblockstore_t::xunitblockstore_t()
         :xvblockstore_t(base::xcontext_t::instance(),0)
        {
        }
 
        xunitblockstore_t::~xunitblockstore_t()
        {
        }
        
        base::xauto_ptr<base::xvblock_t>   xunitblockstore_t::get_genesis_block(const std::string & account)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            base::xvblock_t* & genesis_block_ptr = _account_blocks[0];
            if(NULL == genesis_block_ptr)
            {
                std::string last_block_hash;
                std::string last_full_block_hash;
                std::string body_input;
                std::string body_output;
                
                if(account.find("clock") != std::string::npos)
                    genesis_block_ptr = xclockblock_t::create_clockblock(account,0,0,0,last_block_hash,last_full_block_hash,0,body_input,body_output);
                else
                    genesis_block_ptr = xunitblock_t::create_unitblock(account,0,0,0,last_block_hash,last_full_block_hash,0,body_input,body_output);
                
                genesis_block_ptr->get_cert()->set_viewtoken(-1); //for test only
                xvip2_t _wildaddr{(xvip_t)(-1),(uint64_t)-1};
                genesis_block_ptr->get_cert()->set_validator(_wildaddr);//genesis block not
                genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_authenticated);
                genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_locked);
                genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_committed);
                genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_executed);
                genesis_block_ptr->set_block_flag(base::enum_xvblock_flag_connected);
                genesis_block_ptr->reset_modified_count();
            }
            genesis_block_ptr->add_ref();
            return genesis_block_ptr;
        }
 
        base::xauto_ptr<base::xvblock_t>   xunitblockstore_t::get_latest_cert_block(const std::string & account)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            if(_account_blocks.empty())
            {
                return get_genesis_block(account);
            }
            _account_blocks.rbegin()->second->add_ref();
            return _account_blocks.rbegin()->second;
        }

        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_locked_block(const std::string & account)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if(block_ptr->check_block_flag(base::enum_xvblock_flag_locked))
                {
                    block_ptr->add_ref();
                    return block_ptr;
                }
            }
            return  get_genesis_block(account);
        }
        
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_committed_block(const std::string & account)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if(block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                {
                    block_ptr->add_ref();
                    return block_ptr;
                }
            }
            return  get_genesis_block(account);
        }
        
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_executed_block(const std::string & account)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if(block_ptr->check_block_flag(base::enum_xvblock_flag_executed))
                {
                    block_ptr->add_ref();
                    return block_ptr;
                }
            }
            return  get_genesis_block(account);
        }
        
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_connected_block(const std::string & account)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if(block_ptr->check_block_flag(base::enum_xvblock_flag_executed))
                {
                    block_ptr->add_ref();
                    return block_ptr;
                }
            }
            return  get_genesis_block(account);
        }
        
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::query_block(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account.get_address()];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if( (block_ptr->get_height() == height) && (block_ptr->get_viewid() == viewid) )
                {
                    block_ptr->add_ref();
                    return block_ptr;
                }
            }
            return nullptr;
        }
        
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::query_block(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account.get_address()];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if( (block_ptr->get_height() == height) && (block_ptr->get_block_hash() == blockhash) )
                {
                    block_ptr->add_ref();
                    return block_ptr;
                }
            }
            return nullptr;
        }
        
        base::xblock_vector xunitblockstore_t::query_block(const base::xvaccount_t & account,const uint64_t height)//might mutiple certs at same height
        {
            std::vector<base::xvblock_t*> all_blocks_at_height;
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account.get_address()];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if(block_ptr->get_height() == height)
                {
                    block_ptr->add_ref();
                    all_blocks_at_height.push_back(block_ptr);
                }
            }
            return base::xblock_vector(all_blocks_at_height);
        }
        
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::get_latest_full_block(const std::string & account)
        {
            base::xauto_ptr<base::xvblock_t> _latest_block = get_latest_cert_block(account);//full-block could be cert,lock, or commit status
            if( (_latest_block->get_block_class() == base::enum_xvblock_class_full) || (_latest_block->get_height() == 0) )
            {
                _latest_block->add_ref();//added reference before return
                return _latest_block.get();//note:genesis is qualifed as definition of full block
            }
            return load_block_object(account,_latest_block->get_last_full_block_height(),0,false);
        }
        
        //one api to get latest_commit/latest_lock/latest_cert for better performance
        base::xblock_mptrs     xunitblockstore_t::get_latest_blocks(const base::xvaccount_t & account)
        {
            base::xvblock_t*  cert_block = nullptr;
            base::xvblock_t*  lock_block = nullptr;
            base::xvblock_t*  commit_block = nullptr;
            
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account.get_account()];
            for(auto it = _account_blocks.rbegin(); it != _account_blocks.rend(); ++it)
            {
                base::xvblock_t* block_ptr = it->second;
                if(NULL == cert_block)
                {
                    block_ptr->add_ref();
                    cert_block = block_ptr;
                }
                if(NULL == lock_block)
                {
                    if(block_ptr->check_block_flag(base::enum_xvblock_flag_locked))
                    {
                        block_ptr->add_ref();
                        lock_block = block_ptr;
                    }
                }
                if(NULL == commit_block)
                {
                    if(block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        block_ptr->add_ref();
                        commit_block = block_ptr;
                    }
                }
            }
            return base::xblock_mptrs(cert_block,lock_block,commit_block);
        }
        
        //just load vblock object ,but not load header and body those need load seperately if need
        base::xauto_ptr<base::xvblock_t>     xunitblockstore_t::load_block_object(const std::string & account,const uint64_t height,bool ask_full_load)
        {
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[account];
            auto it = _account_blocks.find(height);
            if(it != _account_blocks.end())
            {
                it->second->add_ref();
                return it->second;
            }
            
            const std::string _header_cert_path = get_store_path() + base::xvblock_t::create_header_path(account, height);
            const std::string _header_cert_bin  = load_value_by_path(_header_cert_path);
            if(_header_cert_bin.empty())
            {
                xwarn("xvblockstore_t::load_block_object,not found block object at %s",_header_cert_path.c_str());
                return nullptr;
            }
            base::xvblock_t* new_block_ptr = base::xvblock_t::create_block_object(_header_cert_bin);
            if(new_block_ptr != NULL)
            {
                new_block_ptr->add_ref();
                _account_blocks[height] = new_block_ptr;
            }
            if(ask_full_load)
            {
                load_block_input(new_block_ptr);
                load_block_output(new_block_ptr);
            }
            return new_block_ptr;
        }
        
        bool    xunitblockstore_t::load_block_input(base::xvblock_t* block_ptr) //load and assign body data into  xvblock_t
        {
            xassert(block_ptr != NULL);
            if(NULL == block_ptr)
                return false;
            
            const std::string full_object_path = get_store_path() + block_ptr->get_input_path();
            const std::string _content = load_value_by_path(full_object_path);
            if(_content.empty())
            {
                xwarn("xvblockstore_t::load_block_input,emtpy content at path(%s)",full_object_path.c_str());
                return false;
            }
            xinfo("xvblockstore_t::load_block_input,loaded it for block:[chainid:%u->account(%s)->height(%llu)->viewid(%llu) at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_store_path().c_str());
            return block_ptr->set_input_data(_content);
        }
        
        bool    xunitblockstore_t::load_block_output(base::xvblock_t* block_ptr) //load and assign body data into  xvblock_t
        {
            xassert(block_ptr != NULL);
            if(NULL == block_ptr)
                return false;
            
            const std::string full_object_path = get_store_path() + block_ptr->get_output_path();
            const std::string _content = load_value_by_path(full_object_path);
            if(_content.empty())
            {
                xwarn("xvblockstore_t::load_block_output,emtpy content at path(%s)",full_object_path.c_str());
                return false;
            }
            xinfo("xvblockstore_t::load_block_output,loaded it for block:[chainid:%u->account(%s)->height(%llu)->viewid(%llu) at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_store_path().c_str());
            return block_ptr->set_output_data(_content);
        }

        bool    xunitblockstore_t::store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks) //better performance
        {
            for(auto it : batch_store_blocks)
            {
                store_block(it);
            }
            return true;
        }
        
        bool    xunitblockstore_t::store_block(base::xvblock_t* block_ptr) //update old one or insert as new
        {
            if(NULL == block_ptr)
                return false;
            
            if(block_ptr->is_deliver(false) == false) //must have full valid data
                return false;
            
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[block_ptr->get_account()];
            base::xvblock_t* & existing = _account_blocks[block_ptr->get_height()];
            if(existing != block_ptr)
            {
                if(existing != NULL)
                    existing->release_ref();
                
                block_ptr->add_ref();
                existing = block_ptr;
            }
            else //sameone
            {
                return true;
            }
            
            if(_account_blocks.size() > 128)
            {
                const uint64_t latest_full_block_height = _account_blocks.rbegin()->second->get_last_full_block_height();
                for(auto it = _account_blocks.begin(); it != _account_blocks.end(); ++it)
                {
                    if( (it->second->get_height() != 0) && (it->second->get_height() != latest_full_block_height) )
                    {
                        it->second->close();//disconnect from prev-block and next-block
                        it->second->release_ref();
                        _account_blocks.erase(it);
                        break;
                    }
                }
            }
        
            const std::string _input_path    = get_store_path() + block_ptr->get_input_path();
            store_value_by_path(_input_path,block_ptr->get_input_data());
            
            const std::string _output_path    = get_store_path() + block_ptr->get_output_path();
            store_value_by_path(_output_path,block_ptr->get_output_data());
            
            const std::string _block_path  = get_store_path() + block_ptr->get_header_path();
            std::string vblock_bin;
            block_ptr->serialize_to_string(vblock_bin);
            store_value_by_path(_block_path,vblock_bin);
            
            return true;
        }
        bool    xunitblockstore_t::delete_block(base::xvblock_t* block_ptr)//return error code indicate what is result
        {
            if(NULL == block_ptr)
                return false;
            
            const std::string _input_path    = get_store_path() + block_ptr->get_input_path();
            const std::string _output_path   = get_store_path() + block_ptr->get_output_path();
            const std::string _block_path    = get_store_path() + block_ptr->get_header_path();
            
            delete_value_by_path(_input_path);
            delete_value_by_path(_output_path);
            delete_value_by_path(_block_path);
            
            xinfo("xvblockstore_t::delete_block,delete block:[chainid:%u->account(%s)->height(%llu)->viewid(%llu) at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_store_path().c_str());
            
            std::map<uint64_t,base::xvblock_t*> & _account_blocks = m_blocks[block_ptr->get_account()];
            auto it  = _account_blocks.find(block_ptr->get_height());
            if(it != _account_blocks.end())
            {
                if(it->second != NULL)
                    it->second->release_ref();
                _account_blocks.erase(it);
            }
            return true;
        }
        
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_genesis_block(const base::xvaccount_t & account)
        {
            return get_genesis_block(account.get_address());
        }
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_cert_block(const base::xvaccount_t & account)
        {
            return get_latest_cert_block(account.get_address());
        }
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_locked_block(const base::xvaccount_t & account)
        {
            return get_latest_locked_block(account.get_address());
        }
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_committed_block(const base::xvaccount_t & account)
        {
            return get_latest_committed_block(account.get_address());
        }
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_executed_block(const base::xvaccount_t & account)
        {
            return get_latest_executed_block(account.get_address());
        }
        base::xauto_ptr<base::xvblock_t>    xunitblockstore_t::get_latest_connected_block(const base::xvaccount_t & account)
        {
            return get_latest_connected_block(account.get_address());
        }
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::get_latest_full_block(const base::xvaccount_t & account)
        {
            return get_latest_full_block(account.get_address());
        }
    
        base::xblock_vector       xunitblockstore_t::load_block_object(const base::xvaccount_t & account,const uint64_t height)
        {
            return nullptr;
        }
        
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::load_block_object(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load)
        {
            return nullptr;
        }
        
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::load_block_object(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load)
        {
            return nullptr;
        }
        
        base::xauto_ptr<base::xvblock_t>  xunitblockstore_t::load_block_object(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,bool ask_full_load)
        {
            return nullptr;
        }
        
        bool  xunitblockstore_t::store_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            return store_block(block);
        }
        bool  xunitblockstore_t::delete_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            return delete_block(block);
        }
        bool  xunitblockstore_t::load_block_input(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            return load_block_input(block);
        }
        bool  xunitblockstore_t::load_block_output(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            return load_block_output(block);
        }
        
        //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
        bool   xunitblockstore_t::clean_caches(const base::xvaccount_t & account)
        {
            return true;
        }
        
        base::xvblock_t*   xunitblockstore_t::create_clock_block(const std::string & account,const std::string & block_input,const std::string & block_output)
        {
            base::xauto_ptr<base::xvblock_t> last_block = get_latest_cert_block(account);
            base::xauto_ptr<base::xvblock_t> last_full_block = get_latest_full_block(account);
            
            base::xvblock_t* new_proposal_block = xclockblock_t::create_clockblock(account,last_block->get_height() + 1,last_block->get_clock() + 1,last_block->get_viewid() + 1,last_block->get_block_hash(),last_full_block->get_block_hash(),last_full_block->get_height(),block_input,block_output);
            
            new_proposal_block->reset_prev_block(last_block.get());
            return new_proposal_block;
        }
        
        base::xvblock_t*   xunitblockstore_t::create_proposal_block(const std::string & account,const std::string & block_input,const std::string & block_output)
        {
            base::xauto_ptr<base::xvblock_t> last_block = get_latest_cert_block(account);
            base::xauto_ptr<base::xvblock_t> last_full_block = get_latest_full_block(account);
            
            base::xvblock_t* new_proposal_block = xunitblock_t::create_unitblock(account,last_block->get_height() + 1,last_block->get_clock() + 1,last_block->get_viewid() + 1,last_block->get_block_hash(),last_full_block->get_block_hash(),last_full_block->get_height(), block_input,block_output);
            
            new_proposal_block->reset_prev_block(last_block.get());
            return new_proposal_block;
        }
 
        //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
        std::string     xunitblockstore_t::get_store_path() const//each store may has own space at DB/disk
        {
            return std::string("/"); //a global & shared store for block as test
        }
        
        const std::string   xunitblockstore_t::load_value_by_path(const std::string & full_path_as_key)
        {
            return m_dumy_store[full_path_as_key];
        }
        
        bool                xunitblockstore_t::delete_value_by_path(const std::string & full_path_as_key)
        {
            auto it = m_dumy_store.find(full_path_as_key);
            if(it != m_dumy_store.end())
                m_dumy_store.erase(it);
                
            return true;
        }
        
        bool                xunitblockstore_t::store_value_by_path(const std::string & full_path_as_key,const std::string & value)
        {
            //m_dumy_store[full_path_as_key] = value;
            return true;
        }
    }
}
