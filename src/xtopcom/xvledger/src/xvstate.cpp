// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "xstatistic/xbasic_size.hpp"
#include "../xvblock.h"
#include "../xvstate.h"
#include "../xvledger.h"
#include "xmetrics/xmetrics.h"

#ifdef DEBUG
    #define _DEBUG_STATE_BINARY_
#endif

namespace top
{
    namespace base
    {
        //*************************************xvbstate_t****************************************
        void xvexestate_t::register_object(xcontext_t & context)
        {
            xstringvar_t::register_object(context);
            xtokenvar_t::register_object(context);
            xnoncevar_t::register_object(context);
            xcodevar_t::register_object(context);
            xmtokens_t::register_object(context);
            xmkeys_t::register_object(context);
            
            xhashmapvar_t::register_object(context);
            
            xvintvar_t<int64_t>::register_object(context);
            xvintvar_t<uint64_t>::register_object(context);
            
            xdequevar_t<int8_t>::register_object(context);
            xdequevar_t<int16_t>::register_object(context);
            xdequevar_t<int32_t>::register_object(context);
            xdequevar_t<int64_t>::register_object(context);
            xdequevar_t<uint64_t>::register_object(context);
            xdequevar_t<std::string>::register_object(context);
            
            xmapvar_t<int8_t>::register_object(context);
            xmapvar_t<int16_t>::register_object(context);
            xmapvar_t<int32_t>::register_object(context);
            xmapvar_t<int64_t>::register_object(context);
            xmapvar_t<uint64_t>::register_object(context);
            xmapvar_t<std::string>::register_object(context);
            //add others standard property
        }
    
        xvexestate_t::xvexestate_t(enum_xdata_type type)
            :xvexegroup_t(type),
             xvaccount_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvexestate_t, 1);
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }

        xvexestate_t::xvexestate_t(const std::string & account_addr,enum_xdata_type type)
            :xvexegroup_t(type),
             xvaccount_t(account_addr)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvexestate_t, 1);
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }

        xvexestate_t::xvexestate_t(const xvexestate_t & obj)
            :xvexegroup_t(obj),
             xvaccount_t(obj.get_address())
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvexestate_t, 1);
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
    
        xvexestate_t::~xvexestate_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvexestate_t, -1);
        }
        
        bool    xvexestate_t::clone_properties_from(xvexestate_t& source)//note: just only clone the state of properties
        {
            xassert(get_child_units().empty());//must be empty
            return clone_units_from(source);
        }
    
        bool    xvexestate_t::take_snapshot(std::string & to_full_state_bin)
        {
            auto canvas  = rebase_change_to_snapshot();
            return (canvas->encode(to_full_state_bin) == enum_xcode_successful);
        }
    
        xauto_ptr<xvcanvas_t>  xvexestate_t::take_snapshot()
        {
            return rebase_change_to_snapshot();
        }
    
        xauto_ptr<xvcanvas_t>  xvexestate_t::rebase_change_to_snapshot() //snapshot for whole xvbstate of every properties
        {
            std::lock_guard<std::recursive_mutex> locker(get_mutex());
            
            xvcanvas_t* new_canvas = new xvcanvas_t();
            const std::map<std::string,xvexeunit_t*> & all_units = get_child_units();
            for(auto & it : all_units)
            {
                xvproperty_t * property_ptr = (xvproperty_t*)it.second;
                xvmethod_t instruction(renew_property_instruction(property_ptr->get_name(),property_ptr->get_obj_type(),property_ptr->get_value()));
                
                if(false == new_canvas->record(instruction))
                {
                    xerror("xvexestate_t::take_snapshot_to_binlog,abort as property fail to take snapshot,propery(%s)",it.second->dump().c_str());
                    
                    new_canvas->release_ref();
                    return nullptr;
                }
                
                #ifdef _DEBUG_STATE_BINARY_
                xinfo("xvexestate_t::rebase,property(%s)->renew(%s)",property_ptr->get_name().c_str(), instruction.dump().c_str());
                #endif
                
            }
            return new_canvas;
        }
  
        std::string  xvexestate_t::get_property_value(const std::string & name) const
        {
            std::lock_guard<std::recursive_mutex> locker(get_mutex());
            
            std::string bin_data;
            xvproperty_t* target = get_property_object(name);
            if(target != nullptr)
                target->serialize_to_string(bin_data);

            return bin_data;
        }

        void xvexestate_t::set_property_value(std::string const & name, std::string const & bin_data, xvcanvas_t * canvas) {
            //std::lock_guard<std::recursive_mutex> locker(get_mutex());

            //xvproperty_t * target = get_property_object(name);
            //if (target != nullptr) {
            //    target->clear_value(canvas);
            //    target->serialize_from_string(bin_data);
            //}
        }

        xvproperty_t*   xvexestate_t::get_property_object(const std::string & name) const
        {
            xvexeunit_t * target = find_child_unit(name);
            if(target != nullptr)
                return (xvproperty_t*)target;
            
            return nullptr;
        }

        std::set<std::string> xvexestate_t::get_all_property_names() const
        {
            std::lock_guard<std::recursive_mutex> locker(get_mutex());
            
            std::set<std::string> names;
            auto const & units = get_child_units();
            for (auto const & pair : units) {
                names.insert(pair.first);
            }
            return names;
        }

        int xvexestate_t::get_property_num() const {
            std::lock_guard<std::recursive_mutex> locker(get_mutex());
            return (int)get_child_units().size();
        }
    
        bool  xvexestate_t::find_property(const std::string & property_name) const //check whether property already existing
        {
            std::lock_guard<std::recursive_mutex> locker(get_mutex());
            
            if(get_property_object(property_name) != nullptr)
                return true;
            
            return false;
        }

        xauto_ptr<xtokenvar_t>  xvexestate_t::new_token_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_token,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_token_var(property_name);
            
            xerror("xvexestate_t::new_token_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xnoncevar_t>      xvexestate_t::new_nonce_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_nonce,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_nonce_var(property_name);
            
            xerror("xvexestate_t::new_nonce_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
 
        xauto_ptr<xcodevar_t>   xvexestate_t::new_code_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_code,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_code_var(property_name);
            
            xerror("xvexestate_t::new_code_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmtokens_t>   xvexestate_t::new_multiple_tokens_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_mtokens,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_multiple_tokens_var(property_name);
            
            xerror("xvexestate_t::new_multiple_tokens_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmkeys_t>   xvexestate_t::new_multiple_keys_var(const std::string & property_name,xvcanvas_t * canvas) //to manage pubkeys of account
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_mkeys,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_multiple_keys_var(property_name);
            
            xerror("xvexestate_t::new_multiple_keys_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
   
        xauto_ptr<xstringvar_t> xvexestate_t::new_string_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_string,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_string_var(property_name);
            
            xerror("xvexestate_t::new_string_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }

        xauto_ptr<xhashmapvar_t>   xvexestate_t::new_hashmap_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_hashmap,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_hashmap_var(property_name);
            
            xerror("xvexestate_t::new_hashmap_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }

        //integer related
        xauto_ptr<xvintvar_t<int64_t>> xvexestate_t::new_int64_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int64,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int64_var(property_name);
            
            xerror("xvexestate_t::new_int64_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xvintvar_t<uint64_t>> xvexestate_t::new_uint64_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_uint64,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_uint64_var(property_name);
            
            xerror("xvexestate_t::new_uint64_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        //xdequevar_t related
        xauto_ptr<xdequevar_t<int8_t>>   xvexestate_t::new_int8_deque_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int8_deque,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int8_deque_var(property_name);
            
            xerror("xvexestate_t::new_int8_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
     
        xauto_ptr<xdequevar_t<int16_t>>  xvexestate_t::new_int16_deque_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int16_deque,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int16_deque_var(property_name);
            
            xerror("xvexestate_t::new_int16_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int32_t>>  xvexestate_t::new_int32_deque_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int32_deque,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int32_deque_var(property_name);
            
            xerror("xvexestate_t::new_int32_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int64_t>>  xvexestate_t::new_int64_deque_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int64_deque,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int64_deque_var(property_name);
            
            xerror("xvexestate_t::new_int64_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<uint64_t>>  xvexestate_t::new_uint64_deque_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_uint64_deque,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_uint64_deque_var(property_name);
            
            xerror("xvexestate_t::new_uint64_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<std::string>>  xvexestate_t::new_string_deque_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_string_deque,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_string_deque_var(property_name);
            
            xerror("xvexestate_t::new_string_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        //xmapvar_t related
        xauto_ptr<xmapvar_t<int8_t>>   xvexestate_t::new_int8_map_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int8_map,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int8_map_var(property_name);
            
            xerror("xvexestate_t::new_int8_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int16_t>>   xvexestate_t::new_int16_map_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int16_map,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int16_map_var(property_name);
            
            xerror("xvexestate_t::new_int16_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int32_t>>   xvexestate_t::new_int32_map_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int32_map,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int32_map_var(property_name);
            
            xerror("xvexestate_t::new_int32_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int64_t>>   xvexestate_t::new_int64_map_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int64_map,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_int64_map_var(property_name);
            
            xerror("xvexestate_t::new_int64_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmapvar_t<uint64_t>>   xvexestate_t::new_uint64_map_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_uint64_map,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_uint64_map_var(property_name);
            
            xerror("xvexestate_t::new_uint64_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmapvar_t<std::string>>   xvexestate_t::new_string_map_var(const std::string & property_name,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_string_map,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_string_map_var(property_name);
            
            xerror("xvexestate_t::new_string_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xtokenvar_t>  xvexestate_t::load_token_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xtokenvar_t* token_obj = (xtokenvar_t*)property_obj->query_interface(enum_xobject_type_vprop_token);
                xassert(token_obj != nullptr);
                if(token_obj != nullptr)
                    token_obj->add_ref();//for returned xauto_ptr
                return token_obj;
            }
            xerror("xvexestate_t::load_token_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xnoncevar_t>  xvexestate_t::load_nonce_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xnoncevar_t* nonce_obj = (xnoncevar_t*)property_obj->query_interface(top::base::enum_xobject_type_vprop_nonce);
                xassert(nonce_obj != nullptr);
                if(nonce_obj != nullptr)
                    nonce_obj->add_ref();//for returned xauto_ptr
                return nonce_obj;
            }
            xerror("xvexestate_t::load_nonce_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xcodevar_t>  xvexestate_t::load_code_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xcodevar_t* code_obj = (xcodevar_t*)property_obj->query_interface(top::base::enum_xobject_type_vprop_code);
                xassert(code_obj != nullptr);
                if(code_obj != nullptr)
                    code_obj->add_ref();//for returned xauto_ptr
                return code_obj;
            }
            xerror("xvexestate_t::load_code_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmtokens_t>    xvexestate_t::load_multiple_tokens_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmtokens_t* tokens_obj = (xmtokens_t*)property_obj->query_interface(top::base::enum_xobject_type_vprop_mtokens);
                xassert(tokens_obj != nullptr);
                if(tokens_obj != nullptr)
                    tokens_obj->add_ref();//for returned xauto_ptr
                return tokens_obj;
            }
            xerror("xvexestate_t::load_multiple_tokens_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmkeys_t>    xvexestate_t::load_multiple_keys_var(const std::string & property_name)//to manage pubkeys of account
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmkeys_t* keys_obj = (xmkeys_t*)property_obj->query_interface(top::base::enum_xobject_type_vprop_mkeys);
                xassert(keys_obj != nullptr);
                if(keys_obj != nullptr)
                    keys_obj->add_ref();//for returned xauto_ptr
                return keys_obj;
            }
            xerror("xvexestate_t::load_multiple_keys_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
 
        xauto_ptr<xstringvar_t> xvexestate_t::load_string_var(const std::string & property_name) const
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xstringvar_t* var_obj = (xstringvar_t*)property_obj->query_interface(top::base::enum_xobject_type_vprop_string);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_string_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }

        xauto_ptr<xhashmapvar_t>xvexestate_t::load_hashmap_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xhashmapvar_t* var_obj = (xhashmapvar_t*)property_obj->query_interface(top::base::enum_xobject_type_vprop_hashmap);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_hashmap_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        //integer related load functions
        xauto_ptr<xvintvar_t<int64_t>>  xvexestate_t::load_int64_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xvintvar_t<int64_t>* token_obj = (xvintvar_t<int64_t>*)property_obj->query_interface(top::base::enum_xobject_type_vprop_int64);
                xassert(token_obj != nullptr);
                if(token_obj != nullptr)
                    token_obj->add_ref();//for returned xauto_ptr
                return token_obj;
            }
            xerror("xvexestate_t::load_int64_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xvintvar_t<uint64_t>>  xvexestate_t::load_uint64_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xvintvar_t<uint64_t>* token_obj = (xvintvar_t<uint64_t>*)property_obj->query_interface(top::base::enum_xobject_type_vprop_uint64);
                xassert(token_obj != nullptr);
                if(token_obj != nullptr)
                    token_obj->add_ref();//for returned xauto_ptr
                return token_obj;
            }
            xerror("xvexestate_t::load_uint64_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        //load functions of deque related
        xauto_ptr<xdequevar_t<int8_t>> xvexestate_t::load_int8_deque_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xdequevar_t<int8_t>* var_obj = (xdequevar_t<int8_t>*)property_obj->query_interface(enum_xobject_type_vprop_int8_deque);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int8_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int16_t>> xvexestate_t::load_int16_deque_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xdequevar_t<int16_t>* var_obj = (xdequevar_t<int16_t>*)property_obj->query_interface(enum_xobject_type_vprop_int16_deque);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int16_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int32_t>> xvexestate_t::load_int32_deque_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xdequevar_t<int32_t>* var_obj = (xdequevar_t<int32_t>*)property_obj->query_interface(enum_xobject_type_vprop_int32_deque);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int32_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int64_t>> xvexestate_t::load_int64_deque_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xdequevar_t<int64_t>* var_obj = (xdequevar_t<int64_t>*)property_obj->query_interface(enum_xobject_type_vprop_int64_deque);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int64_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<uint64_t>> xvexestate_t::load_uint64_deque_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xdequevar_t<uint64_t>* var_obj = (xdequevar_t<uint64_t>*)property_obj->query_interface(enum_xobject_type_vprop_uint64_deque);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_uint64_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<std::string>> xvexestate_t::load_string_deque_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xdequevar_t<std::string>* var_obj = (xdequevar_t<std::string>*)property_obj->query_interface(enum_xobject_type_vprop_string_deque);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_string_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        //load functions of map related
        xauto_ptr<xmapvar_t<int8_t>>   xvexestate_t::load_int8_map_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmapvar_t<int8_t>* var_obj = (xmapvar_t<int8_t>*)property_obj->query_interface(enum_xobject_type_vprop_int8_map);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int8_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int16_t>>   xvexestate_t::load_int16_map_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmapvar_t<int16_t>* var_obj = (xmapvar_t<int16_t>*)property_obj->query_interface(enum_xobject_type_vprop_int16_map);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int16_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int32_t>>   xvexestate_t::load_int32_map_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmapvar_t<int32_t>* var_obj = (xmapvar_t<int32_t>*)property_obj->query_interface(enum_xobject_type_vprop_int32_map);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int32_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int64_t>>   xvexestate_t::load_int64_map_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmapvar_t<int64_t>* var_obj = (xmapvar_t<int64_t>*)property_obj->query_interface(enum_xobject_type_vprop_int64_map);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_int64_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<uint64_t>>   xvexestate_t::load_uint64_map_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmapvar_t<uint64_t>* var_obj = (xmapvar_t<uint64_t>*)property_obj->query_interface(enum_xobject_type_vprop_uint64_map);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_uint64_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<std::string>>   xvexestate_t::load_string_map_var(const std::string & property_name)
        {
            xvproperty_t * property_obj = get_property_object(property_name);
            if(property_obj != nullptr)
            {
                xmapvar_t<std::string>* var_obj = (xmapvar_t<std::string>*)property_obj->query_interface(enum_xobject_type_vprop_string_map);
                xassert(var_obj != nullptr);
                if(var_obj != nullptr)
                    var_obj->add_ref();//for returned xauto_ptr
                return var_obj;
            }
            xerror("xvexestate_t::load_string_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xvproperty_t> xvexestate_t::load_property(const std::string & property_name)
        {
            xvproperty_t * found_target = get_property_object(property_name);
            if(found_target != nullptr)
                found_target->add_ref();//add reference for xauto_ptr;
            return found_target;
        }
    
        xauto_ptr<xvproperty_t> xvexestate_t::new_property(const std::string & property_name,const int propertyType,xvcanvas_t * canvas)
        {
            const xvalue_t result(new_property_internal(property_name,propertyType,canvas));
            if(result.get_error() == enum_xcode_successful)
                return load_property(property_name);
            
            xerror("xvexestate_t::new_property,failed as error:%d for name(%s) and type(%d)",result.get_error(),property_name.c_str(),propertyType);
            return nullptr;
        }
    
        const xvalue_t xvexestate_t::new_property_internal(const std::string & property_name,const int propertyType,xvcanvas_t * canvas)
        {
            xvalue_t param_ptype((vint32_t)propertyType);
            xvalue_t param_pname(property_name);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_new_property ,param_ptype,param_pname);
            
            return execute(instruction,canvas); //excute the instruction
        }
    
        const xvalue_t  xvexestate_t::do_new_property(const xvmethod_t & op,xvcanvas_t * canvas)
        {
            if(op.get_params_count() < 2) //reset must carry type and name at least
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & property_type = op.get_method_params().at(0);
            const xvalue_t & property_name = op.get_method_params().at(1);
        
            xvproperty_t* existingOne = get_property_object(property_name.get_string());
            if(existingOne != nullptr)
            {
                xerror("xvexestate_t::do_new_property,try overwrite existing property of account(%s) by new-property(%s)",get_address().c_str(),property_name.get_string().c_str());
                return xvalue_t(enum_xerror_code_exist);
            }
            if(get_childs_count() >= enum_max_property_count)
            {
                xerror("xvexestate_t::do_new_property,the account has too many properites already,account(%s)",get_address().c_str());
                return xvalue_t(enum_xerror_code_over_limit);
            }
            
            xauto_ptr<xobject_t> object(xcontext_t::create_xobject((enum_xobject_type)property_type.get_int32()));
            xassert(object);
            if(object)
            {
                xvproperty_t * property_obj = (xvproperty_t*)object->query_interface(enum_xobject_type_vproperty);
                xassert(property_obj != NULL);
                if(property_obj != NULL)
                {
                    property_obj->set_unit_name(property_name.get_string());
                    add_child_unit(property_obj);
                    
                    return xvalue_t(enum_xcode_successful);
                }
                else
                {
                    return xvalue_t(enum_xerror_code_bad_object);
                }
            }
            else
            {
                return xvalue_t(enum_xerror_code_not_found);
            }
        }
    
        bool   xvexestate_t::reset_property(const std::string & property_name,const xvalue_t & property_value,xvcanvas_t * canvas)
        {
            xvalue_t param_pname(property_name);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_reset_property ,param_pname,(xvalue_t&)property_value);
            
            //excute the instruction
            auto result = execute(instruction,canvas);
            if(result.get_error() == enum_xcode_successful)
                return true;
            
            xerror("xvexestate_t::reset_property,fail for property_name(%s) at xbstate(%s) as error(%d)",property_name.c_str(),dump().c_str(),result.get_error());
            return false;
        }

        const xvalue_t  xvexestate_t::do_reset_property(const xvmethod_t & op,xvcanvas_t * canvas)
        {
            if(op.get_method_type() != enum_xvinstruct_class_state_function)
                return xvalue_t(enum_xerror_code_bad_type);
            
            if(op.get_method_id() != enum_xvinstruct_state_method_reset_property)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_params_count() != 2) //reset must carry new value
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & property_name  = op.get_method_params().at(0);
            const xvalue_t & property_value = op.get_method_params().at(1);
            
            xvproperty_t * target_property = get_property_object(property_name.get_string());
            if(target_property == nullptr)
            {
                xerror("xvexestate_t::do_reset_property,NOT find property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //note:here move property_value with better performance
            if(target_property->copy_from_value(property_value))
                return xvalue_t(enum_xcode_successful);
            
            xerror("xvexestate_t::do_reset_property,fail reset property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
            return xvalue_t(enum_xerror_code_fail);
        }
    
        xvmethod_t  xvexestate_t::renew_property_instruction(const std::string & property_name,const int  property_type,const xvalue_t & property_value)
        {
            xvalue_t param_ptype((vint32_t)property_type);//#parm-0
            xvalue_t param_pname(property_name); //#parm-1
            return xvmethod_t(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_renew_property ,param_ptype,param_pname,(xvalue_t&)property_value);
        }
    
        bool   xvexestate_t::renew_property(const std::string & property_name,const int property_type,const xvalue_t & property_value,xvcanvas_t * canvas)
        {
            xvmethod_t instruction(renew_property_instruction(property_name,property_type,property_value));
            
            //excute the instruction
            auto result = execute(instruction,canvas);
            if(result.get_error() == enum_xcode_successful)
                return true;
            
            xerror("xvexestate_t::renew_property,fail for property_name(%s) at xbstate(%s) as error(%d)",property_name.c_str(),dump().c_str(),result.get_error());
            return false;
        }
        
        const xvalue_t  xvexestate_t::do_renew_property(const xvmethod_t & op,xvcanvas_t * canvas)
        {
            if(op.get_method_type() != enum_xvinstruct_class_state_function)
                return xvalue_t(enum_xerror_code_bad_type);
            
            if(op.get_method_id() != enum_xvinstruct_state_method_renew_property)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_params_count() != 3) //reset must carry new value
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            //const xvalue_t & property_type  = op.get_method_params().at(0);
            const xvalue_t & property_name  = op.get_method_params().at(1);
            const xvalue_t & property_value = op.get_method_params().at(2);
            
            #ifdef _DEBUG_STATE_BINARY_
            xinfo("xvexestate_t::do_renew,name(%s)<->value(%s)",property_name.get_string().c_str(),property_value.dump().c_str());
            #endif //end of _DEBUG_STATE_BINARY_
            
            xvproperty_t * target_property = get_property_object(property_name.get_string());
            if(target_property == nullptr)
            {
                const xvalue_t new_result(do_new_property(op,canvas));
                if(new_result.get_error() != enum_xcode_successful)
                {
                    xerror("xvexestate_t::do_renew_property,fail new property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
                    return new_result;
                }
                target_property = get_property_object(property_name.get_string());
            }
            if(target_property->copy_from_value(property_value))
                return xvalue_t(enum_xcode_successful);
            
            xerror("xvexestate_t::do_renew_property,fail reset property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
            return xvalue_t(enum_xerror_code_fail);
        }
    
        bool    xvexestate_t::del_property(const std::string & property_name,xvcanvas_t * canvas)
        {
            xvalue_t param_pname(property_name);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_del_property,param_pname);
            
            //excute the instruction
            auto result = execute(instruction,canvas);
            if(result.get_error() == enum_xcode_successful)
                return true;
            
            xerror("xvexestate_t::del_property,fail for property_name(%s) at xbstate(%s) as error(%d)",property_name.c_str(),dump().c_str(),result.get_error());
            return false;
        }
    
        const xvalue_t  xvexestate_t::do_del_property(const xvmethod_t & op,xvcanvas_t * canvas)
        {
            if(op.get_method_type() != enum_xvinstruct_class_state_function)
                return xvalue_t(enum_xerror_code_bad_type);
            
            if(op.get_method_id() != enum_xvinstruct_state_method_del_property)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_params_count() != 1) //reset must carry new value
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & property_name  = op.get_method_params().at(0);
            if(remove_child_unit(property_name.get_string()))
                return xvalue_t(enum_xcode_successful);
            
            xerror("xvexestate_t::do_renew_property,fail del property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
            return xvalue_t(enum_xerror_code_fail);
        }
        std::string xvexestate_t::export_state()
        {
            base::xstream_t _stream(base::xcontext_t::instance());            
            uint8_t version = 0;
            _stream << version;
            int32_t ret = xvexestate_t::do_write(_stream); // not include xvbstate do_write
            if (ret <= 0) {
                xerror("xvexestate_t::reset_state fail-serialize.error=%d",ret);
                return std::string();
            }
            std::string state_bin((char*)_stream.data(), _stream.size());
            return state_bin;
        }

        bool xvexestate_t::reset_state(const std::string & snapshot, xvcanvas_t * canvas)
        {
            xvalue_t param_state(snapshot);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_reset_state,param_state);

            const xvalue_t result(execute(instruction,canvas)); //excute the instruction
            if(result.get_error() == enum_xcode_successful) {
                xdbg("xvexestate_t::reset_state succ.uri=%s",get_execute_uri().c_str());
                return true;
            }                
            
            xwarn("xvexestate_t::reset_state fail.error=%d",result.get_error());
            return false;            
        }

        const xvalue_t  xvexestate_t::do_reset_state(const xvmethod_t & op,xvcanvas_t * canvas)
        {
            xdbg("xvexestate_t::do_reset_state op=%s",op.dump().c_str());
            if(op.get_method_type() != enum_xvinstruct_class_state_function)
                return xvalue_t(enum_xerror_code_bad_type);
            
            if(op.get_method_id() != enum_xvinstruct_state_method_reset_state)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_params_count() != 1) //reset must carry new value
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & state_value  = op.get_method_params().at(0);
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)state_value.get_string().data(),(int32_t)state_value.get_string().size());
            uint8_t version = 0;
            _stream >> version;

            if (version != 0) {
                xerror("xvexestate_t::do_reset_state,fail version invalid.version=%d,value_size=%zu at xbstate(%s)",version,state_value.get_string().size(), dump().c_str());
                return xvalue_t(enum_xerror_code_fail);
            }

            remove_all_child_unit();
            int32_t ret = xvexestate_t::do_read(_stream);
            if (ret > 0)
                return xvalue_t(enum_xcode_successful);
            
            xerror("xvexestate_t::do_reset_state,fail reset.value_size=%zu at xbstate(%s),ret=%d",state_value.get_string().size(), dump().c_str(), ret);
            return xvalue_t(enum_xerror_code_fail);            
        }
            
        const std::string  xvbstate_t::make_unit_name(const std::string & account, const uint64_t blockheight)
        {
            const std::string compose_name = account + "/" + xstring_utl::tostring(blockheight);
            return compose_name;
        }
    
        void xvbstate_t::register_object(xcontext_t & context)
        {
            auto lambda_new_bstate = [](const int type)->xobject_t*{
                return new xvbstate_t();
            };
            xcontext_t::register_xobject2(context,(enum_xobject_type)xvbstate_t::enum_obj_type,lambda_new_bstate);
            
            xvexestate_t::register_object(context);
        }
            
        xvbstate_t::xvbstate_t(enum_xdata_type type)
            :xvexestate_t(type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first
            m_block_height = 0;
            m_block_viewid = 0;
            m_last_full_block_height = 0;
            m_block_versions = 0;
            m_block_types    = 0;
            
            //then set unit name
            //set_unit_name(make_unit_name(std::string(),m_block_height));
            //ask compressed data while serialization
            //set_unit_flag(enum_xdata_flag_acompress);
        }
        
        xvbstate_t::xvbstate_t(const xvblock_t& for_block,xvexeunit_t * parent_unit,enum_xdata_type type)
            :xvexestate_t(for_block.get_account(),type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first
            m_block_types    = for_block.get_header()->get_block_raw_types();
            m_block_versions = for_block.get_header()->get_block_raw_versions();
            
            m_block_height = for_block.get_height();
            m_block_viewid = for_block.get_viewid();
            
            m_last_block_hash = for_block.get_last_block_hash();
            m_last_full_block_hash = for_block.get_last_full_block_hash();
            m_last_full_block_height = for_block.get_last_full_block_height();
            
            //then set unit name
            set_unit_name(make_unit_name(get_address(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);

            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
        }
        
        xvbstate_t::xvbstate_t(const xvblock_t& for_block,xvbstate_t & clone_from,xvexeunit_t * parent_unit,enum_xdata_type type)
            :xvexestate_t(for_block.get_account(),type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first
            m_block_types    = for_block.get_header()->get_block_raw_types();
            m_block_versions = for_block.get_header()->get_block_raw_versions();
            
            m_block_height = for_block.get_height();
            m_block_viewid = for_block.get_viewid();
            
            m_last_block_hash = for_block.get_last_block_hash();
            m_last_full_block_hash = for_block.get_last_full_block_hash();
            m_last_full_block_height = for_block.get_last_full_block_height();
            
            //then set unit name
            set_unit_name(make_unit_name(get_address(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);

            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
            
            clone_properties_from(clone_from);
        }
        
        xvbstate_t::xvbstate_t(const xvheader_t& proposal_header,xvbstate_t & clone_from,uint64_t viewid,xvexeunit_t * parent_unit,enum_xdata_type type)
        :xvexestate_t(proposal_header.get_account(),type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first
            m_block_types    = proposal_header.get_block_raw_types();
            m_block_versions = proposal_header.get_block_raw_versions();
            
            m_block_height = proposal_header.get_height();
            m_block_viewid = viewid;
            
            m_last_block_hash = proposal_header.get_last_block_hash();
            m_last_full_block_hash = proposal_header.get_last_full_block_hash();
            m_last_full_block_height = proposal_header.get_last_full_block_height();
            
            //then set unit name
            set_unit_name(make_unit_name(get_address(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);
            
            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
            
            clone_properties_from(clone_from);
        }

        xvbstate_t::xvbstate_t(const std::string & last_block_hash, xvbstate_t & clone_from, enum_xdata_type type)
        :xvexestate_t(clone_from.get_account(),type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first          
            m_block_height = clone_from.get_block_height() + 1;            
            m_last_block_hash = last_block_hash;
            
            //then set unit name
            set_unit_name(make_unit_name(get_address(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);                      
            clone_properties_from(clone_from);
        }
        
        xvbstate_t::xvbstate_t(const xvheader_t& proposal_header,xvexeunit_t * parent_unit,enum_xdata_type type)
        :xvexestate_t(proposal_header.get_account(),type), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first
            m_block_types    = proposal_header.get_block_raw_types();
            m_block_versions = proposal_header.get_block_raw_versions();
            
            m_block_height = proposal_header.get_height();
            xassert(m_block_height == 0); // it must be genesis block
            m_block_viewid = 0;
            
            m_last_block_hash = proposal_header.get_last_block_hash();
            m_last_full_block_hash = proposal_header.get_last_full_block_hash();
            m_last_full_block_height = proposal_header.get_last_full_block_height();
            
            //then set unit name
            set_unit_name(make_unit_name(get_address(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);
            
            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
        }
        
        //debug & ut-test only
        xvbstate_t::xvbstate_t(const std::string & account,const uint64_t block_height,const uint64_t block_viewid,const std::string & last_block_hash,const std::string &last_full_block_hash,const uint64_t last_full_block_height, const uint32_t raw_block_versions,const uint16_t raw_block_types, xvexeunit_t * parent_unit)
            :xvexestate_t(account,(enum_xdata_type)enum_xobject_type_vbstate), xstatistic::xstatistic_obj_face_t(xstatistic::enum_statistic_vbstate)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
            //init unit name and block height first
            m_block_types    = raw_block_types;
            m_block_versions = raw_block_versions;
            
            m_block_height = block_height;
            m_block_viewid = block_viewid;
            
            m_last_block_hash = last_block_hash;
            m_last_full_block_hash = last_full_block_hash;
            m_last_full_block_height = last_full_block_height;
            
            //then set unit name
            set_unit_name(make_unit_name(get_address(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);

            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
        }
        
        xvbstate_t::xvbstate_t(const xvbstate_t & obj)
            :xvexestate_t(obj), xstatistic::xstatistic_obj_face_t(obj)
        {
            m_block_types    = obj.m_block_types;
            m_block_versions = obj.m_block_versions;
            
            m_block_height = obj.m_block_height;
            m_block_viewid = obj.m_block_viewid;
            
            m_last_block_hash = obj.m_last_block_hash;
            m_last_full_block_hash = obj.m_last_full_block_hash;
            m_last_full_block_height = obj.m_last_full_block_height;
            
            set_unit_name(make_unit_name(get_address(),m_block_height)); //set unit name first
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);

            //finally set parent ptr
            set_parent_unit(obj.get_parent_unit());
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, 1);
        }
        
        xvbstate_t::~xvbstate_t()
        {
            statistic_del();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvbstate, -1);
        }

        void xvbstate_t::update_final_block_info(xvheader_t* _header, uint64_t viewid) {
            assert(m_block_height == _header->get_height());
            assert(m_last_block_hash == _header->get_last_block_hash()); 
            assert(viewid > 0);           
            m_block_types    = _header->get_block_raw_types();
            m_block_versions = _header->get_block_raw_versions();
            m_block_height = _header->get_height();
            m_block_viewid = viewid;
            m_last_block_hash = _header->get_last_block_hash();
            m_last_full_block_hash = _header->get_last_full_block_hash();
            m_last_full_block_height = _header->get_last_full_block_height();
        }

        xvexeunit_t* xvbstate_t::clone() //each property is readonly after clone
        {
            return new xvbstate_t(*this);
        }
        xvbstate_t* xvbstate_t::clone_state()
        {
            return new xvbstate_t(*this);
        }
        
        std::string xvbstate_t::dump() const
        {
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"{xvbstate:%s,height=%" PRIu64 ",viewid=%" PRIu64 ",class=%d,type=%d}",
             get_account().c_str(),get_block_height(),get_block_viewid(),get_block_class(),get_block_type());
            return std::string(local_param_buf);
        }
        
        void*   xvbstate_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vbstate)
                return this;
            
            return xvexestate_t::query_interface(_enum_xobject_type_);
        }
        
        const int   xvbstate_t::get_block_level() const
        {
            return xvheader_t::cal_block_level(m_block_types);
        }
        
        const int   xvbstate_t::get_block_class() const
        {
            return xvheader_t::cal_block_class(m_block_types);
        }
        
        const int    xvbstate_t::get_block_type() const
        {
            return xvheader_t::cal_block_type(m_block_types);
        }

        std::vector<uint8_t> xvbstate_t::get_property_value_in_bytes(std::string const & property_name) const {
            auto const & value_data = get_property_value(property_name);
            return {std::begin(value_data), std::end(value_data)};
        }

        void xvbstate_t::set_property_value_from_bytes(std::string const & name, std::vector<uint8_t> const & bin_data, xvcanvas_t * canvas) {
            //std::string const & value_data = {std::begin(bin_data), std::end(bin_data)};
            //set_property_value(name, value_data, canvas);
        }

        //subclass extend behavior and load more information instead of a raw one
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t    xvbstate_t::do_write(xstream_t & stream)  //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            
            stream << m_block_types;
            stream << m_block_versions;
            
            stream.write_compact_var(m_block_height);
            stream.write_compact_var(m_block_viewid);
            stream.write_compact_var(m_last_full_block_height);
            
            stream.write_tiny_string(get_address());
            stream.write_tiny_string(m_last_block_hash);
            stream.write_tiny_string(m_last_full_block_hash);
            
            xvexestate_t::do_write(stream);
            return (stream.size() - begin_size);
        }
        
        int32_t   xvbstate_t::do_read(xstream_t & stream) //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            
            stream >> m_block_types;
            stream >> m_block_versions;
            
            stream.read_compact_var(m_block_height);
            stream.read_compact_var(m_block_viewid);
            stream.read_compact_var(m_last_full_block_height);
            
            std::string account_addr;
            stream.read_tiny_string(account_addr);
            stream.read_tiny_string(m_last_block_hash);
            stream.read_tiny_string(m_last_full_block_hash);
            
            xvaccount_t::operator=(account_addr);//finaly set account address back
            
            //set unit name immidiately after read them
            set_unit_name(make_unit_name(account_addr,m_block_height));
            xvexestate_t::do_read(stream);
            return (begin_size - stream.size());
        }
    
        //---------------------------------bin log ---------------------------------//
        bool   xvbstate_t::apply_changes_of_binlog(std::deque<base::xvmethod_t> && records) //apply changes to current states
        {
            for(auto & op : records)
            {
                #ifdef _DEBUG_STATE_BINARY_
                xinfo("xvbstate_t::apply,execute %s",op.dump().c_str());
                #endif
                execute(op,nullptr);
            }
            return true;
        }

        bool   xvbstate_t::apply_changes_of_binlog(const std::string & from_bin_log) //apply changes to current states
        {
            std::deque<top::base::xvmethod_t> out_records;
            if(xvcanvas_t::decode_from(from_bin_log, out_records) >= enum_xcode_successful)
            {
                for(auto & op : out_records)
                {
                    #ifdef _DEBUG_STATE_BINARY_
                    xinfo("xvbstate_t::apply,execute %s",op.dump().c_str());
                    #endif
                    execute(op,nullptr);
                }
                return true;
            }
            #ifdef DEBUG
                xerror("decompile_from_binlog failed for bin-log(%s)",from_bin_log.c_str());
            #else
                xerror("decompile_from_binlog failed for bin-log,length(%u)",(uint32_t)from_bin_log.size());
            #endif
            return false;
        }
    
        bool   xvbstate_t::apply_changes_of_binlog(xstream_t & from_bin_log,const uint32_t bin_log_size) //apply changes to current states
        {
            std::deque<top::base::xvmethod_t> out_records;
            if(xvcanvas_t::decode_from(from_bin_log,bin_log_size,out_records) >= enum_xcode_successful)
            {
                for(auto & op : out_records)
                {
                    #ifdef _DEBUG_STATE_BINARY_
                    xinfo("xvbstate_t::apply,execute %s",op.dump().c_str());
                    #endif
                    execute(op,nullptr);
                }
                return true;
            }
            xerror("decompile_from_binlog failed for bin-log,length(%u)",(uint32_t)from_bin_log.size());
            return false;
        }

        size_t xvbstate_t::get_object_size_real() const {
            size_t total_size = sizeof(*this);
            auto ex_size = get_ex_alloc_size();
            total_size +=
                get_size(m_last_block_hash) + get_size(m_last_full_block_hash) + get_size(get_address()) + get_size(get_storage_key()) + ex_size;
            xdbg("------cache size------ xvbstate_t total_size:%zu this:%d,m_last_block_hash:%d,m_last_full_block_hash:%d,address:%d,storage_key:%d,ex_size:%d",
                 total_size,
                 sizeof(*this),
                 get_size(m_last_block_hash),
                 get_size(m_last_full_block_hash),
                 get_size(get_address()),
                 get_size(get_storage_key()),
                 ex_size);
            return total_size;
        }
    };
};
