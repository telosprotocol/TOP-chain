// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvledger.h"
#include "../xvactplugin.h"

namespace top
{
    namespace base
    {
        xvactplugin_t::xvactplugin_t(xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms,enum_xvaccount_plugin_type type)
        {
            m_is_closing = 0; //alive as default
            m_last_access_time_ms = 0;
            m_idle_timeout_ms     = idle_timeout_ms;
            
            m_plugin_type = type;
            m_account_obj = &parent_obj;
            m_account_obj->add_ref();
            
            //XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvactplugin_t, 1);
            xdbg("xvactplugin_t::xvactplugin_t,acccount(%s)-type(%d),objectid(%lld)",m_account_obj->get_address().c_str(),get_plugin_type(),get_obj_id());
        }
        
        xvactplugin_t::~xvactplugin_t()
        {
            m_account_obj->release_ref();
            
            //XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvactplugin_t, -1);
            xdbg("xvactplugin_t::destroy,acccount(%s)-type(%d),objectid(%lld)",m_account_obj->get_address().c_str(),get_plugin_type(),get_obj_id());
        }
        
        void   xvactplugin_t::stop()//mark idle flag
        {
            xatomic_t::xstore(m_is_closing, (uint8_t)1);
        }
        
        bool   xvactplugin_t::close(bool force_async)
        {
            xdbg("xvactplugin_t::close,acccount(%s)-type(%d),objectid(%lld)",m_account_obj->get_address().c_str(),get_plugin_type(),get_obj_id());
            return xobject_t::close(force_async);
        }
    
        const xvid_t         xvactplugin_t::get_xvid()    const
        {
            return m_account_obj->get_xvid();
        }
        
        const std::string &  xvactplugin_t::get_account_address()  const
        {
            return m_account_obj->get_address();
        }
        
        void  xvactplugin_t::set_last_access_time(const uint64_t last_access_time)
        {
            if(m_last_access_time_ms < last_access_time)
                m_last_access_time_ms = last_access_time;
        }
        
        bool xvactplugin_t::is_live(const uint64_t timenow_ms)
        {
            if(is_closing() || is_close() )
                return false;
            
            if(get_refcount() > 2) //note: table & account may hold each reference. so other holding if > 2
                return true;
            
            const uint64_t last_time_ms = m_last_access_time_ms;
            if( timenow_ms > (last_time_ms + m_idle_timeout_ms) )
                return false;
            
            return true;
        }
        
    };//end of namespace of base
};//end of namespace of top
