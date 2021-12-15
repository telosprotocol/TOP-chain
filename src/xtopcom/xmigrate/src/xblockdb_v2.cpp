// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include <iostream>
#include "xvledger/xvbindex.h"
#include "xvledger/xvblock.h"
#include "xvledger/xvdbkey.h"
#include "xbase/xobject_ptr.h"
#include "xblockdb_v2.h"

namespace top
{
    namespace base
    {
        xblockdb_v2_t::xblockdb_v2_t(base::xvdbstore_t* xvdb_ptr) {
            m_xvdb_ptr = xvdb_ptr;
            xassert(xvdb_ptr != NULL);
            xvdb_ptr->add_ref();
        }

        xblockdb_v2_t::~xblockdb_v2_t()
        {
            m_xvdb_ptr->release_ref();
        }

        base::xauto_ptr<base::xvactmeta_t>  xblockdb_v2_t::get_meta(base::xvdbstore_t* dbstore, const base::xvaccount_t & account)
        {
            const std::string full_meta_path = base::xvdbkey_t::create_account_meta_key_old(account);
            const std::string meta_content = dbstore->get_value(full_meta_path);
            base::xvactmeta_t* _meta = new xvactmeta_t(account);  // create empty meta default
            if (!meta_content.empty()) {
                if (_meta->serialize_from_string(meta_content) <= 0) {
                    xerror("xblockdb_v2_t::get_meta,bad meta_serialized_data that not follow spec");
                    return new xvactmeta_t(account);  // return empty meta
                }
            } else {
                xwarn("xblockdb_v2_t::get_meta,meta empty. address=%s,full_meta_path=%s",account.get_address().c_str(),full_meta_path.c_str());
            }
            return _meta;
        }

        base::xauto_ptr<base::xvactmeta_t>  xblockdb_v2_t::get_v3_meta(base::xvdbstore_t* dbstore, const base::xvaccount_t & account)
        {
            // XTODO same key now
            const std::string full_meta_path = base::xvdbkey_t::create_account_meta_key(account);
            const std::string meta_content = dbstore->get_value(full_meta_path);
            base::xvactmeta_t* _meta = new xvactmeta_t(account);  // create empty meta default
            if (!meta_content.empty()) {
                if (_meta->serialize_from_string(meta_content) <= 0) {
                    xerror("xblockdb_v2_t::get_v3_meta,bad meta_serialized_data that not follow spec");
                    return new xvactmeta_t(account);  // return empty meta
                }
            } else {
                xwarn("xblockdb_v2_t::get_v3_meta,meta empty. address=%s,full_meta_path=%s",account.get_address().c_str(),full_meta_path.c_str());
            }
            return _meta;
        }

        uint64_t xblockdb_v2_t::get_genesis_height(xvdbstore_t* dbstore, const base::xvaccount_t & account) {
            const std::string key_path = base::xvdbkey_t::create_chain_key(account);
            std::string value = dbstore->get_value(key_path);
            uint64_t height = 0;
            if (!value.empty()) {
                base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
                stream >> height;
            }
            return height;
        }

        uint64_t xblockdb_v2_t::get_v3_genesis_height(xvdbstore_t* dbstore, const base::xvaccount_t & account) {
            const std::string key_path = base::xvdbkey_t::create_account_span_genesis_height_key(account);
            std::string value = dbstore->get_value(key_path);
            uint64_t height = 0;
            if (!value.empty()) {
                base::xstream_t stream(base::xcontext_t::instance(), (uint8_t *)value.c_str(), value.size());
                stream >> height;
            }
            return height;
        }

        xobject_ptr_t<base::xvbindex_t>   xblockdb_v2_t::read_index_from_db(const std::string & index_db_key_path)
        {
            const std::string index_bin = get_xdbstore()->get_value(index_db_key_path);
            if(index_bin.empty())
            {
                xwarn("xvblockdb_t::read_index_from_db,fail to read from db for path(%s)",index_db_key_path.c_str());
                return NULL;
            }
            
            xobject_ptr_t<base::xvbindex_t> new_index_obj = make_object_ptr<base::xvbindex_t>();
            if(new_index_obj->serialize_from(index_bin) <= 0)
            {
                xerror("xvblockdb_t::read_index_from_db,fail to serialize from db for path(%s)",index_db_key_path.c_str());
                return NULL;
            }
            
            if(new_index_obj->check_modified_flag())
            {
                xerror("xvblockdb_t::read_index_from_db,dirty index from db for path(%s)",index_db_key_path.c_str());
                new_index_obj->reset_modify_flag(); //should not happen,but add exception for incase
            }
            return new_index_obj;
        }

        std::vector<xobject_ptr_t<base::xvbindex_t>>   xblockdb_v2_t::read_indexs(const base::xvaccount_t & account,const uint64_t target_height)
        {
            std::vector<xobject_ptr_t<base::xvbindex_t>> all_blocks_at_height;
            
            const std::string main_entry_key = base::xvdbkey_t::create_block_index_key(account,target_height);
            xobject_ptr_t<base::xvbindex_t> index_entry = read_index_from_db(main_entry_key);
            if(index_entry == NULL) //main entry
            {
                xwarn("xvblockdb_t::read_index_from_db,dont find main entry for height(%" PRIu64 ")",target_height);
                return all_blocks_at_height;
            }
            //NOTE:rebind account address into xvbindex
            index_entry->reset_account_addr(account);
            
            if(index_entry->check_store_flag(base::enum_index_store_flag_main_entry) == false)
            {
                xerror("xvblockdb_t::read_index_from_db,a bad index that lost flag main_entry,index(%s)",index_entry->dump().c_str());
                index_entry->set_store_flag(base::enum_index_store_flag_main_entry); //mark as main entry again
                index_entry->set_modified_flag(); //force to write later
            }
            
            all_blocks_at_height.push_back(index_entry);//transfer owner to vector
            xdbg("xvblockdb_t::read_index_from_db,read main index(%s)",index_entry->dump().c_str());
            
            while(index_entry->get_next_viewid_offset() != 0) //check whether has other view entry
            {
                const std::string other_entry_key = base::xvdbkey_t::create_block_index_key(account,target_height,index_entry->get_next_viewid());
                index_entry = read_index_from_db(other_entry_key);
                if(index_entry == NULL)
                    break;
                //NOTE:rebind account address into xvbindex
                index_entry->reset_account_addr(account);
                all_blocks_at_height.push_back(index_entry);//transfer owner to vector
                xdbg("xvblockdb_t::read_index_from_db,read a index(%s)",index_entry->dump().c_str());
            }
            return all_blocks_at_height;
        }

        bool    xblockdb_v2_t::load_block_object(base::xvbindex_t* index_ptr)
        {
            xassert(index_ptr->get_this_block() == NULL);
            const std::string blockobj_key = base::xvdbkey_t::create_block_object_key(*index_ptr,index_ptr->get_block_hash());
            const std::string blockobj_bin = get_xdbstore()->get_value(blockobj_key);
            if(blockobj_bin.empty()) {
                xerror("xblockdb_v2_t::load_block_object,NOT stored block-object yet,index(%s) ",index_ptr->dump().c_str());
                return false;
            }
            
            base::xauto_ptr<base::xvblock_t> new_block_ptr(base::xvblock_t::create_block_object(blockobj_bin));
            if(!new_block_ptr) {
                xerror("xvblockdb_t::load_block_object,bad data at DB for key(%s)",blockobj_key.c_str());
                return false;
            }

            if(  (index_ptr->get_height()    != new_block_ptr->get_height())
                ||(index_ptr->get_viewid()    != new_block_ptr->get_viewid())
                ||(index_ptr->get_block_hash()!= new_block_ptr->get_block_hash())
                ||(index_ptr->get_account()   != new_block_ptr->get_account()) )
            {
                xerror("xvblockdb_t::read_block_object_from_db,fail as index(%s) != block(%s)",index_ptr->dump().c_str(),new_block_ptr->dump().c_str());
                return false;//invalid params
            }
                
            //sync flags to raw block if apply
            const int index_flags = index_ptr->get_block_flags() & base::enum_xvblock_flags_high4bit_mask;
            const int block_flags = new_block_ptr->get_block_flags() & base::enum_xvblock_flags_high4bit_mask;
            if(index_flags > block_flags)
                new_block_ptr->reset_block_flags(index_ptr->get_block_flags());
            else if(index_flags < block_flags)
            {
                xassert(0); //should not happen at release that dont have test blocks
            }
            // new_block_ptr->set_block_flag(base::enum_xvblock_flag_stored);//force add stored flag
            new_block_ptr->reset_modified_count();//force remove flag of modified
            
            index_ptr->reset_this_block(new_block_ptr.get(),true);//link to raw block for index
            return true;
        }

        bool    xblockdb_v2_t::load_block_input(base::xvbindex_t* index_ptr)
        {
            xassert(index_ptr->get_this_block() != NULL);
            if(index_ptr->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            
            base::xvblock_t * block_ptr = index_ptr->get_this_block();
            if(block_ptr->get_input() != NULL) //now has valid input
            {
                if(  (block_ptr->get_input()->get_resources_hash().empty() == false) //link resoure data
                   &&(block_ptr->get_input()->has_resource_data() == false) ) //but dont have resource avaiable now
                {
                    //which means resource are stored at seperatedly
                    const std::string input_resource_key = base::xvdbkey_t::create_block_input_resource_key(*index_ptr,index_ptr->get_block_hash());
                    const std::string input_resource_bin = get_xdbstore()->get_value(input_resource_key);
                    if(input_resource_bin.empty()) //that possible happen actually
                    {
                        xwarn_err("xvblockdb_t::read_block_input_from_db,fail to read resource from db for path(%s)",input_resource_key.c_str());
                        return false;
                    }
                    if(block_ptr->get_input()->has_resource_data() == false) //double check again
                    {
                        //set_input_resources is thread safe as default implementation
                        //subclass of input need guanree this promise as well
                        if(block_ptr->set_input_resources(input_resource_bin) == false)
                        {
                            xerror("xvblockdb_t::read_block_input_from_db,load bad input-resource for key(%s)",input_resource_key.c_str());
                            return false;
                        }
                    }
                    xdbg("xvblockdb_t::read_block_input_from_db,read block-input resource,block(%s) ",block_ptr->dump().c_str());
                }
            }
            return (block_ptr->get_input() != NULL);
        }

        bool    xblockdb_v2_t::load_block_output(base::xvbindex_t* index_ptr)
        {          
            xassert(index_ptr->get_this_block() != NULL);
            if(index_ptr->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            
            base::xvblock_t * block_ptr = index_ptr->get_this_block();
            xdbg("xvblockdb_t::load_block_output,target index(%s)",index_ptr->dump().c_str());
            if(block_ptr->get_output() != NULL) //now has valid output
            {
                if(  (block_ptr->get_output()->get_resources_hash().empty() == false) //link resoure data
                   &&(block_ptr->get_output()->has_resource_data() == false) ) //but dont have resource avaiable now
                {
                    //which means resource are stored at seperatedly
                    const std::string output_resource_key = base::xvdbkey_t::create_block_output_resource_key(*index_ptr,index_ptr->get_block_hash());                    
                    const std::string output_resource_bin = get_xdbstore()->get_value(output_resource_key);
                    if(output_resource_bin.empty()) //that possible happen actually
                    {
                        xwarn_err("xvblockdb_t::read_block_output_from_db,fail to read resource from db for path(%s)",output_resource_key.c_str());
                        return false;
                    }
                    if(block_ptr->get_output()->has_resource_data() == false)//double check again
                    {
                        //set_output_resources is thread safe as default implementation
                        //subclass of output need guanree this promise as well
                        if(block_ptr->set_output_resources(output_resource_bin) == false)
                        {
                            xerror("xvblockdb_t::read_block_output_from_db,read bad output-resource for key(%s)",output_resource_key.c_str());
                            return false;
                        }
                    }
                    xdbg("xvblockdb_t::read_block_output_from_db,read output resource,block(%s) ",block_ptr->dump().c_str());
                }
            }
            return (block_ptr->get_output() != NULL);
        }
    
        bool   xblockdb_v2_t::load_blocks(const base::xvaccount_t & account,const uint64_t target_height, std::vector<xobject_ptr_t<base::xvblock_t>> & blocks)
        {
            std::vector<xobject_ptr_t<base::xvbindex_t>> _indexes = read_indexs(account, target_height);
            if (_indexes.empty()) {
                xwarn("xblockdb_v2_t::load_blocks,fail-load index.account=%s,height=%ld",account.get_address().c_str(),target_height);
                return false;
            }
            for(auto & bindex : _indexes) {
                if (false == load_block_object(bindex.get())) {
                    xerror("xblockdb_v2_t::load_blocks,fail-load object.account=%s,height=%ld",account.get_address().c_str(),target_height);
                    return false;
                }
                if (false == load_block_input(bindex.get())) {
                    xerror("xblockdb_v2_t::load_blocks,fail-load input.account=%s,height=%ld",account.get_address().c_str(),target_height);
                    return false;
                }
                if (false == load_block_output(bindex.get())) {
                    xerror("xblockdb_v2_t::load_blocks,fail-load output.account=%s,height=%ld",account.get_address().c_str(),target_height);
                    return false;
                }
                xobject_ptr_t<base::xvblock_t> _blockptr;
                bindex->get_this_block()->add_ref();
                _blockptr.attach(bindex->get_this_block());
                blocks.push_back(_blockptr);
                xinfo("xblockdb_v2_t::load_blocks,succ.account=%s,height=%ld",account.get_address().c_str(),target_height);
            }
            return true;
        }
    
    }//end of namespace of base
}//end of namespace top
