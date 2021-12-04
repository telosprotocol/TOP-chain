// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xvledger/xvfilter.h"

namespace top
{
    namespace base
    {
        class xvdbstore_t; //forward delcare
        class xdbevent_t : public xvevent_t
        {
        public:
            enum enum_dbevent_flag : uint8_t
            {
                enum_dbevent_flag_key_migrated          = 0x01, //key has been done migrated
                enum_dbevent_flag_txs_migrated          = 0x02, //tx  has been done migrated
                enum_dbevent_flag_block_index_migrated  = 0x04, //block index  has been done migrated
                enum_dbevent_flag_block_object_migrated = 0x08, //block object has been done migrated
                
                enum_dbevent_flag_block_unpacked        = 0x10, //block(like table) has been unpacked
                enum_dbevent_flag_block_stored          = 0x20, //whole block has been stored to dst db
                enum_dbevent_flag_key_stored            = 0x40, //key has been stored to dst db
                enum_dbevent_flag_txs_stored            = 0x80, //tx has been stored to dst db
            };
        public:
            xdbevent_t(xvdbstore_t* src_db_ptr,xvdbstore_t* dst_db_ptr,enum_xdbevent_code code);
            xdbevent_t(const std::string & db_key,const std::string & db_value,enum_xdbkey_type db_key_type,xvdbstore_t* src_db_ptr,xvdbstore_t* dst_db_ptr,enum_xdbevent_code code);
            virtual ~xdbevent_t();
        private:
            xdbevent_t();
            xdbevent_t(xdbevent_t &&);
            xdbevent_t(const xdbevent_t & obj);
            xdbevent_t& operator = (const xdbevent_t & obj);
            
        public:
            //logicly split full_event_type = [8bit:Event_Category][8bit:Event_KEY]
            //Event_KEY = [4bit:event_code][4bit:key_type]
            //event_code refer enum_xdbevent_code
            static const int get_event_code(const int full_type) {return (full_type & 0xF0);}//4bit
            const int        get_event_code()     const {return ( get_type() & 0xF0);} //4bit
            
            inline const std::string &  get_db_key()   const {return m_db_key;}
            inline const std::string &  get_db_value() const {return m_db_value;}
            inline enum_xdbkey_type     get_db_key_type() const{return m_db_key_type;}
            inline xvdbstore_t*         get_db_store() const {return m_src_store_ptr;}
            inline xvdbstore_t*         get_source_store() const {return m_src_store_ptr;}
            inline xvdbstore_t*         get_target_store() const {return m_dst_store_ptr;}
            
        public: //for performance,let xdb operate db key & value directly
            inline std::string &        get_set_db_key()    {return m_db_key;}
            inline std::string &        get_set_db_value()  {return m_db_value;}
            inline enum_xdbkey_type&    get_set_db_type()   {return m_db_key_type;}
        private:
            xvdbstore_t*        m_src_store_ptr; //note:just copy ptr without reference control
            xvdbstore_t*        m_dst_store_ptr; //note:just copy ptr without reference control
            
            std::string         m_db_key;   //readed from m_src_store_ptr
            std::string         m_db_value; //readed from m_src_store_ptr
            enum_xdbkey_type    m_db_key_type;
        };
    
        class xdbfilter_t : public xvfilter_t
        {
        protected:
            xdbfilter_t();
            xdbfilter_t(xdbfilter_t * front_filter);
            xdbfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter);
            virtual ~xdbfilter_t();
        private:
            xdbfilter_t(xdbfilter_t &&);
            xdbfilter_t(const xdbfilter_t &);
            xdbfilter_t & operator = (const xdbfilter_t &);
            
        protected: //triggered by push_event_back or push_event_front
            virtual enum_xfilter_handle_code fire_event(const xvevent_t & event,xvfilter_t* last_filter) override;
            using xvfilter_t::get_event_handlers;
        };
    
        //generaly handle any key-value of DB
        class xkeyvfilter_t : public xdbfilter_t
        {
        protected:
            xkeyvfilter_t();
            xkeyvfilter_t(xdbfilter_t * front_filter);
            xkeyvfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter);
            virtual ~xkeyvfilter_t();
        private:
            xkeyvfilter_t(xkeyvfilter_t &&);
            xkeyvfilter_t(const xkeyvfilter_t &);
            xkeyvfilter_t & operator = (const xkeyvfilter_t &);
            
        protected:
            virtual enum_xfilter_handle_code  transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter);
            
        private:
            enum_xfilter_handle_code on_keyvalue_transfer(const xvevent_t & event,xvfilter_t* last_filter);
        private:
            BEGIN_DECLARE_EVENT_HANDLER()
                REGISTER_EVENT(enum_xevent_category_db | enum_xdbevent_code_transfer | enum_xdbkey_type_keyvalue,on_keyvalue_transfer)
            END_DECLARE_EVENT_HANDLER()
        };
    
        //handle block
        class xblkfilter_t : public xdbfilter_t
        {
        protected:
            xblkfilter_t();
            xblkfilter_t(xdbfilter_t * front_filter);
            xblkfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter);
            virtual ~xblkfilter_t();
        private:
            xblkfilter_t(xblkfilter_t &&);
            xblkfilter_t(const xblkfilter_t &);
            xblkfilter_t & operator = (const xblkfilter_t &);
            
        protected:
            virtual enum_xfilter_handle_code    transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter);
            virtual enum_xfilter_handle_code    transfer_block_index(xdbevent_t & event,xvfilter_t* last_filter);
            virtual enum_xfilter_handle_code    transfer_block_object(xdbevent_t & event,xvfilter_t* last_filter);
            
        private:
            enum_xfilter_handle_code on_keyvalue_transfer(const xvevent_t & event,xvfilter_t* last_filter);
            enum_xfilter_handle_code on_block_index_transfer(const xvevent_t & event,xvfilter_t* last_filter);
            enum_xfilter_handle_code on_block_object_transfer(const xvevent_t & event,xvfilter_t* last_filter);
        private:
            BEGIN_DECLARE_EVENT_HANDLER()
                REGISTER_EVENT(enum_xevent_category_db | enum_xdbevent_code_transfer | enum_xdbkey_type_keyvalue,on_keyvalue_transfer)
            
                REGISTER_EVENT(enum_xevent_category_db | enum_xdbevent_code_transfer | enum_xdbkey_type_block_index,on_block_index_transfer)
 
                REGISTER_EVENT(enum_xevent_category_db | enum_xdbevent_code_transfer | enum_xdbkey_type_block_object,on_block_object_transfer)
            END_DECLARE_EVENT_HANDLER()
        };
    
        //generaly handle any transaction
        class xtxsfilter_t : public xdbfilter_t
        {
        protected:
            xtxsfilter_t();
            xtxsfilter_t(xdbfilter_t * front_filter);
            xtxsfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter);
            virtual ~xtxsfilter_t();
        private:
            xtxsfilter_t(xtxsfilter_t &&);
            xtxsfilter_t(const xtxsfilter_t &);
            xtxsfilter_t & operator = (const xtxsfilter_t &);
           
        protected:
            virtual enum_xfilter_handle_code    transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter);
            virtual enum_xfilter_handle_code    transfer_tx(xdbevent_t & event,xvfilter_t* last_filter);
            
        private:
            enum_xfilter_handle_code on_keyvalue_transfer(const xvevent_t & event,xvfilter_t* last_filter);
            enum_xfilter_handle_code on_tx_transfer(const xvevent_t & event,xvfilter_t* last_filter);
        private:
            BEGIN_DECLARE_EVENT_HANDLER()
                REGISTER_EVENT(enum_xevent_category_db | enum_xdbevent_code_transfer | enum_xdbkey_type_keyvalue,on_keyvalue_transfer)
 
                REGISTER_EVENT(enum_xevent_category_db | enum_xdbevent_code_transfer | enum_xdbkey_type_transaction ,on_tx_transfer)
            END_DECLARE_EVENT_HANDLER()
        };

    }//end of namespace of base
}//end of namespace top
