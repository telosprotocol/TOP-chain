// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "../xvinstruction.h"
#include "../xvaction.h"

namespace top
{
    namespace base
    {
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name)
        {
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name,xvalue_t & param)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name,param)
        {
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name,xvalue_t & param1,xvalue_t & param2)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name,param1,param2)
        {
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name,xvalue_t & param1,xvalue_t & param2,xvalue_t & param3)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name,param1,param2,param3)
        {
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t()
            :xvmethod_t()
        {
        }
    
        xvaction_t::~xvaction_t()
        {
            close();
        }

        void xvaction_t::close()
        {
        }
    
        xvaction_t::xvaction_t(const xvaction_t & obj)
            :xvmethod_t(obj)
        {
            m_org_tx_hash = obj.m_org_tx_hash;
            parse_uri();
        }
    
        xvaction_t::xvaction_t(xvaction_t && moved)
            :xvmethod_t(moved)
        {
            m_org_tx_hash = moved.m_org_tx_hash;
            parse_uri();
        }
    
        xvaction_t & xvaction_t::operator = (const xvaction_t & obj)
        {
            close(); //close first
            
            xvmethod_t::operator=(obj);
            m_org_tx_hash = obj.m_org_tx_hash;
            parse_uri();
            return *this;
        }
    
        void xvaction_t::parse_uri()
        {
        }
    
        //caller respond to cast (void*) to related  interface ptr
        void*    xvaction_t::query_minterface(const int32_t _enum_xobject_type_) const
        {
            if(_enum_xobject_type_ == enum_xobject_type_vaction)
                return (xvaction_t*)this;
            
            return xvmethod_t::query_minterface(_enum_xobject_type_);
        }
    
        int32_t xvaction_t::do_write(xstream_t & stream) const//serialize header and object,return how many bytes is writed
        {
            stream.write_tiny_string(m_org_tx_hash);
            return xvmethod_t::do_write(stream);
        }
    
        int32_t xvaction_t::do_read(xstream_t & stream)      //serialize header and object,return how many bytes is readed
        {
            close();
            stream.read_tiny_string(m_org_tx_hash);
            const int result = xvmethod_t::do_read(stream);
            parse_uri();
            xassert(result > 0);
            return result;
        }
    
    };//end of namespace of base
};//end of namespace of top
