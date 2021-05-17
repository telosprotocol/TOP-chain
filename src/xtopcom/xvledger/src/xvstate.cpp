// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <stddef.h>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "../xvblock.h"
#include "../xvstate.h"
#include "xbase/xbase.h"
#include "xbase/xdata.h"
#include "xbase/xhash.h"
#include "xbase/xmem.h"
#include "xbase/xobject.h"
#include "xbase/xvmethod.h"
#include "xvexeunit.h"
#include "xvinstruction.h"
#include "xvproperty.h"

namespace top
{
    namespace base
    {
        //*************************************xvbstate_t****************************************
        void xvbstate_t::register_object(xcontext_t & context)
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
    
        const std::string  xvbstate_t::make_unit_name(const std::string & account, const uint64_t blockheight)
        {
            const std::string compose_name = account + "." + xstring_utl::tostring(blockheight);
            return xstring_utl::tostring((uint32_t)xhash64_t::digest(compose_name));//to save space,let use hash32 as unit name for vbstate
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
 
        xvbstate_t::xvbstate_t(enum_xdata_type type)
            :base(type)
        {
            //init unit name and block height first
            m_block_height = 0;
            m_block_viewid = 0;
            m_last_full_block_height = 0;
            m_block_versions = 0;
            m_block_types    = 0;
            
            //then set unit name
            set_unit_name(make_unit_name(std::string(),m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);
            
            xauto_ptr<xvcanvas_t> new_canvas(new xvcanvas_t());
            set_canvas(new_canvas.get());
            
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }

        xvbstate_t::xvbstate_t(xvblock_t& for_block,xvexeunit_t * parent_unit,enum_xdata_type type)
            :base(type)
        {
            //init unit name and block height first
            m_block_types    = for_block.get_header()->get_block_raw_types();
            m_block_versions = for_block.get_header()->get_block_raw_versions();
            
            m_account_addr = for_block.get_account();
            m_block_height = for_block.get_height();
            m_block_viewid = for_block.get_viewid();
            
            m_last_block_hash = for_block.get_last_block_hash();
            m_last_full_block_hash = for_block.get_last_full_block_hash();
            m_last_full_block_height = for_block.get_last_full_block_height();
            
            //then set unit name
            set_unit_name(make_unit_name(m_account_addr,m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);

            xauto_ptr<xvcanvas_t> new_canvas(new xvcanvas_t());
            set_canvas(new_canvas.get());
            
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
            
            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
        }
    
        //debug & ut-test only
        xvbstate_t::xvbstate_t(const std::string & account,const uint64_t block_height,const uint64_t block_viewid,const std::string & last_block_hash,const std::string &last_full_block_hash,const uint64_t last_full_block_height, const uint32_t raw_block_versions,const uint16_t raw_block_types, xvexeunit_t * parent_unit)
            :base((enum_xdata_type)enum_xobject_type_vbstate)
        {
            //init unit name and block height first
            m_block_types    = raw_block_types;
            m_block_versions = raw_block_versions;
            
            m_account_addr = account;
            m_block_height = block_height;
            m_block_viewid = block_viewid;
            
            m_last_block_hash = last_block_hash;
            m_last_full_block_hash = last_full_block_hash;
            m_last_full_block_height = last_full_block_height;
            
            //then set unit name
            set_unit_name(make_unit_name(m_account_addr,m_block_height));
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);
            
            xauto_ptr<xvcanvas_t> new_canvas(new xvcanvas_t());
            set_canvas(new_canvas.get());
            
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
            
            if(parent_unit != NULL)
                set_parent_unit(parent_unit);
        }

        xvbstate_t::xvbstate_t(const xvbstate_t & obj)
            :base(obj)
        {
            m_block_types    = obj.m_block_types;
            m_block_versions = obj.m_block_versions;
            
            m_account_addr = obj.m_account_addr;
            m_block_height = obj.m_block_height;
            m_block_viewid = obj.m_block_viewid;
            
            m_last_block_hash = obj.m_last_block_hash;
            m_last_full_block_hash = obj.m_last_full_block_hash;
            m_last_full_block_height = obj.m_last_full_block_height;
            
            set_unit_name(make_unit_name(m_account_addr,m_block_height)); //set unit name first
            //ask compressed data while serialization
            set_unit_flag(enum_xdata_flag_acompress);
            
            //setup canvas
            xauto_ptr<xvcanvas_t> new_canvas(new xvcanvas_t());
            set_canvas(new_canvas.get());
            //then register execution methods
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
            
            //finally set parent ptr
            set_parent_unit(obj.get_parent_unit());
        }
    
        xvbstate_t::~xvbstate_t()
        {
        }
        
        xvexeunit_t* xvbstate_t::clone() //each property is readonly after clone
        {
            return new xvbstate_t(*this);
        }
        
        bool    xvbstate_t::clone_properties_from(xvbstate_t& source)//note: just only clone the state of properties
        {
            xassert(get_child_units().empty());//must be empty
            return clone_units_from(source);
        }
 
        std::string xvbstate_t::dump() const
        {
            return std::string();
        }
 
        void*   xvbstate_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vbstate)
                return this;
            
            return base::query_interface(_enum_xobject_type_);
        }
    
        //clear canvas assocaited with vbstate and properties,all recored instruction are clear as well
        //note:reset_canvas not modify the actua state of properties/block, it just against for instrution on canvas
        bool         xvbstate_t::reset_canvas()
        {
            //setup canvas
            xauto_ptr<xvcanvas_t> new_canvas(new xvcanvas_t());
            set_canvas(new_canvas.get());
            return true;
        }
    
        std::string  xvbstate_t::get_property_value(const std::string & name)
        {
            std::string bin_data;
            xvproperty_t* target = get_property_object(name);
            if(target != nullptr)
                target->serialize_to_string(bin_data);

            return bin_data;
        }
        
        xvproperty_t*   xvbstate_t::get_property_object(const std::string & name)
        {
            xvexeunit_t * target = find_child_unit(name);
            if(target != nullptr)
                return (xvproperty_t*)target;
            
            return nullptr;
        }
    
        bool  xvbstate_t::find_property(const std::string & property_name) //check whether property already existing
        {
            if(get_property_object(property_name) != nullptr)
                return true;
            
            return false;
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
            
            stream.write_tiny_string(m_account_addr);
            stream.write_tiny_string(m_last_block_hash);
            stream.write_tiny_string(m_last_full_block_hash);

            base::do_write(stream);
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
            
            stream.read_tiny_string(m_account_addr);
            stream.read_tiny_string(m_last_block_hash);
            stream.read_tiny_string(m_last_full_block_hash);
            
            //set unit name immidiately after read them
            set_unit_name(make_unit_name(m_account_addr,m_block_height));
            base::do_read(stream);
            
            return (begin_size - stream.size());
        }
    
        xauto_ptr<xtokenvar_t>  xvbstate_t::new_token_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_token));
            if(result.get_error() == enum_xcode_successful)
                return load_token_var(property_name);
            
            xerror("xvbstate_t::new_token_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xnoncevar_t>      xvbstate_t::new_nonce_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_nonce));
            if(result.get_error() == enum_xcode_successful)
                return load_nonce_var(property_name);
            
            xerror("xvbstate_t::new_nonce_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
 
        xauto_ptr<xcodevar_t>   xvbstate_t::new_code_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_code));
            if(result.get_error() == enum_xcode_successful)
                return load_code_var(property_name);
            
            xerror("xvbstate_t::new_code_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmtokens_t>   xvbstate_t::new_multiple_tokens_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_mtokens));
            if(result.get_error() == enum_xcode_successful)
                return load_multiple_tokens_var(property_name);
            
            xerror("xvbstate_t::new_multiple_tokens_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmkeys_t>   xvbstate_t::new_multiple_keys_var(const std::string & property_name) //to manage pubkeys of account
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_mkeys));
            if(result.get_error() == enum_xcode_successful)
                return load_multiple_keys_var(property_name);
            
            xerror("xvbstate_t::new_multiple_keys_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
   
        xauto_ptr<xstringvar_t> xvbstate_t::new_string_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_string));
            if(result.get_error() == enum_xcode_successful)
                return load_string_var(property_name);
            
            xerror("xvbstate_t::new_string_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }

        xauto_ptr<xhashmapvar_t>   xvbstate_t::new_hashmap_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_hashmap));
            if(result.get_error() == enum_xcode_successful)
                return load_hashmap_var(property_name);
            
            xerror("xvbstate_t::new_hashmap_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }

        //integer related
        xauto_ptr<xvintvar_t<int64_t>> xvbstate_t::new_int64_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int64));
            if(result.get_error() == enum_xcode_successful)
                return load_int64_var(property_name);
            
            xerror("xvbstate_t::new_int64_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xvintvar_t<uint64_t>> xvbstate_t::new_uint64_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_uint64));
            if(result.get_error() == enum_xcode_successful)
                return load_uint64_var(property_name);
            
            xerror("xvbstate_t::new_uint64_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        //xdequevar_t related
        xauto_ptr<xdequevar_t<int8_t>>   xvbstate_t::new_int8_deque_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int8_deque));
            if(result.get_error() == enum_xcode_successful)
                return load_int8_deque_var(property_name);
            
            xerror("xvbstate_t::new_int8_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
     
        xauto_ptr<xdequevar_t<int16_t>>  xvbstate_t::new_int16_deque_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int16_deque));
            if(result.get_error() == enum_xcode_successful)
                return load_int16_deque_var(property_name);
            
            xerror("xvbstate_t::new_int16_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int32_t>>  xvbstate_t::new_int32_deque_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int32_deque));
            if(result.get_error() == enum_xcode_successful)
                return load_int32_deque_var(property_name);
            
            xerror("xvbstate_t::new_int32_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int64_t>>  xvbstate_t::new_int64_deque_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int64_deque));
            if(result.get_error() == enum_xcode_successful)
                return load_int64_deque_var(property_name);
            
            xerror("xvbstate_t::new_int64_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<uint64_t>>  xvbstate_t::new_uint64_deque_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_uint64_deque));
            if(result.get_error() == enum_xcode_successful)
                return load_uint64_deque_var(property_name);
            
            xerror("xvbstate_t::new_uint64_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<std::string>>  xvbstate_t::new_string_deque_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_string_deque));
            if(result.get_error() == enum_xcode_successful)
                return load_string_deque_var(property_name);
            
            xerror("xvbstate_t::new_string_deque_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        //xmapvar_t related
        xauto_ptr<xmapvar_t<int8_t>>   xvbstate_t::new_int8_map_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int8_map));
            if(result.get_error() == enum_xcode_successful)
                return load_int8_map_var(property_name);
            
            xerror("xvbstate_t::new_int8_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int16_t>>   xvbstate_t::new_int16_map_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int16_map));
            if(result.get_error() == enum_xcode_successful)
                return load_int16_map_var(property_name);
            
            xerror("xvbstate_t::new_int16_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int32_t>>   xvbstate_t::new_int32_map_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int32_map));
            if(result.get_error() == enum_xcode_successful)
                return load_int32_map_var(property_name);
            
            xerror("xvbstate_t::new_int32_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int64_t>>   xvbstate_t::new_int64_map_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_int64_map));
            if(result.get_error() == enum_xcode_successful)
                return load_int64_map_var(property_name);
            
            xerror("xvbstate_t::new_int64_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmapvar_t<uint64_t>>   xvbstate_t::new_uint64_map_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_uint64_map));
            if(result.get_error() == enum_xcode_successful)
                return load_uint64_map_var(property_name);
            
            xerror("xvbstate_t::new_uint64_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmapvar_t<std::string>>   xvbstate_t::new_string_map_var(const std::string & property_name)
        {
            const xvalue_t result(new_property_internal(property_name,enum_xobject_type_vprop_string_map));
            if(result.get_error() == enum_xcode_successful)
                return load_string_map_var(property_name);
            
            xerror("xvbstate_t::new_string_map_var,failed as error:%d for name(%s)",result.get_error(),property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xtokenvar_t>  xvbstate_t::load_token_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_token_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xnoncevar_t>  xvbstate_t::load_nonce_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_nonce_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xcodevar_t>  xvbstate_t::load_code_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_code_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xmtokens_t>    xvbstate_t::load_multiple_tokens_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_multiple_tokens_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmkeys_t>    xvbstate_t::load_multiple_keys_var(const std::string & property_name)//to manage pubkeys of account
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
            xerror("xvbstate_t::load_multiple_keys_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
 
        xauto_ptr<xstringvar_t> xvbstate_t::load_string_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_string_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }

        xauto_ptr<xhashmapvar_t>xvbstate_t::load_hashmap_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_hashmap_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        //integer related load functions
        xauto_ptr<xvintvar_t<int64_t>>  xvbstate_t::load_int64_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int64_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xvintvar_t<uint64_t>>  xvbstate_t::load_uint64_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_uint64_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        //load functions of deque related
        xauto_ptr<xdequevar_t<int8_t>> xvbstate_t::load_int8_deque_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int8_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int16_t>> xvbstate_t::load_int16_deque_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int16_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int32_t>> xvbstate_t::load_int32_deque_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int32_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<int64_t>> xvbstate_t::load_int64_deque_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int64_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<uint64_t>> xvbstate_t::load_uint64_deque_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_uint64_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xdequevar_t<std::string>> xvbstate_t::load_string_deque_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_string_deque_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        //load functions of map related
        xauto_ptr<xmapvar_t<int8_t>>   xvbstate_t::load_int8_map_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int8_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int16_t>>   xvbstate_t::load_int16_map_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int16_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int32_t>>   xvbstate_t::load_int32_map_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int32_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<int64_t>>   xvbstate_t::load_int64_map_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_int64_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<uint64_t>>   xvbstate_t::load_uint64_map_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_uint64_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
        
        xauto_ptr<xmapvar_t<std::string>>   xvbstate_t::load_string_map_var(const std::string & property_name)
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
            xerror("xvbstate_t::load_string_map_var,failed to load for name(%s)",property_name.c_str());
            return nullptr;
        }
    
        xauto_ptr<xvproperty_t> xvbstate_t::load_property(const std::string & property_name)
        {
            xvproperty_t * found_target = get_property_object(property_name);
            if(found_target != nullptr)
                found_target->add_ref();//add reference for xauto_ptr;
            return found_target;
        }
    
        xauto_ptr<xvproperty_t> xvbstate_t::new_property(const std::string & property_name,const int propertyType)
        {
            const xvalue_t result(new_property_internal(property_name,propertyType));
            if(result.get_error() == enum_xcode_successful)
                return load_property(property_name);
            
            xerror("xvbstate_t::new_property,failed as error:%d for name(%s) and type(%d)",result.get_error(),property_name.c_str(),propertyType);
            return nullptr;
        }
    
        const xvalue_t xvbstate_t::new_property_internal(const std::string & property_name,const int propertyType)
        {
            xvalue_t param_ptype((vint32_t)propertyType);
            xvalue_t param_pname(property_name);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_new_property ,param_ptype,param_pname);
            
            return execute(instruction,(xvcanvas_t*)get_canvas()); //excute the instruction
        }
    
        const xvalue_t  xvbstate_t::do_new_property(const xvmethod_t & op)
        {
            if(op.get_params_count() < 2) //reset must carry type and name at least
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & property_type = op.get_method_params().at(0);
            const xvalue_t & property_name = op.get_method_params().at(1);
        
            xvproperty_t* existingOne = get_property_object(property_name.get_string());
            if(existingOne != nullptr)
            {
                xerror("xvbstate_t::do_new_property,try overwrite existing property of account(%s) at height(%lld),new-property(%s)",get_account_addr().c_str(),get_block_height(),property_name.get_string().c_str());
                return xvalue_t(enum_xerror_code_exist);
            }
            if(get_childs_count() >= enum_max_property_count)
            {
                xerror("xvbstate_t::do_new_property,the account has too many properites already,account(%s) at height(%lld)",get_account_addr().c_str(),get_block_height());
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
                    property_obj->set_canvas(get_canvas());
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
    
        bool   xvbstate_t::reset_property(const std::string & property_name,const xvalue_t & property_value)
        {
            xvalue_t param_pname(property_name);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_reset_property ,param_pname,(xvalue_t&)property_value);
            
            //excute the instruction
            auto result = execute(instruction,(xvcanvas_t*)get_canvas());
            if(result.get_error() == enum_xcode_successful)
                return true;
            
            xerror("xvbstate_t::reset_property,fail for property_name(%s) at xbstate(%s) as error(%d)",property_name.c_str(),dump().c_str(),result.get_error());
            return false;
        }

        const xvalue_t  xvbstate_t::do_reset_property(const xvmethod_t & op)
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
                xerror("xvbstate_t::do_reset_property,NOT find property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
                return xvalue_t(enum_xerror_code_not_found);
            }
            //note:here move property_value with better performance
            if(target_property->move_from_value((xvalue_t &)property_value))
                return xvalue_t(enum_xcode_successful);
            
            xerror("xvbstate_t::do_reset_property,fail reset property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
            return xvalue_t(enum_xerror_code_fail);
        }
    
        xvmethod_t  xvbstate_t::renew_property_instruction(const std::string & property_name,const int  property_type,const xvalue_t & property_value)
        {
            xvalue_t param_ptype((vint32_t)property_type);//#parm-0
            xvalue_t param_pname(property_name); //#parm-1
            return xvmethod_t(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_renew_property ,param_ptype,param_pname,(xvalue_t&)property_value);
        }
    
        bool   xvbstate_t::renew_property(const std::string & property_name,const int property_type,const xvalue_t & property_value)
        {
            xvmethod_t instruction(renew_property_instruction(property_name,property_type,property_value));
            
            //excute the instruction
            auto result = execute(instruction,(xvcanvas_t*)get_canvas());
            if(result.get_error() == enum_xcode_successful)
                return true;
            
            xerror("xvbstate_t::renew_property,fail for property_name(%s) at xbstate(%s) as error(%d)",property_name.c_str(),dump().c_str(),result.get_error());
            return false;
        }
        
        const xvalue_t  xvbstate_t::do_renew_property(const xvmethod_t & op)
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
            
            xvproperty_t * target_property = get_property_object(property_name.get_string());
            if(target_property == nullptr)
            {
                const xvalue_t new_result(do_new_property(op));
                if(new_result.get_error() != enum_xcode_successful)
                {
                    xerror("xvbstate_t::do_renew_property,fail new property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
                    return new_result;
                }
                target_property = get_property_object(property_name.get_string());
            }
            //note:here move property_value with better performance
            if(target_property->move_from_value((xvalue_t &)property_value))
                return xvalue_t(enum_xcode_successful);
            
            xerror("xvbstate_t::do_renew_property,fail reset property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
            return xvalue_t(enum_xerror_code_fail);
        }
    
        bool    xvbstate_t::del_property(const std::string & property_name)
        {
            xvalue_t param_pname(property_name);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_del_property,param_pname);
            
            //excute the instruction
            auto result = execute(instruction,(xvcanvas_t*)get_canvas());
            if(result.get_error() == enum_xcode_successful)
                return true;
            
            xerror("xvbstate_t::del_property,fail for property_name(%s) at xbstate(%s) as error(%d)",property_name.c_str(),dump().c_str(),result.get_error());
            return false;
        }
    
        const xvalue_t  xvbstate_t::do_del_property(const xvmethod_t & op)
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
            
            xerror("xvbstate_t::do_renew_property,fail del property(%s) at xbstate(%s)",property_name.get_string().c_str(),dump().c_str());
            return xvalue_t(enum_xerror_code_fail);
        }
        
        //---------------------------------bin log ---------------------------------//
        enum_xerror_code  xvbstate_t::encode_change_to_binlog(xvcanvas_t* source_canvas,std::string & output_bin)
        {
            try{
                const int result = source_canvas->encode(xvcanvas_t::enum_compile_optimization_all,output_bin);
                if(result >= enum_xcode_successful)
                {
                    const char bin_type = '1';  //version#1: new bin-log format
                    output_bin.append(1,bin_type);
                    return enum_xcode_successful;
                }
                else
                {
                    xerror("xvbstate_t::encode_change_to_binlog,encode failed as error:%d",result);
                    return (enum_xerror_code)result;
                }
            } catch (int error_code){
                xerror("xvbstate_t::encode_change_to_binlog,throw exception with error:%d",error_code);
                return enum_xerror_code_errno;
            }
            xerror("xvbstate_t::encode_change_to_binlog,throw unknow exception");
            return enum_xerror_code_fail;
        }
        
        enum_xerror_code  xvbstate_t::encode_change_to_binlog(std::string & output_bin)
        {
           return encode_change_to_binlog(get_canvas(),output_bin);
        }
    
        enum_xerror_code  xvbstate_t::encode_change_to_binlog(xstream_t & _ouptput_stream)
        {
            try{
                const int result = get_canvas()->encode(xvcanvas_t::enum_compile_optimization_all,_ouptput_stream);
                //const int result = get_canvas()->encode(xvcanvas_t::enum_compile_optimization_none,_ouptput_stream);
                if(result >= enum_xcode_successful)
                {
                    const char bin_type = '1';  //version#1: new bin-log format
                    _ouptput_stream.push_back((uint8_t*)&bin_type, 1);
                    return enum_xcode_successful;
                }
                else
                {
                    xerror("xvbstate_t::encode_change_to_binlog,encode failed as error:%d",result);
                    return (enum_xerror_code)result;
                }
            } catch (int error_code){
                xerror("xvbstate_t::encode_change_to_binlog,throw exception with error:%d",error_code);
                return enum_xerror_code_errno;
            }
            xerror("xvbstate_t::encode_change_to_binlog,throw unknow exception");
            return enum_xerror_code_fail;
        }
        
        bool  xvbstate_t::rebase_change_to_snapshot() //snapshot for whole xvbstate of every properties
        {
            xauto_ptr<xvcanvas_t> new_canvas(new xvcanvas_t());
            
            const std::map<std::string,xvexeunit_t*> & all_units = get_child_units();
            for(auto & it : all_units)
            {
                xvproperty_t * property_ptr = (xvproperty_t*)it.second;
                xvmethod_t instruction(renew_property_instruction(property_ptr->get_name(),property_ptr->get_obj_type(),property_ptr->get_value()));
                
                if(false == new_canvas->record(this, instruction))
                {
                    xerror("xvbstate_t::take_snapshot_to_binlog,abort as property fail to take snapshot,propery(%s)",it.second->dump().c_str());
                    return false;
                }
            }
            
            set_canvas(new_canvas.get()); //clean all recorded changes by replacing with new canvas
            return true;
        }
        
        enum_xerror_code   xvbstate_t::decode_change_from_binlog(const std::string & from_bin_log,std::deque<top::base::xvmethod_t> & out_records)
        {
            xstream_t _input_stream(xcontext_t::instance(),(uint8_t*)from_bin_log.data(),(uint32_t)from_bin_log.size());
            return decode_change_from_binlog(_input_stream,_input_stream.size(),out_records);
        }
    
        enum_xerror_code   xvbstate_t::decode_change_from_binlog(xstream_t & from_bin_log,const uint32_t bin_log_size,std::deque<top::base::xvmethod_t> & out_records)
        {
            if( (from_bin_log.size() < 1) || ((uint32_t)from_bin_log.size() < bin_log_size) )//at least one bytes
            {
                xerror("xvbstate_t::decode_from_binlog,invalid stream of size(%d) vs bin_log_size(%u)",(int)from_bin_log.size(),bin_log_size);
                return enum_xerror_code_no_data;
            }

            try{
                const char bin_type = *((char*)from_bin_log.data() + bin_log_size - 1);
                if(bin_type >= '1')
                {
                    const int result = xvcanvas_t::decode(from_bin_log,bin_log_size - 1,out_records);
                    if(result >= enum_xcode_successful)
                        from_bin_log.pop_front(1);//pop bin_type now
                    else
                        xerror("xvbstate_t::decode_from_binlog,failed as error(%d)",result);
                    
                    return (enum_xerror_code)result;
                }
                else
                {
                    xerror("xvbstate_t::decode_from_binlog,invalid bin_type(%c)",bin_type);
                    return enum_xerror_code_bad_type;
                }
                
            } catch (int error_code){
                xerror("xvbstate_t::decode_from_binlog,throw exception with error:%d",error_code);
                return enum_xerror_code_errno;
            }
            xerror("xvbstate_t::decode_from_binlog,throw unknow exception");
            return enum_xerror_code_fail;
        }
        
        bool   xvbstate_t::apply_changes_of_binlog(const std::string & from_bin_log) //apply changes to current states
        {
            std::deque<top::base::xvmethod_t> out_records;
            if(decode_change_from_binlog(from_bin_log, out_records) >= enum_xcode_successful)
            {
                for(auto & op : out_records)
                {
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
            if(decode_change_from_binlog(from_bin_log,bin_log_size,out_records) >= enum_xcode_successful)
            {
                for(auto & op : out_records)
                {
                    execute(op,nullptr);
                }
                return true;
            }
            xerror("decompile_from_binlog failed for bin-log,length(%u)",(uint32_t)from_bin_log.size());
            return false;
        }
        

    };
};
