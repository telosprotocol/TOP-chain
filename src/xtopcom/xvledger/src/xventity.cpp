// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvblock.h"
#include "../xventity.h"
#include "xbase/xcontext.h"
#include "xstatistic/xbasic_size.hpp"
#include "xmetrics/xmetrics.h"
 
namespace top
{
    namespace base
    {
        //---------------------------------xventity_t---------------------------------//
        xventity_t::xventity_t(enum_xdata_type type)
            :xdataunit_t(type)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xventity, 1);
            m_exe_module = NULL;
            m_entity_index = uint16_t(-1);
        }
    
        xventity_t::xventity_t(const xventity_t & other)
            :xdataunit_t(other)
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xventity, 1);
            m_exe_module = NULL;
            m_entity_index = other.m_entity_index;
        }
        
        xventity_t::~xventity_t()
        {
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xventity, -1);
            if(m_exe_module != NULL)
                m_exe_module->release_ref();
        }
        
        bool xventity_t::close(bool force_async)
        {
            set_exe_module(NULL); //reset to null
            return xdataunit_t::close(force_async);
        }
        
        void  xventity_t::set_exe_module(xvexemodule_t * exemodule_ptr)
        {
            if(exemodule_ptr != NULL)
                exemodule_ptr->add_ref();
            
            xvexemodule_t * old_ptr = xatomic_t::xexchange(m_exe_module, exemodule_ptr);
            if(old_ptr != NULL)
            {
                old_ptr->release_ref();
                old_ptr = NULL;
            }
        }
    
        //caller need to cast (void*) to related ptr
        void*  xventity_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_ventity)
                return this;
            
            return xdataunit_t::query_interface(_enum_xobject_type_);
        }
        
        int32_t   xventity_t::do_write(xstream_t & stream) //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            stream << m_entity_index;
            return (stream.size() - begin_size);
        }
        
        int32_t   xventity_t::do_read(xstream_t & stream)  //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            stream >> m_entity_index;
            return (begin_size - stream.size());
        }
    
        //---------------------------------xvinentity_t---------------------------------//
        xvinentity_t::xvinentity_t(const std::vector<xvaction_t*> & actions)
            :xventity_t(enum_xdata_type(enum_xobject_type_vinentity))
        {
            for(auto item : actions)
            {
                if(item != NULL)
                {
                    m_actions.emplace_back(*item);
                }
            }
        }
        
        xvinentity_t::xvinentity_t(const std::vector<xvaction_t> & actions)
            :xventity_t(enum_xdata_type(enum_xobject_type_vinentity))
        {
            for(auto & item : actions)
            {
                m_actions.emplace_back(item);
            }
        }

        xvinentity_t::xvinentity_t(const std::string & extend, const std::vector<xvaction_t> & actions)
        :xventity_t(enum_xdata_type(enum_xobject_type_vinentity))
        {
            m_extend_data = extend;
            for(auto & item : actions)
            {
                m_actions.emplace_back(item);
            }
        }
        
        xvinentity_t::xvinentity_t(std::vector<xvaction_t> && actions)
            :xventity_t(enum_xdata_type(enum_xobject_type_vinentity))
        {
            m_actions = std::move(actions);
        }
    
        xvinentity_t::xvinentity_t()
           :xventity_t(enum_xdata_type(enum_xobject_type_vinentity))
        {
        }
    
        xvinentity_t::~xvinentity_t()
        {
            m_actions.clear();
        }
        
        //caller need to cast (void*) to related ptr
        void*   xvinentity_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_vinentity)
                return this;
            
            return xventity_t::query_interface(_enum_xobject_type_);
        }
    
        const std::string xvinentity_t::query_value(const std::string & key) const //virtual key-value for entity
        {
            return std::string();
        }
        
        int32_t     xvinentity_t::do_write(xstream_t & stream)//not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            xventity_t::do_write(stream);
            
            stream.write_compact_var(m_extend_data);
            const uint16_t count = (uint16_t)m_actions.size();
            stream << count;
            for (auto & v : m_actions)
            {
                v.serialize_to(stream);
            }
            
            return (stream.size() - begin_size);
        }
        
        int32_t     xvinentity_t::do_read(xstream_t & stream) //not allow subclass change behavior
        {
            m_actions.clear();
            const int32_t begin_size = stream.size();
            xventity_t::do_read(stream);
            
            stream.read_compact_var(m_extend_data);
            uint16_t count = 0;
            stream >> count;
            for (uint32_t i = 0; i < count; i++)
            {
                xvaction_t action;
                const int res = action.serialize_from(stream);
                if(res <= 0)
                {
                    xerror("xvinentity_t::do_read,fail to read action as error(%d),count(%d)",res,count);
                    m_actions.clear(); //clean others as well
                    return res;
                }
                m_actions.emplace_back(action);
            }
            return (begin_size - stream.size());
        }
    
     
        xvintable_ent::xvintable_ent(const xvheader_t & target,const std::vector<xvaction_t*> & actions)
            :xvinentity_t(actions)
        {
            m_owner = NULL;
            m_owner = new xvheader_t(target);
        }
    
        xvintable_ent::xvintable_ent(const xvheader_t & target,const std::vector<xvaction_t> & actions)
            :xvinentity_t(actions)
        {
            m_owner = NULL;
            m_owner = new xvheader_t(target);
        }
    
        xvintable_ent::xvintable_ent(const xvheader_t & target,std::vector<xvaction_t> && actions)
            :xvinentity_t(actions)
        {
            m_owner = NULL;
            m_owner = new xvheader_t(target);
        }
     
        xvintable_ent::xvintable_ent()
        {
            m_owner = NULL;
        }
    
        xvintable_ent::~xvintable_ent()
        {
            if(m_owner != NULL)
                m_owner->release_ref();
        }
        
        int32_t   xvintable_ent::do_write(xstream_t & stream)//not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            
            std::string vheader_bin;
            if(m_owner != NULL)
            {
                m_owner->serialize_to_string(vheader_bin);
                stream.write_compact_var(vheader_bin);
                //then write actions
                xvinentity_t::do_write(stream);
            }
            else
            {
                stream.write_compact_var(vheader_bin); //write empty header
                xerror("xvintable_ent::do_write,nil header ptr");
            }
            
            return (stream.size() - begin_size);
        }
        
        int32_t     xvintable_ent::do_read(xstream_t & stream) //not allow subclass change behavior
        {
            //clear it first
            if(m_owner != NULL)
            {
                m_owner->release_ref();
                m_owner = NULL;
            }
            const int32_t begin_size = stream.size();
         
            std::string vheader_bin;
            stream.read_compact_var(vheader_bin);
            xassert(vheader_bin.empty() == false);
            if(vheader_bin.empty() == false)
            {
                xvheader_t*  vheader_ptr = xvblock_t::create_header_object(vheader_bin);
                xassert(vheader_ptr != NULL); //should has value
                if(vheader_ptr != NULL)
                {
                    m_owner = vheader_ptr; //create_header_object has added reference
                    //then read actions
                    xvinentity_t::do_read(stream);
                }
            }
            
            return (begin_size - stream.size());
        }
            
        //---------------------------------xvoutentity_t---------------------------------//
        xvoutentity_t::xvoutentity_t(const std::string & state_bin_log)
            :xventity_t(enum_xdata_type(enum_xobject_type_voutentity))
        {
            xassert(false);  // should not use
        }
    
        xvoutentity_t::xvoutentity_t(const xvoutentity_t & obj)
            :xventity_t(obj)
        {
            m_values = obj.m_values;
        }
    
        xvoutentity_t::xvoutentity_t()
            :xventity_t(enum_xdata_type(enum_xobject_type_voutentity))
        {
        }
    
        xvoutentity_t::~xvoutentity_t()
        {
            m_values.clear();
        }
    
        //caller need to cast (void*) to related ptr
        void*   xvoutentity_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_voutentity)
                return this;
            
            return xventity_t::query_interface(_enum_xobject_type_);
        }
        
        void  xvoutentity_t::set_value(const std::string & key, const std::string & value)
        {
            if (!value.empty())
            {
                m_values[key] = value;
            }
        }
        const std::string xvoutentity_t::query_value(const std::string & key) const //virtual key-value for entity
        {
            auto iter = m_values.find(key);
            if (iter != m_values.end()) {
                return iter->second;
            }
            return std::string();
        }
        
        int32_t     xvoutentity_t::do_write(xstream_t & stream)//not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            xventity_t::do_write(stream);
            uint16_t count = (uint16_t)m_values.size();
            stream.write_compact_var(count);
            for (auto & v : m_values)
            {
                stream.write_compact_var(v.first);
                stream.write_compact_var(v.second);
            }
            return (stream.size() - begin_size);
        }
        
        int32_t     xvoutentity_t::do_read(xstream_t & stream) //not allow subclass change behavior
        {
            m_values.clear();
            const int32_t begin_size = stream.size();
            xventity_t::do_read(stream);
            uint16_t count = 0;
            stream.read_compact_var(count);
            for (uint16_t i = 0; i < count; i++)
            {
                std::string key;
                std::string value;
                stream.read_compact_var(key);
                stream.read_compact_var(value);
                m_values[key] = value;
            }
            return (begin_size - stream.size());
        }
        
        //---------------------------------xvbinentity_t---------------------------------//
        xvbinentity_t::xvbinentity_t()
            :xventity_t(enum_xdata_type(enum_xobject_type_binventity))
        {
        }
        
        xvbinentity_t::xvbinentity_t(const std::string & raw_bin_data)
            :xventity_t(enum_xdata_type(enum_xobject_type_binventity))
        {
            m_raw_data = raw_bin_data;
        }
        
        xvbinentity_t::~xvbinentity_t()
        {
        }
        
        //caller need to cast (void*) to related ptr
        void*   xvbinentity_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == enum_xobject_type_binventity)
                return this;
            
            return xventity_t::query_interface(_enum_xobject_type_);
        }
        
        int32_t   xvbinentity_t::do_write(xstream_t & stream) //allow subclass extend behavior
        {
            const int32_t begin_size = stream.size();
            xventity_t::do_write(stream);
            
            stream << m_raw_data;
            return (stream.size() - begin_size);
        }
        
        int32_t   xvbinentity_t::do_read(xstream_t & stream)  //allow subclass extend behavior
        {
            m_raw_data.clear();
            const int32_t begin_size = stream.size();
            xventity_t::do_read(stream);
            
            stream >> m_raw_data;
            return (begin_size - stream.size());
        }

        //---------------------------------xvexemodule_t---------------------------------//
        xvexemodule_t::xvexemodule_t(enum_xobject_type type)
            :xobject_t(type)
        {
            m_resources_obj = NULL;
        }
    
        xvexemodule_t::xvexemodule_t(std::vector<xventity_t*> && entitys,const std::string & raw_resource_data,enum_xobject_type type)
            :xobject_t(type)
        {
            m_resources_obj = NULL;
            set_resources_data(raw_resource_data);
            
            m_entitys = entitys; //transfered owner of ptrs
            entitys.clear();
            for(size_t i = 0; i < m_entitys.size(); ++i)//reset index of entity
            {
                xventity_t * v = m_entitys[i];
                v->set_entity_index(i);
                v->set_exe_module(this);
            }
        }
        
        xvexemodule_t::xvexemodule_t(const std::vector<xventity_t*> & entitys, const std::string & raw_resource_data,enum_xobject_type type)
            :xobject_t(type)
        {
            m_resources_obj = NULL;
            set_resources_data(raw_resource_data);
            
            for(size_t i = 0; i < entitys.size(); ++i)
            {
                xventity_t * v = entitys[i];
                v->add_ref();
                v->set_entity_index(i);
                v->set_exe_module(this);
                m_entitys.push_back(v);
            }
        }
    
        xvexemodule_t::xvexemodule_t(std::vector<xventity_t*> && entitys,xstrmap_t* resource_obj, enum_xobject_type type)
            :xobject_t(type)
        {
            m_resources_obj = NULL;
            if(nullptr != resource_obj && resource_obj->empty() == false)
            {
                resource_obj->add_ref();
                m_resources_obj = resource_obj;
            }
            m_entitys = std::move(entitys);
            for(size_t i = 0; i < m_entitys.size(); ++i)//reset index of entity
            {
                xventity_t * v = m_entitys[i];
                v->set_entity_index(i);
                v->set_exe_module(this);
            }
        }
        
        xvexemodule_t::xvexemodule_t(const std::vector<xventity_t*> & entitys,xstrmap_t* resource_obj, enum_xobject_type type)
            :xobject_t(type)
        {
            m_resources_obj = NULL;
            if(nullptr != resource_obj && resource_obj->empty() == false)
            {
                resource_obj->add_ref();
                m_resources_obj = resource_obj;
            }
            
            for(size_t i = 0; i < entitys.size(); ++i)
            {
                xventity_t * v = entitys[i];
                v->add_ref();
                v->set_entity_index(i);
                v->set_exe_module(this);
                m_entitys.push_back(v);
            }
        }
        
        xvexemodule_t::~xvexemodule_t()
        {
            for (auto & v : m_entitys)
            {
                v->close();
                v->release_ref();
            }
            
            if(m_resources_obj != NULL)
            {
                m_resources_obj->close();
                m_resources_obj->release_ref();
            }
        }
        
        bool xvexemodule_t::close(bool force_async)
        {
            for (auto & v : m_entitys)
                v->close();
            
            if(m_resources_obj != NULL)
                m_resources_obj->close();
            
            return xobject_t::close(force_async);
        }
        
        //note:not safe for multiple thread at this layer
        const std::string xvexemodule_t::query_resource(const std::string & key)//virtual key-value for query resource
        {
            auto_reference<xstrmap_t> map_ptr(xatomic_t::xload(m_resources_obj));
            if(map_ptr != nullptr)
            {
                std::string value;
                map_ptr->get(key,value);
                return value;
            }
            return std::string(); //return empty one
        }
        
        const std::string xvexemodule_t::get_resources_data() //serialzie whole extend resource into one single string
        {
            auto_reference<xstrmap_t> map_ptr(xatomic_t::xload(m_resources_obj ));
            if(map_ptr == nullptr)
                return std::string();
            
            if(map_ptr->size() == 0)
                return std::string();
            
            std::string raw_bin;
            map_ptr->serialize_to_string(raw_bin);
            return raw_bin;
        }
        
        //xvblock has verifyed that raw_resource_data matched by raw_resource_hash
        bool   xvexemodule_t::set_resources_hash(const std::string & raw_resources_hash)
        {
            m_resources_hash = raw_resources_hash;
            return true;
        }

        int32_t xvexemodule_t::get_ex_alloc_size() const {
            int32_t ex_size = 0;
            ex_size += m_entitys.capacity() * sizeof(xventity_t*);
            for (auto & entity : m_entitys) {
                if (entity != nullptr) {
                    ex_size += sizeof(xventity_t);
                    xdbg("xvexemodule_t::get_ex_alloc_size ------cache size------- entity_size:%d", sizeof(xventity_t));
                }
            }

            if (m_resources_obj != nullptr) {
                auto & str_map = m_resources_obj->get_map();
                for (auto & pair : str_map) {
                    auto key_size = get_size(pair.first);
                    auto value_size = get_size(pair.second);
                    // each map node alloc 48B
                    ex_size += (key_size + value_size + 48);
                    xdbg("-----cache size----- xvexemodule_t key:%d,value:%d,node:48", key_size, value_size);
                }
                // // root node alloc 48B
                // xdbg("-----cache size----- xvexemodule_t root node:48");
                // ex_size += 48;
            }

            ex_size += get_size(m_resources_hash);
            xdbg("xvexemodule_t::get_ex_alloc_size ------cache size------- m_resources_hash:%d", get_size(m_resources_hash));
            return ex_size;
        }
        
        //xvblock has verifyed that raw_resource_data matched by raw_resource_hash
        bool   xvexemodule_t::set_resources_data(const std::string & raw_resource_data)
        {
            if(raw_resource_data.empty() == false)
            {
                xstream_t _stream(xcontext_t::instance(),(uint8_t*)raw_resource_data.data(),(uint32_t)raw_resource_data.size());
                xdataunit_t*  _data_obj_ptr = xdataunit_t::read_from(_stream);
                xassert(_data_obj_ptr != NULL);
                if(NULL == _data_obj_ptr)
                    return false;
                
                xstrmap_t*  map_ptr = (xstrmap_t*)_data_obj_ptr->query_interface(xdataobj_t:: enum_xdata_type_string_map);
                xassert(map_ptr != NULL);
                if(map_ptr == NULL)
                {
                    _data_obj_ptr->release_ref();
                    return false;
                }
                
                xstrmap_t * old_ptr = xatomic_t::xexchange(m_resources_obj, map_ptr);
                if(old_ptr != NULL)
                {
                    xcontext_t::instance().delay_release_object(old_ptr);
                }
            }
            return true;
        }
    
        int32_t   xvexemodule_t::serialize_to_string(bool include_resource,std::string & bin_data)
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            const int result = serialize_to(include_resource,_stream);
            if(result > 0)
                bin_data.assign((const char*)_stream.data(),_stream.size());
            
            return result;
        }
        
        int32_t   xvexemodule_t::serialize_to(bool include_resource,xstream_t & stream)
        {
            return do_write(include_resource, stream);
        }
        
        int32_t   xvexemodule_t::serialize_from_string(bool include_resource,const std::string & bin_data) //wrap function fo serialize_from(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            const int result = serialize_from(include_resource,_stream);
            return result;
        }
        
        int32_t   xvexemodule_t::serialize_from(bool include_resource,xstream_t & stream)//not allow subclass change behavior
        {
            return do_read(include_resource,stream);
        }
        
        int32_t     xvexemodule_t::do_write(bool include_resource,xstream_t & stream)//not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            stream.write_tiny_string(m_resources_hash);
            
            //XTODO, add flag to write m_resources_obj as well
            
            const uint16_t count = (uint16_t)m_entitys.size();
            stream << count;
            for (auto & item : m_entitys)
            {
                item->serialize_to(stream);
            }
            if (include_resource) {
                std::string resource = get_resources_data();
                stream.write_compact_var(resource);
            }
            return (stream.size() - begin_size);
        }
        
        int32_t     xvexemodule_t::do_read(bool include_resource,xstream_t & stream) //not allow subclass change behavior
        {
            const int32_t begin_size = stream.size();
            stream.read_tiny_string(m_resources_hash);
            
            //XTODO, add flag to read m_resources_obj as well
            
            uint16_t count = 0;
            stream >> count;
            for (uint32_t i = 0; i < count; i++)
            {
                xventity_t* entity = (xventity_t*)(base::xdataunit_t::read_from(stream));
                if(NULL == entity)
                {
                    xerror("xvexemodule_t::do_read,fail to read entity from stream");
                    return enum_xerror_code_bad_stream;
                }
                m_entitys.push_back(entity);
            }
            if (include_resource) {
                if (stream.size() <= 0) {
                    xerror("xvexemodule_t::do_read,fail to read resource from stream");
                    return enum_xerror_code_bad_stream;
                }
                std::string resource;
                stream.read_compact_var(resource);
                if (false == set_resources_data(resource)) {
                    xerror("xvexemodule_t::do_read,fail set resource from stream");
                    return enum_xerror_code_bad_stream;                    
                }
            }
            return (begin_size - stream.size());
        }
      
    };//end of namespace of base
};//end of namespace of top
