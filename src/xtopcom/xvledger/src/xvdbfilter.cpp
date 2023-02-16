// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include "xbase/xatom.h"
#include "xvdbstore.h"
#include "xvdbfilter.h"

namespace top
{
    namespace base
    {
        //*************************************xdbevent_t****************************************//
        xdbevent_t::xdbevent_t(xvdbstore_t* src_db_ptr,xvdbstore_t* dst_db_ptr,enum_xdbevent_code code)
            :xvevent_t(static_cast<uint16_t>(enum_xevent_category_db) | static_cast<uint16_t>(code) | static_cast<uint16_t>(enum_xdbkey_type_keyvalue))
        {
            m_db_key_type   = enum_xdbkey_type_keyvalue;
            m_src_store_ptr = src_db_ptr;
            m_dst_store_ptr = dst_db_ptr;
        }
    
        xdbevent_t::xdbevent_t(const std::string & db_key,const std::string & db_value,enum_xdbkey_type db_key_type,xvdbstore_t* src_db_ptr,xvdbstore_t* dst_db_ptr,enum_xdbevent_code code)
          : xvevent_t(static_cast<uint16_t>(enum_xevent_category_db) | static_cast<uint16_t>(code) | static_cast<uint16_t>(db_key_type))
        {
            m_db_key        = db_key;
            m_db_value      = db_value;
            m_db_key_type   = db_key_type;
            
            m_src_store_ptr = src_db_ptr;
            m_dst_store_ptr = dst_db_ptr;
        }
    
        xdbevent_t::~xdbevent_t()
        {
        }

        //*************************************xdbfilter_t****************************************//
        xdbfilter_t::xdbfilter_t()
        {
        }
    
        xdbfilter_t::xdbfilter_t(xdbfilter_t * front_filter)
            :xvfilter_t(front_filter)
        {
        }
        
        xdbfilter_t::xdbfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter)
            :xvfilter_t(front_filter,back_filter)
        {
        }
        
        xdbfilter_t::~xdbfilter_t()
        {
        }
    
        enum_xfilter_handle_code xdbfilter_t::fire_event(const xvevent_t & event,xvfilter_t* last_filter)
        {
            if(is_close())
            {
                xwarn("xdbfilter_t::fire_event,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(event.get_event_category() != enum_xevent_category_db)
            {
                xerror("xdbfilter_t::fire_event,bad event category for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }

            const uint8_t event_key = event.get_event_key();
            if(event_key >= enum_max_event_keys_count)
            {
                xerror("xdbfilter_t::fire_event,bad event id for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }
            
            xevent_handler * target_handler = get_event_handlers()[event_key];
            if(target_handler != nullptr)
                return (*target_handler)(event,last_filter);
            
            return enum_xfilter_handle_code_ignore;
        }
        
        //*************************************xkeyvfilter_t****************************************//
        xkeyvfilter_t::xkeyvfilter_t()
        {
            INIT_EVENTS_HANDLER();
        }
    
        xkeyvfilter_t::xkeyvfilter_t(xdbfilter_t * front_filter)
            :xdbfilter_t(front_filter)
        {
            INIT_EVENTS_HANDLER();
        }
    
        xkeyvfilter_t::xkeyvfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter)
            :xdbfilter_t(front_filter,back_filter)
        {
            INIT_EVENTS_HANDLER();
        }
    
        xkeyvfilter_t::~xkeyvfilter_t()
        {
        }
    
        enum_xfilter_handle_code xkeyvfilter_t::on_keyvalue_transfer(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbevent_t* db_event_ptr = (xdbevent_t*)&event;
            
            enum_xfilter_handle_code result = enum_xfilter_handle_code_ignore;
            if(db_event_ptr->check_event_flag(xdbevent_t::enum_dbevent_flag_key_migrated) == false)
            {
                result = transfer_keyvalue(*db_event_ptr,last_filter);//transfer key first if need
                if( (enum_xfilter_handle_code_success == result) || (enum_xfilter_handle_code_finish == result) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_key_migrated);
            }
            return result;
        }
    
        enum_xfilter_handle_code xkeyvfilter_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            return enum_xfilter_handle_code_ignore;
        }
 
        //*************************************xbksfilter_t****************************************//
        xblkfilter_t::xblkfilter_t()
        {
            INIT_EVENTS_HANDLER();
        }
    
        xblkfilter_t::xblkfilter_t(xdbfilter_t * front_filter)
            :xdbfilter_t(front_filter)
        {
            INIT_EVENTS_HANDLER();
        }
        
        xblkfilter_t::xblkfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter)
            :xdbfilter_t(front_filter,back_filter)
        {
            INIT_EVENTS_HANDLER();
        }
        
        xblkfilter_t::~xblkfilter_t()
        {
        }
    
        enum_xfilter_handle_code xblkfilter_t::on_keyvalue_transfer(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbevent_t* db_event_ptr = (xdbevent_t*)&event;
            
            //transfer key first if need
            enum_xfilter_handle_code result = enum_xfilter_handle_code_ignore;
            if(db_event_ptr->check_event_flag(xdbevent_t::enum_dbevent_flag_key_migrated) == false)
            {
                result = transfer_keyvalue(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == result) || (enum_xfilter_handle_code_finish == result) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_key_migrated);
            }

            //then check whether need do more deeper handle
            if(db_event_ptr->get_db_key_type() == enum_xdbkey_type_block_index)
            {
                const int subres = transfer_block_index(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == subres) || (enum_xfilter_handle_code_finish == subres) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_block_index_migrated);
            }
            else if(db_event_ptr->get_db_key_type() == enum_xdbkey_type_block_object)
            {
                const int subres = transfer_block_object(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == subres) || (enum_xfilter_handle_code_finish == subres) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_block_object_migrated);
            }
            return result;
        }
    
        enum_xfilter_handle_code xblkfilter_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            return enum_xfilter_handle_code_ignore;
        }
    
        enum_xfilter_handle_code xblkfilter_t::on_block_index_transfer(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbevent_t* db_event_ptr = (xdbevent_t*)&event;
 
            enum_xfilter_handle_code result = enum_xfilter_handle_code_ignore;
            if(db_event_ptr->check_event_flag(xdbevent_t::enum_dbevent_flag_block_index_migrated) == false)
            {
                result = transfer_block_index(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == result) || (enum_xfilter_handle_code_finish == result) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_block_index_migrated);
            }
            return result;
        }
    
        enum_xfilter_handle_code xblkfilter_t::transfer_block_index(xdbevent_t & event,xvfilter_t* last_filter)
        {
            return enum_xfilter_handle_code_ignore;
        }
 
        enum_xfilter_handle_code xblkfilter_t::on_block_object_transfer(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbevent_t* db_event_ptr = (xdbevent_t*)&event;
           
            enum_xfilter_handle_code result = enum_xfilter_handle_code_ignore;
            if(db_event_ptr->check_event_flag(xdbevent_t::enum_dbevent_flag_block_object_migrated) == false)
            {
                result = transfer_block_object(*db_event_ptr,last_filter);//then transfer block object
                if( (enum_xfilter_handle_code_success == result) || (enum_xfilter_handle_code_finish == result) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_block_object_migrated);
            }
            return result;
        }
    
        enum_xfilter_handle_code   xblkfilter_t::transfer_block_object(xdbevent_t & event,xvfilter_t* last_filter)
        {
            return enum_xfilter_handle_code_ignore;
        }
    
        //*************************************xtxsfilter_t****************************************//
        xtxsfilter_t::xtxsfilter_t()
        {
            INIT_EVENTS_HANDLER();
        }
    
        xtxsfilter_t::xtxsfilter_t(xdbfilter_t * front_filter)
            :xdbfilter_t(front_filter)
        {
            INIT_EVENTS_HANDLER();
        }
        
        xtxsfilter_t::xtxsfilter_t(xdbfilter_t * front_filter,xdbfilter_t * back_filter)
            :xdbfilter_t(front_filter,back_filter)
        {
            INIT_EVENTS_HANDLER();
        }
        
        xtxsfilter_t::~xtxsfilter_t()
        {
        }
    
        enum_xfilter_handle_code xtxsfilter_t::on_keyvalue_transfer(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbevent_t* db_event_ptr = (xdbevent_t*)&event;
            //transfer key first if need
            enum_xfilter_handle_code result = enum_xfilter_handle_code_ignore;
            if(db_event_ptr->check_event_flag(xdbevent_t::enum_dbevent_flag_key_migrated) == false)
            {
                result = transfer_keyvalue(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == result) || (enum_xfilter_handle_code_finish == result) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_key_migrated);
            }

            //then check whether need do more deeper handle
            if(db_event_ptr->get_db_key_type() == enum_xdbkey_type_transaction)
            {
                const int subres = transfer_tx(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == subres) || (enum_xfilter_handle_code_finish == subres) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_txs_migrated);
            }
            return result;
        }
    
        enum_xfilter_handle_code  xtxsfilter_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            return enum_xfilter_handle_code_ignore;
        }
        
        enum_xfilter_handle_code xtxsfilter_t::on_tx_transfer(const xvevent_t & event,xvfilter_t* last_filter)
        {
            xdbevent_t* db_event_ptr = (xdbevent_t*)&event;
           
            enum_xfilter_handle_code result = enum_xfilter_handle_code_ignore;
            if(db_event_ptr->check_event_flag(xdbevent_t::enum_dbevent_flag_txs_migrated) == false)
            {
                result = transfer_tx(*db_event_ptr,last_filter);
                if( (enum_xfilter_handle_code_success == result) || (enum_xfilter_handle_code_finish == result) )
                    db_event_ptr->set_event_flag(xdbevent_t::enum_dbevent_flag_txs_migrated);
            }
            return result;
        }
    
        enum_xfilter_handle_code   xtxsfilter_t::transfer_tx(xdbevent_t & event,xvfilter_t* last_filter)
        {
            return enum_xfilter_handle_code_ignore;
        }
 
    }//end of namespace of base
}//end of namespace top
