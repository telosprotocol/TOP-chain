// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <vector>
#include "xbase/xatom.h"
#include "xvfilter.h"

namespace top
{
    namespace base
    {
        //*************************************xvfilter_t****************************************//
        xvfilter_t::xvfilter_t()
            : xsysobject_t(enum_sys_object_type_filter)
        {
            m_front_filter = nullptr;
            m_back_filter  = nullptr;
            memset(m_event_handlers, 0, sizeof(m_event_handlers));
            
            xkinfo("xvfilter_t::xvfilter_t");
        }
    
        xvfilter_t::xvfilter_t(xvfilter_t * front_filter)
          : xsysobject_t(enum_sys_object_type_filter)
        {
            m_front_filter = nullptr;
            m_back_filter  = nullptr;
            memset(m_event_handlers, 0, sizeof(m_event_handlers));
            
            m_front_filter = front_filter;
            if(front_filter != nullptr)
                front_filter->add_ref();
            
            xkinfo("xvfilter_t::xvfilter_t");
        }
    
        xvfilter_t::xvfilter_t(xvfilter_t * front_filter,xvfilter_t * back_filter)
            : xsysobject_t(enum_sys_object_type_filter)
        {
            m_front_filter = nullptr;
            m_back_filter  = nullptr;
            memset(m_event_handlers, 0, sizeof(m_event_handlers));
            
            m_front_filter = front_filter;
            m_back_filter  = back_filter;
            
            if(front_filter != nullptr)
                front_filter->add_ref();
            
            if(back_filter != nullptr)
                back_filter->add_ref();
            
            xkinfo("xvfilter_t::xvfilter_t");
        }
    
        xvfilter_t::~xvfilter_t()
        {
            xkinfo("xvfilter_t::destroyed");
            
            if(m_front_filter != nullptr)
                m_front_filter->release_ref();
            
            if(m_back_filter != nullptr)
                m_back_filter->release_ref();
            
            for(int i = 0; i < enum_max_event_keys_count; ++i)
            {
                xevent_handler * handler = m_event_handlers[i];
                if(handler != nullptr)
                    delete handler;
            }
        }
    
        bool xvfilter_t::close(bool force_async)//must call close before release
        {
            xkinfo("xvfilter_t::close");
            if(is_close() == false)
            {
                xsysobject_t::close(force_async); //mark closed first
                
                xvfilter_t*old_front_ptr = xatomic_t::xexchange(m_front_filter, (xvfilter_t*)NULL);
                if(old_front_ptr != nullptr)
                {
                    old_front_ptr->release_ref();
                }
                
                xvfilter_t*old_back_ptr = xatomic_t::xexchange(m_back_filter, (xvfilter_t*)NULL);
                if(old_back_ptr != nullptr)
                {
                    old_back_ptr->close(force_async);
                    old_back_ptr->release_ref();
                }
            }
            xkinfo("xvfilter_t::close,finished");
            return true;
        }
    
        //caller respond to cast (void*) to related  interface ptr
        void*  xvfilter_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_sys_object_type_filter)
                return this;
            
            return xsysobject_t::query_interface(_enum_xobject_type_);
        }
    
        bool xvfilter_t::reset_front_filter(xvfilter_t * front_filter)
        {
            if(is_close())
                return false;
            
            if(front_filter != nullptr)
            {
                if(m_front_filter != nullptr)//not allow overwrite exiting one
                {
                    xassert(m_front_filter == nullptr);
                    return false;
                }
                front_filter->add_ref();
            }

            xvfilter_t*old_ptr = xatomic_t::xexchange(m_front_filter, front_filter);
            if(old_ptr != nullptr)
                old_ptr->release_ref();
            
            return true;
        }
    
        bool xvfilter_t::reset_back_filter(xvfilter_t * back_filter)
        {
            if(is_close())
                return false;
            
            if(back_filter != nullptr)
            {
                if(m_back_filter != nullptr)//not allow overwrite exiting one
                {
                    xassert(m_back_filter == nullptr);
                    return false;
                }
                back_filter->add_ref();
            }

            xvfilter_t*old_ptr = xatomic_t::xexchange(m_back_filter, back_filter);
            if(old_ptr != nullptr)
                old_ptr->release_ref();
            
            return true;
        }
    
        enum_xfilter_handle_code  xvfilter_t::push_event_back(const xvevent_t & event,xvfilter_t* last_filter)//throw event from prev front to back
        {
            if(is_close())
                return enum_xfilter_handle_code_closed;
            
            const enum_xfilter_handle_code result = fire_event(event,last_filter);
            if(result < enum_xfilter_handle_code_success)
            {
                xwarn("xvfilter_t::push_event_back,error(%d) of executed event(0x%x)",result,event.get_type());
                return result;
            }
            else if(result == enum_xfilter_handle_code_finish)
            {
                xdbg("xvfilter_t::push_event_back,finish to execute event(0x%x)",result,event.get_type());
                return result; //stop and return result
            }
            
            if(m_back_filter != nullptr)//continue run to next(back) filter
                return m_back_filter->push_event_back(event,this);
            
            return enum_xfilter_handle_code_finish;
        }
        
        enum_xfilter_handle_code  xvfilter_t::push_event_front(const xvevent_t & event,xvfilter_t* last_filter)//push event from back to front
        {
            if(is_close())
                return enum_xfilter_handle_code_closed;
            
            const enum_xfilter_handle_code result = fire_event(event,last_filter);
            if(result < enum_xfilter_handle_code_success)
            {
                xwarn("xvfilter_t::push_event_front,error(%d) of executed event(0x%x)",result,event.get_type());
                return result;
            }
            else if(result == enum_xfilter_handle_code_finish)
            {
                xdbg("xvfilter_t::push_event_front,finish to execute event(0x%x)",result,event.get_type());
                return result; //stop and return result
            }
            
            if(m_front_filter != nullptr) //continue to prev(front) filter
                return m_front_filter->push_event_front(event,this);
            
            return enum_xfilter_handle_code_finish;
        }
        
        enum_xfilter_handle_code xvfilter_t::fire_event(const xvevent_t & event,xvfilter_t* last_filter)
        {
            if(is_close())
            {
                xwarn("xvfilter_t::fire_event,closed");
                return enum_xfilter_handle_code_closed;
            }
            
            const uint8_t event_key = event.get_event_key();
            if(event_key >= enum_max_event_keys_count)
            {
                xerror("xvfilter_t::fire_event,bad event id for event(0x%x)",event.get_type());
                return enum_xfilter_handle_code_bad_type;
            }
            
            xevent_handler * target_handler = m_event_handlers[event_key];
            if(target_handler != nullptr)
                return (*target_handler)(event,last_filter);
            
            return enum_xfilter_handle_code_ignore;
        }
    
        bool   xvfilter_t::register_event(const uint16_t full_event_key,const xevent_handler & api_function)
        {
            const uint8_t event_key = xvevent_t::get_event_key(full_event_key);
            if(event_key >= enum_max_event_keys_count)
            {
                xerror("xvfilter_t::register_event,bad event id over max value,event(0x%x)",full_event_key);
                return false;
            }
            if(m_event_handlers[event_key] != nullptr)
            {
                xerror("xvfilter_t::register_event,repeat register the event(0x%x)",full_event_key);
                return false;
            }
            
            xevent_handler * handler = new xevent_handler(api_function);
            m_event_handlers[event_key] = handler;
            return true;
        }
 
    }//end of namespace of base
}//end of namespace top
