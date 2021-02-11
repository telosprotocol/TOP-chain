// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xvblock.h"
#include "xbase/xvledger.h"
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
            int32_t   serialize_to_string(std::string & bin_data);
            int32_t   serialize_from_string(const std::string & bin_data);

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
            uint64_t  _highest_connect_block_height; //indicated the last block who is connected all the way to genesis block or last full-block
            std::string _highest_connect_block_hash;
            std::string _highest_execute_block_hash;
        };

        //each account has own virtual store
        class xblockacct_t : public base::xobject_t,public base::xvaccount_t
        {
        protected:
            enum
            {
                enum_max_cached_blocks = 32,
            };
        public:
            static bool         init_account(base::xcontext_t &_context);//do initialize for all accounts
            static std::string  get_meta_path(base::xvaccount_t & _account);
        public:
            xblockacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore);
        protected:
            virtual ~xblockacct_t();
        private:
            xblockacct_t();
            xblockacct_t(const xblockacct_t &);
            xblockacct_t & operator = (const xblockacct_t &);

        public://indicated the last block who is connected allway to genesis block
            virtual bool      init();
            inline const   uint64_t         get_last_access_time() const {return m_last_access_time_ms;} //UTC ms
            inline const   uint64_t         get_idle_duration()    const {return m_idle_timeout_ms;}
            void    set_last_access_time(const uint64_t last_access_time);

            bool                is_live(const uint64_t timenow_ms); //test whether has been idel status
            bool                save_meta();
            virtual bool        close(bool force_async = true) override;
            virtual std::string dump() const override;  //just for debug purpose

            //clean unsed caches of account to recall memory
            bool                clean_caches();
            //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
            bool                reset_cache_timeout(const uint32_t max_idle_time_ms);

        public://return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
            base::xvblock_t*        get_genesis_block();            //genesis block
            base::xvblock_t*        get_latest_cert_block();             //highest height/view# for any status
            base::xvblock_t*        get_latest_locked_block();      //block with locked status
            base::xvblock_t*        get_latest_committed_block();   //block with committed status
            base::xvblock_t*        get_latest_executed_block();    //block with executed status
            base::xvblock_t*        get_latest_connected_block();   //block has connected to genesis
            base::xvblock_t*        get_latest_full_block();        //block has full state,genesis is a full block
            base::xvblock_t*        get_latest_current_block(bool ask_full_load = false);     //block has connected to cert/lock/commit block
            //one api to get latest_commit/latest_lock/latest_cert for better performance
            bool                    get_latest_blocks_list(base::xvblock_t* & cert_block,base::xvblock_t* & lock_block,base::xvblock_t* & commit_block);

            base::xvblock_t*        get_block(const uint64_t height, const uint64_t viewid);
            base::xvblock_t*        get_block(const uint64_t height, const std::string & blockhash);
            base::xblock_vector     get_blocks(const uint64_t height);

        public://return raw ptr with added reference,caller respond to release it after that

            //just load vblock object but not load header and body those need load seperately if need. create a new one if not found
            virtual base::xvblock_t*        load_block_object(const uint64_t height,bool ask_full_load);  //load from db/store ,it must be a lock/commit block
            virtual bool                    load_block_input(base::xvblock_t* block);  //load and assign input data into  xvblock_t
            virtual bool                    load_block_output(base::xvblock_t* block); //load and assign output data into xvblock_t

            virtual bool                    store_block(base::xvblock_t* block); //update old one or insert as new
            virtual bool                    delete_block(base::xvblock_t* block);//return error code indicate what is result
            virtual bool                    delete_block(uint64_t height);//return error code indicate what is result

            virtual bool                    store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks); //better performance

            virtual bool                    execute_block(base::xvblock_t* block); //execute block and update state of acccount
        protected:
            virtual bool                    store_block_by_path(const std::string & block_path, base::xvblock_t* block_ptr);
            virtual base::xvblock_t*        load_block_by_path(const std::string & block_base_path,bool ask_full_load = true);//caller response to release return ptr

            virtual const std::string       load_value_by_path(const std::string & full_path_as_key);
            virtual bool                    delete_value_by_path(const std::string & full_path_as_key);
            virtual bool                    store_value_by_path(const std::string & full_path_as_key,const std::string & value);
            inline const std::string &      get_blockstore_path()   const {return m_blockstore_path;}

        protected:
            inline xstore_face_t*           get_store() { return m_persist_db; }
            base::xvblockstore_t*           get_blockstore() { return m_blockstore; }
            bool              save_to_xdb(base::xvblock_t* this_block);
            bool              save_block(base::xvblock_t* this_block); //save block to persisted storage
            //to connect prev block, load_block may call load_block again to get prev-block, reenter_allow_count decide how many times can reenter
            base::xvblock_t*  load_block(const uint64_t height,int reenter_allow_count);       //load block from persisted storage
            virtual bool      clean_blocks(const size_t keep_blocks_count);  //not allow cache too much blocks

            //query block from cached map,return raw ptr without adding reference
            base::xvblock_t*  query_block(const uint64_t target_height,base::enum_xvblock_flag request_flag);
            base::xvblock_t*  query_latest_block(base::enum_xvblock_flag request_flag);

            //there might be mutiple candidates(blocks) at same height,so clean unqualified ones
            bool              clean_candidates(base::xvblock_t* compare_by_block);
            bool              clean_candidates(std::map<uint64_t,base::xvblock_t*> & view_map,const int filter_by_block_flags);

            //connect this_block to prev_block and next_block
            //connect_blockmay call load_block -->call connect again to get prev-block, reenter_allow_count decide how many times can reenter
            bool              connect_block(base::xvblock_t* this_block,int reenter_allow_count);
            virtual bool      connect_block(base::xvblock_t* this_block,const std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> >::iterator & this_block_height_it,int reload_allow_count);

            bool              unload_subblock_from_container(base::xvblock_t* container_block);
            void              check_meta();
        protected: //event handle
            virtual bool      on_block_changed(base::xvblock_t* _block);            //stage change from cert->lock->commit->executed ...
            virtual bool      on_block_stored_to_xdb(base::xvblock_t* _block);      //persist stored
            virtual bool      on_block_miss_at_xdb(const uint64_t target_height);   //fail to load block at xdb
            virtual bool      on_block_removing_from_cache(base::xvblock_t* _block);//remove from cache

            void              notify_commit_store_event(base::xvblock_t* this_block);
        private:
            void              close_blocks(); //clean all cached blocks
            bool              is_replace_existing_block(base::xvblock_t* existing_block, base::xvblock_t* this_block);

        private:
            uint64_t        m_last_access_time_ms; //UTC ms
            uint64_t        m_idle_timeout_ms;     //how long(ms) it will change to idle status
            std::string     m_blockstore_path;
            xstore_face_t*  m_persist_db;
            base::xvblockstore_t* m_blockstore;
        protected:
            xacctmeta_t *   m_meta;
            std::string     m_last_save_vmeta_bin;
            std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> > m_all_blocks;  // < height#, <view#,block*> > sort from lower to higher
        };

        //xchainacct_t transfer block status from lower stage to higher : from cert ->lock->commit
        class xchainacct_t : public xblockacct_t
        {
        public:
            xchainacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore);
        protected:
            virtual ~xchainacct_t();
        private:
            xchainacct_t();
            xchainacct_t(const xchainacct_t &);
            xchainacct_t & operator = (const xchainacct_t &);

        private:
            bool            process_block(base::xvblock_t* this_block);
            virtual bool    connect_block(base::xvblock_t* this_block,const std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> >::iterator & this_block_height_it,int reenter_allow_count) override;
        private:
            uint64_t  _lowest_commit_block_height;  //clean committed blocks first
        };

    };//end of namespace of vstore
};//end of namespace of top
