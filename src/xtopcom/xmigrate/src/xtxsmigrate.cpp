// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxsmigrate.h"

namespace top
{
    namespace base
    {
        xtxsmigrate_t::xtxsmigrate_t()
        {
            xkinfo("xtxsmigrate_t::xtxsmigrate_t");
        }
    
        xtxsmigrate_t:: ~xtxsmigrate_t()
        {
            xkinfo("xtxsmigrate_t::destroyed");
        }
        
        bool  xtxsmigrate_t::is_valid(const uint32_t obj_ver) //check version
        {
            return xtxsfilter_t::is_valid(obj_ver);
        }
    
        int   xtxsmigrate_t::init(const xvconfig_t & config_obj)
        {
            return enum_xcode_successful;
        }
    
        bool  xtxsmigrate_t::close(bool force_async)//close filter
        {
            xkinfo("xtxsmigrate_t::close");
            if(is_close() == false)
            {
                xtxsfilter_t::close(force_async); //mark closed flag first
                //XTODO,add own clean logic
            }
            xkinfo("xtxsmigrate_t::close,finished");
            return true;
        }
        
        //caller respond to cast (void*) to related  interface ptr
        void*  xtxsmigrate_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_sys_object_type_filter)
                return this;
            
            return xtxsfilter_t::query_interface(_enum_xobject_type_);
        }
    
        enum_xfilter_handle_code xtxsmigrate_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            if(is_close())
            {
                xwarn("xtxsmigrate_t::transfer_keyvalue,closed");
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
            return enum_xfilter_handle_code_ignore;
        }
    
        enum_xfilter_handle_code xtxsmigrate_t::transfer_tx(xdbevent_t & event,xvfilter_t* last_filter)
        {
            if(is_close())
            {
                xwarn("xtxsmigrate_t::transfer_keyvalue,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            //XTODO add code to migrate tx
            return enum_xfilter_handle_code_success;
        }
            
    }//end of namespace of base
}//end of namespace top
