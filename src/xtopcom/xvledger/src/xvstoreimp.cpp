// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvdbstore.h"
#include "../xvblockstore.h"
#include "../xvstatestore.h"
#include "../xveventbus.h"
#include "../xvledger.h"
 
namespace top
{
    namespace base
    {
        xvdbstore_t::xvdbstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vxdbstore)
        {
        }
        
        xvdbstore_t::~xvdbstore_t()
        {
        };
        
        //caller need to cast (void*) to related ptr
        void*   xvdbstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vxdbstore)
                return this;
            
            return xobject_t::query_interface(_enum_xobject_type_);
        }
        
        xvblockstore_t::xvblockstore_t(base::xcontext_t & _context,const int32_t target_thread_id)
            :xiobject_t(_context,target_thread_id,(enum_xobject_type)enum_xobject_type_vblockstore)
        {
            xvblock_t::register_object(xcontext_t::instance()); //should only have one xvblockstore_t per process
        }
        
        xvblockstore_t::~xvblockstore_t()
        {
        };
        
        //caller need to cast (void*) to related ptr
        void*   xvblockstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vblockstore)
                return this;
            
            return xiobject_t::query_interface(_enum_xobject_type_);
        }
        //only allow remove flag within xvblockstore_t
        void  xvblockstore_t::remove_block_flag(xvblock_t* to_block, enum_xvblock_flag flag)
        {
            if(to_block != NULL)
                to_block->remove_block_flag(flag);
        }
    
        xveventbus_t::xveventbus_t()
            :xobject_t(enum_xevent_route_path_by_mbus)
        {
        }
        xveventbus_t::~xveventbus_t()
        {
        }
    
         void*   xveventbus_t::query_interface(const int32_t type)
         {
             if(type == enum_xobject_type_veventbus)
                 return this;
         
             return xobject_t::query_interface(type);
         }
        
        bool   xveventbus_t::handle_event(const xvevent_t & ev)
        {
            if(ev.get_type() == enum_xevent_route_path_by_mbus)
            {
                auto * ev_ptr = const_cast<xvevent_t *>(&ev);
                ev_ptr->add_ref();
                mbus::xevent_ptr_t mbus_ev_ptr;
                mbus_ev_ptr.attach(dynamic_cast<mbus::xevent_t *>(ev_ptr));

                push_event(mbus_ev_ptr);
                return true;
            }
            return false;
        }
    
        xvstatestore::xvstatestore()
            :xobject_t(enum_xobject_type_vstatestore)
        {
            
        }
    
        xvstatestore::~xvstatestore()
        {
        }
    
        void*   xvstatestore::query_interface(const int32_t type)
        {
            if(type == enum_xobject_type_vstatestore)
                return this;
            
            return xobject_t::query_interface(type);
        }
    
        bool                   xvstatestore::get_block_state(xvblock_t * target_block) //once successful,assign xvbstate_t into block
        {
            return false;
        }
    
        xauto_ptr<xvbstate_t> xvstatestore::get_block_state(xvaccount_t & account,const uint64_t height,const uint64_t view_id)
        {
            return nullptr;
        }
    
        xauto_ptr<xvbstate_t> xvstatestore::get_block_state(xvaccount_t & account,const uint64_t height,const std::string& block_hash)
        {
            return nullptr;
        }
        
    };//end of namespace of base
};//end of namespace of top
