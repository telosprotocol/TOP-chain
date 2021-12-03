// Copyright (c) 2018-Present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include "xvledger/xvsysobj.h"
#include "xvledger/xvdbkey.h"

namespace top
{
    namespace base
    {
        enum enum_xfilter_handle_code
        {
            enum_xfilter_handle_code_bad_type       = -4, //invalid event types
            enum_xfilter_handle_code_closed         = -3, //filter has been closed
            enum_xfilter_handle_code_interrupt      = -2, //stop pass event
            enum_xfilter_handle_code_error          = -1, //unknow error
            enum_xfilter_handle_code_success        = 0,  //success handle one
            enum_xfilter_handle_code_ignore         = 1,  //not implement,or not process
            enum_xfilter_handle_code_finish         = 2,  //finish everything
        };
    
        //[8bit:category][4bit:op_code][4bit:key_type]
        //get_event_key = [4bit:op_code][4bit:key_type]
        enum enum_xevent_category 
        {
            enum_xevent_category_db             = 0x0100, //DB
        };
        enum enum_xdbevent_code
        {
            //enum_max_event_keys_count = (enum_xdbevent_code_max >> 4) * (enum_xdbkey_type_max + 1)
            enum_xdbevent_code_transfer         = 0x0010, //transfer src DB to dest DB
            enum_xdbevent_code_max              = 0x0040, //not over this max value
        };

        //filter-chain  [filter]<->[filter]<->[filter]<->[filter]<->[filter]
        //event-flow:   push_event_back->[filter]->[filter]->[filter]
        //event-flow:   [filter]<-[filter]<-[filter]<-push_event_front
        class xvfilter_t : public xsysobject_t
        {
        protected:
            typedef std::function< enum_xfilter_handle_code (const xvevent_t & event,xvfilter_t* last_filter) > xevent_handler;
            //just register by object_key,instead of object_type
            static const int16_t get_register_type()
            {
                return xsysobject_t::enum_sys_object_type_undef;//not register type
            }
            enum
            {
                enum_max_event_keys_count   = ((enum_xdbevent_code_max >> 4) * (enum_xdbkey_type_max + 1)),
            };
        protected:
            xvfilter_t();
            xvfilter_t(xvfilter_t * front_filter);
            xvfilter_t(xvfilter_t * front_filter,xvfilter_t * back_filter);
            virtual ~xvfilter_t();
        private:
            xvfilter_t(const xvfilter_t &);
            xvfilter_t & operator = (const xvfilter_t &);
            
        public:
            //throw event from prev front to back
            enum_xfilter_handle_code    push_event_back(const xvevent_t & event,xvfilter_t* last_filter);
            //push event from back to front
            enum_xfilter_handle_code    push_event_front(const xvevent_t & event,xvfilter_t* last_filter);
            
            bool            reset_front_filter(xvfilter_t * front_filter);
            bool            reset_back_filter(xvfilter_t * front_filter);
            virtual bool    close(bool force_async = false) override; //must call close before release
            //caller respond to cast (void*) to related  interface ptr
            virtual void*   query_interface(const int32_t _enum_xobject_type_) override;
        protected:
            xvfilter_t*     get_front() const {return m_front_filter;}
            xvfilter_t*     get_back()  const {return m_back_filter;}
            
            bool            register_event(const uint16_t full_event_key,const xevent_handler & api_function);
            inline xevent_handler**get_event_handlers() {return m_event_handlers;}
            
        private: //triggered by push_event_back or push_event_front
            virtual enum_xfilter_handle_code fire_event(const xvevent_t & event,xvfilter_t* last_filter);
            
        private:
            xvfilter_t *    m_front_filter;
            xvfilter_t *    m_back_filter;
            xevent_handler* m_event_handlers[enum_max_event_keys_count];
        };
    
        //convenient macro to register register_event
        #define BEGIN_DECLARE_EVENT_HANDLER() template<typename _T> void register_event_internal(_T * pThis){
            #define REGISTER_EVENT(eventkey,entry) register_event((const uint8_t)eventkey,std::bind(&_T::entry,pThis,std::placeholders::_1,std::placeholders::_2));
        #define END_DECLARE_EVENT_HANDLER() }
    
        #define INIT_EVENTS_HANDLER() register_event_internal(this)
    
    }//end of namespace of base
}//end of namespace top
