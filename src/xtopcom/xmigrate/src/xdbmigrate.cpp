// Copyright (c) 2018-Present Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <iostream>
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
        xmigratedb_t::xmigratedb_t(int db_kinds, const std::string & db_path)
        {
            m_store_path = db_path;
            m_db_face_ptr = db::xdb_factory_t::create(db_kinds,db_path);
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

        bool   xmigratedb_t::compact_range(const std::string & begin_key,const std::string & end_key)
        {
            return m_db_face_ptr->compact_range(begin_key, end_key);
        }
    
        //iterator each key of prefix.note: go throuh whole db if prefix is empty
        bool  xmigratedb_t::read_range(const std::string& prefix,db::xdb_iterator_callback callback,void * cookie)
        {
            return m_db_face_ptr->read_range(prefix,callback,cookie);
        }

        bool   xmigratedb_t::get_estimate_num_keys(uint64_t & num) const
        {
            return m_db_face_ptr->get_estimate_num_keys(num);
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
            const std::string dst_db_version = config_obj.get_config(root_path + "/dst_version");
            if(src_db_path.empty() || dst_db_path.empty() || dst_db_version.empty())
            {
                xerror("xdbmigrate_t::init,not found DB config at bad config(%s)",config_obj.dump().c_str());
                return enum_xerror_code_bad_config;
            }

            m_dst_db_version = dst_db_version;
            
            //step#2: open dst db and check if aready newst db version
            int dst_db_kind = db::xdb_kind_kvdb | db::xdb_kind_high_compress;
            m_dst_store_ptr = new xmigratedb_t(dst_db_kind, dst_db_path);
            if(m_dst_store_ptr->open_db() == false)
            {
                xerror("xdbmigrate_t::init,failed to open dst DB at path(%s)",dst_db_path.c_str());
                return enum_xerror_code_bad_config;
            }

            std::string actual_db_version = m_dst_store_ptr->get_value(xvdbkey_t::get_xdb_version_key());
            if (actual_db_version == dst_db_version)
            {
                xkinfo("xdbmigrate_t::init,succ aready migrate.dst db version(%s) for dst db path(%s)",dst_db_version.c_str(), dst_db_path.c_str());
                return enum_xcode_successful;
            }

            //step#3: open src db
            if (src_db_path == dst_db_path)
            {
                m_dst_store_ptr->add_ref();
                m_src_store_ptr = m_dst_store_ptr;
            }
            else
            {
                // src db path not exist is normal case, set target db version directly
                if (!base::xfile_utl::folder_exist(src_db_path))
                {
                    m_dst_store_ptr->set_value(xvdbkey_t::get_xdb_version_key(), dst_db_version);
                    xkinfo("xdbmigrate_t::init,succ src db path folder not exist.dst db version(%s) for dst db path(%s),src db path(%s)",dst_db_version.c_str(), dst_db_path.c_str(), src_db_path.c_str());
                    return enum_xcode_successful;
                }
                int src_db_kind = db::xdb_kind_kvdb | db::xdb_kind_readonly | db::xdb_kind_no_multi_cf;  // db v2 has no multi cf
                m_src_store_ptr = new xmigratedb_t(src_db_kind, src_db_path);
                if(m_src_store_ptr->open_db() == false)
                {
                    xerror("xdbmigrate_t::init,failed to open src DB at path(%s)",src_db_path.c_str());
                    return enum_xerror_code_bad_config;
                }
            }

            //step#4 : load filters at order
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
            
            std::string src_db_version = m_src_store_ptr->get_value(xvdbkey_t::get_xdb_version_key());
            xkinfo("xdbmigrate_t::init,finised. src db path = %s, src db version = %s, dst db path = %s, dst db version = %s",
                src_db_path.c_str(), src_db_version.c_str(), dst_db_path.c_str(), dst_db_version.c_str());
            std::cout << "xdbmigrate_t::init,finised. src db path = " << src_db_path << ", src db version = " << src_db_version << ", dst db path = " << dst_db_path << ", dst db version = " << dst_db_version << std::endl;
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
        
        std::vector<xdbevent_t*> xdbmigrate_t::thread_dbevent_get(uint32_t thread_index)
        {
            std::lock_guard<std::mutex> _lock(th_locks[thread_index]);
            auto & events_queue = th_dbevents[thread_index];
            if (events_queue.empty()) {
                return {};
            }
            
            std::vector<xdbevent_t*> events = events_queue;
            events_queue.clear();
            return events;
        }
        size_t xdbmigrate_t::thread_dbevent_set(uint32_t thread_index, xdbevent_t* dbevent)
        {
            std::lock_guard<std::mutex> _lock(th_locks[thread_index]);
            th_dbevents[thread_index].push_back(dbevent);
            return th_dbevents[thread_index].size();
        }

        void xdbmigrate_t::thread_process_dbevent(uint32_t thread_index)
        {
            std::cout << "xdbmigrate_t::thread_process_dbevent begin.thread_index=" << thread_index << " current_time_s = " << base::xtime_utl::gettimeofday() << std::endl;
            bool is_empty = false;
            uint32_t total_count = 0;
            do
            {
                is_empty = false;
                std::vector<xdbevent_t*> events = thread_dbevent_get(thread_index);
                if (!events.empty()) {
                    for (auto & event : events) {
                        m_filter_objects[0]->push_event_back(*event, nullptr);
                        event->release_ref();
                    }
                    total_count += (uint32_t)events.size();
                    m_processed_count += (uint32_t)events.size();
                } else {
                    is_empty = true;
                    sleep(1);
                }
            } while (!b_key_scan_finish || !is_empty);

            // do again when b_key_scan_finish == true
            do
            {
                is_empty = false;
                std::vector<xdbevent_t*> events = thread_dbevent_get(thread_index);
                if (!events.empty()) {
                    for (auto & event : events) {
                        m_filter_objects[0]->push_event_back(*event, nullptr);
                        event->release_ref();
                    }
                    total_count += (uint32_t)events.size();
                    m_processed_count += (uint32_t)events.size();
                } else {
                    is_empty = true;
                }
            } while (!b_key_scan_finish || !is_empty);
            std::cout << "xdbmigrate_t::thread_process_dbevent end.thread_index=" << thread_index << " total_count = " << total_count << " current_time_s = " << base::xtime_utl::gettimeofday() << std::endl;
        }

        bool  xdbmigrate_t::db_scan_callback_with_multi_thread(const std::string& key, const std::string& value,void*cookie)
        {
            xdbmigrate_t * pthis = (xdbmigrate_t*)cookie;
            return pthis->db_scan_callback_with_multi_thread(key,value);
        }

        bool  xdbmigrate_t::db_scan_callback_with_multi_thread(const std::string& key, const std::string& value)
        {
            if(is_close()) //stop handle it while closed
            {
                xerror("xdbmigrate_t::db_scan_callback_with_multi_thread,closed");
                return false; //stop scan when return false
            }
               
            if(m_filter_objects.empty() == false)
            {
                enum_xdbkey_type db_key_type = xvdbkey_t::get_dbkey_type(key);//carry real type
                xdbevent_t* db_event = new xdbevent_t(key,value,db_key_type,m_src_store_ptr,m_dst_store_ptr,enum_xdbevent_code_transfer);

                xdbg("xdbmigrate_t::db_scan_callback_with_multi_thread key=%s,key_type=%d", key.c_str(), db_key_type);

                if(db_event->get_event_code() == enum_xdbevent_code_transfer)
                {
                    if(m_src_store_ptr == m_dst_store_ptr)//same store
                        db_event->set_event_flag(xdbevent_t::enum_dbevent_flag_key_stored);
                }
                
                uint32_t thread_index = m_scaned_keys_num++ % THREAD_NUM;
                size_t queue_size = thread_dbevent_set(thread_index, db_event);

                if (m_scaned_keys_num - m_processed_count > 1000000) {
                    sleep(5);
                    // std::cout << "xdbmigrate_t::db_scan_callback_with_multi_thread wait process.m_scaned_keys_num=" << m_scaned_keys_num << " m_processed_count=" << m_processed_count << std::endl;
                }

                if (m_scaned_keys_num % 1000000 == 0)
                {
                    xinfo("xdbmigrate_t::db_scan_callback_with_multi_thread total estimate num = %ld, current num = %ld", m_total_keys_num, m_scaned_keys_num);
                    std::cout << "xdbmigrate_t::db_scan_callback_with_multi_thread total estimate num = " << m_total_keys_num << " m_scaned_keys_num=" << m_scaned_keys_num << " m_processed_count=" << m_processed_count << " current_time_s = " << base::xtime_utl::gettimeofday() << std::endl;
                }
                return true;
            }
            return false;
        }

        void xdbmigrate_t::scan_key_with_multi_thread()
        {
            b_key_scan_finish = false;            
            for (uint32_t i = 0; i < THREAD_NUM; i++) {
                std::thread th(&xdbmigrate_t::thread_process_dbevent, this, i);
                all_thread.emplace_back(std::move(th));
            }
            sleep(1);
            m_src_store_ptr->read_range("", db_scan_callback_with_multi_thread,this);

            b_key_scan_finish = true;
            for (size_t i = 0; i < THREAD_NUM; i++) {
                all_thread[i].join();
            }
        }

        void xdbmigrate_t::scan_key_without_multi_thread()
        {
            m_src_store_ptr->read_range("", db_scan_callback,this);
        }

        bool  xdbmigrate_t::run(const int32_t cur_thread_id,const uint64_t timenow_ms)
        {
            xkinfo("xdbmigrate_t::run");
            if(is_close() == false)
            {
                if(m_src_store_ptr != nullptr)
                {
                    int64_t begin_s = base::xtime_utl::gettimeofday();
                    m_src_store_ptr->get_estimate_num_keys(m_total_keys_num);
                    xinfo("xdbmigrate_t::run begin. src db total estimate num = %ld", m_total_keys_num);
                    std::cout << "xdbmigrate_t::run begin. src db total estimate num = " << m_total_keys_num << std::endl;

#if 1  // XTODO  use multi thread for better performance
                    scan_key_with_multi_thread();
#else
                    scan_key_without_multi_thread();
#endif

#if 0  // TODO(jimmy)
                    std::string begin_key;
                    std::string end_key;
                    m_dst_store_ptr->compact_range(begin_key, end_key);
#endif

                    // finally, update db version to dst db version
                    m_dst_store_ptr->set_value(xvdbkey_t::get_xdb_version_key(), m_dst_db_version);

                    int64_t end_s = base::xtime_utl::gettimeofday();
                    uint64_t dst_db_keys_num = 0;
                    m_dst_store_ptr->get_estimate_num_keys(dst_db_keys_num);
                    xinfo("xdbmigrate_t::run finish. dst db total estimate num = %ld, scan key num = %ld, total_time_s = %ld", dst_db_keys_num, m_scaned_keys_num, end_s - begin_s);
                    std::cout << "xdbmigrate_t::run finish. dst db total estimate num = " << dst_db_keys_num << ", scan key num = " << m_scaned_keys_num << ", total_time_s = " << end_s - begin_s << std::endl;
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

                xdbg("xdbmigrate_t::db_scan_callback key=%s,key_type=%d", key.c_str(), db_key_type);

                if(db_event.get_event_code() == enum_xdbevent_code_transfer)
                {
                    if(m_src_store_ptr == m_dst_store_ptr)//same store
                        db_event.set_event_flag(xdbevent_t::enum_dbevent_flag_key_stored);
                }
                m_filter_objects[0]->push_event_back(db_event, nullptr);

                if (m_scaned_keys_num++ % 1000000 == 0)
                {
                    xinfo("xdbmigrate_t::db_scan_callback total estimate num = %ld, current num = %ld", m_total_keys_num, m_scaned_keys_num);
                    std::cout << "xdbmigrate_t::db_scan_callback total estimate num = " << m_total_keys_num << ", current num = " << m_scaned_keys_num << " current_time_s = " << base::xtime_utl::gettimeofday() << std::endl;
                }
                return true;
            }
            return false;
        }
    
    }//end of namespace of base
}//end of namespace top
