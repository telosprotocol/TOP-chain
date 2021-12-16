// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvdrecycle.h"
#include "../xvledger.h"

namespace top
{
    namespace base
    {
        xvdrecycle_t::xvdrecycle_t(enum_vdata_recycle_type type)
        {
            m_recycle_type = type;
        }
    
        xvdrecycle_t::~xvdrecycle_t()
        {
        }
    
        //only allow xvdrecycle_mgr mangage reference
        int32_t   xvdrecycle_t::add_ref()
        {
            return xobject_t::add_ref();
        }
    
        int32_t   xvdrecycle_t::release_ref()
        {
            return xobject_t::release_ref();
        }
    
        xblockrecycler_t::xblockrecycler_t()
            :xvdrecycle_t(enum_vdata_recycle_type_block)
        {
        }
    
        xblockrecycler_t::~xblockrecycler_t()
        {
        }
    
        xstaterecycler_t::xstaterecycler_t()
            :xvdrecycle_t(enum_vdata_recycle_type_state)
        {
        }
        
        xstaterecycler_t::~xstaterecycler_t()
        {
        }
    
        xvdrecycle_mgr::xvdrecycle_mgr()
        {
            memset(m_recyclers_obj,0,sizeof(m_recyclers_obj));
            memset(m_recycler_switch,0,sizeof(m_recycler_switch));//off as default
        }
    
        xvdrecycle_mgr::~xvdrecycle_mgr()
        {
            for(int i = 0; i < enum_vdata_recycle_type_end; ++i)
            {
                xvdrecycle_t* obj_ptr = m_recyclers_obj[i];
                m_recyclers_obj[i] = NULL;
                if(obj_ptr != NULL)
                    obj_ptr->release_ref();
            }
        }
    
        //only allow xvchain_t mangage reference
        int32_t   xvdrecycle_mgr::add_ref()
        {
            return xobject_t::add_ref();
        }
        
        int32_t   xvdrecycle_mgr::release_ref()
        {
            return xobject_t::release_ref();
        }
    
        bool  xvdrecycle_mgr::turn_on_recycler(enum_vdata_recycle_type target)
        {
            if(((uint32_t)target) >= enum_vdata_recycle_type_end)
            {
                xassert(0);
                return false;
            }
            m_recycler_switch[target] = 1;
            return true;
        }
    
        bool  xvdrecycle_mgr::turn_off_recycler(enum_vdata_recycle_type target)
        {
            if(((uint32_t)target) >= enum_vdata_recycle_type_end)
            {
                xassert(0);
                return false;
            }
            m_recycler_switch[target] = 0;
            return true;
        }
    
        xblockrecycler_t* xvdrecycle_mgr::get_block_recycler(bool break_through)
        {
            return (xblockrecycler_t*)get_recycler(enum_vdata_recycle_type_block, break_through);
        }
    
        bool  xvdrecycle_mgr::set_block_recycler(xblockrecycler_t& new_recycler)
        {
            xkinfo("xvdrecycle_mgr::set_block_recycler");
            return set_recycler(new_recycler);
        }
    
        xvdrecycle_t*   xvdrecycle_mgr::get_recycler(enum_vdata_recycle_type target, bool break_through)
        {
            if (!break_through) {
                if(m_recycler_switch[target] <= 0) //not turn on
                    return NULL;
                
                if(xvchain_t::instance().is_auto_prune_enable() == false)
                    return NULL;//global switch is off
            }
                        
            return m_recyclers_obj[target];
        }
    
        bool   xvdrecycle_mgr::set_recycler(xvdrecycle_t& new_recycler)
        {
            xvdrecycle_t* old_ptr = m_recyclers_obj[new_recycler.get_recycle_type()];
            if(old_ptr != NULL) //not allow replace it dynamic
            {
                xerror("xvdrecycle_mgr::set_recycler,try overwrite existing recycler(%d)",new_recycler.get_recycle_type());
                return false;
            }
            
            //transfer ownership to xvdrecycle_mgr
            m_recyclers_obj[new_recycler.get_recycle_type()] = &new_recycler;
            return true;
        }
    
    }
}
