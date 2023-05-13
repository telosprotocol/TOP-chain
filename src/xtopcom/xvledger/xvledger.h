// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include "xvaccount.h"
#include "xvactplugin.h"
#include "xvdbstore.h"
#include "xvblockstore.h"
#include "xvcontractstore.h"
#include "xvtxstore.h"
#include "xveventbus.h"
#include "xvdrecycle.h"
#include "xbase/xlock.h"
#include "xdb/xdb_factory.h"

namespace top
{
    namespace base
    {
        class xvtable_t;
        class xvbook_t;
        class xvledger_t;
        class xvchain_t;
 

    
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
    
        enum enum_xvaccount_config
        {
            enum_max_active_acconts         = 2048,  //max active accounts number
            enum_units_group_count          = 256,   //same with max subaddr
            enum_max_expire_check_count     = 4096,  //not clean too much at each loop            
#ifdef DEBUG
            enum_timer_check_interval           = 1000,  //check every 1 seconds            
            enum_account_idle_timeout_ms        = 10*60*1000, //account change to idle status if not access
            enum_block_plugin_idle_timeout_ms   = 10*60*1000,  //idle duration for plugin
            enum_state_plugin_idle_timeout_ms   = 10*60*1000,  //idle duration for plugin
#else
            enum_timer_check_interval           = 10000,  //check every 10 seconds    
            enum_account_idle_timeout_ms        = 20*60*1000, //account change to idle status if not access within 20 minutes
            enum_block_plugin_idle_timeout_ms   = 20*60*1000,  //idle duration for plugin  15minutes
            enum_state_plugin_idle_timeout_ms   = 20*60*1000,  //idle duration for plugin  15minutes
#endif
            enum_account_save_meta_interval = 64, //force save meta every 64 modification
            enum_account_save_meta_offset   = 8,  //force save meta when height offset skip
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
        public: //multiple thread safe
            virtual bool            handle_event(const xvevent_t & ev) override;//add entry of handle event
            const uint64_t          get_idle_duration() const;
            
            //status machine: [normal<=>idle->closing->closed]
            bool                    is_idle() const   {return (m_is_idle != 0);}
            bool                    is_closing()const {return (m_is_closing != 0);}
            //bool is_close() is already defined
            void                    update_idle_start_time_ms(uint64_t current_time_ms);

            xauto_ptr<xvactplugin_t>get_plugin(enum_xvaccount_plugin_type plugin_type);
           
            xauto_ptr<xvactplugin_t>get_set_plugin(xvactplugin_t * new_plugin_obj, bool monitor);
            xauto_ptr<xvactplugin_t>get_set_plugin(enum_xvaccount_plugin_type plugin_type,std::function<xvactplugin_t*(xvaccountobj_t&) > & lambda_to_create, bool monitor);
            
        public: //multiple thread safe
            const xblockmeta_t      get_block_meta();
            const xstatemeta_t      get_state_meta();
            const xsyncmeta_t       get_sync_meta();
            const xvactmeta_t       get_full_meta();
            
            bool                    set_latest_executed_block(const uint64_t height);
            // bool                    get_latest_executed_block(uint64_t & block_height);
            const uint64_t          get_latest_executed_block_height();
            uint64_t                get_lowest_executed_block_height();
            bool                    set_lowest_executed_block_height(const uint64_t height);

            bool                    save_meta(bool carry_process_id = true);
            bool                    update_block_meta(xvactplugin_t * plugin);
        protected:
            std::recursive_mutex&   get_table_lock();
            xspinlock_t&            get_spin_lock()  {return m_spin_lock;}
            xvactmeta_t*            get_meta();
            bool                    recover_meta(xvactmeta_t & _meta);//check whether recover the lost as reboot
            
            bool                    set_block_meta(const xblockmeta_t & new_meta);
            bool                    set_state_meta(const xstatemeta_t & new_meta);
            bool                    set_sync_meta(const xsyncmeta_t & new_meta);
            
        private: //only open for table object
            virtual bool            is_live(const uint64_t timenow_ms) override;//test whether has been idel status
            virtual bool            close(bool force_async = true) override;
            virtual bool            stop(); //convert to closing status if possible
            //return status of currently,return true when it is idled
            bool                    update_idle_status();
            bool                    save_meta(const std::string & vmeta_bin);
            
            //the returned ptr is not reference safe,use careully
            xvactplugin_t*          get_plugin_unsafe(enum_xvaccount_plugin_type plugin_type);
            bool                    set_plugin_unsafe(xvactplugin_t * plugin_obj,xvactplugin_t*& old_ptr,bool monitor);//caller need release old_ptr whent it is not nullptr
 
            //note:try_close_plugin may try hold lock and check is_live agian ,and if so then close
            //return result whether closed
            bool                    try_close_plugin(const uint64_t timenow_ms,enum_xvaccount_plugin_type plugin_type);
            
        private:
            xspinlock_t         m_spin_lock;
            xvtable_t&          m_ref_table; //link to table
            xvactmeta_t*        m_meta_ptr; //meta data of account
            //note: only support max 8 plugins for one object as considering size and reality
            xvactplugin_t*      m_plugins[enum_xvaccount_plugin_max];
        private:
            uint64_t            m_idle_start_time_ms; //UTC ms
            uint64_t            m_idle_timeout_ms;     //how long(ms) it will change to idle status
            uint8_t             m_is_idle; //atomic indicate whether is beeing idle status, 1 = true, 0 = false
            uint8_t             m_is_closing;//status before close but after idle.[normal<=>idle->closing->closed]
            uint8_t             m_is_keep_forever;  //table/book object never be release/close for performance
        };
    
        //note: zone_index = bucket_index:range of [0,15], book_index: range of [0,127], table_index: range of [0,7]
        //note: 12bit chain_id = net_id of same definition as XIP,but valid range for xchain is limited as [0,4095]
        //each table manage unlimited accounts
        class xvtable_t : public xionode_t
        {
            friend class xvbook_t;
            friend class xvaccountobj_t;
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
            
            inline const uint64_t      get_table_index() const {return m_table_index;}
            inline const uint32_t      get_table_combine_addr() const {return m_table_combine_addr;}
            
            inline std::recursive_mutex&  get_lock() {return m_lock;}
            inline xvbook_t &             get_book() {return m_ref_book;}
            
        private:
            xvaccountobj_t*            create_account_unsafe(const std::string & account_address);
            xvaccountobj_t*            get_account_unsafe(const std::string & account_address);
            xvaccountobj_t*            find_account_unsafe(const std::string & account_address);
            bool                       close_account_unsafe(const std::string & account_address);
            bool                       try_close_account(const int64_t current_time_ms,const std::string & account_address);
            
            void                       monitor_plugin(xvactplugin_t * plugin_obj);
            void                       monitor_account(xvaccountobj_t * account_obj);
            
            bool                       on_timer_for_accounts(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms);
            bool                       on_timer_for_plugins(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms);
            bool                       on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms);
            bool                       on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms);
            
            //param of force_clean indicate whether force to close valid account 
            virtual bool               clean_all(bool force_clean = false);//clean all accounts & but table self still ok to use
            virtual bool               on_process_close();//send process_close event to every objects
        private:
            xspinlock_t&               get_spin_lock()  {return m_spin_lock;}
            
        private:
            std::recursive_mutex   m_lock;
            xspinlock_t            m_spin_lock;
            xvbook_t&              m_ref_book; //link to book
            uint64_t               m_table_index;         //define uint64_t just for performance
            uint32_t               m_table_combine_addr; //[ledgerid:16bit][book:7bit][table:3bit]
            uint32_t               m_reserved_4byte;
            uint64_t               m_current_time_ms{0};
            
            std::map<std::string,xvaccountobj_t*>   m_accounts;
            std::multimap<uint64_t,xvactplugin_t*>  m_monitor_plugins;//key:expired_time(UTC ms), value: xvactplugin_t*,sort from lower
            std::multimap<uint64_t,xvaccountobj_t*> m_monitor_accounts;//key:expired_time(UTC ms), value: xvaccountobj_t*,sort from lower
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
            virtual bool            on_process_close();//send process_close event to every objects
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
            virtual bool            on_process_close();//send process_close event to every objects
            
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
                enum_xvchain_plugin_recycle_mgr   = 0x06, //manage xvdrecycle
 
                enum_xvchain_plugin_type_max      = 0x07, //max value,DONT over it
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
            inline const uint32_t       get_current_process_id() const {return m_current_process_id;}
            inline const uint64_t       get_process_start_time() const {return m_proces_start_time;}//gmt
            inline const std::string&   get_data_dir_path()      const {return m_data_dir_path;}
            inline bool                 is_auto_prune_enable()   const {return (m_is_auto_prune != 0);}
             
        public://note:each bucket/ledger may have own db and blockstore etc
            xvdbstore_t*                get_xdbstore(); //global shared db instance
            xvtxstore_t*                get_xtxstore();   //global shared xvtxstore_t instance
            xvblockstore_t*             get_xblockstore();//global shared blockstore instance
            // xvstatestore_t*             get_xstatestore();//global shared statestore instance
            xvcontractstore_t*          get_xcontractstore();//global shared statestore instance
            xveventbus_t*               get_xevmbus(); //global mbus object
            xvdrecycle_mgr*             get_xrecyclemgr(); //global recycler manager
            xvdrecycle_t*               get_xrecycler(enum_vdata_recycle_type type);//quick path

        public://node storage and prune settings
            void                        set_node_type(bool is_storage_node, bool has_other_node);
            inline bool                 is_storage_node() const {return m_is_storage_node.load();}
            inline bool                 has_other_node() const {return m_has_other_node.load();}                      
            bool                        need_store_unitstate(int zone_index) const;
            bool                        need_store_units(int zone_index) const;  

        public:
            bool                        set_xdbstore(xvdbstore_t * new_store);//set global shared instance
            bool                        set_xtxstore(xvtxstore_t * new_store);
            bool                        set_xblockstore(xvblockstore_t * new_store);//set global shared instance
            // bool                        set_xstatestore(xvstatestore_t* new_sotre);
            bool                        set_xcontractstore(xvcontractstore_t * new_store);
            bool                        set_xevmbus(xveventbus_t * new_mbus);
            
            bool                        set_data_dir_path(const std::string & dir_path);
            void                        enable_auto_prune(bool enable);
            
            //param of force_clean indicate whether force to close valid account
            virtual bool                clean_all(bool force_clean = false);//just do clean but not never destory objects of ledger/book/table
            
            virtual bool                save_all(); //save all unsaved data and meta etc
            virtual bool                on_process_close();//send process_close event to every objects
            uint16_t                    get_round_number() {return m_round_number;}
            void                        add_round_number() {m_round_number++;}
            void                        get_db_config_custom(std::vector<db::xdb_path_t> &extra_db_path, int &extra_db_kind);
        protected:
            virtual xvledger_t*         create_ledger_object(const uint64_t ledger_id);//give default implementation
            bool                        set_xrecyclemgr(xvdrecycle_mgr* new_mgr);
        private:
            std::recursive_mutex    m_lock;
            uint8_t                 m_is_auto_prune;//1 means on,0 means off
            bool                    m_node_init{false};
            std::atomic<bool>       m_is_storage_node{true}; //default yes for all store
            std::atomic<bool>       m_has_other_node{true};//default yes for all store
            std::atomic<bool>       m_store_units{true};//default store all units
            std::atomic<bool>       m_store_unitstates{true};//default store all unitstates
            uint8_t                 m_reserved_2;
            uint16_t                m_round_number;
            uint32_t                m_chain_id;//aka network_id
            uint32_t                m_current_node_roles;//multiple roles
            uint32_t                m_current_process_id;
            uint64_t                m_proces_start_time; //GMT times as seconds
            std::string             m_data_dir_path;
        protected:
            xvledger_t*   m_ledgers[enum_vchain_has_buckets_count];
        private:
            static xvchain_t *      __global_vchain_instance;
        };
        ///////////////////////provide general structure for xledger and related //////////////////
    
    }//end of namespace of base
}//end of namespace top
