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
        enum enum_blockstore_event
        {
            enum_blockstore_event_committed  = 1, //block is committed
            enum_blockstore_event_revoke     = 2, //block is revoke and removed by consensus
            enum_blockstore_event_stored     = 4, //block is stored persistedly
        };

        enum enum_blockstore_metrics_type
        {
            enum_blockstore_metrics_type_block_index        = 1,
            enum_blockstore_metrics_type_block_object       = 2,
            enum_blockstore_metrics_type_block_input_res    = 3,
            enum_blockstore_metrics_type_block_output_res   = 4,
        };
        class xblockevent_t
        {
        public:
            xblockevent_t(enum_blockstore_event type,base::xvbindex_t* target)
            {
                _event_type     = type;
                _target_index   = target;
                if(target != NULL)
                    target->add_ref();
            }
            xblockevent_t(xblockevent_t && obj)
            {
                _event_type     = obj._event_type;
                _target_index   = obj._target_index;
                obj._target_index = NULL;
            }
            xblockevent_t(const xblockevent_t & obj)
            {
                _event_type     = obj._event_type;
                _target_index   = obj._target_index;
                if(_target_index != NULL)
                    _target_index->add_ref();
            }
            xblockevent_t & operator = (const xblockevent_t & obj)
            {
                base::xvbindex_t* old_ptr = _target_index;

                _event_type     = obj._event_type;
                _target_index   = obj._target_index;
                if(_target_index != NULL)
                    _target_index->add_ref();

                if(old_ptr != NULL)
                    old_ptr->release_ref();

                return *this;
            }
            ~xblockevent_t()
            {
                if(_target_index != NULL)
                    _target_index->release_ref();
            }
        private:
            xblockevent_t();
        public:
            inline enum_blockstore_event get_type() const {return _event_type;}
            inline base::xvbindex_t*     get_index()const {return _target_index;}
        protected:
            enum_blockstore_event   _event_type;
            base::xvbindex_t*       _target_index;
        };

        //each account has own virtual store
        class xblockacct_t : public base::xvblockplugin_t,public base::xvaccount_t
        {
        protected:
            enum{enum_max_cached_blocks = 32};
        public:
            uint64_t            get_latest_committed_block_height() const { return m_meta->_highest_commit_block_height; }
            uint64_t            get_latest_connected_block_height() const { return m_meta->_highest_connect_block_height; }
        public:
            xblockacct_t(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,const std::string & blockstore_path,base::xvdbstore_t* xvdb_ptr);
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
            const int              get_max_cache_size() const;
            bool                   clean_caches(bool clean_all,bool force_release_unused_block); //clean unsed caches of account to recall memory
            base::xvdbstore_t*     get_xdbstore();
            
            //inline const std::string&   get_address() const {return m_account_ptr->get_address();}
            //inline const std::string&   get_account() const {return m_account_ptr->get_account();}

            const int              get_cache_size();
            inline int             get_block_level() const {return m_meta->_block_level;}
            inline const std::string &   get_blockstore_path()   const {return m_blockstore_path;};

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

            bool                   load_block_object(base::xvbindex_t* index_ptr, const int atag = 0);
            bool                   load_index_input(base::xvbindex_t* target_block);
            bool                   load_index_output(base::xvbindex_t* target_block);
            size_t                 load_index_by_height(const uint64_t target_height);
            bool                   delete_block_from_db(base::xvbindex_t* index_ptr);

        public://operated for raw block
            bool                   store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks); //better performance
            bool                   store_block(base::xvblock_t* new_raw_block);
            bool                   delete_block(base::xvblock_t* target_block);//return error code indicate what is result
            bool                   delete_block(const uint64_t target_height);//return error code indicate what is result
            bool                   load_block_input(base::xvblock_t* target_block);
            bool                   load_block_output(base::xvblock_t* target_block);
            bool                   load_block_flags(base::xvblock_t* target_block);//update block'flags
        public:
            // genesis connected information
            bool        set_genesis_height(const std::string &height);
            const std::string    get_genesis_height();
            bool        set_block_span(const uint64_t height,  const std::string &span);
            bool        delete_block_span(const uint64_t height);
            const std::string get_block_span(const uint64_t height);

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

            bool                write_index_to_db(const uint64_t target_height);
            bool                write_index_to_db(std::map<uint64_t,base::xvbindex_t*> & indexes);
            bool                write_index_to_db(base::xvbindex_t* index_obj);
            base::xvbindex_t*   read_index_from_db(const std::string & index_db_key_path);
            //return map sorted by viewid from lower to high,caller respond to release ptr later
            std::vector<base::xvbindex_t*> read_index_from_db(const uint64_t target_height);

            //connect this_block to prev_block and next_block
            //connect_blockmay call load_block -->call connect again to get prev-block, reenter_allow_count decide how many times can reenter
            virtual bool        process_index(base::xvbindex_t* this_block){return false;}
            virtual bool        connect_index(base::xvbindex_t* this_block){return false;}

        private:
            virtual bool        init_meta(const base::xvactmeta_t & meta) override;
            virtual bool        save_data() override;
            void                close_blocks(); //clean all cached blocks
            bool                clean_blocks(const int keep_blocks_count,bool force_release_unused_block);
            bool                on_block_revoked(base::xvbindex_t* index_ptr);
            bool                on_block_committed(base::xvbindex_t* index_ptr);
            bool                store_txs_to_db(base::xvbindex_t* index_ptr);

        protected: //compatible for old version,e.g read meta and other stuff
            const std::string   load_value_by_path(const std::string & full_path_as_key);
            bool                store_value_by_path(const std::string & full_path_as_key,const std::string & value);
            bool                delete_value_by_path(const std::string & full_path_as_key);
            bool                push_event(enum_blockstore_event type,base::xvbindex_t* target);
        private:
            std::string         m_blockstore_path;
        protected:
            base::xblockmeta_t * m_meta;
            base::xvdbstore_t  * m_xvdb_ptr;
            std::deque<xblockevent_t> m_events_queue;  //stored event
            std::map<uint64_t,std::map<uint64_t,base::xvbindex_t*> > m_all_blocks;  // < height#, <view#,block*> > sort from lower to higher
        };

        //xchainacct_t transfer block status from lower stage to higher : from cert ->lock->commit
        class xchainacct_t : public xblockacct_t
        {
        public:
            xchainacct_t(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,const std::string & blockstore_path,base::xvdbstore_t* xvdb_ptr);
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
