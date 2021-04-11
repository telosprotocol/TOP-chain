// Copyright (c) 2018-2020 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject.h"
#include "xbase/xdata.h"
#include "xbase/xvblock.h"

namespace top
{
    namespace core
    {

        class xvstage_t;
    
    
        
        //mini-object with minimal overhead
        class xvmobj_t  : public base::xobject_t
        {
        private:
            uint16_t    m_length;  //max 65535 bytes as security limit
        };
        
        //each script produce one frame
        class xvframe_t : public xvmobj_t
        {
        };
        
        //state frame to present property or asset
        class xvstate_t : public xvframe_t
        {
        };
        
        //present the full state of property
        class xvfullstate_ : public xvstate_t
        {
        };
        
        //some frame could include script as output
        class xvaction_t : public xvframe_t
        {
        };
        
        //total 2 bit,max 4 script languages
        enum enum_xvscript_language :uint8_t
        {
            enum_xvscript_language_xkql      = 0,  //key-value query language(KQL, like SQL) ,work for DB-like
            enum_xvscript_language_xshell    = 1,  //TOP stack-based script lanauage
            enum_xvscript_language_xlua      = 2,  //standard lua script with extend instructions
            enum_xvscript_language_reserved  = 3,  //reserved for future
            
            enum_xvscript_language_max       = 4,  //never over this
        };
        //script(include one or multiple instructions)
        class xvscript_t : public xvmobj_t
        {
        public:
            virtual xvframe_t*      execute(xvstage_t & canvas) = 0;
            enum_xvscript_language  get_language() const {return m_language_type;}
        private:
            enum_xvscript_language  m_language_type;
        };
        
        //transaction is one of type for event
        class xvevent_t : public xvmobj_t
        {
        public:
            virtual const std::string   get_hash()        = 0;   //get hash of event
            virtual xvscript_t *        generate_script() = 0;  //generate action script
        };
        
        class xvinput_t : public base::xdataunit_t
        {
        private:
            std::vector<xvevent_t*> m_events;
        };
        
        //output of xvblock_t
        class xvoutput_t : public base::xdataunit_t
        {
        private:
            std::vector<xvframe_t*>  m_frames;
        };
        
     
    };//end of namespace of base
};//end of namespace top
