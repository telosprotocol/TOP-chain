// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "xstatistic/xbasic_size.hpp"
#include "../xvexeunit.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        //*************************************xvexeunit_t****************************************//
        xvexeunit_t::xvexeunit_t(enum_xdata_type type)
            :xdataunit_t(type)
        {
            m_parent_unit = nullptr;
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exeunit, 1);
        }
    
        xvexeunit_t::xvexeunit_t(xvexeunit_t * parent_unit,const std::string & unit_name,enum_xdata_type type)
            :xdataunit_t(type)
        {
            m_parent_unit = nullptr;
            m_unit_name = unit_name;
            m_execute_uri = m_unit_name; //init
            
            if(parent_unit != nullptr)
            {
                parent_unit->add_ref();
                m_parent_unit = parent_unit;
            }
            if(parent_unit != nullptr)
                m_execute_uri = parent_unit->get_execute_uri() + "/" + get_unit_name();
            else
                m_execute_uri = get_unit_name();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exeunit, 1);
        }
    
        xvexeunit_t::xvexeunit_t(const xvexeunit_t & obj)
            :xdataunit_t(obj)
        {
            m_parent_unit = nullptr;
            m_unit_name = obj.m_unit_name;
            m_execute_uri = m_unit_name; //init
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exeunit, 1);
        }

        xvexeunit_t::~xvexeunit_t()
        {
            set_parent_unit(nullptr);
            m_name_methods.clear();
            m_id_methods.clear();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exeunit, -1);
        }
        
        bool  xvexeunit_t::close(bool force_async)
        {
            //leave others at de-construction function
            set_parent_unit(nullptr);
            return xdataunit_t::close(force_async);
        }
        
        void*  xvexeunit_t::query_interface(const int32_t _enum_xobject_type_)//caller need to cast (void*) to related ptr
        {
            if(_enum_xobject_type_ == enum_xobject_type_exe_unit)
                return this;
            
            return xdataunit_t::query_interface(_enum_xobject_type_);
        }
        
        void  xvexeunit_t::set_unit_name(const std::string & name)
        {
            m_unit_name = name;
            if(m_execute_uri.empty())
                m_execute_uri = name;
        }
        
        void  xvexeunit_t::set_parent_unit(xvexeunit_t * parent_ptr)
        {
            if(xatomic_t::xload(m_parent_unit) != parent_ptr)// if anything changed
            {
#if 1  // TODO(jimmy) parent_ptr not add ref, child not hold parent ptr
                xatomic_t::xexchange(m_parent_unit, parent_ptr);
#else
                if(parent_ptr != nullptr)
                    parent_ptr->add_ref();
                
                xvexeunit_t * old_ptr = xatomic_t::xexchange(m_parent_unit, parent_ptr);
                if(old_ptr != nullptr)
                    old_ptr->release_ref();
#endif
            }

            //rebuild execution uri at everytime
            std::string new_uri;
            if(parent_ptr != nullptr)
                new_uri = parent_ptr->get_execute_uri() + "/" + get_unit_name();
            else
                new_uri = get_unit_name();
            
            if(new_uri != m_execute_uri)
                m_execute_uri = new_uri;
        }
 
        bool  xvexeunit_t::register_method(const uint8_t method_type,const uint8_t method_id,const xvstdfunc_t & api_function)
        {
            if(method_id > INT8_MAX)
            {
                xassert(0);
                return false;
            }
            const int method_key = (((int)method_type) << 8) | method_id;
            m_id_methods[method_key] = api_function;
            return true;
        }
    
        bool  xvexeunit_t::register_method(const uint8_t method_type,const std::string & method_name,const xvstdfunc_t & api_function)
        {
            if(method_name.empty())
            {
                xassert(0);
                return false;
            }
            const std::string method_key = xstring_utl::tostring(method_type) + "." + method_name;
            m_name_methods[method_key] = api_function;
            return true;
        }
 
        //call instruction(operator) with related arguments
        const xvalue_t xvexeunit_t::execute(const xvmethod_t & op,xvcanvas_t * canvas)//might throw exception for error
        {
            xvexeunit_t * parent_unit_ptr = get_parent_unit();
            if(op.get_method_uri() != get_execute_uri())
            {
                if(parent_unit_ptr != nullptr)
                    return parent_unit_ptr->execute(op, canvas);
                else
                    return not_impl(op);
            }

            if(op.is_name_method())
            {
                const std::string method_key = xstring_utl::tostring(op.get_method_type()) + "." + op.get_method_name();
                auto it = m_name_methods.find(method_key);
                if(it != m_name_methods.end())
                {
                    const xvalue_t res(it->second(op,canvas));
                    if( (res.get_type() != xvalue_t::enum_xvalue_type_error) || (res.get_error() == enum_xcode_successful) )
                    {
                        if(canvas != nullptr)
                            canvas->record(op); //record after successful
                    }
                    else
                    {
                        xerror("xvexeunit_t::execute,catch error(%d) for method_name(%s) of uri(%s)",res.get_error(),method_key.c_str(),op.get_method_uri().c_str());
                    }
                    return res;
                }
            }
            else
            {
                const int method_key = (op.get_method_type() << 8) | op.get_method_id();
                auto it = m_id_methods.find(method_key);
                if(it != m_id_methods.end())
                {
                    const xvalue_t res(it->second(op,canvas));
                    if( (res.get_type() != xvalue_t::enum_xvalue_type_error) || (res.get_error() == enum_xcode_successful) )
                    {
                        if(canvas != nullptr)
                            canvas->record(op);//record after successful
                    }
                    else
                    {
                        xerror("xvexeunit_t::execute,catch error(%d) for method_key(%d) of uri(%s)",res.get_error(),method_key,op.get_method_uri().c_str());
                    }
                    return res;
                }
            }
            xassert(false);
            return not_impl(op);
        }
    
        const xvalue_t   xvexeunit_t::not_impl(const xvmethod_t & op) const
        {
            if(op.is_name_method())
            {
                const std::string method_key = xstring_utl::tostring(op.get_method_type()) + "." + op.get_method_name();
                xerror("xvexeunit_t::executenot_impl for name-method(%s) of uri(%s)",method_key.c_str(),op.get_method_uri().c_str());
            }
            else
            {
                const int method_key = (op.get_method_type() << 8) | op.get_method_id();
                xerror("xvexeunit_t::executenot_impl for id-method(%d) of uri(%s)",method_key,op.get_method_uri().c_str());
            }
            return xvalue_t();
        }

        int32_t xvexeunit_t::get_ex_alloc_size() const {
            // each node alloc 72 for std::map<int, xvstdfunc_t> and 
            int32_t ex_size = get_size(m_unit_name) + get_size(m_execute_uri) + ((m_id_methods.size() + m_name_methods.size()) * 72);
            for (auto & pair : m_name_methods) {
                auto key_size = get_size(pair.first);
                ex_size += key_size;
                xdbg("-----cache size----- xvexeunit_t m_name_methods key:%d", key_size);
            }
            return ex_size;
        }
    
        xvexegroup_t::xvexegroup_t(enum_xdata_type type)
            :xvexeunit_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exegroup, 1);
        }
    
        xvexegroup_t::xvexegroup_t(xvexeunit_t * parent_unit,const std::string unit_name,enum_xdata_type type)
            :xvexeunit_t(parent_unit,unit_name,type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exegroup, 1);
        }
    
        xvexegroup_t::xvexegroup_t(const xvexegroup_t & obj)
            :xvexeunit_t(obj)
        {
            clone_units_from(obj);
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exegroup, 1);
        }

        xvexegroup_t::~xvexegroup_t()
        {
            m_lock.lock();
            for(auto & u : m_child_units)
            {
                u.second->set_parent_unit(nullptr);//reset to parent ptr anyway
                u.second->release_ref();
            }
            m_child_units.clear();
            m_lock.unlock();
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_exegroup, -1);
        }
        
        bool  xvexegroup_t::clone_units_from(const xvexegroup_t & source)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            for(auto & u : source.m_child_units)
            {
                xvexeunit_t * clone_unit = u.second->clone();
                xassert(clone_unit != nullptr);
                if(clone_unit != nullptr)
                {
                    add_child_unit(clone_unit);
                    clone_unit->release_ref();
                }
            }
            return true;
        }
    
        bool  xvexegroup_t::close(bool force_async)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
#if 0 // TODO(jimmy) no need call close to release object
            if(is_close() == false)
            {
                for(auto & u : m_child_units)
                {
                    u.second->set_parent_unit(nullptr);//reset to parent ptr
                }
            }
#endif
            return xvexeunit_t::close(force_async);
        }
    
        void   xvexegroup_t::set_parent_unit(xvexeunit_t * parent_ptr)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            xvexeunit_t::set_parent_unit(parent_ptr);//set self first
            
            for(auto & u : m_child_units)
                u.second->set_parent_unit(this); //reset and rebuild execution uri again
        }
    
        bool   xvexegroup_t::add_child_unit(xvexeunit_t * child)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            xassert(child != nullptr);
            if(child == nullptr)
                return false;
            
            auto it = m_child_units.find(child->get_unit_name());
            if(it != m_child_units.end())
            {
                if(it->second == child) //already existing
                    return true;

                it->second->release_ref();//clean existing ptr
                
                child->add_ref();   //hold reference first
                it->second = child; //copy ptr first
                
                child->set_parent_unit(this); //setup execution uri and parent ptr
            }
            else
            {
                child->add_ref(); //hold reference
                child->set_parent_unit(this); //setup execution uri and parent ptr first
                m_child_units[child->get_unit_name()] = child;//copy ptr,the above code did add_ref already
            }
            return true;
        }
    
        bool   xvexegroup_t::remove_child_unit(const std::string & unit_name)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            auto it = m_child_units.find(unit_name);
            if(it != m_child_units.end())
            {
                it->second->set_parent_unit(NULL);
                it->second->release_ref();
                m_child_units.erase(it);
                return true;
            }
            return false;
        }

        bool  xvexegroup_t::remove_all_child_unit()
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for(auto it = m_child_units.begin(); it != m_child_units.end(); )
            {
                it->second->set_parent_unit(NULL);
                it->second->release_ref();
                it = m_child_units.erase(it);
            }
            return true;
        }

        xvexeunit_t *   xvexegroup_t::find_child_unit(const std::string & unit_name) const
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            auto it = m_child_units.find(unit_name);
            if(it != m_child_units.end())
            {
                return it->second;
            }
            return  nullptr;
        }
    
        //call instruction(operator) with related arguments
        const xvalue_t  xvexegroup_t::execute(const xvmethod_t & op,xvcanvas_t * canvas)  //might throw exception for error
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            const std::string & target_execution_uri = op.get_method_uri();
            const std::string & this_execution_uri = get_execute_uri();
            if(target_execution_uri == this_execution_uri)
                return xvexeunit_t::execute(op,canvas);
            
            if(target_execution_uri.size() > this_execution_uri.size())
            {
                auto pos = target_execution_uri.find(this_execution_uri);
                if(pos == 0) //at current branch
                {
                    const std::string left_path = target_execution_uri.substr(this_execution_uri.size() + 1);//skip '/'
                    if(left_path.empty() == false)//remaining path
                    {
                        auto next_dot_pos = left_path.find_first_of('/');
                        if(next_dot_pos != std::string::npos)//remaining path is xxx/yyy
                        {
                            const std::string unit_name = left_path.substr(0,next_dot_pos-0);
                            auto it = m_child_units.find(unit_name);
                            if(it != m_child_units.end()) //found target unit
                                return it->second->execute(op,canvas);
                        }
                        else //that is last portion(as unit name)
                        {
                            auto it = m_child_units.find(left_path);
                            if(it != m_child_units.end()) //found target unit
                                return it->second->execute(op,canvas);
                        }
                    }
                }
            }

            xvexeunit_t * parent_unit_ptr = get_parent_unit();
            if(parent_unit_ptr != nullptr)
                return parent_unit_ptr->execute(op, canvas);
            
            return not_impl(op);
        }

        //subclass extend behavior and load more information instead of a raw one
        //return how many bytes readout /writed in, return < 0(enum_xerror_code_type) when have error
        int32_t    xvexegroup_t::do_write(xstream_t & stream)  //allow subclass extend behavior
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            const int32_t begin_size = stream.size();
 
            const uint16_t count = (uint16_t)m_child_units.size();
            stream << count;
            for (auto it = m_child_units.begin(); it != m_child_units.end(); ++it)
            {
                it->second->serialize_to(stream);
            }
    
            return (stream.size() - begin_size);
        }
        
        int32_t   xvexegroup_t::do_read(xstream_t & stream) //allow subclass extend behavior
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            const int32_t begin_size = stream.size();
            
            uint16_t count = 0;
            stream >> count;
            for (uint32_t i = 0; i < count; i++)
            {
                xdataunit_t* data_unit_ptr = base::xdataunit_t::read_from(stream);
                xassert(data_unit_ptr != nullptr);
                if(data_unit_ptr != nullptr)
                {
                    xvexeunit_t * exe_unit_obj = (xvexeunit_t*)data_unit_ptr->query_interface(enum_xobject_type_exe_unit);
                    xassert(exe_unit_obj != nullptr);
                    if(exe_unit_obj != nullptr)
                    {
                        add_child_unit(exe_unit_obj);  //then add
                        exe_unit_obj->release_ref();  //now ready to remove reference
                    }
                    else //clean bad ptr
                    {
                        data_unit_ptr->close();
                        data_unit_ptr->release_ref();
                    }
                }
            }
            return (begin_size - stream.size());
        }

        int32_t xvexegroup_t::get_ex_alloc_size() const {
            int32_t ex_size = 0;
            for (auto & pair : m_child_units) {
                auto key_size = get_size(pair.first);
                auto value_size = sizeof(pair.second);
                if (pair.second != nullptr) {
#if defined(DEBUG)
                    auto vexeunit_size = sizeof(*pair.second);
                    auto vexeunit_ex_size = pair.second->get_ex_alloc_size();
                    xdbg("-----cache size----- xvexemodule_t vexeunit_size:%d,vexeunit_ex_size:%d", vexeunit_size, vexeunit_ex_size);
#endif
                }
                // each map node alloc 48B
                ex_size += (key_size + value_size + 48);
                xdbg("-----cache size----- xvexemodule_t key:%d,value:%d,node:48", key_size, value_size);
            }

            return ex_size;
        }
    };
};
