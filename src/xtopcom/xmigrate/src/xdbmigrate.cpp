// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xvledger/xvdbfilter.h"
#include "xdb/xdb_factory.h"
#include "xdbmigrate.h"
#include "xkeymigrate.h"
#include "xblkmigrate.h"
#include "xtxsmigrate.h"

namespace top
{
    namespace base
    {
        xmigratedb_t::xmigratedb_t(const std::string & db_path)
        {
            m_store_path = db_path;
            m_db_face_ptr = db::xdb_factory_t::create(db::xdb_kind_kvdb,db_path);
            xkinfo("xmigratedb_t::xmigratedb_t,db_path(%s)",db_path.c_str());
        }
    
        xmigratedb_t::~xmigratedb_t()
        {
            xkinfo("xmigratedb_t::destroryed,db_path(%s)",m_store_path.c_str());
            m_db_face_ptr->close();
        }
    
        bool  xmigratedb_t::open_db()
        {
            return m_db_face_ptr->open();
        }
    
        bool  xmigratedb_t::close_db()
        {
            return m_db_face_ptr->close();
        }
    
        bool xmigratedb_t::set_value(const std::string &key, const std::string &value)
        {
            return m_db_face_ptr->write(key, value);
        }
        
        bool xmigratedb_t::delete_value(const std::string &key)
        {
            return m_db_face_ptr->erase(key);
        }
        
        const std::string xmigratedb_t::get_value(const std::string &key) const
        {
            std::string value;
            bool success = m_db_face_ptr->read(key, value);
            if (!success)
            {
                return std::string();
            }
            return value;
        }
        
        bool  xmigratedb_t::delete_values(std::vector<std::string> & to_deleted_keys)
        {
            std::map<std::string, std::string> empty_put;
            return m_db_face_ptr->batch_change(empty_put, to_deleted_keys);
        }
        
        //prefix must start from first char of key
        bool   xmigratedb_t::read_range(const std::string& prefix, std::vector<std::string>& values)
        {
            return m_db_face_ptr->read_range(prefix,values);
        }
        
        //note:begin_key and end_key must has same style(first char of key)
        bool   xmigratedb_t::delete_range(const std::string & begin_key,const std::string & end_key)
        {
            return m_db_face_ptr->delete_range(begin_key,end_key);
        }
        
        //key must be readonly(never update after PUT),otherwise the behavior is undefined
        bool   xmigratedb_t::single_delete(const std::string & target_key)//key must be readonly(never update after PUT),otherwise the behavior is undefined
        {
            return m_db_face_ptr->single_delete(target_key);
        }
    
        //iterator each key of prefix.note: go throuh whole db if prefix is empty
        bool  xmigratedb_t::read_range(const std::string& prefix,db::xdb_iterator_callback callback,void * cookie)
        {
            return m_db_face_ptr->read_range(prefix,callback,cookie);
        }
    
        xdbmigrate_t::xdbmigrate_t()
        {
            m_src_store_ptr = nullptr;
            m_dst_store_ptr = nullptr;
            
            xkinfo("xdbmigrate_t::xdbmigrate_t");
        }
        
        xdbmigrate_t::~xdbmigrate_t()
        {
            xkinfo("xdbmigrate_t::desroyed");
            
            for(auto it : m_filter_objects)
            {
                xvfilter_t * obj_ptr = it;
                if(obj_ptr != nullptr)
                {
                    obj_ptr->close();
                    obj_ptr->release_ref();
                }
            }
            m_filter_objects.clear();
            
            if(m_src_store_ptr != nullptr)
            {
                if(m_src_store_ptr->is_close() == false)
                {
                    m_src_store_ptr->close_db();
                    m_src_store_ptr->close();
                }
                m_src_store_ptr->release_ref();
            }
            if( (m_dst_store_ptr != nullptr) && (m_dst_store_ptr != m_src_store_ptr) )
            {
                if(m_dst_store_ptr->is_close() == false)
                {
                    m_dst_store_ptr->close_db();
                    m_dst_store_ptr->close();
                }
                m_dst_store_ptr->release_ref();
            }
        }
    
        bool  xdbmigrate_t::is_valid(const uint32_t obj_ver)//check version
        {
            return xvmigrate_t::is_valid(obj_ver);
        }
        
        int   xdbmigrate_t::init(const xvconfig_t & config_obj)
        {
            xkinfo("xdbmigrate_t::init");
            
            if(is_close()) //stop handle it while closed
            {
                xerror("xdbmigrate_t::init,moduled closed");
                return enum_xerror_code_closed;
            }
            
            //step#0: prepare config
            xvmigrate_t::init(config_obj);
            const std::string root_path = get_register_key();//"/init/migrate/db"
            
            //step#1 : init & check
            const std::string src_db_path = config_obj.get_config(root_path + "/src_path");
            const std::string dst_db_path = config_obj.get_config(root_path + "/dst_path");
            if(src_db_path.empty())
            {
                xerror("xdbmigrate_t::init,not found DB config at bad config(%s)",config_obj.dump().c_str());
                return enum_xerror_code_bad_config;
            }
            
            //step#2: construct src and target db
            m_src_store_ptr = new xmigratedb_t(src_db_path);
            if(m_src_store_ptr->open_db() == false)
            {
                xerror("xdbmigrate_t::init,failed to open src DB at path(%s)",src_db_path.c_str());
                return enum_xerror_code_bad_config;
            }
            if( (src_db_path == dst_db_path) || (dst_db_path.empty()) )
            {
                m_src_store_ptr->add_ref();
                m_dst_store_ptr = m_src_store_ptr;
            }
            else //seperated db
            {
                m_dst_store_ptr = new xmigratedb_t(dst_db_path);
                if(m_dst_store_ptr->open_db() == false)
                {
                    xerror("xdbmigrate_t::init,failed to open dst DB at path(%s)",dst_db_path.c_str());
                    return enum_xerror_code_bad_config;
                }
            }

            //step#3 : load filters at order
            const int filters_count = (int)xstring_utl::toint32(config_obj.get_config(root_path + "/size"));
            for(int i = 0; i < filters_count; ++i)
            {
                const std::string config_path = root_path + "/" + xstring_utl::tostring(i);
                const std::string object_key  = config_obj.get_config(config_path + "/object_key");
                const uint32_t object_version = get_object_version();//force aligned with dbmigrated for each filter
                if(object_key.empty())
                {
                    xerror("xdbmigrate_t::init,bad config(%s)",config_obj.dump().c_str());
                    return enum_xerror_code_bad_config;
                }
                
                xsysobject_t * sys_object = xvsyslibrary::instance().create_object(object_key,object_version);
                if(nullptr == sys_object)
                {
                    xerror("xdbmigrate_t::init,failed to load filter object of key(%s) and version(0x%x)",object_key.c_str(),object_version);
                    return enum_xerror_code_not_found;
                }
                xvfilter_t * filter_object = (xvfilter_t*)sys_object->query_interface(enum_sys_object_type_filter);
                if(nullptr == filter_object)
                {
                    xerror("xdbmigrate_t::init,failed to conver to filter object of key(%s) and version(0x%x)",object_key.c_str(),object_version);
                    sys_object->close();
                    sys_object->release_ref();
                    return enum_xerror_code_bad_object;
                }
                
                if(filter_object->init(config_obj) != enum_xcode_successful)
                {
                    xerror("xdbmigrate_t::init,failed to init filter object of key(%s) and version(0x%x)",object_key.c_str(),object_version);
                    
                    filter_object->close();
                    filter_object->release_ref();
                    return enum_xerror_code_fail;
                }
                m_filter_objects.push_back(filter_object);
            }
            
            //step#4 : connect each filter together
            if(m_filter_objects.size() > 0)
            {
                xvfilter_t * front_filter = nullptr;
                for(auto it : m_filter_objects)
                {
                    if(nullptr == front_filter) //first one
                    {
                        front_filter = it;
                    }
                    else
                    {
                        front_filter->reset_back_filter(it);
                        front_filter = it;
                    }
                }
            }
            
            xkinfo("xdbmigrate_t::init,finised");
            return enum_xcode_successful;
        }
        
        bool  xdbmigrate_t::start(const int32_t at_thread_id)
        {
            xkinfo("xdbmigrate_t::start");
            if(is_close()) //stop handle it while closed
            {
                xerror("xdbmigrate_t::start,moduled closed");
                return false;
            }
            
            //start filters first
            for(auto it : m_filter_objects)
            {
                it->start(at_thread_id);
            }
            //then start scan whole db
            bool result = xvmigrate_t::start(at_thread_id);
            xkinfo("xdbmigrate_t::start,finished");
            return result;
        }
    
        bool  xdbmigrate_t::close(bool force_async) //close module
        {
            xkinfo("xdbmigrate_t::close");
            if(is_close() == false)
            {
                xvmigrate_t::close(force_async);//mark closed flag first
                
                //close every filter
                for(auto it : m_filter_objects)
                {
                    it->close(false);
                }
                
                //then closed db instance
                if(m_src_store_ptr != nullptr)
                {
                    if(m_src_store_ptr->close_db())
                        m_src_store_ptr->close();
                }
                if( (m_dst_store_ptr != nullptr) && (m_dst_store_ptr != m_src_store_ptr) )
                {
                    if(m_dst_store_ptr->close_db())
                        m_dst_store_ptr->close();
                }
            }
            xkinfo("xdbmigrate_t::close,finished");
            return true;
        }
        
        bool  xdbmigrate_t::run(const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xkinfo("xdbmigrate_t::run");
            if(is_close() == false)
            {
                if(m_src_store_ptr != nullptr)
                {
                    //scan all keys
                    m_src_store_ptr->read_range("", db_scan_callback,this);
                }
                //add loop here if need continue running
                
                //exit this module
                close();
            }
            
            xkinfo("xdbmigrate_t::run,finished");
            return true;
        }
    
        bool  xdbmigrate_t::db_scan_callback(const std::string& key, const std::string& value,void*cookie)
        {
            xdbmigrate_t * pthis = (xdbmigrate_t*)cookie;
            return pthis->db_scan_callback(key,value);
        }
    
        bool  xdbmigrate_t::db_scan_callback(const std::string& key, const std::string& value)
        {
            if(is_close()) //stop handle it while closed
            {
                xwarn("xdbmigrate_t::db_scan_callback,closed");
                return false; //stop scan when return false
            }
               
            if(m_filter_objects.empty() == false)
            {
                enum_xdbkey_type db_key_type = xvdbkey_t::get_dbkey_type(key);//carry real type
                xdbevent_t db_event(key,value,db_key_type,m_src_store_ptr,m_dst_store_ptr,enum_xdbevent_code_transfer);
                if(db_event.get_event_code() == enum_xdbevent_code_transfer)
                {
                    if(m_src_store_ptr == m_dst_store_ptr)//same store
                        db_event.set_event_flag(xdbevent_t::enum_dbevent_flag_key_stored);
                }
                m_filter_objects[0]->push_event_back(db_event, nullptr);
                return true;
            }
            return false;
        }
    
    }//end of namespace of base
}//end of namespace top
