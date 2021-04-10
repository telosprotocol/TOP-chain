// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "../xvproperty.h"

namespace top
{
    namespace base
    {
        //---------------------------------xvproperty_t---------------------------------//
        xvproperty_t::xvproperty_t(enum_xdata_type type)
            :xvexeunit_t(type)
        {
            m_value_ptr = nullptr;
            m_value_ptr = new xvalueobj_t();
        }
    
        xvproperty_t::xvproperty_t(const std::string & name, const xvalue_t & value,enum_xdata_type type)
            :xvexeunit_t(type)
        {
            m_value_ptr = nullptr;
            m_value_ptr = new xvalueobj_t(name,value);
            set_unit_name(name);//set unit name
        }
    
        xvproperty_t::xvproperty_t(const xvproperty_t & const_obj)
            :xvexeunit_t(const_obj)
        {
            m_value_ptr = nullptr;
    
            xvproperty_t & obj = (xvproperty_t &)const_obj;
            if(obj.get_value().get_type() <= xvalue_t::enum_xvalue_type_small_value)
            {
                //do deep copy for fixed & small data
                m_value_ptr = new xvalueobj_t(obj.get_name(),obj.get_value());
            }
            else if( (obj.get_value().get_type() == xvalue_t::enum_xvalue_type_string) && (obj.get_value().get_string().size() < 256) )
            {
                //do deep copy for small data
                m_value_ptr = new xvalueobj_t(obj.get_name(),obj.get_value());
            }
            else
            {
                m_value_ptr = obj.load_value_obj();//copy ptr only
                m_value_ptr->add_ref(); //hold own reference
                m_value_ptr->set_readonly_flag(); //set shared flag
            }
            set_unit_name(const_obj.get_name());//copy unit name finally
        }

        xvproperty_t::~xvproperty_t()
        {
            m_value_ptr->release_ref();
        }
        
        xvalueobj_t*  xvproperty_t::load_value_obj() const
        {
            return m_value_ptr;
        }
    
        bool  xvproperty_t::set_value_obj(xvalueobj_t * new_val_obj)
        {
            if(new_val_obj != nullptr)
            {
                new_val_obj->add_ref(); //hold reference first,then switch it
                xvalueobj_t * old_ptr = xatomic_t::xexchange(m_value_ptr, new_val_obj);
                if(old_ptr != nullptr)
                    old_ptr->release_ref();
                
                return true;
            }
            xassert(new_val_obj != nullptr);
            return false;
        }
    
        const xvalue_t&  xvproperty_t::get_value() const //readonly access
        {
            return load_value_obj()->get_value();
        }
    
        const xvalue_t&  xvproperty_t::get_writable_value()
        {
            xvalueobj_t * exist_value = load_value_obj();
            if(exist_value->is_readonly() == false) //most path hit
            {
                return exist_value->get_value();
            }
            
            xauto_ptr<xvalueobj_t> new_value_obj(new xvalueobj_t(exist_value->get_name(),exist_value->get_value()));
            set_value_obj(new_value_obj.get());
            return new_value_obj->get_value();
        }

        //update value,not safe for multiple_thrad
        bool   xvproperty_t::copy_from_value(const xvalue_t & new_val)
        {
            xvalue_t & writable_value = (xvalue_t&)get_writable_value();
            writable_value = new_val;
            return true;
        }
        
        bool   xvproperty_t::move_from_value(xvalue_t & new_val)
        {
            xvalue_t & writable_value = (xvalue_t&)get_writable_value();
            writable_value = std::move(new_val);
            return true;
        }
    
        void*   xvproperty_t::query_interface(const int32_t _enum_xobject_type_) //caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vproperty)
                return this;
            
            return xvexeunit_t::query_interface(_enum_xobject_type_);
        }
    
        void       xvproperty_t::set_unit_name(const std::string & name)
        {
            if(m_value_ptr->get_name() != name)
                m_value_ptr->set_name(name);//copy to value self
            return xvexeunit_t::set_unit_name(name);
        }
    
        //subclass extend behavior and load more information instead of a raw one
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t    xvproperty_t::do_write(xstream_t & stream)  //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            stream.write_tiny_string(get_unit_name());
            ((xvalue_t& )m_value_ptr->get_value()).serialize_to(stream);
            return (stream.size() - begin_size);
        }
    
        int32_t    xvproperty_t::do_read(xstream_t & stream)  //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            
            std::string unit_name;
            stream.read_tiny_string(unit_name);
            set_unit_name(unit_name);//set property name first
            
            xauto_ptr<xvalueobj_t> new_value(new xvalueobj_t());
            if(((xvalue_t& )new_value->get_value()).serialize_from(stream) > 0)
            {
                new_value->set_name(unit_name); //copy name as well
                set_value_obj(new_value.get());
            }
            else
            {
               xassert(0);
            }
            return (begin_size - stream.size());
        }

        //---------------------------------xtokenvar_t---------------------------------//
        IMPL_REGISTER_OBJECT(xtokenvar_t);
            
        xtokenvar_t::xtokenvar_t()
            :xvproperty_t(std::string(),vtoken_t(0),enum_xdata_type(enum_xobject_type_vprop_token))
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
    
        xtokenvar_t::xtokenvar_t(const std::string & name,const vtoken_t value)
            :xvproperty_t(name,value,enum_xdata_type(enum_xobject_type_vprop_token))
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
    
        xtokenvar_t::xtokenvar_t(const xtokenvar_t & obj)
            :xvproperty_t(obj)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }

        xtokenvar_t::~xtokenvar_t()
        {
        }
    
        xvexeunit_t* xtokenvar_t::clone()
        {
            return new xtokenvar_t(*this);
        }
    
        void*  xtokenvar_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_token)
                return this;
            
            return xvproperty_t::query_interface(_enum_xobject_type_);
        }
    
        const vtoken_t  xtokenvar_t::get_balance()
        {
            return get_value().get_token();
        }
        
        const xvalue_t xtokenvar_t::do_query(const xvmethod_t & op) //read-only
        {
            return xvalue_t(get_balance());
        }
    
        const vtoken_t xtokenvar_t::deposit(const vtoken_t add_token)
        {
            xvalue_t param(add_token);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_deposit_token,param);
            return (vtoken_t)execute(instruction,(xvcanvas_t*)get_canvas()).get_token();
        }
        
        const vtoken_t xtokenvar_t::withdraw(const vtoken_t sub_token)
        {
            xassert(sub_token >= 0);
            if((int64_t)sub_token <= 0)
                return get_balance();
            
            xvalue_t current_balance(xvmethod_t(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_query_token));
            xvalue_t withdraw_value(sub_token);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function,enum_xvinstruct_state_method_withdraw_token,current_balance,withdraw_value);
            
            return (vtoken_t)execute(instruction,(xvcanvas_t*)get_canvas()).get_token();
        }
        
        const xvalue_t xtokenvar_t::do_deposit(const xvmethod_t & op)
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_deposit_token)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 1) || (op.get_method_params().size() != 1) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & added_token = op.get_method_params().at(0);
            if(added_token.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const vtoken_t new_balance = (vtoken_t)(get_value().get_token() + added_token.get_token());
            xvalue_t new_value(new_balance);
            move_from_value(new_value);
            
            return xvalue_t(new_balance);
        }
        
        const xvalue_t xtokenvar_t::do_withdraw(const xvmethod_t & op)
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_withdraw_token)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if( (op.get_params_count() != 2) || (op.get_method_params().size() != 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & balance_query_function = op.get_method_params().at(0);
            if( (balance_query_function.get_type() != xvalue_t::enum_xvalue_type_vmethod) || (balance_query_function.get_vmethod() == nullptr) )
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & withdraw_value = op.get_method_params().at(1);
            if(withdraw_value.get_type() != xvalue_t::enum_xvalue_type_token)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t current_balance(execute(*(balance_query_function.get_vmethod()),nullptr));
            const int64_t ask_withdraw = withdraw_value.get_token();
            if(ask_withdraw < 0)
                return xvalue_t(enum_xerror_code_bad_param);
            
            const vtoken_t new_balance = (vtoken_t)(current_balance.get_token() - ask_withdraw);
            xvalue_t new_value(new_balance);
            move_from_value(new_value);

            return xvalue_t(new_balance);
        }
        
        //---------------------------------xtokenvar_t---------------------------------//
        IMPL_REGISTER_OBJECT(xnoncevar_t);
        
        xnoncevar_t::xnoncevar_t()
            :xvproperty_t(std::string(),vnonce_t(0),enum_xdata_type(enum_xobject_type_vprop_nonce))
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xnoncevar_t::xnoncevar_t(const std::string & name,const vnonce_t value)
            :xvproperty_t(name,value,enum_xdata_type(enum_xobject_type_vprop_nonce))
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xnoncevar_t::xnoncevar_t(const xnoncevar_t & obj)
            :xvproperty_t(obj)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xnoncevar_t::~xnoncevar_t()
        {
        }
        
        xvexeunit_t* xnoncevar_t::clone()
        {
            return new xnoncevar_t(*this);
        }

        void*  xnoncevar_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_nonce)
                return this;
            
            return xvproperty_t::query_interface(_enum_xobject_type_);
        }
     
        const vnonce_t xnoncevar_t::get_nonce()
        {
            return get_value().get_nonce();
        }
        
        const vnonce_t xnoncevar_t::alloc_nonce() //return the new available nonce
        {
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_nonce_alloc);
            return (vnonce_t)execute(instruction,(xvcanvas_t*)get_canvas()).get_nonce();
        }
        
        const xvalue_t  xnoncevar_t::do_alloc(const xvmethod_t & op)
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_nonce_alloc)
                return xvalue_t(enum_xerror_code_bad_method);
            
            const vnonce_t new_nonce = (vnonce_t)(get_value().get_nonce() + 1);
            xvalue_t new_value(new_nonce);
            move_from_value(new_value);
            
            return xvalue_t(new_nonce);
        }

        //---------------------------------xstringvar_t---------------------------------//
        IMPL_REGISTER_OBJECT(xstringvar_t);
        xstringvar_t::xstringvar_t(enum_xdata_type type)
            :xvproperty_t(std::string(),std::string(),type)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xstringvar_t::xstringvar_t(const std::string & property_name, const std::string & property_value,enum_xdata_type type)
            :xvproperty_t(property_name,property_value,type)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xstringvar_t::xstringvar_t(const xstringvar_t & obj)
            :xvproperty_t(obj)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xstringvar_t::~xstringvar_t()
        {
        }
        
        xvexeunit_t* xstringvar_t::clone()
        {
            return new xstringvar_t(*this);
        }
        
        void*  xstringvar_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_string)
                return this;
            
            return xvproperty_t::query_interface(_enum_xobject_type_);
        }
     
        const std::string & xstringvar_t::query() const //return whole string
        {
            return get_value().get_string();
        }
    
        bool  xstringvar_t::reset() //erase to empty string
        {
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_string_reset);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
    
        bool  xstringvar_t::reset(const std::string & value) //erase_all and set all
        {
            xvalue_t new_string_value(value);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_string_reset,new_string_value);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
    
        const xvalue_t  xstringvar_t::do_reset(const xvmethod_t & op)
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_string_reset)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_method_params().size() > 1) //carry too much params
                return xvalue_t(enum_xerror_code_invalid_param_count);
                
            std::string new_string;
            if(op.get_method_params().size() == 1) //carry new string value
            {
                const xvalue_t & new_string_param = op.get_method_params().at(0);
                if(new_string_param.get_type() != xvalue_t::enum_xvalue_type_string)
                    return xvalue_t(enum_xerror_code_invalid_param_type);
                
                new_string = new_string_param.get_string();
            }
            xvalue_t new_value(new_string);
            move_from_value(new_value);
            
            return xvalue_t(enum_xcode_successful);
        }
        
        //---------------------------------xcodevar_t---------------------------------//
        IMPL_REGISTER_OBJECT(xcodevar_t);
    
        xcodevar_t::xcodevar_t()
            :xstringvar_t(std::string(),std::string(),enum_xdata_type(enum_xobject_type_vprop_code))
        {
        }
        
        xcodevar_t::xcodevar_t(const std::string & property_name, const std::string & property_value)
            :xstringvar_t(property_name,property_value,enum_xdata_type(enum_xobject_type_vprop_code))
        {
        }
        
        xcodevar_t::xcodevar_t(const xcodevar_t & obj)
            :xstringvar_t(obj)
        {
        }
        
        xcodevar_t::~xcodevar_t()
        {
        }
        
        xvexeunit_t* xcodevar_t::clone()
        {
            return new xcodevar_t(*this);
        }
        
        void*  xcodevar_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_code)
                return this;
            
            return xstringvar_t::query_interface(_enum_xobject_type_);
        }
    
        bool  xcodevar_t::deploy_code(const std::string & values)//erase_all and set all
        {
            if(get_value().get_string().empty() == false) //once deploy, never allow to modify
                return false;
            
            return xstringvar_t::reset(values);
        }
        
        const xvalue_t xcodevar_t::do_reset(const xvmethod_t & op)
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_string_reset)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(get_value().get_string().empty() == false) //once deploy, never allow to modify
                return xvalue_t(enum_xerror_code_bad_privilege);
            
            return xstringvar_t::do_reset(op);
        }
    
        //---------------------------------xhashmapvar_t---------------------------------//
        IMPL_REGISTER_OBJECT(xhashmapvar_t);
        xhashmapvar_t::xhashmapvar_t(enum_xdata_type type)
            :xvproperty_t(std::string(),std::map<std::string,std::string>(),type)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xhashmapvar_t::xhashmapvar_t(const std::string & property_name, const std::map<std::string,std::map<std::string,std::string> > & property_value,enum_xdata_type type)
            :xvproperty_t(property_name,property_value,type)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xhashmapvar_t::xhashmapvar_t(const xhashmapvar_t & obj)
            :xvproperty_t(obj)
        {
            REGISTER_XVIFUNC_ID_API(enum_xvinstruct_class_state_function);
        }
        
        xhashmapvar_t::~xhashmapvar_t()
        {
        }
        
        xvexeunit_t* xhashmapvar_t::clone()
        {
            return new xhashmapvar_t(*this);
        }
        
        void*  xhashmapvar_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_hashmap)
                return this;
            
            return xvproperty_t::query_interface(_enum_xobject_type_);
        }
    
        bool   xhashmapvar_t::find(const std::string & key)//test whether key is existing or not
        {
            auto map_obj = get_value().get_hashmap();
            xassert(map_obj != nullptr);
            if(map_obj != nullptr)
            {
                auto it = map_obj->find(key);
                if(it != map_obj->end())
                    return true;
            }
            return false;
        }
    
        const std::map<std::string,std::string> xhashmapvar_t::query(const std::string & key)
        {
            auto map_obj = get_value().get_hashmap();
            xassert(map_obj != nullptr);
            if(map_obj != nullptr)
            {
                auto it = map_obj->find(key);
                if(it != map_obj->end())
                    return it->second;
            }
            return std::map<std::string,std::string>();
        }
    
        const std::string   xhashmapvar_t::query(const std::string & key,const std::string & field) //key must be exist,otherwise return empty string
        {
            auto map_obj = get_value().get_hashmap();
            xassert(map_obj != nullptr);
            if(map_obj != nullptr)
            {
                auto it = map_obj->find(key);
                if(it != map_obj->end())
                {
                    auto & submap = it->second;
                    auto subiter = submap.find(field);
                    if(subiter != submap.end())
                        return subiter->second;
                }
                    
            }
            return std::string();
        }

        bool  xhashmapvar_t::erase(const std::string & key)//erase evey filed assocated with this key
        {
            xvalue_t target_key(key);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_hashmap_erase,target_key);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
    
        bool  xhashmapvar_t::erase(const std::string & key,const std::string & field)//only successful when key already existing
        {
            xvalue_t target_key(key);
            xvalue_t target_field(field);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_hashmap_erase,target_key,target_field);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
    
        bool  xhashmapvar_t::insert(const std::string & key,const std::string & field,const std::string & value)//create key if not found key
        {
            xvalue_t new_key(key);
            xvalue_t new_field(field);
            xvalue_t new_value(value);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_hashmap_insert,new_key,new_field,new_value);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
        
        bool  xhashmapvar_t::reset() //erase all key and values
        {
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_hashmap_reset);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
    
        bool  xhashmapvar_t::reset(const std::map<std::string,std::map<std::string,std::string> > & key_values)//erase_all and set all
        {
            xvalue_t newhashmap(key_values);
            xvmethod_t instruction(get_execute_uri(),enum_xvinstruct_class_state_function, enum_xvinstruct_state_method_hashmap_reset);
            return (execute(instruction,(xvcanvas_t*)get_canvas()).get_error() == enum_xcode_successful);
        }
    
        const xvalue_t  xhashmapvar_t::do_insert(const xvmethod_t & op)  //create key if not found key
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_hashmap_insert)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_method_params().size() != 3) //must carry key.field.value
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            const xvalue_t & insert_key_param = op.get_method_params().at(0);
            if(insert_key_param.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & insert_field_param = op.get_method_params().at(1);
            if(insert_field_param.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const xvalue_t & insert_value_param = op.get_method_params().at(2);
            if(insert_value_param.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            const std::string target_key_string = insert_key_param.get_string();
            std::map<std::string,std::map<std::string,std::string> >* map_obj = (std::map<std::string,std::map<std::string,std::string> >*)get_writable_value().get_hashmap();
            xassert(map_obj != nullptr);
            if(map_obj != nullptr)
            {
                (*map_obj)[target_key_string][insert_field_param.get_string()] = insert_value_param.get_string();
                return xvalue_t(enum_xcode_successful);
            }
            return xvalue_t(enum_xerror_code_bad_vproperty);
        }
    
        const xvalue_t  xhashmapvar_t::do_erase(const xvmethod_t & op)  //only successful when key already existing
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_hashmap_erase)
                return xvalue_t(enum_xerror_code_bad_method);
            
            const int params_count = (int)op.get_method_params().size();
            if( (params_count <= 0) || (params_count > 2) )
                return xvalue_t(enum_xerror_code_invalid_param_count);
                
            const xvalue_t & erase_key_param = op.get_method_params().at(0);
            if(erase_key_param.get_type() != xvalue_t::enum_xvalue_type_string)
                return xvalue_t(enum_xerror_code_invalid_param_type);
            
            std::string target_field;
            if(params_count == 2)//erase key.field
            {
                const xvalue_t & erase_field_param = op.get_method_params().at(1);
                if(erase_field_param.get_type() != xvalue_t::enum_xvalue_type_string)
                    return xvalue_t(enum_xerror_code_invalid_param_type);
                
                target_field = erase_field_param.get_string();
            }
            
            std::map<std::string,std::map<std::string,std::string> >* map_obj = (std::map<std::string,std::map<std::string,std::string> >*)get_writable_value().get_hashmap();
            xassert(map_obj != nullptr);
            if(map_obj != nullptr)
            {
                auto it_map = map_obj->find(erase_key_param.get_string());
                if(it_map != map_obj->end())
                {
                    if(target_field.empty() == false)
                    {
                        auto & submap = it_map->second;
                        auto   subiter_map = submap.find(target_field);
                        if(subiter_map != submap.end())
                        {
                            submap.erase(subiter_map);
                            if(submap.empty())
                                map_obj->erase(it_map);
                        }
                    }
                    else //erase whole key
                    {
                        map_obj->erase(it_map);
                    }
                }
                return xvalue_t(enum_xcode_successful);
            }
            return xvalue_t(enum_xerror_code_bad_vproperty);
        }
    
        const xvalue_t  xhashmapvar_t::do_reset(const xvmethod_t & op)  //erase_all ,then set all
        {
            if(op.get_method_id() != enum_xvinstruct_state_method_hashmap_reset)
                return xvalue_t(enum_xerror_code_bad_method);
            
            if(op.get_method_params().size() > 1) //carry too many pararms
                return xvalue_t(enum_xerror_code_invalid_param_count);
            
            if(op.get_method_params().size() == 1)
            {
                const xvalue_t & new_map_param = op.get_method_params().at(0);
                if(new_map_param.get_type() != xvalue_t::enum_xvalue_type_hashmap)
                    return xvalue_t(enum_xerror_code_invalid_param_type);
                
                if(new_map_param.get_hashmap() == nullptr)
                    return xvalue_t(enum_xerror_code_bad_param);
                
                copy_from_value(new_map_param);
            }
            else
            {
                std::map<std::string,std::map<std::string,std::string> > empty;
                copy_from_value(empty);
            }
            return xvalue_t(enum_xcode_successful);
        }
        
    
        //---------------------------------xvintvar_t<T>---------------------------------//
        template<typename T>
        const int   xvintvar_t<T>::query_obj_type()        {return enum_xdata_type_undefine;}
        template<>
        const int   xvintvar_t<int8_t>::query_obj_type()   {return enum_xobject_type_vprop_int8;}
        template<>
        const int   xvintvar_t<int16_t>::query_obj_type()  {return enum_xobject_type_vprop_int16;}
        template<>
        const int   xvintvar_t<int32_t>::query_obj_type()  {return enum_xobject_type_vprop_int32;}
        template<>
        const int   xvintvar_t<int64_t>::query_obj_type()  {return enum_xobject_type_vprop_int64;}
        template<>
        const int   xvintvar_t<uint64_t>::query_obj_type() {return enum_xobject_type_vprop_uint64;}
    
        //---------------------------------xdequevar_t---------------------------------//
        template<typename T>
        const int   xdequevar_t<T>::query_obj_type()           {return enum_xdata_type_undefine;}
        template<>
        const int   xdequevar_t<int8_t>::query_obj_type()      {return enum_xobject_type_vprop_int8_deque;}
        template<>
        const int   xdequevar_t<int16_t>::query_obj_type()     {return enum_xobject_type_vprop_int16_deque;}
        template<>
        const int   xdequevar_t<int32_t>::query_obj_type()     {return enum_xobject_type_vprop_int32_deque;}
        template<>
        const int   xdequevar_t<int64_t>::query_obj_type()     {return enum_xobject_type_vprop_int64_deque;}
        template<>
        const int   xdequevar_t<uint64_t>::query_obj_type()    {return enum_xobject_type_vprop_uint64_deque;}
        template<>
        const int   xdequevar_t<std::string>::query_obj_type() {return enum_xobject_type_vprop_string_deque;}
    
        //---------------------------------xmapvar_t---------------------------------//
        template<typename T>
        const int   xmapvar_t<T>::query_obj_type()           {return enum_xdata_type_undefine;}
        template<>
        const int   xmapvar_t<int8_t>::query_obj_type()      {return enum_xobject_type_vprop_int8_map;}
        template<>
        const int   xmapvar_t<int16_t>::query_obj_type()     {return enum_xobject_type_vprop_int16_map;}
        template<>
        const int   xmapvar_t<int32_t>::query_obj_type()     {return enum_xobject_type_vprop_int32_map;}
        template<>
        const int   xmapvar_t<int64_t>::query_obj_type()     {return enum_xobject_type_vprop_int64_map;}
        template<>
        const int   xmapvar_t<uint64_t>::query_obj_type()    {return enum_xobject_type_vprop_uint64_map;}
        template<>
        const int   xmapvar_t<std::string>::query_obj_type() {return enum_xobject_type_vprop_string_map;}
    
        //---------------------------------xmtokens_t---------------------------------//
        IMPL_REGISTER_OBJECT(xmtokens_t);
    
        xmtokens_t::xmtokens_t()
            :base(std::string(),std::map<std::string,int64_t>(),(enum_xdata_type)enum_xobject_type_vprop_mtokens)
        {
        }
       
        xmtokens_t::xmtokens_t(const std::string & property_name, const std::map<std::string,int64_t> & property_value)
            :base(property_name,property_value,(enum_xdata_type)enum_xobject_type_vprop_mtokens)
        {
        }
        
        xmtokens_t::xmtokens_t(const xmtokens_t & obj)
            :base(obj)
        {
        }
       
        xmtokens_t::~xmtokens_t()
        {
        };
        
        //caller need to cast (void*) to related ptr
        void*    xmtokens_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_mtokens)
                return this;
            
            return base::query_interface(_enum_xobject_type_);
        }
    
        xvexeunit_t*  xmtokens_t::clone() //clone a new object with same state
        {
            return new xmtokens_t(*this);
        }

        //read interface
        const int64_t  xmtokens_t::get_balance(const std::string & token_name)
        {
            if(find(token_name) == false) //safety enhancement
                return 0;
            
            return base::query(token_name);
        }
        
        //write interface
        bool  xmtokens_t::deposit(const std::string & token_name ,const int64_t add_token)//return the updated balance
        {
            xassert(add_token > 0);
            if(add_token <= 0)
                return false;
            
            const int64_t cur_balance = get_balance(token_name);
            return base::insert(token_name, (cur_balance + add_token));
        }
    
        bool  xmtokens_t::withdraw(const std::string & token_name,const int64_t sub_token)//return the updated balance
        {
            xassert(sub_token > 0);
            if(sub_token <= 0)
                return false;
            
            const int64_t cur_balance = get_balance(token_name);
            return base::insert(token_name, (cur_balance - sub_token));
        }
    
        //---------------------------------xmtokens_t---------------------------------//
        IMPL_REGISTER_OBJECT(xmkeys_t);
        
        xmkeys_t::xmkeys_t()
            :base(std::string(),std::map<std::string,std::string>(),(enum_xdata_type)enum_xobject_type_vprop_mkeys)
        {
        }
        
        xmkeys_t::xmkeys_t(const std::string & property_name, const std::map<std::string,std::string> & property_value)
            :base(property_name,property_value,(enum_xdata_type)enum_xobject_type_vprop_mkeys)
        {
        }
        
        xmkeys_t::xmkeys_t(const xmkeys_t & obj)
            :base(obj)
        {
        }
        
        xmkeys_t::~xmkeys_t()
        {
        };
        
        //caller need to cast (void*) to related ptr
        void*    xmkeys_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vprop_mkeys)
                return this;
            
            return base::query_interface(_enum_xobject_type_);
        }
        
        xvexeunit_t*  xmkeys_t::clone() //clone a new object with same state
        {
            return new xmkeys_t(*this);
        }
        
        //read interface
        const std::string  xmkeys_t::query_key(const std::string & key_name)
        {
            return base::query(key_name);
        }
        
        //write interface
        bool  xmkeys_t::deploy_key(const std::string & key_name ,const std::string & key_string)
        {
            xassert(key_string.empty() == false);
            if(key_string.empty())
                return false;
            
            if(find(key_name) == false) //safety enhancement
                return false;
            
            return base::insert(key_name,key_string);
        }

    };
};
