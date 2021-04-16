// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xcontext.h"
#include "xbase/xutl.h"
#include "../xvexeunit.h"

namespace top
{
    namespace base
    {
        const int  xvcanvas_t::compile(std::deque<xvmethod_t> & input_records,const int compile_options,xstream_t & output_stream)
        {
            output_stream << (int32_t)input_records.size();
            if(enum_compile_optimization_none == (compile_options & enum_compile_optimization_mask) ) //dont ask any optimization
            {
                for(auto & record : input_records)
                {
                    const std::string & full_path = record.get_method_uri();
                    xassert(full_path.empty() == false);
                    record.serialize_to(output_stream);
                }
            }
            else //ask some optimization
            {
                //split every method to [base_path].[unitname]
                std::string last_base_path; //init as empty
                std::string last_unit_name; //init as empty
                for(auto & record : input_records)
                {
                    const std::string & full_path = record.get_method_uri();
                    const std::string::size_type lastdot = full_path.find_last_of('.');
                    if(lastdot != std::string::npos) //found
                    {
                        const std::string cur_base_path = full_path.substr(0,lastdot);//[)
                        const std::string cur_unit_name = full_path.substr(lastdot + 1); //skip dot and keep left part
                        xassert(cur_base_path.empty() == false);
                        xassert(cur_unit_name.empty() == false);
                        if( last_base_path.empty() || (last_base_path != cur_base_path) ) //new base path
                        {
                            record.serialize_to(output_stream);
                            last_base_path = cur_base_path; //assign to current base
                            last_unit_name = cur_unit_name; //assign to current unit
                        }
                        else if(last_unit_name.empty() || (last_unit_name != cur_unit_name) )
                        {
                            const std::string compressed_uri = "." + cur_unit_name;//remove duplicated base path to save space
                            xvmethod_t clone_record(record,compressed_uri);
                            clone_record.serialize_to(output_stream);
                            
                            last_base_path = cur_base_path; //assign to current base
                            last_unit_name = cur_unit_name; //assign to current unit
                        }
                        else //same base_path and unit name as last instruction
                        {
                            std::string empty_uri; //so use empty uri to save space
                            xvmethod_t clone_record(record,empty_uri);
                            clone_record.serialize_to(output_stream);
                        }
                    }
                    else //found root record
                    {
                        last_base_path.clear();//clear first
                        last_unit_name.clear();//clear first
                        last_base_path = full_path;
                        record.serialize_to(output_stream);
                    }
                }
            }
            return enum_xcode_successful;
        }
    
        const int  xvcanvas_t::decompile(xstream_t & input_stream,std::deque<xvmethod_t> & output_records)
        {
            //split every method to [base_path].[unitname]
            std::string last_base_path; //init as empty
            std::string last_unit_name; //init as empty
            output_records.clear(); //clear first
            
            int32_t total_records = 0;
            input_stream >> total_records;
            for(int32_t i = 0; i < total_records; ++i)
            {
                xvmethod_t record;
                if(record.serialize_from(input_stream) > 0)
                {
                    const std::string & org_full_path = record.get_method_uri();
                    if(org_full_path.empty()) //use last base path and unit name
                    {
                        const std::string cur_base_path = last_base_path;
                        const std::string cur_unit_name = last_unit_name;
                        if(cur_base_path.empty() || cur_unit_name.empty())
                        {
                            xassert(0);
                            output_records.clear();
                            return enum_xerror_code_bad_data;
                        }
                        const std::string restored_full_uri = cur_base_path + "." + cur_unit_name;
                        xvmethod_t clone_record(record,restored_full_uri);
                        output_records.emplace_back(clone_record);
                    }
                    else
                    {
                        const std::string::size_type lastdot = org_full_path.find_last_of('.');
                        if(lastdot != std::string::npos) //found
                        {
                            std::string cur_base_path = org_full_path.substr(0,lastdot);//[)
                            std::string cur_unit_name = org_full_path.substr(lastdot + 1); //skip dot and left part
                            
                            if(cur_base_path.empty())
                                cur_base_path = last_base_path;
                            if(cur_unit_name.empty())
                                cur_unit_name = last_unit_name;
                            
                            if(cur_base_path.empty() || cur_unit_name.empty())
                            {
                                xassert(0);
                                output_records.clear();
                                return enum_xerror_code_bad_data;
                            }
                            const std::string restored_full_uri = cur_base_path + "." + cur_unit_name;
                            if(restored_full_uri != org_full_path) //it is a compressed record
                            {
                                xvmethod_t clone_record(record,restored_full_uri);
                                output_records.emplace_back(clone_record);
                            }
                            else //it is a full record
                            {
                                output_records.emplace_back(record);
                            }
                            last_base_path = cur_base_path;
                            last_unit_name = cur_unit_name;
                        }
                        else //found root record
                        {
                            output_records.emplace_back(record);
                            last_base_path = org_full_path;
                            last_unit_name.clear();
                        }
                    }
                }
                else
                {
                    xassert(0);
                    output_records.clear();
                    return enum_xerror_code_bad_data;
                }
            }
            return enum_xcode_successful;
        }
    
        //raw_instruction code -> optimization -> original_length -> compressed
        const int  xvcanvas_t::encode(std::deque<xvmethod_t> & input_records,const int encode_options,xstream_t & output_bin)
        {
            base::xautostream_t<1024> _raw_stream(xcontext_t::instance());
            compile(input_records,encode_options,_raw_stream);
            return xstream_t::compress_to_stream(_raw_stream, _raw_stream.size(),output_bin);
        }
        
        const int  xvcanvas_t::encode(std::deque<xvmethod_t> & input_records,const int encode_options,std::string & output_bin)
        {
            base::xautostream_t<1024> _raw_stream(xcontext_t::instance());
            compile(input_records,encode_options,_raw_stream);
            return xstream_t::compress_to_string(_raw_stream,_raw_stream.size(),output_bin);
        }
    
        const int  xvcanvas_t::decode(xstream_t & input_bin,const uint32_t bin_size,std::deque<xvmethod_t> & output_records)
        {
            xautostream_t<1024> uncompressed_stream(xcontext_t::instance()); //1K is big enough for most packet
            const int decompress_result = xstream_t::decompress_from_stream(input_bin,bin_size,uncompressed_stream);
            if(decompress_result > 0)
                return decompile(uncompressed_stream,output_records);

            xerror("xvcanvas_t::decode,decompress_from_stream failed as err(%d)",decompress_result);
            return decompress_result;
        }
    
        const int  xvcanvas_t::decode(const std::string & input_bin,std::deque<xvmethod_t> & output_records)
        {
            xautostream_t<1024> uncompressed_stream(xcontext_t::instance()); //1K is big enough for most packet
            const int decompress_result = xstream_t::decompress_from_string(input_bin,uncompressed_stream);
            if(decompress_result > 0)
                return decompile(uncompressed_stream,output_records);
            
            xerror("xvcanvas_t::decode,decompress_from_string failed as err(%d)",decompress_result);
            return decompress_result;
        }
    
        xvcanvas_t::xvcanvas_t()
        {
        }
        
        xvcanvas_t::xvcanvas_t(const std::string & bin_log)
        {
            try{
                if(bin_log.empty() == false)
                {
                    xstream_t _stream(xcontext_t::instance(),(uint8_t*)bin_log.data(),(uint32_t)bin_log.size());
                    xvcanvas_t::decode(_stream,_stream.size(),m_records);
                }
            }catch(...){
                xassert(0);
                m_records.clear();
            }
        }
    
        xvcanvas_t::~xvcanvas_t()
        {
            m_records.clear();
        }
        
        bool   xvcanvas_t::record(const xvexeunit_t * exeobject,const xvmethod_t & op) //record instruction
        {
            m_records.emplace_back(op);
            return true;
        }
        
        const int  xvcanvas_t::encode(const int encode_options,xstream_t & output_bin) //compile all recorded op with optimization option
        {
            return xvcanvas_t::encode(m_records,encode_options,output_bin);
        }
    
        const int  xvcanvas_t::encode(const int encode_options,std::string & output_bin)//compile all recorded op with optimization option
        {
            return xvcanvas_t::encode(m_records,encode_options,output_bin);
        }
        
        //*************************************xvexeunit_t****************************************//
        xvexeunit_t::xvexeunit_t(enum_xdata_type type)
            :xdataunit_t(type)
        {
            m_parent_unit = nullptr;
            m_canvas = nullptr;
        }
    
        xvexeunit_t::xvexeunit_t(xvexeunit_t * parent_unit,const std::string & unit_name,enum_xdata_type type)
            :xdataunit_t(type)
        {
            m_parent_unit = nullptr;
            m_canvas = nullptr;
            m_unit_name = unit_name;
            m_execute_uri = m_unit_name; //init
            
            if(parent_unit != nullptr)
            {
                parent_unit->add_ref();
                m_parent_unit = parent_unit;
            }
            if(parent_unit != nullptr)
                m_execute_uri = parent_unit->get_execute_uri() + "." + get_unit_name();
            else
                m_execute_uri = get_unit_name();
        }
    
        xvexeunit_t::xvexeunit_t(const xvexeunit_t & obj)
            :xdataunit_t(obj)
        {
            m_parent_unit = nullptr;
            m_canvas = nullptr;
            m_unit_name = obj.m_unit_name;
            m_execute_uri = m_unit_name; //init
        }

        xvexeunit_t::~xvexeunit_t()
        {
            set_parent_unit(nullptr);
            set_canvas(nullptr);
            m_name_methods.clear();
            m_id_methods.clear();
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
                if(parent_ptr != nullptr)
                    parent_ptr->add_ref();
                
                xvexeunit_t * old_ptr = xatomic_t::xexchange(m_parent_unit, parent_ptr);
                if(old_ptr != nullptr)
                    old_ptr->release_ref();
            }

            //rebuild execution uri at everytime
            std::string new_uri;
            if(parent_ptr != nullptr)
                new_uri = parent_ptr->get_execute_uri() + "." + get_unit_name();
            else
                new_uri = get_unit_name();
            
            if(new_uri != m_execute_uri)
                m_execute_uri = new_uri;
        }
    
        void   xvexeunit_t::set_canvas(const xvcanvas_t * const_target)
        {
            xvcanvas_t * target = (xvcanvas_t*)const_target;
            if(target != nullptr)
                target->add_ref();
            
            xvcanvas_t * old_ptr = xatomic_t::xexchange(m_canvas, target);
            if(old_ptr != nullptr)
                old_ptr->release_ref();
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
                    if(canvas != nullptr)
                        canvas->record(this,op);
                    
                    #ifdef DEBUG
                    const xvalue_t res(it->second(op));
                    if( (res.get_type() == xvalue_t::enum_xvalue_type_error) && (res.get_error() != enum_xcode_successful) )
                    {
                        xerror("xvexeunit_t::execute,catch error(%d) for method_name(%s) of uri(%s)",res.get_error(),method_key.c_str(),op.get_method_uri().c_str());
                    }
                    return res;
                    #else
                    return it->second(op);
                    #endif
                }
            }
            else
            {
                const int method_key = (op.get_method_type() << 8) | op.get_method_id();
                auto it = m_id_methods.find(method_key);
                if(it != m_id_methods.end())
                {
                    if(canvas != nullptr)
                        canvas->record(this,op);
                    
                    if(it->second)
                    {
                        #ifdef DEBUG
                        const xvalue_t res(it->second(op));
                        if( (res.get_type() == xvalue_t::enum_xvalue_type_error) && (res.get_error() != enum_xcode_successful) )
                        {
                            xerror("xvexeunit_t::execute,catch error(%d) for method_key(%d) of uri(%s)",res.get_error(),method_key,op.get_method_uri().c_str());
                        }
                        return res;
                        #else
                        return it->second(op);
                        #endif
                    }
                }
            }
            return not_impl(op);
        }
    
        xvexegroup_t::xvexegroup_t(enum_xdata_type type)
            :xvexeunit_t(type)
        {
        }
    
        xvexegroup_t::xvexegroup_t(xvexeunit_t * parent_unit,const std::string unit_name,enum_xdata_type type)
            :xvexeunit_t(parent_unit,unit_name,type)
        {
        }
    
        xvexegroup_t::xvexegroup_t(const xvexegroup_t & obj)
            :xvexeunit_t(obj)
        {
            clone_units_from(obj);
        }

        xvexegroup_t::~xvexegroup_t()
        {
            for(auto & u : m_child_units)
            {
                u.second->set_parent_unit(nullptr);//reset to parent ptr anyway
                u.second->release_ref();
            }
            m_child_units.clear();
        }
        
        bool  xvexegroup_t::clone_units_from(const xvexegroup_t & source)
        {
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
            if(is_close() == false)
            {
                for(auto & u : m_child_units)
                {
                    u.second->set_parent_unit(nullptr);//reset to parent ptr
                }
            }
            return xvexeunit_t::close(force_async);
        }
        
        void   xvexegroup_t::set_canvas(const xvcanvas_t * target)
        {
            xvexeunit_t::set_canvas(target); //set self first
            for(auto & u : m_child_units) //then reset child 'canvas
            {
                u.second->set_canvas(target);//reset to parent ptr
            }
        }
    
        void   xvexegroup_t::set_parent_unit(xvexeunit_t * parent_ptr)
        {
            xvexeunit_t::set_parent_unit(parent_ptr);//set self first
            
            for(auto & u : m_child_units)
                u.second->set_parent_unit(this); //reset and rebuild execution uri again
        }
    
        bool   xvexegroup_t::add_child_unit(xvexeunit_t * child)
        {
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
    
        xvexeunit_t *   xvexegroup_t::find_child_unit(const std::string & unit_name)
        {
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
            const std::string & target_execution_uri = op.get_method_uri();
            const std::string & this_execution_uri = get_execute_uri();
            if(target_execution_uri == this_execution_uri)
                return xvexeunit_t::execute(op,canvas);
            
            if(target_execution_uri.size() > this_execution_uri.size())
            {
                auto pos = target_execution_uri.find(this_execution_uri);
                if(pos == 0) //at current branch
                {
                    const std::string left_path = target_execution_uri.substr(this_execution_uri.size() + 1);//skip '.'
                    if(left_path.empty() == false)
                    {
                        auto next_dot_pos = left_path.find_first_of('.');
                        if(next_dot_pos != std::string::npos)
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
    };
};
