// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xkeymigrate.h"

namespace top
{
    namespace base
    {
        xkeymigrate_t::xkeymigrate_t()
        {
            xkinfo("xkeymigrate_t::xkeymigrate_t");
        }
    
        xkeymigrate_t:: ~xkeymigrate_t()
        {
            xkinfo("xkeymigrate_t::destroyed");
        }
        
        bool  xkeymigrate_t::is_valid(const uint32_t obj_ver) //check version
        {
            return xkeyvfilter_t::is_valid(obj_ver);
        }
    
        int   xkeymigrate_t::init(const xvconfig_t & config_obj)
        {
            return enum_xcode_successful;
        }
    
        bool  xkeymigrate_t::close(bool force_async)//close filter
        {
            xkinfo("xkeymigrate_t::close");
            if(is_close() == false)
            {
                xkeyvfilter_t::close(force_async); //mark closed flag first
                //XTODO,add own clean logic
            }
            xkinfo("xkeymigrate_t::close,finished");
            return true;
        }
    
        //caller respond to cast (void*) to related  interface ptr
        void*  xkeymigrate_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_sys_object_type_filter)
                return this;
            
            return xkeyvfilter_t::query_interface(_enum_xobject_type_);
        }
    
        enum_xfilter_handle_code  xkeymigrate_t::fire_event(const xvevent_t & event,xvfilter_t* last_filter)
        {
            if(is_close())
            {
                xwarn("xkeymigrate_t::fire_event,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(event.get_event_category() != enum_xevent_category_db)
            {
                xerror("xkeymigrate_t::fire_event,bad event category for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }
            
            //logicly split full_event_type = [8bit:Event_Category][8bit:Event_KEY]
            //Event_KEY = [4bit:code][4bit:key_type]
            uint8_t event_key = event.get_event_key();
            if( (event_key & enum_xdbevent_code_transfer) != 0)
            {
                //force to convert key_type to enum_xdbkey_type_keyvalue,while migrating
                event_key = (event_key & 0xF0) | enum_xdbkey_type_keyvalue;
            }
            if(event_key >= enum_max_event_keys_count)
            {
                xerror("xkeymigrate_t::fire_event,bad event id for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }
            
            xevent_handler * target_handler = get_event_handlers()[event_key];
            if(target_handler != nullptr)
                return (*target_handler)(event,last_filter);
            
            return enum_xfilter_handle_code_ignore;
        }
    
        enum_xfilter_handle_code xkeymigrate_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            if(is_close())
            {
                xwarn("xkeymigrate_t::transfer_keyvalue,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(get_object_version() > 0)
            {
                //XTODO add code for specific version
            }
            
            if(event.check_event_flag(xdbevent_t::enum_dbevent_flag_key_stored) == false)
            {
                if(event.get_target_store() != nullptr)
                {
                    if(event.get_target_store()->set_value(event.get_db_key(), event.get_db_value()))
                        event.set_event_flag(xdbevent_t::enum_dbevent_flag_key_stored);
                }
            }
            return enum_xfilter_handle_code_success;
        }
            
    }//end of namespace of base
}//end of namespace top
