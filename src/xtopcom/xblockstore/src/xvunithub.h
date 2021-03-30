// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <map>
#include <mutex>
#include "xbase/xvledger.h"
#include "xstore/xstore_face.h"

namespace top
{
    namespace store
    {
        class xblockacct_t;
        //note: layers for store :  [xvblock-store] --> [xstore] -->[xdb]
        class xvblockstore_impl : public base::xvblockstore_t,public base::xiobject_t,public base::xtimersink_t
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
            static bool  init_store(base::xcontext_t &_context);//do initialize for all store objects
        public:
            xvblockstore_impl(const std::string & blockstore_path,xstore_face_t&  _persist_db,base::xcontext_t & _context,const int32_t target_thread_id);
        protected:
            virtual ~xvblockstore_impl();
        private:
            xvblockstore_impl();
            xvblockstore_impl(const xvblockstore_impl &);
            xvblockstore_impl & operator = (const xvblockstore_impl &);

        public:
            virtual bool        close(bool force_async = true) override; //must call close before release object,otherwise object never be cleanup

        public: //return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect
            virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const std::string & account) override;//genesis block
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const std::string & account)override;//highest view# for any status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const std::string & account)    override;//block with locked status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const std::string & account) override;//block with committed status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const std::string & account)  override;//block with executed status
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const std::string & account) override;//block connected to genesis/full block
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_full_block(const std::string & account) override;//block has full state,genesis is a full block
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_current_block(const std::string & account, bool ask_full_load) override;//block has connected to cert/lock/commit block

            //ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const std::string & account,const uint64_t height,bool ask_full_load = true) override;
            virtual bool                load_block_input(base::xvblock_t* block)  override;//load and assign input data into block
            virtual bool                load_block_output(base::xvblock_t* block) override;//load and assign output data into block

            virtual bool                store_block(base::xvblock_t* block)  override; //return false if fail to store
            virtual bool                delete_block(base::xvblock_t* block) override; //return false if fail to delete

        public://better performance,and return raw ptr with added reference,caller respond to release it after that.
            //please refer enum_xvblock_flag definition for terms of lock,commit,execute,connect

            virtual base::xauto_ptr<base::xvblock_t>  get_genesis_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_cert_block(const base::xvaccount_t & account)  override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_locked_block(const base::xvaccount_t & account)   override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_committed_block(const base::xvaccount_t & account)override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_executed_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_connected_block(const base::xvaccount_t & account)override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_full_block(const base::xvaccount_t & account) override;
            virtual base::xauto_ptr<base::xvblock_t>  get_latest_current_block(const base::xvaccount_t & account, bool ask_full_load) override;

            //ask_full_load decide load header only or include input/output(that can be loaded seperately by load_block_input/output)
            virtual base::xauto_ptr<base::xvblock_t>  load_block_object(const base::xvaccount_t & account,const uint64_t height,bool ask_full_load = true)override;
            virtual bool                load_block_input(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                load_block_output(const base::xvaccount_t & account,base::xvblock_t* block) override;

            virtual bool                store_block(const base::xvaccount_t & account,base::xvblock_t* block) override;
            virtual bool                delete_block(const base::xvaccount_t & account,base::xvblock_t* block) override;

            //note: block must be committed and connected
            virtual bool                execute_block(const base::xvaccount_t & account,base::xvblock_t* block) override; //execute block and update state of acccount

        public://batch process api
            virtual base::xblock_mptrs  get_latest_blocks(const base::xvaccount_t & account) override;
            virtual bool                store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks) override;

        public: //just query at cached blocks(without persist db involved),mostly used for query cert-only block
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid) override;
            virtual base::xauto_ptr<base::xvblock_t>  query_block(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash) override;

            virtual base::xblock_vector query_block(const base::xvaccount_t & account,const uint64_t height) override;//might mutiple certs at same height

        public:
            //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
            virtual bool                clean_caches(const base::xvaccount_t & account) override;

            //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
            virtual bool                reset_cache_timeout(const base::xvaccount_t & account,const uint32_t max_idle_time_ms) override;
        protected:
            uint32_t                    cal_group_index_from_account(const std::string & account);
            uint32_t                    cal_group_index_from_account(const base::xvaccount_t & account);

            xblockacct_t*               get_block_account(const uint32_t group_index,const std::string & account);
            //a full path to load vblock could be  get_store_path()/create_object_path()/xvblock_t::name()
            virtual std::string         get_store_path() const override {return m_store_path;}//each store may has own space at DB/disk

        private:
            virtual bool                on_object_close() override;
            virtual bool                on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool                on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool                on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;

            virtual int32_t             do_write(base::xstream_t & stream) override;//write whole object to binary
            virtual int32_t             do_read(base::xstream_t & stream) override; //read from binary and regeneate content

        private: //just access by self thread
            base::xtimer_t*                          m_raw_timer;
            std::multimap<uint64_t,xblockacct_t*>    m_monitor_expire;//key:expired_time(UTC ms), value: xblockaccount_t*,sort from lower
        private://below are accessed by muliple threads
            std::recursive_mutex                     m_group_locks[enum_units_group_count];
            std::map<std::string,xblockacct_t*>      m_group_units[enum_units_group_count];
            std::string                              m_store_path;
            xstore_face_t*                           m_persist_db;
        };

    };//end of namespace of vstore
};//end of namespace of top
