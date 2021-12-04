// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvdbstore.h"
#include "../xvblockstore.h"
#include "../xvcontractstore.h"
#include "../xvtxstore.h"
#include "../xveventbus.h"
#include "../xvledger.h"
#include "../xvdbkey.h"
#include "../xvcontract.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        //----------------------------------------xvdbstore_t----------------------------------------//
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

        //----------------------------------------xvtxstore_t----------------------------------------//
        xvtxstore_t::xvtxstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vtxstore)
        {
        }

        xvtxstore_t::~xvtxstore_t()
        {
        };

        //caller need to cast (void*) to related ptr
        void*   xvtxstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            xassert(false);
            if(_enum_xobject_type_ == enum_xobject_type_vtxstore)
                return this;

            return xobject_t::query_interface(_enum_xobject_type_);
        }

        //----------------------------------------xvblockstore_t----------------------------------------//
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

        //----------------------------------------xvcontractstore_t----------------------------------------//
        xvcontractstore_t::xvcontractstore_t()
            :xobject_t((enum_xobject_type)enum_xobject_type_vcontractstore)
        {
        }

        xvcontractstore_t::~xvcontractstore_t()
        {
        }

        //caller need to cast (void*) to related ptr
        void*   xvcontractstore_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vcontractstore)
                return this;

            return xobject_t::query_interface(_enum_xobject_type_);
        }

        const std::string        xvcontractstore_t::get_sys_tep0_contract_uri(const std::string & contract_addr,const uint32_t contract_version)
        {
            return xvcontract_t::create_contract_uri(contract_addr,const_TEP0_contract_name, contract_version);
        }

        const std::string        xvcontractstore_t::get_sys_tep1_contract_uri(const std::string & contract_addr,const uint32_t contract_version)
        {
            return xvcontract_t::create_contract_uri(contract_addr,const_TEP1_contract_name, contract_version);
        }

        const std::string        xvcontractstore_t::get_sys_tep2_contract_uri(const std::string & contract_addr,const uint32_t contract_version)
        {
            return xvcontract_t::create_contract_uri(contract_addr,const_TEP2_contract_name, contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_tep0_contract(const std::string & contract_addr,const uint32_t contract_version)
        {
            return new xvcontract_TEP0(contract_addr,contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_tep1_contract(const std::string & contract_addr,const uint32_t contract_version)
        {
            return new xvcontract_TEP1(contract_addr,contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_tep2_contract(const std::string & contract_addr,const uint32_t contract_version)
        {
            return new xvcontract_TEP2(contract_addr,contract_version);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_contract(const std::string & contract_uri)
        {
            std::string contract_addr;
            std::string contract_name;
            uint32_t    contract_ver = 0;
            if(false == xvcontract_t::parse_contract_uri(contract_uri,contract_addr,contract_name,contract_ver))
            {
                xerror("xvcontractstore_t::get_sys_contract,bad uri(%s)",contract_uri.c_str());
                return nullptr;
            }
            return get_sys_contract(contract_addr,contract_name,contract_ver);
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_sys_contract(const std::string & contract_addr,const std::string & contract_name,const uint32_t version)
        {
            if(contract_name.empty() == false)
            {
                if(contract_name == const_TEP0_contract_name)
                    return get_sys_tep0_contract(contract_addr,version);
                else if(contract_name == const_TEP1_contract_name)
                    return get_sys_tep1_contract(contract_addr,version);
                else if(contract_name == const_TEP2_contract_name)
                    return get_sys_tep2_contract(contract_addr,version);

                xerror("xvcontractstore_t::get_sys_contract,fail to load system contract for contract_addr(%s)/contract_name(%s)/version(%u)",contract_addr.c_str(),contract_name.c_str(),version);
                return nullptr;
            }
            else
            {
                xerror("xvcontractstore_t::get_sys_contract,try load non-system contract for contract_addr(%s)/contract_name(%s)/version(%u)",contract_addr.c_str(),contract_name.c_str(),version);
                return nullptr;
            }
        }

        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_usr_contract(const std::string & contract_addr)
        {
            xerror("xvcontractstore_t::get_usr_contract,fail to load contract for contract_addr(%s)",contract_addr.c_str());
            return nullptr;
        }

        //universal api
        xauto_ptr<xvcontract_t>  xvcontractstore_t::get_contract(const std::string & contract_uri)
        {
            std::string contract_addr;
            std::string contract_name;
            uint32_t    contract_ver = 0;
            if(false == xvcontract_t::parse_contract_uri(contract_uri,contract_addr,contract_name,contract_ver))
            {
                xerror("xvcontractstore_t::get_contract,bad uri(%s)",contract_uri.c_str());
                return nullptr;
            }
            if(contract_name.empty() == false)
                return get_sys_contract(contract_addr,contract_name,contract_ver);
            else
                return get_usr_contract(contract_addr);
        }

        //----------------------------------------xveventbus_t----------------------------------------//
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

    };//end of namespace of base
};//end of namespace of top
