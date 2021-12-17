// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>
#include <map>
#include "xvblockdb.h"
#include "xbkstoreutl.h"

namespace top
{
    namespace store
    {
        //each account has own virtual store
        class xblockacct_t : public xvblockplugin_t
        {
        protected:
            enum{enum_max_cached_blocks = 32};
        public:
            uint64_t            get_latest_cert_block_height() const {return m_meta->_highest_cert_block_height;}

            uint64_t            get_latest_committed_block_height() const { return m_meta->_highest_commit_block_height; }
            uint64_t            get_latest_connected_block_height() const { return m_meta->_highest_connect_block_height; }
        public:
            xblockacct_t(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr);
        protected:
            virtual ~xblockacct_t();
        private:
            xblockacct_t();
            xblockacct_t(xblockacct_t &&);
            xblockacct_t(const xblockacct_t &);
            xblockacct_t & operator = (const xblockacct_t &);

        public://indicated the last block who is connected allway to genesis block
            virtual bool           close(bool force_async = true) override;
            virtual std::string    dump() const override;  //just for debug purpose
            
            virtual base::xvbindex_t*              create_index(base::xvblock_t & new_raw_block) = 0;
            virtual std::vector<base::xvbindex_t*> read_index(const uint64_t target_height) = 0;
            virtual bool           write_index(base::xvbindex_t* this_index) = 0;
            
            virtual bool           write_block(base::xvbindex_t* index_ptr) = 0;
            virtual bool           write_block(base::xvbindex_t* index_ptr,base::xvblock_t * new_block_ptr) = 0;
            
            virtual bool           store_block(base::xvblock_t* new_raw_block);
            //better performance with batch mode
            virtual bool           store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks);

            virtual bool           store_committed_unit_block(base::xvblock_t* new_raw_block);
            virtual bool           try_update_account_index(uint64_t height, uint64_t viewid, bool update_pre_block);
            
            virtual bool           delete_block(base::xvblock_t* target_block);
            virtual bool           delete_block(const uint64_t target_height);
            
            xvblockdb_t*           get_blockdb_ptr() const {return m_blockdb_ptr;}
        public:
            const int              get_max_cache_size() const;
            const int              get_cached_size() const;
            bool                   clean_caches(bool clean_all,bool force_release_unused_block); //clean unsed caches of account to recall memory
            
            //inline const std::string&   get_address() const {return m_account_ptr->get_address();}
            //inline const std::string&   get_account() const {return m_account_ptr->get_account();}
            inline int             get_block_level() const {return m_meta->_block_level;}
           
            const std::deque<xblockevent_t> move_events(); //move /transfer all events to outside
            bool                            process_events();
            bool                            process_events(const std::deque<xblockevent_t> & events);

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
            base::xvbindex_t*      load_latest_connected_index();   //block has connected to the last full

            base::xvbindex_t*      load_latest_full_index();        //block has full state,genesis is a full block
            base::xvbindex_t*      load_latest_committed_full_index();  // full block with committed status

            bool                   load_latest_index_list(base::xvbindex_t* & cert_block,base::xvbindex_t* & lock_block,base::xvbindex_t* & commit_block);     //latest commit/lock/cert

            int                    load_index(const uint64_t target_height);//return how many index at height
            base::xvbindex_t*      load_index(const uint64_t target_height,const uint64_t view_id, const int atag = 0);
            base::xvbindex_t*      load_index(const uint64_t target_height,const std::string & block_hash, const int atag = 0);
            base::xvbindex_t*      load_index(const uint64_t target_height,base::enum_xvblock_flag request_flag, const int atag = 0);
            std::vector<base::xvbindex_t*>  load_indexes(const uint64_t target_height);//load indexes from db for height
            size_t                 load_index_by_height(const uint64_t target_height);

            
            bool                    set_unit_proof(const std::string& unit_proof, uint64_t height);
            const std::string       get_unit_proof(uint64_t height);

        protected: //help functions
            bool                resort_index_of_store(const uint64_t target_height);
            bool                resort_index_of_store(std::map<uint64_t,base::xvbindex_t*> & target_height_map);
            const uint64_t      cal_index_base_weight(base::xvbindex_t * index);
            //define weight system for block' weight = ([status]) + [prev-connected]
            //to speed up clean up any forked or useless block, let it allow store first then rebase it
            bool                rebase_chain_at_height(const uint64_t target_height);
            bool                rebase_chain_at_height(std::map<uint64_t,base::xvbindex_t*> & target_height_map);

            bool                precheck_new_index(base::xvbindex_t * new_index);
            bool                precheck_new_index(base::xvbindex_t * new_index,std::map<uint64_t,base::xvbindex_t*> & target_height_map);

            base::xvbindex_t*   new_index(base::xvblock_t* new_raw_block);
            base::xvbindex_t*   cache_index(base::xvbindex_t* this_block);//return cached ptr for performance
            base::xvbindex_t*   cache_index(base::xvbindex_t* this_block,std::map<uint64_t,base::xvbindex_t*> & target_height_map);

            bool                link_neighbor(base::xvbindex_t* this_block);//just connect prev and next index of list
            bool                full_connect_to(base::xvbindex_t* this_block);//connect to all the way to fullblock or geneis
            bool                update_meta_metric(base::xvbindex_t* new_block_ptr );

            //connect this_block to prev_block and next_block
            //connect_blockmay call load_block -->call connect again to get prev-block, reenter_allow_count decide how many times can reenter
            virtual bool        process_index(base::xvbindex_t* this_block){return false;}
            virtual bool        connect_index(base::xvbindex_t* this_block){return false;}

            //compatible for old version,e.g read meta and other stuff
            bool                push_event(enum_blockstore_event type,base::xvbindex_t* target);

            void                update_bindex(base::xvbindex_t* this_block);
            
        private:
            virtual bool        init_meta(const base::xvactmeta_t & meta) override;
            bool                recover_meta(const base::xvactmeta_t & _meta);//recover at plugin level if possible;
            virtual bool        save_data() override;
            void                close_blocks(); //clean all cached blocks
            bool                clean_blocks(const int keep_blocks_count,bool force_release_unused_block);
            bool                on_block_revoked(base::xvbindex_t* index_ptr);
            bool                on_block_committed(base::xvbindex_t* index_ptr);
            
        protected:
            base::xblockmeta_t * m_meta;
            xvblockdb_t*         m_blockdb_ptr;
            std::deque<xblockevent_t> m_events_queue;  //stored event
            std::map<uint64_t,std::map<uint64_t,base::xvbindex_t*> > m_all_blocks;  // < height#, <view#,block*> > sort from lower to higher
        };

        //xchainacct_t transfer block status from lower stage to higher : from cert ->lock->commit
        class xchainacct_t : public xblockacct_t
        {
        public:
            xchainacct_t(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr);
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
    
        class xunitbkplugin : public xchainacct_t
        {
        public:
            xunitbkplugin(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr);
        protected:
            virtual ~xunitbkplugin();
        private:
            xunitbkplugin();
            xunitbkplugin(const xunitbkplugin &);
            xunitbkplugin & operator = (const xunitbkplugin &);
        protected:
            virtual base::xvbindex_t*  create_index(base::xvblock_t& new_raw_block) override;
            
            virtual std::vector<base::xvbindex_t*> read_index(const uint64_t target_height) override;
            virtual bool   write_index(base::xvbindex_t* this_block) override;
            
            virtual bool   write_block(base::xvbindex_t* index_ptr) override;
            virtual bool   write_block(base::xvbindex_t* index_ptr,base::xvblock_t * new_block_ptr) override;
            
            virtual bool   store_block(base::xvblock_t* new_raw_block) override;
        };
    
        class xtablebkplugin : public xchainacct_t
        {
        public:
            xtablebkplugin(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr);
        protected:
            virtual ~xtablebkplugin();
        private:
            xtablebkplugin();
            xtablebkplugin(const xtablebkplugin &);
            xtablebkplugin & operator = (const xtablebkplugin &);
        protected:
            virtual base::xvbindex_t*  create_index(base::xvblock_t& new_raw_block) override;
            
            virtual std::vector<base::xvbindex_t*> read_index(const uint64_t target_height) override;
            virtual bool   write_index(base::xvbindex_t* this_index) override;
                 
            virtual bool   write_block(base::xvbindex_t* index_ptr) override;
            virtual bool   write_block(base::xvbindex_t* index_ptr,base::xvblock_t * new_block_ptr) override;
            
            virtual bool   store_block(base::xvblock_t* new_raw_block) override;
        };

    };//end of namespace of vstore
};//end of namespace of top
