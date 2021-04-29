// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include "xvledger/xvledger.h"
#include "../xblockstore_face.h"

namespace top
{
    namespace store
    {
        class xblockacct_t;
        //note: layers for store :  [xvblock-store] --> [xstore] -->[xdb]
        class xvblockstore_impl : public base::xvblockstore_t,public base::xtimersink_t
        {
            enum
            {
                enum_max_active_acconts         = 2048,  //max active accounts number
                enum_units_group_count          = 256,   //same with max subaddr
                enum_max_expire_check_count     = 4096,  //not clean too much at each loop

                #ifdef DEBUG //short timeout to give stress for program
                enum_account_idle_check_interval= 10000,   //check every 5 seconds
                enum_account_idle_timeout_ms    = 60000,   //account change to idle status if not access within 60 seconds
                #else
                enum_account_idle_check_interval= 10000,   //check every 10 seconds
                enum_account_idle_timeout_ms    = 60000,   //account change to idle status if not access within 120 seconds
                #endif
            };
        public:
            xvblockstore_impl(const std::string & blockstore_path,base::xcontext_t & _context,const int32_t target_thread_id);
        protected:
            virtual ~xvblockstore_impl();
        private:
            xvblockstore_impl();
            xvblockstore_impl(xvblockstore_impl &&);
            xvblockstore_impl(const xvblockstore_impl &);
            xvblockstore_impl & operator = (const xvblockstore_impl &);

        public:
            virtual bool        close(bool force_async = true) override; //must call close before release object,otherwise object never be cleanup
            
        public://better performance,and return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect

            virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const base::xvaccount_t & account)  override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const base::xvaccount_t & account)   override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const base::xvaccount_t & account)override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const base::xvaccount_t & account)override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_genesis_connected_block(const base::xvaccount_t & account) override; //block has connected to genesis
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_full_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_full_block(const base::xvaccount_t & account) override;
 
            //ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
            virtual base::xblock_vector               load_block_object(const base::xvaccount_t & account,const uint64_t height) override;
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load) override;
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load) override;
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,bool ask_full_load) override; //just return the highest viewid of matched flag
            
            virtual bool                load_block_input(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                load_block_output(const base::xvaccount_t & account,base::xvblock_t* block) override;
            //load xvboffdata_t and set into xvblock_t
            virtual bool                load_block_offdata(const base::xvaccount_t & account,base::xvblock_t* block) override;
            
            virtual bool                store_block(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                delete_block(const base::xvaccount_t & account,base::xvblock_t* block) override;
 

        public://batch process api
            virtual base::xblock_mptrs  get_latest_blocks(const base::xvaccount_t & account) override;
            virtual bool                store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks) override;

        public: //just query at cached blocks(without persist db involved),mostly used for query cert-only block
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid) override;
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash) override;
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block) override; //just return the highest viewid of matched flag
            virtual base::xblock_vector query_block(const base::xvaccount_t & account,const uint64_t height) override;//might mutiple certs at same height

        public:
            virtual base::xvbindex_vector              load_block_index(const base::xvaccount_t & account,const uint64_t height) override;
            virtual base::xauto_ptr<base::xvbindex_t>  load_block_index(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid) override;
            virtual base::xauto_ptr<base::xvbindex_t>  load_block_index(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash) override;
            virtual base::xauto_ptr<base::xvbindex_t>  load_block_index(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block) override;//just return the highest viewid of matched flag
            
        public:
            //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
            virtual bool                clean_caches(const base::xvaccount_t & account) override;

            //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
            virtual bool                reset_cache_timeout(const base::xvaccount_t & account,const uint32_t max_idle_time_ms) override;
            
        public://execute_block will move to statestore soon
            //note: block must be committed and connected
            virtual bool                 execute_block(const base::xvaccount_t & account,base::xvblock_t* block) override; //execute block and update state of acccount
            virtual base::xvtransaction_store_ptr_t  query_tx(const std::string & txhash, base::enum_transaction_subtype type) override;
        
        public:
            virtual bool                 exist_genesis_block(const base::xvaccount_t & account) override; 

        protected:
            base::xauto_ptr<xblockacct_t>get_block_account(base::xvtable_t * target_table,const std::string & account_address);
            
            base::xvblock_t *            load_block_from_index(xblockacct_t* target_account, base::xauto_ptr<base::xvbindex_t> target_index,const uint64_t target_height,bool ask_full_load);
           
            //store table/book blocks if they are
            bool                        store_block(base::xauto_ptr<xblockacct_t> & container_account,base::xvblock_t * container_block);
           
            //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
            virtual std::string          get_store_path() const override {return m_store_path;}//each store may has own space at DB/disk

        private:
            bool                        on_block_stored(base::xvblock_t* this_block_ptr);//event for block store
            
            virtual bool                on_object_close() override;
            virtual bool                on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool                on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool                on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;

        private: //just access by self thread
            base::xtimer_t*                          m_raw_timer;
            std::multimap<uint64_t,xblockacct_t*>    m_monitor_expire;//key:expired_time(UTC ms), value: xblockaccount_t*,sort from lower
        private://below are accessed by muliple threads
            std::string                              m_store_path;
        };

    };//end of namespace of vstore
};//end of namespace of top
