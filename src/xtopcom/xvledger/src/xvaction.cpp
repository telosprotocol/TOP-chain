// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "../xvinstruction.h"
#include "../xvaction.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = 0;
            m_max_tgas    = 0;
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name,xvalue_t & param)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name,param)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = 0;
            m_max_tgas    = 0;
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name,xvalue_t & param1,xvalue_t & param2)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name,param1,param2)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = 0;
            m_max_tgas    = 0;
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t(const std::string & tx_hash,const std::string & caller_addr,const std::string & target_uri,const std::string & method_name,xvalue_t & param1,xvalue_t & param2,xvalue_t & param3)
            :xvmethod_t(target_uri,enum_xvinstruct_class_contract_function,method_name,param1,param2,param3)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = 0;
            m_max_tgas    = 0;
            m_org_tx_hash = tx_hash;
            set_caller_uri(caller_addr);
            parse_uri();
        }
    
        xvaction_t::xvaction_t()
            :xvmethod_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = 0;
            m_max_tgas    = 0;
        }
    
        xvaction_t::~xvaction_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, -1);
            close();
        }

        void xvaction_t::close()
        {
        }
    
        xvaction_t::xvaction_t(const xvaction_t & obj)
            :xvmethod_t(obj)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = obj.m_used_tgas;
            m_max_tgas    = obj.m_max_tgas;
            m_org_tx_hash = obj.m_org_tx_hash;
            m_org_tx_action_id = obj.m_org_tx_action_id;
            parse_uri();
        }
    
        xvaction_t::xvaction_t(xvaction_t && moved)
            :xvmethod_t(moved)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaction, 1);
            m_used_tgas   = moved.m_used_tgas;
            m_max_tgas    = moved.m_max_tgas;
            m_org_tx_hash = moved.m_org_tx_hash;
            m_org_tx_action_id = moved.m_org_tx_action_id;
            parse_uri();
        }
    
        xvaction_t & xvaction_t::operator = (const xvaction_t & obj)
        {
            close(); //close first
            
            xvmethod_t::operator=(obj);
            m_used_tgas   = obj.m_used_tgas;
            m_max_tgas    = obj.m_max_tgas;
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
            const int32_t begin_size = stream.size();
            
            stream.write_tiny_string(m_org_tx_hash);
            stream << m_org_tx_action_id;
            xvmethod_t::do_write(stream);
            
            if(get_method_result() != nullptr)
            {
                get_method_result()->serialize_to(stream);
            }
            else
            {
                xvalue_t def_result(enum_xerror_code_not_handled); //means not executed yet
                def_result.serialize_to(stream);
            }
            
            return (stream.size() - begin_size);
        }
    
        int32_t xvaction_t::do_read(xstream_t & stream)      //serialize header and object,return how many bytes is readed
        {
            close();
            const int32_t begin_size = stream.size();
            
            stream.read_tiny_string(m_org_tx_hash);
            stream >> m_org_tx_action_id;
            const int result = xvmethod_t::do_read(stream);
            xassert(result > 0);
            
            xvalue_t method_result;
            method_result.serialize_from(stream);
            copy_result(method_result);
            
            parse_uri();
            return (begin_size - stream.size());
        }
        
        bool   xvaction_t::withdraw_tgas(const uint64_t tgas) //return false if not have enough tags left
        {
            if( (m_used_tgas + tgas) <= m_max_tgas)
            {
                xatomic_t::xadd(m_used_tgas,tgas);
                return true;
            }
            return false;
        }

        //----------------------------------------xvactions_t-------------------------------------//
        int32_t xvactions_t::do_write(base::xstream_t & stream) const {
            const int32_t begin_size = stream.size();
            const uint32_t count = (uint32_t)m_actions.size();
            stream << count;
            for (auto & v : m_actions) {
                v.serialize_to(stream);
            }
            return (stream.size() - begin_size);
        }

        int32_t xvactions_t::do_read(base::xstream_t & stream)
        {
            m_actions.clear();
            const int32_t begin_size = stream.size();
            uint32_t count = 0;
            stream >> count;
            for (uint32_t i = 0; i < count; i++) {
                base::xvaction_t action;
                const int res = action.serialize_from(stream);
                if(res <= 0) {
                    xerror("xvactions_t::do_read,fail to read action as error=%d,i=%d,count=%d",res,i,count);
                    m_actions.clear(); //clean others as well
                    return res;
                }
                m_actions.emplace_back(action);
            }
            return (begin_size - stream.size());
        }

        int32_t xvactions_t::serialize_to_string(std::string & _str) const {
            base::xstream_t _raw_stream(base::xcontext_t::instance());
            int32_t ret = do_write(_raw_stream);
            _str.assign((const char*)_raw_stream.data(),_raw_stream.size());
            return ret;
        }

        int32_t xvactions_t::serialize_from_string(const std::string & _str) {
            base::xstream_t _stream(base::xcontext_t::instance(), (uint8_t *)_str.data(), (int32_t)_str.size());
            int32_t ret = do_read(_stream);
            return ret;
        }

        void xvactions_t::add_action(xvaction_t const& action) {
            m_actions.push_back(action);
        }

    };//end of namespace of base
};//end of namespace of top
