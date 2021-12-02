// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xblkmigrate.h"

namespace top
{
    namespace base
    {
        xblkmigrate_t::xblkmigrate_t()
        {
            xkinfo("xblkmigrate_t::xblkmigrate_t");
        }
    
        xblkmigrate_t:: ~xblkmigrate_t()
        {
            xkinfo("xblkmigrate_t::destroyed");
        }
        
        bool  xblkmigrate_t::is_valid(const uint32_t obj_ver) //check version
        {
            return  xblkfilter_t::is_valid(obj_ver);
        }
    
        int   xblkmigrate_t::init(const xvconfig_t & config_obj)
        {
            return enum_xcode_successful;
        }
    
        bool  xblkmigrate_t::close(bool force_async)//close filter
        {
            xkinfo("xblkmigrate_t::close");
            if(is_close() == false)
            {
                xblkfilter_t::close(force_async); //mark closed flag first
                //XTODO,add own clean logic
            }
            xkinfo("xblkmigrate_t::close,finished");
            return true;
        }
    
        //caller respond to cast (void*) to related  interface ptr
        void*  xblkmigrate_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_sys_object_type_filter)
                return this;
            
            return xblkfilter_t::query_interface(_enum_xobject_type_);
        }
    
        enum_xfilter_handle_code xblkmigrate_t::transfer_keyvalue(xdbevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xblkmigrate_t::transfer_keyvalue for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xblkmigrate_t::transfer_keyvalue,closed");
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
                    xdbg("xblkmigrate_t::transfer_keyvalue set_value_to_dst_db with no changed. key=%s,for event(0x%x)",event.get_db_key().c_str(), event.get_type());
                    if(event.get_target_store()->set_value(event.get_db_key(), event.get_db_value()))
                        event.set_event_flag(xdbevent_t::enum_dbevent_flag_key_stored);
                }
            }
            return enum_xfilter_handle_code_ignore;
        }
    
        enum_xfilter_handle_code    xblkmigrate_t::transfer_block_index(xdbevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xblkmigrate_t::transfer_block_index for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xblkmigrate_t::transfer_block_index,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(get_object_version() > 0)
            {
                //XTODO add code for specific version
            }
            return enum_xfilter_handle_code_success;
        }
        enum_xfilter_handle_code    xblkmigrate_t::transfer_block_object(xdbevent_t & event,xvfilter_t* last_filter)
        {
            xdbg("xblkmigrate_t::transfer_block_object for event(0x%x)",event.get_type());
            if(is_close())
            {
                xwarn("xblkmigrate_t::transfer_block_object,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            if(get_object_version() > 0)
            {
                //XTODO add code for specific version
            }
            return enum_xfilter_handle_code_success;
        }
            
    }//end of namespace of base
}//end of namespace top
