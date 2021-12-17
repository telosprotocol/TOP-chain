// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xvblockdb.h"
#include "xvledger/xvdbkey.h"
#include "xblockstore/xblockstore_face.h"

#if defined(ENABLE_METRICS)
    #include "xmetrics/xmetrics.h"
#endif

namespace top
{
    namespace store
    {
        void   update_block_write_metrics(base::enum_xvblock_level _level, base::enum_xvblock_class _class, enum_blockstore_metrics_type metrics_type, size_t bin_size)
        {
        #if defined(ENABLE_METRICS)
            if (metrics_type == enum_blockstore_metrics_type_block_index)
            {
                if (_level == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_index_table_write, bin_size);
                } else if (_level == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_index_unit_write, bin_size);
                } else {
                    XMETRICS_GAUGE(metrics::store_block_index_other_write, bin_size);
                }
            }
            else if (metrics_type == enum_blockstore_metrics_type_block_object)
            {
                if (_level == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_table_write, 1);
                    if (_class == base::enum_xvblock_class_full) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_full, bin_size);
                    } else if (_class == base::enum_xvblock_class_light) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_light, bin_size);
                    } else {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_empty, bin_size);
                    }
                } else if (_level == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_unit_write, 1);
                    if (_class == base::enum_xvblock_class_full) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_full, bin_size);
                    } else if (_class == base::enum_xvblock_class_light) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_light, bin_size);
                    } else {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_empty, bin_size);
                    }
                } else {
                    XMETRICS_GAUGE(metrics::store_block_other_write, 1);
                    XMETRICS_GAUGE(metrics::store_dbsize_block_other, bin_size);
                }
            }
            else if (metrics_type == enum_blockstore_metrics_type_block_input_res)
            {
                if (_level == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_input_table_write, 1);
                    if (_class == base::enum_xvblock_class_full) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_full, bin_size);
                    } else if (_class == base::enum_xvblock_class_light) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_light, bin_size);
                    }
                } else if (_level == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_input_unit_write, 1);
                    if (_class == base::enum_xvblock_class_full) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_full, bin_size);
                    } else if (_class == base::enum_xvblock_class_light) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_light, bin_size);
                    }
                } else {
                }
            }
            else if (metrics_type == enum_blockstore_metrics_type_block_output_res)
            {
                if (_level == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_output_table_write, 1);
                    if (_class == base::enum_xvblock_class_full) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_full, bin_size);
                    } else if (_class == base::enum_xvblock_class_light) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_table_light, bin_size);
                    }
                } else if (_level == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_output_unit_write, 1);
                    if (_class == base::enum_xvblock_class_full) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_full, bin_size);
                    } else if (_class == base::enum_xvblock_class_light) {
                        XMETRICS_GAUGE(metrics::store_dbsize_block_unit_light, bin_size);
                    }
                } else {
                }
            }
        #endif
        }
    
        xvblockdb_t::xvblockdb_t(base::xvdbstore_t* xvdb_ptr)
        {
            m_blockstore_version = enum_xblockstore_version_0; //default version
            m_xvdb_ptr = xvdb_ptr;
            xassert(xvdb_ptr != NULL);
            xvdb_ptr->add_ref();
            
            const std::string version_value = xvdb_ptr->get_value(base::xvdbkey_t::get_blockstore_version_key());
            m_blockstore_version = base::xstring_utl::toint32(version_value);
            
            //XTODO,debug purpose to force to upgrade to new version
            m_blockstore_version = enum_xblockstore_prunable_version;
        }
    
        xvblockdb_t::~xvblockdb_t()
        {
            m_xvdb_ptr->release_ref();
        }
        
        bool    xvblockdb_t::load_block_input(base::xvbindex_t* target_index)
        {
            if(NULL == target_index)
            {
                xassert(0); //should not happen
                return false;
            }
            
            if(target_index->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            
            if(target_index->get_this_block() == NULL)
                read_block_object_from_db(target_index);
            
            if(target_index->get_this_block() == NULL) //check again
            {
                xerror("xvblockdb_t::load_block_input,fail to load associatd raw block(%s)",target_index->dump().c_str());
                return false;
            }
            xdbg("xvblockdb_t::load_block_input,target index(%s)",target_index->dump().c_str());
            return  read_block_input_from_db(target_index,target_index->get_this_block(),get_xdbstore());
        }
    
        bool    xvblockdb_t::load_block_input(base::xvbindex_t* target_index,base::xvblock_t * target_block)
        {
            if( (NULL == target_index) || (NULL == target_block))
            {
                xassert(0); //should not happen
                return false;
            }
            
            if(  (target_index->get_height()    != target_block->get_height())
               ||(target_index->get_viewid()    != target_block->get_viewid())
               ||(target_index->get_block_hash()!= target_block->get_block_hash())
               ||(target_index->get_account()   != target_block->get_account()) )
            {
                xerror("xvblockdb_t::load_block_input,fail as index(%s) != block(%s)",target_index->dump().c_str(),target_block->dump().c_str());
                return false;
            }
            
            if(target_index->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            
            xdbg("xvblockdb_t::load_block_input,target index(%s)",target_index->dump().c_str());
            return  read_block_input_from_db(target_index,target_block,get_xdbstore());
        }
        
        bool    xvblockdb_t::load_block_output(base::xvbindex_t* target_index)
        {
            if(NULL == target_index)
                return false;
            
            if(target_index->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            
            if(target_index->get_this_block() == NULL)
                read_block_object_from_db(target_index);
            
            if(target_index->get_this_block() == NULL) //check again
            {
                xerror("xvblockdb_t::load_block_output,fail to load associatd raw block(%s)",target_index->dump().c_str());
                return false;
            }
            xdbg("xvblockdb_t::load_block_output,target index(%s)",target_index->dump().c_str());
            return  read_block_output_from_db(target_index,target_index->get_this_block(),get_xdbstore());
        }
    
        bool    xvblockdb_t::load_block_output(base::xvbindex_t* target_index,base::xvblock_t * target_block)
        {
            if( (NULL == target_index) || (NULL == target_block))
            {
                xassert(0); //should not happen
                return false;
            }
            
            if(  (target_index->get_height()    != target_block->get_height())
               ||(target_index->get_viewid()    != target_block->get_viewid())
               ||(target_index->get_block_hash()!= target_block->get_block_hash())
               ||(target_index->get_account()   != target_block->get_account()) )
            {
                xerror("xvblockdb_t::load_block_output,fail as index(%s) != block(%s)",target_index->dump().c_str(),target_block->dump().c_str());
                return false;
            }
            
            if(target_index->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            
            xdbg("xvblockdb_t::load_block_output,target index(%s)",target_index->dump().c_str());
            return  read_block_output_from_db(target_index,target_block,get_xdbstore());
        }
    
        bool    xvblockdb_t::load_block_object(base::xvbindex_t* index_ptr, const int atag)
        {
            if(NULL == index_ptr)
                return false;
            
            xdbg("xvblockdb_t::load_block_object,target index(%s)",index_ptr->dump().c_str());
            if(index_ptr->get_this_block() != NULL)
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                #endif
                return true;
            }
            else
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                #endif
            }
            return read_block_object_from_db(index_ptr);
        }
        
        int    xvblockdb_t::save_block(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
            {
                xassert(0);
                return -1;//invalid params
            }
            return save_block(index_ptr,index_ptr->get_this_block());
        }
        
        int    xvblockdb_t::save_block(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if( (NULL == index_ptr) || (NULL == block_ptr) )
            {
                xassert(0);
                return -1;//invalid params
            }
            
            if(  (index_ptr->get_height()    != block_ptr->get_height())
               ||(index_ptr->get_viewid()    != block_ptr->get_viewid())
               ||(index_ptr->get_block_hash()!= block_ptr->get_block_hash())
               ||(index_ptr->get_account()   != block_ptr->get_account()) )
            {
                xerror("xvblockdb_t::save_block,fail as index(%s) != block(%s)",index_ptr->dump().c_str(),block_ptr->dump().c_str());
                return -1;//invalid params
            }
            
            const int object_stored_flag = write_block_object_to_db(index_ptr,block_ptr);
            if(object_stored_flag < 0)
            {
                xerror("xvblockdb_t::save_block,fail for write_block_object_to_db,index(%s) and  block(%s)",index_ptr->dump().c_str(),block_ptr->dump().c_str());
                return object_stored_flag;
            }
            
            //new version(>=1) of block may serialize seperately
            if(block_ptr->get_block_class() == base::enum_xvblock_class_nil)
            {
                return (object_stored_flag | base::enum_index_store_flag_input_resource | base::enum_index_store_flag_output_resource);
            }
            
            const int input_stored_flag  = write_block_input_to_db(index_ptr,block_ptr);
            const int output_stored_flag = write_block_output_to_db(index_ptr,block_ptr);
 
            int combined_stored_flags = object_stored_flag;
            if(input_stored_flag > 0)
                combined_stored_flags |= input_stored_flag;
            if(output_stored_flag > 0)
                combined_stored_flags |= output_stored_flag;
            
            return combined_stored_flags;//return flags to caller who need set xvbindex_t
        }

        bool    xvblockdb_t::delete_block(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;

            std::vector<std::string> deleted_key_list;
            //step#1: remove index first
            {
                if(index_ptr->check_store_flag(base::enum_index_store_flag_main_entry))
                {
                    const std::string index_key = create_block_index_key(*index_ptr,index_ptr->get_height());
                    deleted_key_list.push_back(index_key);
                }
                else
                {
                    const std::string index_key = create_block_index_key(*index_ptr,index_ptr->get_height(),index_ptr->get_viewid());
                    deleted_key_list.push_back(index_key);
                }
            }
            //step#2: remove raw block at db
            {
                const std::string block_obj_key = create_block_object_key(index_ptr);
                deleted_key_list.push_back(block_obj_key);
            }
            //delete input
            {
                // const std::string block_input_key = create_block_input_key(index_ptr);
                // deleted_key_list.push_back(block_input_key);
                
                const std::string input_resource_key = create_block_input_resource_key(index_ptr);
                deleted_key_list.push_back(input_resource_key);
            }
            //delete output
            {
                // const std::string block_output_key = create_block_output_key(index_ptr);
                // deleted_key_list.push_back(block_output_key);
                
                const std::string output_resource_key = create_block_output_resource_key(index_ptr);
                deleted_key_list.push_back(output_resource_key);
            }
            
            return get_xdbstore()->delete_values(deleted_key_list);
        }

        //return bool indicated whether has anything writed into db
        bool   xvblockdb_t::write_index_to_db(base::xvbindex_t* index_obj)
        {
            if(NULL == index_obj)
            {
                xassert(index_obj != NULL);
                return false;
            }

            bool exist_modified_flag = index_obj->check_modified_flag();
            index_obj->reset_modify_flag(); //clear the flag of modification before save
            
            std::string index_bin;
            if(index_obj->serialize_to(index_bin) <= 0)
            {
                if(exist_modified_flag)
                    index_obj->set_modified_flag();//restore flag as fail
                
                xerror("xvblockdb_t::write_index_to_db,fail to serialize_to,index'dump(%s)",index_obj->dump().c_str());
                return false;
            }
                       
            bool is_stored_db_successful = false;
            if(index_obj->check_store_flag(base::enum_index_store_flag_main_entry)) //main index for this height
            {
                const std::string key_path = create_block_index_key(*index_obj,index_obj->get_height());
                is_stored_db_successful = get_xdbstore()->set_value(key_path,index_bin);
                xdbg("xvblockdb_t::write_index_to_db for main entry.index=%s",index_obj->dump().c_str());
            }
            else
            {
                const std::string key_path = create_block_index_key(*index_obj,index_obj->get_height(),index_obj->get_viewid());
                is_stored_db_successful = get_xdbstore()->set_value(key_path,index_bin);
                xdbg("xvblockdb_t::write_index_to_db for other entry.index=%s",index_obj->dump().c_str());
            }
            
            update_block_write_metrics(index_obj->get_block_level(), index_obj->get_block_class(), enum_blockstore_metrics_type_block_index, index_bin.size());
            
            if(false == is_stored_db_successful)
            {
                if(exist_modified_flag)
                    index_obj->set_modified_flag();//restore flag as fail
                
                xerror("xvblockdb_t::write_index_to_db,fail to writed into db,index dump(%s)",index_obj->dump().c_str());
                return false;
            }
            return true;
        }
        
        //return map sorted by viewid from lower to high,caller respond to release ptr later
        std::vector<base::xvbindex_t*>   xvblockdb_t::read_index_from_db(const base::xvaccount_t & account,const uint64_t target_height)
        {
            std::vector<base::xvbindex_t*> all_blocks_at_height;
            
            const std::string main_entry_key = create_block_index_key(account,target_height);
            base::xvbindex_t* index_entry = read_index_from_db(main_entry_key);
            if(index_entry == NULL) //main entry
            {
                xwarn("xvblockdb_t::read_index_from_db,dont find main entry for account=%s,height(%" PRIu64 ")",account.get_address().c_str(), target_height);
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
                const std::string other_entry_key = create_block_index_key(account,target_height,index_entry->get_next_viewid());
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
        
        base::xvbindex_t*   xvblockdb_t::read_index_from_db(const std::string & index_db_key_path)
        {
            #if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::store_block_index_read, 1);
            #endif
            const std::string index_bin = get_xdbstore()->get_value(index_db_key_path);
            if(index_bin.empty())
            {
                xdbg("xvblockdb_t::read_index_from_db,fail to read from db for path(%s)",index_db_key_path.c_str());
                return NULL;
            }
            
            base::xvbindex_t * new_index_obj = new base::xvbindex_t();
            if(new_index_obj->serialize_from(index_bin) <= 0)
            {
                xerror("xvblockdb_t::read_index_from_db,fail to serialize from db for path(%s)",index_db_key_path.c_str());
                new_index_obj->release_ref();
                return NULL;
            }
            
            if(new_index_obj->check_modified_flag())
            {
                xerror("xvblockdb_t::read_index_from_db,dirty index from db for path(%s)",index_db_key_path.c_str());
                new_index_obj->reset_modify_flag(); //should not happen,but add exception for incase
            }
            return new_index_obj;
        }
    
        int    xvblockdb_t::write_block_object_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return -1;//invalid params
            
            const int stored_flags = base::enum_index_store_flag_input_entity | base::enum_index_store_flag_output_entity | base::enum_index_store_flag_mini_block;
            //raw block not stored header yet
            if(  (index_ptr->check_store_flag(base::enum_index_store_flag_mini_block) == false)
               ||(block_ptr->get_modified_count() > 0) )
            {
                std::string blockobj_bin;
                block_ptr->serialize_to_string(blockobj_bin);
                const std::string blockobj_key = create_block_object_key(index_ptr);
                if(get_xdbstore()->set_value(blockobj_key, blockobj_bin))
                {
                    update_block_write_metrics(block_ptr->get_block_level(), block_ptr->get_block_class(), enum_blockstore_metrics_type_block_object, blockobj_bin.size());
                    
                    xinfo("xvblockdb_t::write_block_object_to_db,stored DB at key(%s) for block(%s) and index_ptr(%s)",blockobj_key.c_str(),block_ptr->dump().c_str(), index_ptr->dump().c_str());
                    
                    block_ptr->reset_modified_count();//cleanup flag of modification
                    //has stored entity of input/output inside of block
                    return stored_flags;
                }
                else
                {
                    xerror("xvblockdb_t::write_block_object_to_db,fail to store header for block(%s),index(%s)",block_ptr->dump().c_str(),index_ptr->dump().c_str());
                    return -2; //failed
                }
            }
            return stored_flags;
        }
        
        bool    xvblockdb_t::read_block_object_from_db(base::xvbindex_t* index_ptr)
        {
            return read_block_object_from_db(index_ptr,get_xdbstore());
        }
        
        bool    xvblockdb_t::read_block_object_from_db(base::xvbindex_t* index_ptr,base::xvdbstore_t* from_db)
        {
            if(index_ptr->get_this_block() == NULL)
            {
                #if defined(ENABLE_METRICS)
                if (index_ptr->get_block_level() == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_table_read, 1);
                } else if (index_ptr->get_block_level() == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_unit_read, 1);
                } else {
                    XMETRICS_GAUGE(metrics::store_block_other_read, 1);
                }
                #endif
                
                const std::string blockobj_key = create_block_object_key(index_ptr);
                const std::string blockobj_bin = from_db->get_value(blockobj_key);
                if(blockobj_bin.empty())
                {
                    if(index_ptr->check_store_flag(base::enum_index_store_flag_mini_block)) //has stored header and cert
                        xerror("xvblockdb_t::read_block_object_from_db,fail to find item at DB for key(%s)",blockobj_key.c_str());
                    else
                        xwarn("xvblockdb_t::read_block_object_from_db,NOT stored block-object yet,index(%s) ",index_ptr->dump().c_str());
                    
                    return false;
                }
                
                base::xauto_ptr<base::xvblock_t> new_block_ptr(base::xvblock_t::create_block_object(blockobj_bin));
                if(!new_block_ptr)
                {
                    xerror("xvblockdb_t::read_block_object_from_db,bad data at DB for key(%s)",blockobj_key.c_str());
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
                    #ifndef DEBUG
                    xassert(0); //should not happen at release that dont have test blocks
                    #endif
                }
                
                new_block_ptr->set_block_flag(base::enum_xvblock_flag_stored);//force add stored flag
                new_block_ptr->reset_modified_count();//force remove flag of modified
                
                if(index_ptr->get_this_block() == NULL)//double check again
                    index_ptr->reset_this_block(new_block_ptr.get(),true);//link to raw block for index
            }
            return (index_ptr->get_this_block() != NULL);
        }
        
        int    xvblockdb_t::write_block_input_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return -1; //invalid params
            
            if(block_ptr->get_input() == NULL)
            {
                xassert(0);
                return -1; //invalid params
            }
            
            if(index_ptr->check_store_flag(base::enum_index_store_flag_input_resource) == false)
            {
                if(block_ptr->get_input()->get_resources_hash().empty() == false)
                {
                    const std::string input_res_bin = block_ptr->get_input()->get_resources_data();
                    if(input_res_bin.empty() == false)
                    {
                        update_block_write_metrics(block_ptr->get_block_level(), block_ptr->get_block_class(), enum_blockstore_metrics_type_block_input_res, input_res_bin.size());
                        
                        const std::string input_res_key = create_block_input_resource_key(index_ptr);
                        if(get_xdbstore()->set_value(input_res_key, input_res_bin))
                        {
                            xdbg("xvblockdb_t::write_block_input_to_db,store input resource to DB for block(%s),bin_size=%zu",index_ptr->dump().c_str(), input_res_bin.size());
                            return base::enum_index_store_flag_input_resource;
                        }
                        else
                        {
                            xerror("xvblockdb_t::write_block_input_to_db,fail to store input resource for block(%s)",index_ptr->dump().c_str());
                            return -3; //failed
                        }
                    }
                    else //fail to found resource data for input of block
                    {
                        xerror("xvblockdb_t::write_block_input_to_db,fail to found input resource for block(%s)",index_ptr->dump().c_str());
                        return -2; //failed
                    }
                }
            }
            return base::enum_index_store_flag_input_resource; //nothing changed
        }
 
        bool    xvblockdb_t::read_block_input_from_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr,base::xvdbstore_t* from_db)
        {
            if(block_ptr == NULL)
                return false;
            
            if(block_ptr->get_input() != NULL) //now has valid input
            {
                if(  (block_ptr->get_input()->get_resources_hash().empty() == false) //link resoure data
                   &&(block_ptr->get_input()->has_resource_data() == false) ) //but dont have resource avaiable now
                {
                    #if defined(ENABLE_METRICS)
                    XMETRICS_GAUGE(metrics::store_block_input_read, 1);
                    #endif
                    //which means resource are stored at seperatedly
                    const std::string input_resource_key = create_block_input_resource_key(index_ptr);
                    
                    const std::string input_resource_bin = from_db->get_value(input_resource_key);
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
        
        int    xvblockdb_t::write_block_output_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return -1; //invalid params
            
            if(block_ptr->get_output() == NULL)
            {
                xassert(0);
                return -1; //invalid params
            }
            
            if(index_ptr->check_store_flag(base::enum_index_store_flag_output_resource) == false)
            {
                if(block_ptr->get_output()->get_resources_hash().empty() == false)
                {
                    const std::string output_res_bin = block_ptr->get_output()->get_resources_data();
                    if(output_res_bin.empty() == false)
                    {
                        const std::string output_res_key = create_block_output_resource_key(index_ptr);
                        if(get_xdbstore()->set_value(output_res_key, output_res_bin))
                        {
                            update_block_write_metrics(block_ptr->get_block_level(), block_ptr->get_block_class(), enum_blockstore_metrics_type_block_output_res, output_res_bin.size());

                            xdbg("xvblockdb_t::write_block_output_to_db,store output resource to DB for block(%s),bin_size=%zu",index_ptr->dump().c_str(), output_res_bin.size());
                            return base::enum_index_store_flag_output_resource;
                        }
                        else
                        {
                            xerror("xvblockdb_t::write_block_output_to_db,fail to store output resource for block(%s)",index_ptr->dump().c_str());
                            return -3; //failed
                        }
                    }
                    else
                    {
                        xerror("xvblockdb_t::write_block_output_to_db,fail to find output resource for block(%s)",index_ptr->dump().c_str());
                        return -2; //failed
                    }
                }
            }
            return base::enum_index_store_flag_output_resource;
        }
        
        bool    xvblockdb_t::read_block_output_from_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr,base::xvdbstore_t* from_db)
        {
            if(NULL == block_ptr)
                return false;
            
            if(block_ptr->get_output() != NULL) //now has valid output
            {
                if(  (block_ptr->get_output()->get_resources_hash().empty() == false) //link resoure data
                   &&(block_ptr->get_output()->has_resource_data() == false) ) //but dont have resource avaiable now
                {
                    #if defined(ENABLE_METRICS)
                    XMETRICS_GAUGE(metrics::store_block_output_read, 1);
                    #endif
                    //which means resource are stored at seperatedly
                    const std::string output_resource_key = create_block_output_resource_key(index_ptr);
                    
                    const std::string output_resource_bin = from_db->get_value(output_resource_key);
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
        
        //note: caller need release block ptr
        std::vector<base::xvblock_t*>  xvblockdb_t::read_prunable_block_object_from_db(base::xvaccount_t & account,const uint64_t target_height)
        {
            std::vector<base::xvblock_t*> all_blocks_at_height;
            
            base::xvblock_t* main_entry_block_ptr = NULL;
            //step#1: try load committed block directly (hit most case)
            const std::string target_block_key = base::xvdbkey_t::create_prunable_block_object_key(account,target_height);
            const std::string target_block_bin = get_xdbstore()->get_value(target_block_key);
            if(target_block_bin.empty() == false)
            {
                main_entry_block_ptr = base::xvblock_t::create_block_object(target_block_bin);
                xassert(main_entry_block_ptr != NULL);
                if(main_entry_block_ptr)
                {
                    all_blocks_at_height.push_back(main_entry_block_ptr);//transfer ownership to all_blocks_at_height
                    if(main_entry_block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        xinfo("read_prunable_block_object_from_db,loaded commit block(%s) ",main_entry_block_ptr->dump().c_str());
                        return all_blocks_at_height;//found committed block
                    }
                    else
                    {
                        xerror("read_prunable_block_object_from_db,non-commit at mainentry of account(%s) for block(%s) ",account.get_address().c_str(),main_entry_block_ptr->dump().c_str());
                    }
                }
                else
                {
                    xerror("read_prunable_block_object_from_db,bad main block of account(%s) at height(%lld) ",account.get_address().c_str(),target_height);
                }
            }
            else
            {
                xinfo("read_prunable_block_object_from_db,NOT found main block of account(%s) at height(%lld) ",account.get_address().c_str(),target_height);
                return all_blocks_at_height;
            }
            
            //step#2: try load other blocks at same height
            std::vector<std::string> sub_blocks_bin;
            const std::string sub_block_prefix = target_block_key + "/";
            get_xdbstore()->read_range(sub_block_prefix, sub_blocks_bin);
            for (std::string & block_value : sub_blocks_bin)
            {
                base::xvblock_t* new_block_ptr = base::xvblock_t::create_block_object(block_value);
                xassert(new_block_ptr != NULL);
                if(new_block_ptr)
                {
                    all_blocks_at_height.push_back(new_block_ptr);//transfer ownership to all_blocks_at_height
                    if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                        xinfo("read_prunable_block_object_from_db,second-entry loaded commit block(%s) ",new_block_ptr->dump().c_str());
                    else
                        xinfo("read_prunable_block_object_from_db,second-entry loaded non-commit block(%s) ",new_block_ptr->dump().c_str());
                        
                    //read until committed block
                    if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                        break;
                }
            }
                            
            xinfo("read_prunable_block_object_from_db,loaded %d blocks of account(%s) at height(%lld) ",(int)all_blocks_at_height.size(),account.get_address().c_str(),target_height);
            return all_blocks_at_height;//caller need release block ptr
        }
        
        //compatible for old version,e.g read meta and other stuff
        const std::string   xvblockdb_t::load_value_by_path(const std::string & full_path_as_key)
        {
            return get_xdbstore()->get_value(full_path_as_key);
        }
        bool                xvblockdb_t::delete_value_by_path(const std::string & full_path_as_key)
        {
            return get_xdbstore()->delete_value(full_path_as_key);
        }
        bool                xvblockdb_t::store_value_by_path(const std::string & full_path_as_key,const std::string & value)
        {
            if(value.empty())
                return true;
            
            return get_xdbstore()->set_value(full_path_as_key,value);
        }
    
        const std::string  xvblockdb_t::create_block_index_key(const base::xvaccount_t & account,const uint64_t target_height)
        {
            return base::xvdbkey_t::create_prunable_block_index_key(account,target_height);
        }
    
        const std::string  xvblockdb_t::create_block_index_key(const base::xvaccount_t & account,const uint64_t target_height,const uint64_t target_viewid)
        {
            return base::xvdbkey_t::create_prunable_block_index_key(account,target_height,target_viewid);
        }
    
        const std::string  xvblockdb_t::create_block_index_key(base::xvbindex_t * index_ptr,const uint64_t target_height,const uint64_t target_viewid)
        {
            return base::xvdbkey_t::create_prunable_block_index_key(*index_ptr,target_height,target_viewid);
        }

        const std::string  xvblockdb_t::create_block_object_key(base::xvbindex_t * index_ptr)
        {
            if(index_ptr->check_store_flag(base::enum_index_store_flag_non_index))//just store raw-block
            {
                if( index_ptr->check_block_flag(base::enum_xvblock_flag_committed)) //main-entry block
                {
                    return base::xvdbkey_t::create_prunable_block_object_key(*index_ptr,index_ptr->get_height());
                }
                else
                {
                    return base::xvdbkey_t::create_prunable_block_object_key(*index_ptr,index_ptr->get_height(),index_ptr->get_viewid());
                }
            }
            else //the persisted index always point to fixed postion to store raw block
            {
                return base::xvdbkey_t::create_prunable_block_object_key(*index_ptr,index_ptr->get_height(),index_ptr->get_viewid());
            }
        }
    
        const std::string  xvblockdb_t::create_block_input_key(base::xvbindex_t * index_ptr)
        {
            return base::xvdbkey_t::create_prunable_block_input_key(*index_ptr, index_ptr->get_height(), index_ptr->get_viewid());
        }
          
        const std::string  xvblockdb_t::create_block_input_resource_key(base::xvbindex_t * index_ptr)
        {
            return base::xvdbkey_t::create_prunable_block_input_resource_key(*index_ptr,index_ptr->get_height(), index_ptr->get_viewid());
        }
        
        const std::string  xvblockdb_t::create_block_output_key(base::xvbindex_t * index_ptr)
        {
            return base::xvdbkey_t::create_prunable_block_output_key(*index_ptr,index_ptr->get_height(), index_ptr->get_viewid());
        }
    
        const std::string  xvblockdb_t::create_block_output_resource_key(base::xvbindex_t * index_ptr)
        {
            return base::xvdbkey_t::create_prunable_block_output_resource_key(*index_ptr,index_ptr->get_height(), index_ptr->get_viewid());
        }

    };//end of namespace of vstore
};//end of namespace of top
