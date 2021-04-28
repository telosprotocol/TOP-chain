// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <mutex>
#include "xvaccount.h"
#include "xvdbstore.h"
#include "xvblockstore.h"
#include "xvstatestore.h"
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
            enum_xvaccount_plugin_end        = enum_xvaccount_plugin_executemgr + 1,
            enum_xvaccount_plugin_max        = 0x08,
        };
        
        class xvaccountobj_t : public xobject_t,public xvaccount_t
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
            std::recursive_mutex&   get_lock();
            virtual bool            handle_event(const xvevent_t & ev) override;//add entry of handle event
            
            //the returned ptr has done add_ref before return,so release it when nolonger need manually
            xobject_t*              get_plugin(enum_xvaccount_plugin_type plugin);
            //return to indicate setup successful or not
            bool                    set_plugin(xobject_t * plugin_obj,enum_xvaccount_plugin_type plugin_type);
            
        private: //overwrite to protected mode for following api
            virtual bool            close(bool force_async = true) override;
          
            //the returned ptr has done add_ref before return,so release it when nolonger need manually
            xobject_t*              get_plugin_unsafe(enum_xvaccount_plugin_type plugin_type);
            //return to indicate setup successful or not
            bool                    set_plugin_unsafe(xobject_t * plugin_obj,enum_xvaccount_plugin_type plugin_type);
            
        private:
            xvtable_t&          m_ref_table; //link to table
            //note: only support max 8 plugins for one object as considering size and reality
            xobject_t*          m_plugins[enum_xvaccount_plugin_max];
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
        public:
            //return raw ptr that has been add_ref,caller need manually release it
            //but it is multiple thread safe
            xvaccountobj_t*            get_account_unsafe(const std::string & account_address);
            xauto_ptr<xvaccountobj_t>  get_account(const std::string & account_address);
            xauto_ptr<xvaccountobj_t>  get_account(const xvaccount_t & account_obj){return get_account(account_obj.get_address());}
            
            xauto_ptr<xobject_t>       get_account_plugin(const std::string & account_address,enum_xvaccount_plugin_type plugin_type);
            //return raw ptr that has been add_ref,caller need manually release it
            xobject_t*                 get_account_plugin_unsafe(const std::string & account_address,enum_xvaccount_plugin_type plugin_type);
            
            //return to indicate setup successful or not
            bool                       set_account_plugin(const std::string & account_address,xobject_t * new_plugin_obj,enum_xvaccount_plugin_type plugin_type);
            
            bool                       close_account(const std::string & account_address);

            inline const uint64_t      get_table_index() const {return m_table_index;}
            inline const uint32_t      get_table_combine_addr() const {return m_table_combine_addr;}
            
            inline std::recursive_mutex&  get_lock() {return m_lock;}
        private:
            //param of force_clean indicate whether force to close valid account 
            virtual bool               clean_all(bool force_clean = false);//clean all accounts & but table self still ok to use
            
        #ifdef DEBUG //debug only purpose
            virtual int32_t   add_ref() override;
            virtual int32_t   release_ref() override;
        #endif
        private:
            std::recursive_mutex   m_lock;
            uint64_t               m_table_index;         //define uint64_t just for performance
            std::map<std::string,xvaccountobj_t*> m_accounts;
            uint32_t               m_table_combine_addr; //[ledgerid:16bit][book:7bit][table:3bit]
        };
        
        //each book manage 8 tables
        class xvbook_t : public xionode_t
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
            xvtable_t*              get_table(const xvid_t & account_id);//return raw ptr as perforamnce
            const uint64_t          get_book_index() const {return m_book_index;}//return uint64_t just for performance
            const uint32_t          get_book_combine_addr() const {return m_book_combine_addr;}//combined address
            
        protected://internal or subcalss use only
            //param of force_clean indicate whether force to close valid account
            virtual bool            clean_all(bool force_clean = false); //just clean all accounts but table object is not release
            virtual xvtable_t*      create_table_object(const uint32_t table_index);//give default implementation
        private:
            std::recursive_mutex    m_lock;
            uint64_t                m_book_index;         //define uint64_t just for performance
        protected:
            xvtable_t*   m_tables[enum_vbook_has_tables_count];
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
            //the returned ptr has done add_ref before return,so release it when nolonger need manually
            //but it is multiple thread safe
            xvaccountobj_t*             get_account_unsafe(const std::string & account_address);
            xvaccountobj_t*             get_account_unsafe(const xvaccount_t & account_obj);
            
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
            xvbook_t*   m_books[enum_vbucket_has_books_count];
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
                enum_xvchain_plugin_txs_store     = 0x03, //manage all transactions
                enum_xvchain_plugin_event_mbus    = 0x04, //manage mbus 
                enum_xvchain_plugin_type_end      = 0x04,
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
            //the returned ptr has done add_ref before return,so release it when nolonger need manually
            //it is multiple thread safe
            xvaccountobj_t*             get_account_unsafe(const std::string & account_address);
            xvaccountobj_t*             get_account_unsafe(const xvaccount_t & account_obj);
            
            inline const int            get_chain_id()     const {return (int)m_chain_id;}
            inline const int            get_network_id()   const {return (int)m_chain_id;}
            
        public://note:each bucket/ledger may have own db and blockstore etc
            xvdbstore_t*                get_xdbstore(); //global shared db instance
            xvtxstore_t*                get_xtxstore();   //global shared xvtxstore_t instance
            xvblockstore_t*             get_xblockstore();//global shared blockstore instance
            xvstatestore_t*             get_xstatestore();//global shared statestore instance
            xveventbus_t*               get_xevmbus(); //global mbus object
        public:
            bool                        set_xdbstore(xvdbstore_t * new_store);//set global shared instance
            bool                        set_xtxstore(xvtxstore_t * new_store);
            bool                        set_xblockstore(xvblockstore_t * new_store);//set global shared instance
            bool                        set_xstatestore(xvstatestore_t* new_sotre);
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
        };
        ///////////////////////provide general structure for xledger and related //////////////////
    
    }//end of namespace of base
}//end of namespace top
