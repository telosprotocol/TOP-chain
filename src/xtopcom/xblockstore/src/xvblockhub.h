// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#include "xvledger/xvbindex.h"
#include "xvledger/xvledger.h"
#include "xblockstore/xblockstore_face.h"

namespace top
{
    namespace store
    {
        class xacctmeta_t : public base::xdataobj_t
        {
        public:
            enum {enum_obj_type = base::xdataunit_t::enum_xdata_type_vaccountmeta};
            xacctmeta_t();
        protected:
            virtual ~xacctmeta_t();
        private:
            xacctmeta_t(const xacctmeta_t &);
            xacctmeta_t & operator = (const xacctmeta_t &);
        public:
            virtual int32_t   serialize_to_string(std::string & bin_data) override;
            virtual int32_t   serialize_from_string(const std::string & bin_data) override;

            virtual std::string  dump() const override;
        private:
            //not safe for multiple threads
            virtual int32_t      do_write(base::xstream_t & stream) override; //serialize whole object to binary
            virtual int32_t      do_read(base::xstream_t & stream) override; //serialize from binary and regeneate content

            //caller respond to cast (void*) to related  interface ptr
            virtual void*        query_interface(const int32_t _enum_xobject_type_) override;
        public:
            static xacctmeta_t* load(const std::string & meta_serialized_data);
        public:
            uint64_t  _highest_cert_block_height;    //latest certificated block but not changed to lock/commit status
            uint64_t  _highest_lock_block_height;    //latest locked block that not allow fork
            uint64_t  _highest_commit_block_height;  //latest commited block to allow change state of account,like balance.
            uint64_t  _highest_execute_block_height; //latest executed block that has executed and change state of account
            uint64_t  _highest_full_block_height;    //latest full-block height for this account
            uint64_t  _highest_connect_block_height; //indicated the last block who is connected all the way to last full-block
            std::string _highest_connect_block_hash;
            std::string _highest_execute_block_hash;
            //reserved _lowest_genesis_connect_height to trune block
            uint64_t  _lowest_genesis_connect_height;  //[_lowest_genesis_connect_height,_highest_genesis_connect_height]
            uint64_t  _highest_genesis_connect_height;//indicated the last block who is connected to genesis block
            std::string _highest_genesis_connect_hash;
            uint64_t _highest_sync_height;          // higest continous block started from highest full table block
        private:
             uint8_t   _meta_spec_version; //add version control for compatible case
        };

        //each account has own virtual store
        class xblockacct_t : public base::xobject_t,public base::xvaccount_t
        {
        protected:
            enum{enum_max_cached_blocks = 32};
        public:
            static std::string  get_meta_path(base::xvaccount_t & _account);
        public:
            xblockacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path);
        protected:
            virtual ~xblockacct_t();
        private:
            xblockacct_t();
            xblockacct_t(xblockacct_t &&);
            xblockacct_t(const xblockacct_t &);
            xblockacct_t & operator = (const xblockacct_t &);

        public://indicated the last block who is connected allway to genesis block
            virtual bool           init();
            virtual bool           close(bool force_async = true) override;
            virtual bool           is_live(const uint64_t timenow_ms) override;//test whether has been idel status
            virtual std::string    dump() const override;  //just for debug purpose
            bool                   clean_caches(bool clean_all); //clean unsed caches of account to recall memory
            bool                   save_meta();
            
            inline const uint64_t  get_idle_duration()    const {return m_idle_timeout_ms;}
            inline const uint64_t  get_last_access_time() const {return m_last_access_time_ms;} //UTC ms
            void                   set_last_access_time(const uint64_t last_access_time);
            //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
            bool                   reset_cache_timeout(const uint32_t max_idle_time_ms);
            const int              get_cache_size();
            
            inline const std::string &   get_blockstore_path()   const {return m_blockstore_path;};
            
        public://just search at cache layer
            std::vector<base::xvbindex_t*>  query_index(const uint64_t height);
            base::xvbindex_t*      query_index(const uint64_t height, const uint64_t viewid);
            base::xvbindex_t*      query_index(const uint64_t height, const std::string & blockhash);
            base::xvbindex_t*      query_index(const uint64_t target_height,base::enum_xvblock_flag request_flag);

            base::xvbindex_t*      query_latest_index(base::enum_xvblock_flag request_flag);
            base::xvbindex_t*      query_latest_index(base::enum_xvblock_class request_class);
            
            //one api to get latest_commit/latest_lock/latest_cert for better performance
            bool                   query_latest_index_list(base::xvbindex_t* & cert_block,base::xvbindex_t* & lock_block,base::xvbindex_t* & commit_block);
            
        public: //query cache first then try load from db
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
            base::xvbindex_t*      load_genesis_index();            //genesis block
            base::xvbindex_t*      load_latest_cert_index();        //highest height/view# for any status
            base::xvbindex_t*      load_latest_locked_index();      //block with locked status
            base::xvbindex_t*      load_latest_committed_index();   //block with committed status
            base::xvbindex_t*      load_latest_executed_index();    //block with executed status
            base::xvbindex_t*      load_latest_connected_index();   //block has connected to the last full
            base::xvbindex_t*      load_latest_genesis_connected_index();  //block has connected to genesis;
            base::xvbindex_t*      load_latest_full_index();        //block has full state,genesis is a full block
            base::xvbindex_t*      load_latest_committed_full_index();  // full block with committed status
 
            bool                   load_latest_index_list(base::xvbindex_t* & cert_block,base::xvbindex_t* & lock_block,base::xvbindex_t* & commit_block);     //latest commit/lock/cert
            
            int                    load_index(const uint64_t target_height);//return how many index at height
            base::xvbindex_t*      load_index(const uint64_t target_height,const uint64_t view_id);
            base::xvbindex_t*      load_index(const uint64_t target_height,const std::string & block_hash);
            base::xvbindex_t*      load_index(const uint64_t target_height,base::enum_xvblock_flag request_flag);
            bool                   load_block_object(base::xvbindex_t* index_ptr);
            bool                   load_index_input(base::xvbindex_t* target_block);
            bool                   load_index_output(base::xvbindex_t* target_block);
            bool                   load_index_offdata(base::xvbindex_t* target_block);
            
        public://operated for raw block
            bool                   store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks); //better performance
            bool                   store_block(base::xvblock_t* new_raw_block);
            bool                   delete_block(base::xvblock_t* target_block);//return error code indicate what is result
            bool                   delete_block(const uint64_t target_height);//return error code indicate what is result
            bool                   load_block_input(base::xvblock_t* target_block);
            bool                   load_block_output(base::xvblock_t* target_block);
            bool                   load_block_offdata(base::xvblock_t* target_block);
            
            bool                   execute_block(base::xvblock_t* block_ptr); //execute block and update state of acccount
            bool                   execute_block(base::xvbindex_t* index_ptr);//usall internal useonly
            void                   try_execute_all_block();
            
        protected: //help functions
            bool                cache_index(base::xvbindex_t* this_block);
            bool                link_neighbor(base::xvbindex_t* this_block);//just connect prev and next index of list
            bool                mark_connected_flag(base::xvbindex_t* this_block);
            bool                update_meta_metric(base::xvbindex_t* new_block_ptr );
            
            bool                write_block_to_db(base::xvbindex_t* index_ptr);
            bool                write_block_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * linked_block_ptr);
            
            bool                write_block_object_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
            bool                read_block_object_from_db(base::xvbindex_t* index_ptr);
            
            bool                write_block_input_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
            bool                read_block_input_from_db(base::xvbindex_t* index_ptr);
            bool                read_block_input_from_db(base::xvblock_t* block_ptr);
            
            bool                write_block_output_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
            bool                read_block_output_from_db(base::xvbindex_t* index_ptr);
            bool                read_block_output_from_db(base::xvblock_t * block_ptr);
            
            //manage data related xvboffdata_t
            bool                write_block_offdata_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr);
            bool                read_block_offdata_from_db(base::xvblock_t * block_ptr);
            
            bool                write_index_to_db(const uint64_t target_height);
            bool                write_index_to_db(std::map<uint64_t,base::xvbindex_t*> & indexes);
            bool                write_index_to_db(base::xvbindex_t* index_obj,bool allo_db_event);
            base::xvbindex_t*   read_index_from_db(const std::string & index_db_key_path);
            //return map sorted by viewid from lower to high,caller respond to release ptr later
            std::vector<base::xvbindex_t*> read_index_from_db(const uint64_t target_height);
      
            //connect this_block to prev_block and next_block
            //connect_blockmay call load_block -->call connect again to get prev-block, reenter_allow_count decide how many times can reenter
            virtual bool        process_index(base::xvbindex_t* this_block){return false;}
            virtual bool        connect_index(base::xvbindex_t* this_block){return false;}
       
        private:
            void                close_blocks(); //clean all cached blocks
            bool                clean_blocks(const int keep_blocks_count,bool force_release_unused_block);
            bool                on_block_stored(base::xvbindex_t* index_ptr);
            bool                store_txs_to_db(base::xvbindex_t* index_ptr);
            
        protected: //compatible for old version,e.g read meta and other stuff
            const std::string   load_value_by_path(const std::string & full_path_as_key);
            bool                store_value_by_path(const std::string & full_path_as_key,const std::string & value);
            bool                delete_value_by_path(const std::string & full_path_as_key);
           
        private:
            uint64_t        m_last_access_time_ms; //UTC ms
            uint64_t        m_idle_timeout_ms;     //how long(ms) it will change to idle status
            std::string     m_blockstore_path;
            std::string     m_last_save_vmeta_bin;
        protected:
            xacctmeta_t *   m_meta;
            std::map<uint64_t,std::map<uint64_t,base::xvbindex_t*> > m_all_blocks;  // < height#, <view#,block*> > sort from lower to higher
        };

        //xchainacct_t transfer block status from lower stage to higher : from cert ->lock->commit
        class xchainacct_t : public xblockacct_t
        {
        public:
            xchainacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path);
        protected:
            virtual ~xchainacct_t();
        private:
            xchainacct_t();
            xchainacct_t(const xchainacct_t &);
            xchainacct_t & operator = (const xchainacct_t &);

        private:
            virtual bool    process_index(base::xvbindex_t* this_block) override;
            virtual bool    connect_index(base::xvbindex_t* this_block) override;
        private:
            uint64_t  _lowest_commit_block_height;  //clean committed blocks first
        };

    };//end of namespace of vstore
};//end of namespace of top
