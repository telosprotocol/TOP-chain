// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include "xvaccount.h"
#include "xvdbstore.h"
#include "xvblockstore.h"
#include "xvstatestore.h"
#include "xvcontractstore.h"
#include "xvtxstore.h"
#include "xveventbus.h"

namespace top
{
    namespace base
    {
        class xvtable_t;
        class xvbook_t;
        class xvledger_t;
        class xvchain_t;
        class xvaccountobj_t;

        /*
        XID/xvvid  definition as total 64bit =
        {
            -[32bit]    //index(could be as hash32(account_address)
            -[26bit]    //prefix  e.g. xledger defined as below
                -[16bit:ledger_id]
                    -[12bit:net#/chain#]
                    -[4 bit:zone#/bucket-index]
                -[10bit:subaddr_of_ledger]
                    -[7 bit:book-index]
                    -[3 bit:table-index]
            -[enum_xid_level :3bit]
            -[enum_xid_type  :3bit]
        }
        */
    
        enum enum_xvaccount_plugin_type //max 8 plugins
        {
            enum_xvaccount_plugin_start      = 0x00,
            enum_xvaccount_plugin_txsmgr     = 0x00, //manage transactions
            enum_xvaccount_plugin_blockmgr   = 0x01, //manage blocks
            enum_xvaccount_plugin_statemgr   = 0x02, //manage states
            enum_xvaccount_plugin_indexmgr   = 0x03, //manage indexs
            enum_xvaccount_plugin_executemgr = 0x04, //manage contract & function execute
            
            //note:update max once add new plugin at above
            enum_xvaccount_plugin_end        = enum_xvaccount_plugin_executemgr,
            enum_xvaccount_plugin_max        = 0x08,
        };
    
        enum enum_xvaccount_config
        {
            enum_max_active_acconts         = 2048,  //max active accounts number
            enum_units_group_count          = 256,   //same with max subaddr
            enum_max_expire_check_count     = 4096,  //not clean too much at each loop
            
            enum_account_idle_check_interval= 50000,  //check every 50 seconds
            enum_account_idle_timeout_ms    = 300000, //account change to idle status if not access within 300 seconds

            enum_plugin_idle_check_interval = 10000,  //check every 10 seconds
            enum_plugin_idle_timeout_ms     = 60000,  //idle duration for plugin
            
            enum_account_save_meta_interval = 64, //force save meta every 64 modification
        };
    
        class xvactplugin_t : public xobject_t
        {
        protected:
            xvactplugin_t(xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms,enum_xvaccount_plugin_type type);
            virtual ~xvactplugin_t();
        private:
            xvactplugin_t();
            xvactplugin_t(xvactplugin_t &&);
            xvactplugin_t(const xvactplugin_t &);
            xvactplugin_t & operator = (const xvactplugin_t &);
            
        public:
            enum_xvaccount_plugin_type get_plugin_type()   const {return m_plugin_type;}
            const std::string &     get_account_address()  const ;
            xvaccountobj_t*         get_account_obj()      const {return m_account_obj;}
            inline const uint64_t   get_idle_duration()    const {return m_idle_timeout_ms;}
            inline const uint64_t   get_last_access_time() const {return m_last_access_time_ms;} //UTC ms
            void                    set_last_access_time(const uint64_t last_access_time);
           
            //test whether has been idel status
            virtual bool            is_live(const uint64_t timenow_ms) override;
            virtual bool            close(bool force_async = true) override;
        private:
            xvaccountobj_t * m_account_obj;
            uint64_t         m_last_access_time_ms; //UTC ms
            uint64_t         m_idle_timeout_ms;     //how long(ms) it will change to idle status
            enum_xvaccount_plugin_type m_plugin_type;
        };
            
        class xvaccountobj_t : public xiobject_t,public xvaccount_t
        {
            friend class xvtable_t;
            
        protected://max as 8 tables per book
            xvaccountobj_t(xvtable_t & parent_object,const std::string & account_address);
            virtual ~xvaccountobj_t();
        private:
            xvaccountobj_t();
            xvaccountobj_t(xvaccountobj_t &&);
            xvaccountobj_t(const xvaccountobj_t &);
            xvaccountobj_t & operator = (const xvaccountobj_t &);
        public:
            virtual bool            handle_event(const xvevent_t & ev) override;//add entry of handle event
            const uint64_t          get_idle_duration() const;
            
            xauto_ptr<xvactplugin_t>get_plugin(enum_xvaccount_plugin_type plugin_type);
            
        public:
            bool    set_block_meta(const xblockmeta_t & new_meta);
            bool    set_state_meta(const xstatemeta_t & new_meta);
            bool    set_sync_meta(const xsyncmeta_t & new_meta);
            bool    set_index_meta(const xindxmeta_t & new_meta);
            bool    set_latest_executed_block(const uint64_t height, const std::string & blockhash);
            
            const xblockmeta_t  get_block_meta();
            const xstatemeta_t  get_state_meta();
            const xindxmeta_t   get_index_meta();
            const xsyncmeta_t   get_sync_meta();
            bool                save_meta();
            
        protected:
            std::recursive_mutex&   get_table_lock();
            std::recursive_mutex&   get_book_lock();
            
        private: //only open for table object
            bool                    is_idle() const {return (m_is_idle != 0);}
            virtual bool            is_live(const uint64_t timenow_ms) override;//test whether has been idel status
            virtual bool            close(bool force_async = true) override;
            xvactmeta_t*            get_meta();
               
            //the returned ptr is not reference safe,use careully
            xvactplugin_t*          get_plugin_unsafe(enum_xvaccount_plugin_type plugin_type);
            //return to indicate setup successful or not
            bool                    set_plugin_unsafe(xvactplugin_t * plugin_obj);//plugin_obj must be valid
            bool                    reset_plugin_unsafe(enum_xvaccount_plugin_type plugin_type);//cleanup
            bool                    reset_plugin_unsafe(xvactplugin_t * target_plugin_obj);//check & cleanup
        private:
            xvtable_t&          m_ref_table; //link to table
            xvactmeta_t*        m_meta_ptr; //meta data of account
            //note: only support max 8 plugins for one object as considering size and reality
            xvactplugin_t*      m_plugins[enum_xvaccount_plugin_max];
        private:
            uint64_t            m_last_saved_meta_hash;
            uint64_t            m_idle_start_time_ms; //UTC ms
            uint64_t            m_idle_timeout_ms;     //how long(ms) it will change to idle status
            uint8_t             m_is_idle; //atomic indicate whether is beeing idle status, 1 = true, 0 = false
            uint8_t             m_is_keep_forever;  //table/book object never be release/close for performance
        };
    
        //note: zone_index = bucket_index:range of [0,15], book_index: range of [0,127], table_index: range of [0,7]
        //note: 12bit chain_id = net_id of same definition as XIP,but valid range for xchain is limited as [0,4095]
        //each table manage unlimited accounts
        class xvtable_t : public xionode_t
        {
            friend class xvbook_t;
        protected://max as 8 tables per book
            xvtable_t(xvbook_t & parent_object,const int32_t thread_id,const uint8_t table_index);
            virtual ~xvtable_t();
        private:
            xvtable_t();
            xvtable_t(xvtable_t &&);
            xvtable_t(const xvtable_t &);
            xvtable_t & operator = (const xvtable_t &);
            
        public: //multiple thread safe
            xauto_ptr<xvaccountobj_t>  get_account(const std::string & account_address);
            xauto_ptr<xvaccountobj_t>  get_account(const xvaccount_t & account_obj){return get_account(account_obj.get_address());}
            bool                       close_account(const std::string & account_address);
            
            xauto_ptr<xvactplugin_t>   get_account_plugin(const std::string & account_address,enum_xvaccount_plugin_type plugin_type);
            //return bool to indicate setup successful or not
            bool                       set_account_plugin(xvactplugin_t * new_plugin_obj);
            bool                       reset_account_plugin(const std::string & account_address,enum_xvaccount_plugin_type plugin_type); //cleanup
            bool                       reset_account_plugin(xvactplugin_t * target_plugin); //clean
            
            inline const uint64_t      get_table_index() const {return m_table_index;}
            inline const uint32_t      get_table_combine_addr() const {return m_table_combine_addr;}
            
            inline std::recursive_mutex&  get_lock() {return m_lock;}
            inline xvbook_t &             get_book() {return m_ref_book;}
            
        public:
            
            
        private:
            xvaccountobj_t*            create_account_unsafe(const std::string & account_address);
            bool                       close_account_unsafe(const std::string & account_address);
            xvaccountobj_t*            get_account_unsafe(const std::string & account_address);
            xvactplugin_t*             get_account_plugin_unsafe(const std::string & account_address,enum_xvaccount_plugin_type plugin_type);
            
            void                       monitor_plugin(xvactplugin_t * plugin_obj);
            void                       monitor_account(xvaccountobj_t * account_obj);
            bool                       on_timer_for_accounts(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms);
            bool                       on_timer_for_plugins(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms);
            bool                       on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms);
            bool                       on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms);
            
            //param of force_clean indicate whether force to close valid account 
            virtual bool               clean_all(bool force_clean = false);//clean all accounts & but table self still ok to use
            
        #ifdef DEBUG //debug only purpose
            virtual int32_t   add_ref() override;
            virtual int32_t   release_ref() override;
        #endif
        private:
            std::recursive_mutex   m_lock;
            xvbook_t&              m_ref_book; //link to book
            uint64_t               m_table_index;         //define uint64_t just for performance
            std::map<std::string,xvaccountobj_t*>   m_accounts;
            std::multimap<uint64_t,xvactplugin_t*>  m_monitor_plugins;//key:expired_time(UTC ms), value: xvactplugin_t*,sort from lower
            std::multimap<uint64_t,xvaccountobj_t*> m_monitor_accounts;//key:expired_time(UTC ms), value: xvaccountobj_t*,sort from lower
            uint32_t               m_table_combine_addr; //[ledgerid:16bit][book:7bit][table:3bit]
        };
        
        //each book manage 8 tables
        class xvbook_t : public xionode_t,public xtimersink_t
        {
            friend class xvledger_t;
        protected://max as 32 books per bucket
            xvbook_t(xvledger_t & parent_object,const int32_t thread_id,const uint8_t book_index);
            virtual ~xvbook_t();
        private:
            xvbook_t();
            xvbook_t(xvbook_t &&);
            xvbook_t(const xvbook_t &);
            xvbook_t & operator = (const xvbook_t &);
        public:
            inline std::recursive_mutex&  get_lock() {return m_lock;}
            xvtable_t*              get_table(const xvid_t & account_id);//return raw ptr as perforamnce
            const uint64_t          get_book_index() const {return m_book_index;}//return uint64_t just for performance
            const uint32_t          get_book_combine_addr() const {return m_book_combine_addr;}//combined address
            
        protected://internal or subcalss use only
            //param of force_clean indicate whether force to close valid account
            virtual bool            clean_all(bool force_clean = false); //just clean all accounts but table object is not release
            virtual xvtable_t*      create_table_object(const uint32_t table_index);//give default implementation
            
        protected: //interface xtimersink_t
            virtual bool            on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //attached into io-thread
            virtual bool            on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms) override;   //detach means it detach
            virtual bool            on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms) override;
            
        private:
            std::recursive_mutex    m_lock;
            uint64_t                m_book_index;         //define uint64_t just for performance
        protected:
            xvtable_t*   m_tables[enum_vbook_has_tables_count];
            xtimer_t*    m_monitor_timer;
            uint32_t     m_book_combine_addr;  //[ledgerid:16bit][book:7bit][table:3bit]
        };
        
        //each ledger manage 32 books which manage 8 tables,in other words each leadger manage 256 tables
        class xvledger_t  : public xionode_t
        {
            friend class xvchain_t;
        protected://16bit id = [12bit:netid/chainid][4bit:bucket_index]
            xvledger_t(xvchain_t & parent_object,const int32_t thread_id,const uint16_t vledger_id);
            virtual ~xvledger_t();
        private:
            xvledger_t();
            xvledger_t(xvledger_t &&);
            xvledger_t(const xvledger_t &);
            xvledger_t & operator = (const xvledger_t &);
        public:
            xvbook_t*               get_book(const xvid_t & account_id);//return raw ptr as perforamnce
            xvtable_t*              get_table(const xvid_t & account_id);//return raw ptr as perforamnce
            xvtable_t*              get_table(const std::string & account_address);//wrap function
            xvtable_t*              get_table(const xvaccount_t & account_obj){return get_table(account_obj.get_xvid());}
            
            xauto_ptr<xvaccountobj_t>   get_account(const std::string & account_address);
            xauto_ptr<xvaccountobj_t>   get_account(const xvaccount_t & account_obj);
            
            inline const int        get_ledger_id()    const {return (int)m_ledger_id;}
            inline const int        get_chain_id()     const {return ((int)m_ledger_id >> 4);}
            inline const int        get_network_id()   const {return ((int)m_ledger_id >> 4);}
            inline const int        get_bucket_index() const {return ((int)m_ledger_id & 0x0F);}
            inline const int        get_zone_index()   const {return ((int)m_ledger_id & 0x0F);}
            
        protected:
            //param of force_clean indicate whether force to close valid account
            virtual bool            clean_all(bool force_clean = false); //just do clean but not never destory objects of book/table
            virtual xvbook_t*       create_book_object(const uint32_t book_index);//give default implementation
            
        private:
            std::recursive_mutex    m_lock;
            uint64_t                m_ledger_id;
        protected:
            xvbook_t*        m_books[enum_vbucket_has_books_count];
        };
        
        //each chain manage 16 zone/buckets, each bucket/zone has unique ledger
        class xvchain_t : public xionode_t
        {
            enum enum_xvchain_plugin_type //max 8 plugins
            {
                enum_xvchain_plugin_type_start    = 0x00,
                enum_xvchain_plugin_kdb_store     = 0x00, //manage key-value db
                enum_xvchain_plugin_block_store   = 0x01, //manage all blocks,assocated with xvblockstore_t interface
                enum_xvchain_plugin_state_store   = 0x02, //manage all states
                enum_xvchain_plugin_contract_store= 0x03, //manage vcontract
                enum_xvchain_plugin_txs_store     = 0x04, //manage all transactions
                enum_xvchain_plugin_event_mbus    = 0x05, //manage mbus
                enum_xvchain_plugin_type_end      = 0x05,
            };
        public:
            static xvchain_t &  instance(const uint16_t chain_id = 0);
        protected:
            xvchain_t(const int32_t thread_id,const uint16_t chain_id);//12bit chain_id(aka network_id)
            virtual ~xvchain_t();
        private:
            xvchain_t();
            xvchain_t(xvchain_t &&);
            xvchain_t(const xvchain_t &);
            xvchain_t & operator = (const xvchain_t &);
        public:
            xvledger_t*                 get_ledger(const xvid_t & account_id);
            xvtable_t*                  get_table(const xvid_t & account_id);
            xvtable_t*                  get_table(const std::string & account_address);//wrap function
            xvtable_t*                  get_table(const xvaccount_t & account_obj){return get_table(account_obj.get_xvid());}
            
            xauto_ptr<xvaccountobj_t>   get_account(const std::string & account_address);
            xauto_ptr<xvaccountobj_t>   get_account(const xvaccount_t & account_obj);
            
            inline const int            get_chain_id()     const {return (int)m_chain_id;}
            inline const int            get_network_id()   const {return (int)m_chain_id;}
            
        public://note:each bucket/ledger may have own db and blockstore etc
            xvdbstore_t*                get_xdbstore(); //global shared db instance
            xvtxstore_t*                get_xtxstore();   //global shared xvtxstore_t instance
            xvblockstore_t*             get_xblockstore();//global shared blockstore instance
            xvstatestore_t*             get_xstatestore();//global shared statestore instance
            xvcontractstore_t*          get_xcontractstore();//global shared statestore instance
            xveventbus_t*               get_xevmbus(); //global mbus object
        public:
            bool                        set_xdbstore(xvdbstore_t * new_store);//set global shared instance
            bool                        set_xtxstore(xvtxstore_t * new_store);
            bool                        set_xblockstore(xvblockstore_t * new_store);//set global shared instance
            bool                        set_xstatestore(xvstatestore_t* new_sotre);
            bool                        set_xcontractstore(xvcontractstore_t * new_store);
            bool                        set_xevmbus(xveventbus_t * new_mbus);
        
            //param of force_clean indicate whether force to close valid account
            virtual bool                clean_all(bool force_clean = false);//just do clean but not never destory objects of ledger/book/table
        protected:
            virtual xvledger_t*     create_ledger_object(const uint64_t ledger_id);//give default implementation
        private:
            std::recursive_mutex    m_lock;
            uint64_t                m_chain_id;//aka network_id
        protected:
            xvledger_t*   m_ledgers[enum_vchain_has_buckets_count];
        private:
            static xvchain_t *      __global_vchain_instance;
        };
        ///////////////////////provide general structure for xledger and related //////////////////
    
    }//end of namespace of base
}//end of namespace top
