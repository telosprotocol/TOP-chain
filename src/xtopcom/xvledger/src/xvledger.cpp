// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvledger.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xmetrics/xmetrics.h"
#include <json/value.h>
#include <json/reader.h>
#include <json/writer.h>
#include <fstream>
#include <sstream>
#ifdef DEBUG
    #define DEBUG_XVLEDGER
#endif

namespace top
{
    namespace base
    {
        xvaccountobj_t::xvaccountobj_t(xvtable_t & parent_object,const std::string & account_address)
             : xiobject_t(*parent_object.get_context(),parent_object.get_thread_id(),  enum_xobject_type_vaccount),
              xvaccount_t(account_address),
              m_ref_table(parent_object)//note:table object never be release/deleted,so here just hold reference
        {
            m_meta_ptr = NULL;
            m_is_idle = 1;  //init as idled status
            m_is_closing = 0; //init as running
            m_is_keep_forever = 0;
            memset(m_plugins,0,sizeof(m_plugins));
            m_idle_start_time_ms  = get_time_now();
            m_idle_timeout_ms     = enum_account_idle_timeout_ms;
            
            // not keek contract/table account forever for reducing memory overhead.
            // if(is_user_address() == false) //keep contract/table account forever at memory
            //     m_is_keep_forever  = 1;
            
            xinfo("xvaccountobj_t::xvaccountobj_t,acccount(%s)-xvid(%llu),m_is_keep_forever(%d),m_is_idle(%d)",get_address().c_str(),get_xvid(),m_is_keep_forever,m_is_idle);
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaccountobj, 1);
        }
    
        xvaccountobj_t::~xvaccountobj_t()
        {
            xdbg("xvaccountobj_t::destroy,acccount(%s)-xvid(%llu)",get_address().c_str(),get_xvid());
            for(int i = 0; i < enum_xvaccount_plugin_max; ++i)
            {
                xvactplugin_t* old_ptr = m_plugins[i];
                if(old_ptr != NULL)//catch exception case if have
                {
                    old_ptr->close(false);
                    old_ptr->release_ref();
                }
            }
            
            if(m_meta_ptr != NULL)
            {
                m_meta_ptr->close();
                m_meta_ptr->release_ref();
            }
            
            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xvaccountobj, -1);
        }
        
        const uint64_t  xvaccountobj_t::get_idle_duration() const
        {
            return m_idle_timeout_ms;
        }
    
        //status-change  :  live <-> idle -> closed
        bool xvaccountobj_t::is_live(const uint64_t timenow_ms)
        {
            if(is_closing() || is_close()) //be closed
                return false;
            
            if(m_is_keep_forever != 0) //table/book keep forever
                return true;
            
            if(   (is_idle() == false)//still active
               || (get_refcount() > 2) ) //more holder except table(hold 2 reference)
            {
                xdbg("xvaccountobj_t::is_live %s idle=%d,refcount=%d",get_account().c_str(), is_idle(),get_refcount());
                return true;
            }

            const uint64_t idle_start_time_ms = m_idle_start_time_ms;
            if( timenow_ms > (idle_start_time_ms + m_idle_timeout_ms) )
                return false;//time to close it since idle too long
            xdbg_info("xvaccountobj_t::is_live %s idle_start_time_ms=%ld,m_idle_timeout_ms=%ld,timenow_ms=%ld",get_account().c_str(), idle_start_time_ms,m_idle_timeout_ms,timenow_ms);
            return true;//still watching
        }
        
        //return status of currently,return true when it is idled
        bool  xvaccountobj_t::update_idle_status()
        {
            for(int i = enum_xvaccount_plugin_start; i <= enum_xvaccount_plugin_end; ++i)
            {
                xvactplugin_t* item = m_plugins[i];
                if( (item != nullptr) && (item->is_close() == false) && (item->is_closing() == false) )
                {
                    if(m_is_idle != 0)
                    {
                        xassert(0 == m_is_idle);
                        m_is_idle = 0;
                    }
                    return false; //indicate non-idle
                }
            }
            
            if(m_is_idle != 1)
            {
                m_is_idle = 1; //enter idling status
                m_idle_start_time_ms = get_time_now();
                
                #ifdef DEBUG_XVLEDGER
                xdbg_info("xvaccountobj_t::update_idle_status,enter idle status for account(%s)",get_address().c_str());
                #endif
            }
            return true;
        }

        void xvaccountobj_t::update_idle_start_time_ms(uint64_t current_time_ms)
        {
            if (current_time_ms > m_idle_start_time_ms) {
                m_idle_start_time_ms = current_time_ms;      
            }
        }
        
        bool xvaccountobj_t::stop() //convert to closing status if possible
        {
            if(is_close())
                return false;
            
            xdbg_info("xvaccountobj_t::stop,acccount(%s)",get_address().c_str());
            if(is_closing() == false)
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                m_is_closing = 1; //mark closing first
                
                //stop_plugin and update meta from plugin
                for(int i = 0; i < enum_xvaccount_plugin_max; ++i)
                {
                    xvactplugin_t* old_ptr = m_plugins[i];
                    if(old_ptr != NULL)//catch exception case if have
                    {
                        old_ptr->stop();//mark first to prevent reuse it anymore
                        
                        const xblockmeta_t* blockmeta = old_ptr->get_block_meta();
                        if(blockmeta != NULL) //block meta need update in-time(synchronization) as it 'lazy mode
                        {
                            get_meta()->set_block_meta(*blockmeta);
                            
                            //note:dont enable log at release since it sensitive for spinlock
                            #ifdef DEBUG_XVLEDGER
                            xdbg_info("xvaccountobj_t::stop,account(%s) meta=%s",get_address().c_str(), blockmeta->ddump().c_str());
                            #endif
                        }
                    }
                }
            }
            return true;
        }
    
        bool xvaccountobj_t::close(bool force_async)
        {
            xdbg_info("xvaccountobj_t::close,acccount(%s)",get_address().c_str());
            if(is_close() == false)
            {
                xobject_t::close(false); //force mark close flag
                
                std::vector<xvactplugin_t*> valid_plugins;
                {
                    xauto_lock<xspinlock_t> locker(get_spin_lock());
                    for(int i = 0; i < enum_xvaccount_plugin_max; ++i)
                    {
                        xvactplugin_t* old_ptr = m_plugins[i];
                        if(old_ptr != NULL)
                        {
                            xatomic_t::xreset(m_plugins[i]);
                            valid_plugins.push_back(old_ptr);
                        }
                    }
                }
                
                for(auto plugin_ptr : valid_plugins)
                {
                    if(plugin_ptr != NULL)//catch exception case if have
                    {
                        plugin_ptr->close(force_async);//true means that plugin do async job for some heavy process
                        plugin_ptr->release_ref();
                    }
                }
            }
            return true;
        }
 
        std::recursive_mutex&   xvaccountobj_t::get_table_lock()
        {
            return m_ref_table.get_lock();
        }
        
        //fetch and update together
        bool   xvaccountobj_t::update_block_meta(xvactplugin_t * plugin)
        {
            if(is_close())//not allow write anymore at closed status
                return false;
            
            if(NULL == plugin)
                return false;
            
            uint64_t new_highest_cert_height = 0;
            
            std::string vmeta_bin;
            bool update_success = false;
            if(NULL != plugin->get_block_meta())
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                xvactmeta_t * meta_ptr = get_meta();
                update_success = meta_ptr->set_block_meta(*plugin->get_block_meta());
                new_highest_cert_height = meta_ptr->_highest_cert_block_height;
                if(update_success)
                {
                    if(  (meta_ptr->get_modified_count() > enum_account_save_meta_interval)
                       ||(is_table_address() && new_highest_cert_height >= (meta_ptr->get_highest_saved_block_height() + enum_account_save_meta_interval) )
                       ||(!is_table_address() && new_highest_cert_height >= (meta_ptr->get_highest_saved_block_height() + enum_account_save_meta_offset) )
                       ||(meta_ptr->_highest_commit_block_height <= 2) )//froce save to db for the first 3 blocks
                    {
                        m_meta_ptr->update_meta_process_id(base::xvchain_t::instance().get_current_process_id());
                        m_meta_ptr->serialize_to_string(vmeta_bin);
                        m_meta_ptr->reset_modified_count();
                    }
                }
            }
            
            bool result = save_meta(vmeta_bin);
            if(result)
            {
                //update_highest_saved_block_height safe to call without lock
                get_meta()->update_highest_saved_block_height(new_highest_cert_height);
            }
            return update_success;
        }
           
        bool   xvaccountobj_t::set_block_meta(const xblockmeta_t & new_meta)
        {
            if(is_close())//not allow write anymore at closed status
                return false;
            
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            xvactmeta_t * meta_ptr = get_meta();
            const bool result = meta_ptr->set_block_meta(new_meta);
            if(result)
            {
                //note:dont enable log too much since it sensitive for spinlock
                #ifdef DEBUG_XVLEDGER
                xdbg_info("xvaccountobj_t::set_block_meta,meta=%s",new_meta.ddump().c_str());
                #endif
            }
            return result;
        }
        
        bool   xvaccountobj_t::set_state_meta(const xstatemeta_t & new_meta)
        {
            if(is_close())//not allow write anymore at closed status
                return false;
            
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            xvactmeta_t * meta_ptr = get_meta();
            const bool result = meta_ptr->set_state_meta(new_meta);
            if(result)
            {
                //note:dont enable log too much since it sensitive for spinlock
                #ifdef DEBUG_XVLEDGER
                xdbg_info("xvaccountobj_t::set_state_meta,meta=%s",new_meta.ddump().c_str());
                #endif
            }
            return result;
        }
        
        bool   xvaccountobj_t::set_sync_meta(const xsyncmeta_t & new_meta)
        {
            if(is_close())//not allow write anymore at closed status
                return false;
            
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            xvactmeta_t * meta_ptr = get_meta();
            const bool result = meta_ptr->set_sync_meta(new_meta);
            if(result)
            {
                //note:dont enable log too much since it sensitive for spinlock
                #ifdef DEBUG_XVLEDGER
                xdbg_info("xvaccountobj_t::set_sync_meta,meta=%s",new_meta.ddump().c_str());
                #endif
            }
            return result;
        }
        
        const xblockmeta_t  xvaccountobj_t::get_block_meta()
        {
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            return get_meta()->get_block_meta();
        }
        
        const xstatemeta_t  xvaccountobj_t::get_state_meta()
        {
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            return get_meta()->get_state_meta();
        }

        const xsyncmeta_t   xvaccountobj_t::get_sync_meta()
        {
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            return get_meta()->get_sync_meta();
        }
    
        const xvactmeta_t    xvaccountobj_t::get_full_meta()
        {
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            return *get_meta();
        }
    
        bool   xvaccountobj_t::set_latest_executed_block(const uint64_t height)
        {
            if(is_close())//not allow write anymore at closed status
                return false;
            
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            xvactmeta_t * meta_ptr = get_meta();
            const bool result = meta_ptr->set_latest_executed_block(height);

            return result;
        }
        
        const uint64_t   xvaccountobj_t::get_latest_executed_block_height()
        {
            //note:meta_ptr never be destroy,it is safe to get it without lock
            xvactmeta_t * meta_ptr = get_meta();
            const uint64_t atom_copy = base::xatomic_t::xload( meta_ptr->get_state_meta()._highest_execute_block_height);
            return atom_copy;
        }

        bool   xvaccountobj_t::set_lowest_executed_block_height(const uint64_t height)
        {
            if(is_close())//not allow write anymore at closed status
                return false;
            
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            xvactmeta_t * meta_ptr = get_meta();
            const bool result = meta_ptr->set_lowest_executed_block(height);

            return result;
        }

        uint64_t   xvaccountobj_t::get_lowest_executed_block_height()
        {
            //note:meta_ptr never be destroy,it is safe to get it without lock
            xvactmeta_t * meta_ptr = get_meta();
            const uint64_t atom_copy = base::xatomic_t::xload( meta_ptr->get_state_meta()._lowest_execute_block_height);
            return atom_copy;
        }
      
        bool   xvaccountobj_t::recover_meta(xvactmeta_t & _meta)//recover at account level if possible
        {
            return false;//default do nothing at account level
        }
    
        xvactmeta_t*  xvaccountobj_t::get_meta()
        {
            if( (m_meta_ptr != NULL) && (m_meta_ptr->is_close() == false) )
                return m_meta_ptr;
                   
            XMETRICS_GAUGE(metrics::store_block_meta_read, 1);
            const std::string full_meta_path = base::xvdbkey_t::create_account_meta_key(*this);
            const std::string meta_content = xvchain_t::instance().get_xdbstore()->get_value(full_meta_path);
   
            xvactmeta_t* new_meta_ptr = xvactmeta_t::load(*this,meta_content);
            const bool recovered_something = recover_meta(*new_meta_ptr);//try to recover the lost if rebooted
            if(recovered_something)//makeup the lost to DB
            {
                new_meta_ptr->update_meta_process_id(base::xvchain_t::instance().get_current_process_id()); //update process id to avoid double-saving
                std::string  recovered_meta_bin;
                new_meta_ptr->serialize_to_string(recovered_meta_bin);
                xvchain_t::instance().get_xdbstore()->set_value(full_meta_path,recovered_meta_bin);
            }

            xvactmeta_t * old_ptr = base::xatomic_t::xexchange(m_meta_ptr,new_meta_ptr);
            if(old_ptr != NULL)
            {
                old_ptr->close(); //close first
                
                xwarn("xvaccountobj_t::meta->get_meta,new_meta(%s) vs old_meta(%s)",new_meta_ptr->dump().c_str(),old_ptr->dump().c_str());
                old_ptr->release_ref();//then release
            }
            else
            {
                #ifdef DEBUG_XVLEDGER
                xinfo("xvaccountobj_t::meta->get_meta,new_meta(%s)",new_meta_ptr->dump().c_str());
                #endif
            }
            return m_meta_ptr;
        }
        
        bool  xvaccountobj_t::save_meta(const std::string & vmeta_bin)
        {
            if(vmeta_bin.empty() == false)
            {
                XMETRICS_GAUGE(metrics::store_block_meta_write, vmeta_bin.size());
                const std::string full_meta_path = base::xvdbkey_t::create_account_meta_key(*this);
                if(xvchain_t::instance().get_xdbstore()->set_value(full_meta_path,vmeta_bin))
                {
                    // #ifdef DEBUG_XVLEDGER
                    xinfo("xvaccountobj_t::meta->save_meta,%s,meta(%s)",get_address().c_str(), m_meta_ptr->dump().c_str());
                    // #endif
                    return true;
                }
                else //failure handle
                {
                    get_meta()->add_modified_count(true);//XTODO,may add enum_account_save_meta_interval

                    xerror("xvaccountobj_t::meta->save_meta,fail to write db for account(%s)",get_address().c_str());
                    return false;
                }
            }
            return false;
        }
        
        bool  xvaccountobj_t::save_meta(bool carry_process_id)
        {
            std::string vmeta_bin;
            if(m_meta_ptr != NULL)
            {
                const uint32_t last_modified_count = m_meta_ptr->get_modified_count();
                if(last_modified_count > 0)
                {
                    xauto_lock<xspinlock_t> locker(get_spin_lock());
                    if(carry_process_id)
                        m_meta_ptr->update_meta_process_id(base::xvchain_t::instance().get_current_process_id());
                    else
                        m_meta_ptr->update_meta_process_id(0);//0 means has been saved anything,no-need to recover op
                    
                    m_meta_ptr->serialize_to_string(vmeta_bin);
                    m_meta_ptr->reset_modified_count();//optimisic prediction
                }
            }
            
            bool result = save_meta(vmeta_bin);
            if(result)
            {
                //update_highest_saved_block_height safe to call without lock
                get_meta()->update_highest_saved_block_height(get_meta()->_highest_cert_block_height);
            }
            return result;
        }
    
        xauto_ptr<xvactplugin_t> xvaccountobj_t::get_plugin(enum_xvaccount_plugin_type plugin_type)
        {
            if(is_close()) //object not avaiable
            {
                xerror("xvaccountobj_t::get_plugin,closed account(%s)",get_address().c_str());
                return nullptr;
            }
            
            xvactplugin_t * plugin_ptr = NULL;
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                plugin_ptr = get_plugin_unsafe(plugin_type);
                if(plugin_ptr != NULL)
                {
                    plugin_ptr->add_ref();//note:add reference for xauto_ptr
                    
                    // const uint64_t _timenow = get_time_now();//XTODO(jimmy) optimize use account access time from table timer time
                    const uint64_t _timenow = m_idle_start_time_ms;
                    plugin_ptr->set_last_access_time(_timenow);
                }
            }
            if(NULL == plugin_ptr)
            {
                #ifdef DEBUG_XVLEDGER
                xdbg("xvaccountobj_t::get_plugin,not find plugin of type(%d) for account(%s) ",plugin_type,get_address().c_str());
                #endif
            }
            
            return plugin_ptr;
        }

        //locked by table or account self in advance
        xvactplugin_t*    xvaccountobj_t::get_plugin_unsafe(enum_xvaccount_plugin_type plugin_type)
        {
            if((int)plugin_type >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::get_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d)",plugin_type,enum_xvaccount_plugin_max);
                return nullptr;
            }

            xvactplugin_t* plugin_ptr = xatomic_t::xload(m_plugins[plugin_type]); // TODO(jimmy) optimize: already spin lock, no need xload
            if(plugin_ptr != nullptr)
            {
                if( (plugin_ptr->is_close() == false) && (plugin_ptr->is_closing() == false) )// TODO(jimmy) optimize: no need close check
                {
                    return plugin_ptr;
                }
            }
            return nullptr;//converted to xauto_ptr automatically
        }

        //locked by table or account self in advance
        bool  xvaccountobj_t::set_plugin_unsafe(xvactplugin_t * new_plugin_obj,xvactplugin_t*& old_plugin_obj,bool monitor)
        {
            if(nullptr == new_plugin_obj) //valid check
            {
                xassert(0);
                return false;
            }
         
            new_plugin_obj->add_ref();//hold reference for m_plugins[type]
            const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
            new_plugin_obj->set_last_access_time(_timenow);
            
            //replace existing one by new ptr
            old_plugin_obj = xatomic_t::xexchange(m_plugins[new_plugin_obj->get_plugin_type()],new_plugin_obj);
            if(old_plugin_obj == new_plugin_obj) //same one
            {
                old_plugin_obj->release_ref();
                old_plugin_obj = NULL;//note:must reset to null
                return true;//nothing changed
            }
            else if(NULL != old_plugin_obj)//successful
            {
                xdbgassert(old_plugin_obj->is_closing());
                xdbgassert(old_plugin_obj->is_close());
                old_plugin_obj->stop();//mark closing flag first
                //caller need release it
            }
            //send to monitoring thread, only monitor plugin can push account to active status
            if (monitor) {
                const uint8_t old_status = m_is_idle;
                m_is_idle = 0; //actived
                if(old_status != 0)
                {
                    xdbg_info("xvaccountobj_t::set_plugin_unsafe,enter active status for account(%s)",get_address().c_str());
                }                
                m_ref_table.monitor_plugin(new_plugin_obj);
            }            
            return true;
        }
        
        xauto_ptr<xvactplugin_t>  xvaccountobj_t::get_set_plugin(xvactplugin_t * new_plugin_obj, bool monitor)
        {
            if(nullptr == new_plugin_obj) //valid check
            {
                xassert(0);
                return nullptr;
            }
            if(new_plugin_obj->get_plugin_type() >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::get_set_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d)",new_plugin_obj->get_plugin_type(),enum_xvaccount_plugin_max);
                return nullptr;
            }
            if(new_plugin_obj->is_close() || new_plugin_obj->is_closing() )
            {
                xerror("xvaccountobj_t::get_set_plugin,a closed/idled plugin(%s)",new_plugin_obj->dump().c_str());
                return get_plugin(new_plugin_obj->get_plugin_type());
            }
            if(is_close()) //object not avaiable
            {
                xerror("xvaccountobj_t::get_set_plugin,closed account(%s)",get_address().c_str());
                return nullptr;
            }
            
            xvactplugin_t*  old_plugin_obj = NULL;
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                //pre-fetch existing one
                {
                    xvactplugin_t * existing_ptr = get_plugin_unsafe(new_plugin_obj->get_plugin_type());
                    if(existing_ptr != NULL)
                    {
                        existing_ptr->add_ref();
                        const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
                        existing_ptr->set_last_access_time(_timenow);
                        
                        return existing_ptr; //not allow overwrited existing working plugin
                    }
                }
                new_plugin_obj->init_meta(*get_meta());
                //now accupy slot and monitor it
                set_plugin_unsafe(new_plugin_obj,old_plugin_obj,monitor);
            }

            //do close outside of spin-lock, because it might re-enter
            if(old_plugin_obj != NULL)
            {
                xdbgassert(old_plugin_obj->is_closing());
                xdbgassert(old_plugin_obj->is_close());
                
                old_plugin_obj->close(false);//force closed existing one
                old_plugin_obj->release_ref();//quickly release reference
            }
            
            //add reference to auto_ptr
            new_plugin_obj->add_ref();
            xdbg_info("xvaccountobj_t::get_set_plugin account=%s,plugin type %d", get_address().c_str(), new_plugin_obj->get_plugin_type());
            return new_plugin_obj;
        }
    
        xauto_ptr<xvactplugin_t> xvaccountobj_t::get_set_plugin(enum_xvaccount_plugin_type plugin_type,std::function<xvactplugin_t*(xvaccountobj_t&)> & lambda, bool monitor)
        {
            if(is_close()) //object not avaiable
            {
                xerror("xvaccountobj_t::get_set_plugin,closed account(%s)",get_address().c_str());
                return nullptr;
            }
            
            xvactplugin_t*  old_plugin_obj = NULL;
            xvactplugin_t * new_plugin_ptr = NULL;
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                //pre-fetch existing one
                {
                    xvactplugin_t * existing_ptr = get_plugin_unsafe(plugin_type);
                    if(existing_ptr != NULL)
                    {
                        existing_ptr->add_ref();//note:add reference for xauto_ptr
                        const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
                        existing_ptr->set_last_access_time(_timenow);
                        
                        return existing_ptr;
                    }
                }
                
                //new plugin instance
                new_plugin_ptr = lambda(*this);//note:ownership transfer to auto_ptr when return
                new_plugin_ptr->init_meta(*get_meta());
                
                //exception check
                if(NULL == new_plugin_ptr)
                {
                    xassert(new_plugin_ptr != NULL);
                    return nullptr;
                }
                if(new_plugin_ptr->is_close() || new_plugin_ptr->is_closing() )
                {
                    xerror("xvaccountobj_t::get_set_plugin,a closed/idled plugin(%s)",new_plugin_ptr->dump().c_str());
                    new_plugin_ptr->release_ref();
                    return nullptr;
                }
                if(  (new_plugin_ptr->get_plugin_type() >= enum_xvaccount_plugin_max)
                   ||(new_plugin_ptr->get_plugin_type() != plugin_type) )
                {
                    xerror("xvaccountobj_t::get_set_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d),vs ask_plugin_type(%d)",new_plugin_ptr->get_plugin_type(),enum_xvaccount_plugin_max,plugin_type);
                    
                    new_plugin_ptr->release_ref();
                    return nullptr;
                }
                
                //finally accupy slot and monitor it
                set_plugin_unsafe(new_plugin_ptr,old_plugin_obj,monitor);//may hold additional reference
            }
            
            //do close outside of spin-lock, because it might re-enter
            if(old_plugin_obj != NULL)//it must alread closed
            {
                xdbgassert(old_plugin_obj->is_closing());
                xdbgassert(old_plugin_obj->is_close());
                
                old_plugin_obj->close(false);//force closed existing one
                old_plugin_obj->release_ref();//quickly release reference
            }
            //note: new_plugin_ptr has been added referene as lambda(*this)
            return new_plugin_ptr;
        }
 
        //note:try_reset_plugin may try hold lock and check is_live agian ,and if so then close
        bool  xvaccountobj_t::try_close_plugin(const uint64_t timenow_ms,enum_xvaccount_plugin_type plugin_type)
        {
            if(is_close()) //object not avaiable
            {
                xwarn("xvaccountobj_t::try_close_plugin,closed account(%s)",get_address().c_str()); // TODO(jimmy)
                return true;
            }
            if((int)plugin_type >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::try_close_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d)",plugin_type,enum_xvaccount_plugin_max);
                return true;
            }
            
            bool do_close = false;
            xvactplugin_t* target_plugin_obj = NULL;
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                target_plugin_obj = xatomic_t::xload(m_plugins[plugin_type]);
                if(target_plugin_obj != NULL)
                {
                    if(target_plugin_obj->is_live(timenow_ms) == false)
                    {
                        //once get here ,it guarentee nobody hold plugin object ptr except xvaccountobj_t and xvtable
                        
                        //step #1: mark target idle first,to avoid reuse it
                        target_plugin_obj->stop();//idle is a pre-status before closed
                        
                        //step #2: remove from plugins slot,now ownership transfered to target_plugin_obj
                        xatomic_t::xstore(m_plugins[plugin_type],(xvactplugin_t*)NULL);
                        
                        //step #3: update status of account
                        update_idle_status();
                        
                        //step #4: save unsaved data to db if need
                        target_plugin_obj->save_data();//note: that might have i/o related op
                        
                        //step #5: update meta of block
                        //note: it safe to directly fetch block meta since right now no other accessing blockstore
                        const xblockmeta_t* blockmeta = target_plugin_obj->get_block_meta();
                        if(blockmeta != NULL) //block meta need update in-time(synchronization) as it 'lazy mode
                        {
                            get_meta()->set_block_meta(*blockmeta);
                            
                            //note:dont enable log at release since it sensitive for spinlock
                            #ifdef DEBUG_XVLEDGER
                            xdbg_info("xvaccountobj_t::try_close_plugin,account(%s) meta=%s",get_address().c_str(), blockmeta->ddump().c_str());
                            #endif
                        }
                        
                        //step #6: trigger close by setting flag
                        do_close = true;
                    }
                }
                else //plugin has been closed and removed
                {
                    xwarn("xvaccountobj_t::try_close_plugin,dont find plugin of type(%d) of account(%s)",plugin_type,get_address().c_str());
                    return true;
                }
            }
            if(do_close)
            {
                //do close outside of spin-lock, because it might re-enter
                //and close is a heavy job
                target_plugin_obj->close(false);
                target_plugin_obj->release_ref();//release it finally
                
                #ifdef DEBUG_XVLEDGER
                xdbg_info("xvaccountobj_t::try_close_plugin,close plugin of type(%d) of account(%s)",plugin_type,get_address().c_str());
                #endif
            }
            return do_close;
        }
    
        bool  xvaccountobj_t::handle_event(const xvevent_t & ev)
        {
            //add your code here
            return false;
        }
    
        //---------------------------------------------xvtable_t---------------------------------------------//
        xvtable_t::xvtable_t(xvbook_t & parent_object,const int32_t thread_id,const uint8_t table_index)
            :xionode_t(parent_object, thread_id,enum_xobject_type_vtable),
             m_ref_book(parent_object)
        {
            m_reserved_4byte = 0;
            m_table_index = table_index;
            m_table_combine_addr = parent_object.get_book_combine_addr() | table_index;
            xkinfo("xvtable_t::xvbook_t,table_index(%d) with full_addr(%d)",m_table_index,m_table_combine_addr);
        }
        xvtable_t::~xvtable_t()
        {
            xerror("xvtable_t::destory,table_index(%d) with full_addr(%d)",m_table_index,m_table_combine_addr);
            //never destory & release,should not enter here
            clean_all(true);
        }
    
        bool   xvtable_t::clean_all(bool force_clean)
        {
            xwarn("xvtable_t::clean_all,table_index(%d) with full_addr(%d)",m_table_index,m_table_combine_addr);

            if(force_clean)
            {
                //do close under the protection of table 'mutext lock,since plugin might be still running at this case
                //step#1: acquired mutext lock of table
                std::lock_guard<std::recursive_mutex> locker(m_lock);
                //at here all plugin has doen their job because step#1 has acquired mutex lock
                
                //step#2: acquired spin lock of table and clean accounts
                std::map<std::string,xvaccountobj_t*> clone_accounts;
                {
                    xauto_lock<xspinlock_t> locker(get_spin_lock());
                    clone_accounts = m_accounts;
                    m_accounts.clear();
                    for(auto it : clone_accounts)//quickly mark stop for account
                    {
                        it.second->stop();//force to stop and upload meta into account
                        it.second->save_meta(false);//then save meta
                    }
                }
                //step#3: final close them
                {
                    for(auto it : clone_accounts)
                    {
                        it.second->close(false);//force to close(save raw data as well)
                        it.second->release_ref();//release reference
                    }
                }

            }
            else//just clean closed ones
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                for(auto it = m_accounts.begin(); it != m_accounts.end();)
                {
                    auto old = it; //just copy the old value
                    ++it;
                    if(old->second->is_close())
                    {
                        old->second->release_ref();
                        m_accounts.erase(old);
                    }
                }
            }
            return true;
        }
    
        bool    xvtable_t::on_process_close()//send process_close event to every objects
        {
            return clean_all(true);
        }
 
        void    xvtable_t::monitor_plugin(xvactplugin_t * plugin_obj)
        {
            xinfo("xvtable_t::monitor_plugin account=%s,plugin type(%d)",plugin_obj->get_account().c_str() ,plugin_obj->get_plugin_type());
            //push to monitor queue
            {
                std::function<void(void*)> _add_new_plugin_job = [this](void* _raw_ptr)->void{
                    xvactplugin_t * _new_obj = (xvactplugin_t*)_raw_ptr;
                    //transfer ownership of _new_obj to m_monitor_accounts now,so no-need release manually
                    m_monitor_plugins.insert(std::multimap<uint64_t,xvactplugin_t*>::value_type(_new_obj->get_idle_duration() + get_time_now(),_new_obj));
                };
                plugin_obj->add_ref();//add reference for _add_new_account_job
                post_call(_add_new_plugin_job,(void*)plugin_obj);//send account ptr to store'thread to manage lifecycle
            }
        }
    
        void    xvtable_t::monitor_account(xvaccountobj_t * account_obj)
        {
            xinfo("xvtable_t::monitor_account account=%s,account_obj=%p",account_obj->get_account().c_str());
            //push to monitor queue
            {
                std::function<void(void*)> _add_new_account_job = [this](void* _account_ptr)->void{
                    xvaccountobj_t * _new_obj = (xvaccountobj_t*)_account_ptr;
                    //transfer ownership of _new_obj to m_monitor_accounts now,so no-need release manually
                    m_monitor_accounts.insert(std::multimap<uint64_t,xvaccountobj_t*>::value_type(_new_obj->get_idle_duration() + get_time_now(),_new_obj));
                };
                account_obj->add_ref();//add reference for _add_new_account_job
                post_call(_add_new_account_job,(void*)account_obj);//send account ptr to store'thread to manage lifecycle
            }
        }
    
        xvaccountobj_t* xvtable_t::find_account_unsafe(const std::string & account_address)
        {
            auto it = m_accounts.find(account_address);
            if(it != m_accounts.end())
            {
                return it->second;
            }
            return NULL;
        }
    
        xvaccountobj_t*     xvtable_t::create_account_unsafe(const std::string & account_address)
        {
            xvaccountobj_t * new_account_obj = new xvaccountobj_t(*this,account_address);
            return new_account_obj;
        }
    
        xauto_ptr<xvaccountobj_t>   xvtable_t::get_account(const std::string & account_address)
        {
            xauto_lock<xspinlock_t> locker(get_spin_lock());
            xvaccountobj_t * account_ptr = get_account_unsafe(account_address);
            account_ptr->add_ref(); //add reference to pair xauto_ptr
            
            return account_ptr;
        }
        
        xvaccountobj_t*   xvtable_t::get_account_unsafe(const std::string & account_address)
        {
            const uint64_t current_time_ms = m_current_time_ms;  // TODO(jimmy) used table timer time for performance
            auto & exist_account_ptr = m_accounts[account_address];
            if(   (exist_account_ptr != NULL)
               && (exist_account_ptr->is_close() == false)
               && (exist_account_ptr->is_closing() == false)
            ) //valid account object
            {
                exist_account_ptr->update_idle_start_time_ms(current_time_ms);// refresh access time
                return exist_account_ptr;
            }

            xvaccountobj_t * old_account_obj = exist_account_ptr; //backup first
            xvaccountobj_t * new_account_obj = create_account_unsafe(account_address);
            exist_account_ptr = new_account_obj;//overwrite the referenced exist_account_ptr
            if(old_account_obj != NULL)//it must has been closed
            {
                old_account_obj->close(false);//force makr as closed
                old_account_obj->release_ref();//release it
            }
            //push to monitor queue
            new_account_obj->update_idle_start_time_ms(current_time_ms);// refresh access time
            monitor_account(new_account_obj);
            return new_account_obj;
        }
            
        bool xvtable_t::try_close_account(const int64_t current_time_ms,const std::string & account_address)
        {
            xdbg_info("xvtable_t::try_close_account,account(%s)",account_address.c_str());
            bool  do_close = false;
            xvaccountobj_t * target_account_ptr = NULL;
            {
                xauto_lock<xspinlock_t> locker(get_spin_lock());
                
                //find target account object ptr
                auto it = m_accounts.find(account_address);
                if(it != m_accounts.end())
                {
                    target_account_ptr = it->second;
                    if(target_account_ptr != NULL)
                    {
                        if(target_account_ptr->is_live(current_time_ms) == false)
                        {
                            //once get here ,it guarentee nobody hold target_account_ptr object ptr except xvtable
                            //actually also guanrentee no-plugin is avaiable in this account object
                            
                            //step #1: mark account ptr as closing status to avoid reuse it
                            target_account_ptr->stop();//also push all plugin submit their data
                            
                            //step #2: remove from table ' slot,now ownership be transfered to target_account_ptr
                            m_accounts.erase(it);
                            
                            //step #3: save all unsaved thing if have.but at most case it just return as no-change
                            target_account_ptr->save_meta(); //note:io related job
                            
                            //step #4: set flag to clean it
                            do_close = true;
                        }
                    }
                    else //account object reset to null
                    {
                        m_accounts.erase(it);
                        xwarn("xvtable_t::try_close_account,find null account(%s)",account_address.c_str());
                        return true;
                    }
                }
                else //account object has been removed
                {
                    xwarn("xvtable_t::try_close_account,dont find account(%s)",account_address.c_str());
                    return true;
                }
            }
            if(do_close)
            {
                if(target_account_ptr != NULL)
                {
                    target_account_ptr->close(false);//close
                    target_account_ptr->release_ref();//release it
                }
            }
            return do_close;
        }
    
        bool   xvtable_t::on_timer_for_accounts(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms)
        {
            std::deque<xvaccountobj_t*> _remonitor_list;
            
            int   expired_items_count = 0;
            auto  expire_it = m_monitor_accounts.begin();
            while(expire_it != m_monitor_accounts.end())
            {
                //map or multiplemap are sorted as < operation as default
                if(current_time_ms < (int64_t)expire_it->first )
                    break;
                
                xvaccountobj_t* _test_for_account = expire_it->second;
                if(_test_for_account != nullptr)
                {
                    if(_test_for_account->is_live(current_time_ms) == false)//quick test whether idle too long
                    {
                        if(_test_for_account->is_close())//has been closed by on_process_close()
                        {
                            xkinfo("xvtable_t::timer,found closed account(%s)",_test_for_account->get_address().c_str());
                            _test_for_account->release_ref();//destroy it finally
                        }
                        else //do reguare  check
                        {
                            _test_for_account->save_meta();//do heavy job at this thread to reduce io within table' spinlock
                            if(try_close_account(current_time_ms, _test_for_account->get_address()))
                            {
                                xdbg("xvtable_t::timer,closed account(%s)",_test_for_account->get_address().c_str());
                                _test_for_account->release_ref();//destroy it finally
                            }
                            else
                                _remonitor_list.push_back(_test_for_account);//transfer to list
                        }
                    }
                    else
                    {
                        //save meta at every cycle if have any change
                        _test_for_account->save_meta();//do heavy job at this thread
                        _remonitor_list.push_back(_test_for_account);//transfer to list
                    }
                }
                auto old = expire_it; //just copy the old value
                ++expire_it;
                m_monitor_accounts.erase(old);
                ++expired_items_count;
            }
            
            for(auto it : _remonitor_list)
            {
                m_monitor_accounts.insert(std::multimap<uint64_t,xvaccountobj_t*>::value_type(it->get_idle_duration() + current_time_ms,it));
            }
            return true;
        }
        
        bool   xvtable_t::on_timer_for_plugins(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms)
        {
            std::deque<xvactplugin_t*> _remonitor_list;
            
            int   expired_items_count = 0;
            auto  expire_it = m_monitor_plugins.begin();
            while(expire_it != m_monitor_plugins.end())
            {
                //map or multiplemap are sorted as < operation as default
                if(current_time_ms < (int64_t)expire_it->first )
                    break;
                
                xvactplugin_t* _test_for_plugin = expire_it->second;
                if(_test_for_plugin != nullptr)
                {
                    if(_test_for_plugin->is_live(current_time_ms) == false)//quick test
                    {
                        if(_test_for_plugin->is_close())//has been closed by on_process_close()
                        {
                            xinfo("xvtable_t::timer,found closed plugin(%d) of account(%s)",_test_for_plugin->get_plugin_type(),_test_for_plugin->get_account_obj()->get_address().c_str());
                            _test_for_plugin->release_ref();//destroy it finally
                        }
                        else
                        {
                            //try do heave job first
                            {
                                m_lock.lock();
                                _test_for_plugin->save_data();
                                m_lock.unlock();
                            }
                            //close_plugin may try hold lock and check is_live agian ,and if so may close
                            if(_test_for_plugin->get_account_obj()->try_close_plugin(current_time_ms,_test_for_plugin->get_plugin_type()))
                            {
                                //force to save meta once one plugin is offline
                                _test_for_plugin->get_account_obj()->save_meta();
                                
                                xdbg("xvtable_t::timer,closed plugin(%d) of account(%s)",_test_for_plugin->get_plugin_type(),_test_for_plugin->get_account_obj()->get_address().c_str());
                                _test_for_plugin->release_ref();//destroy it finally
                            }
                            else
                                _remonitor_list.push_back(_test_for_plugin);//transfer to list
                        }
                    }
                    else
                    {
                        _remonitor_list.push_back(_test_for_plugin);//transfer to list
                    }
                }
                auto old = expire_it; //just copy the old value
                ++expire_it;
                m_monitor_plugins.erase(old);
                ++expired_items_count;
            }
            
            for(auto it : _remonitor_list)
            {
                m_monitor_plugins.insert(std::multimap<uint64_t,xvactplugin_t*>::value_type(it->get_idle_duration() + current_time_ms,it));
            }
            return true;
        }
    
        bool   xvtable_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms)
        {
            m_current_time_ms = current_time_ms; // XTODO save for performance
            on_timer_for_plugins(thread_id, timer_id, current_time_ms, start_timeout_ms);
            on_timer_for_accounts(thread_id, timer_id, current_time_ms, start_timeout_ms);
            return true;
        }
    
        bool  xvtable_t::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms)
        {
            //never happend actually
            xerror("xvtable_t::on_timer_stop,timer is stopped!,m_table_index(%llu)",m_table_index);
            return true;
        }
 
        //---------------------------------------------xvbook_t---------------------------------------------//
        xvbook_t::xvbook_t(xvledger_t & parent_object,const int32_t thread_id,const uint8_t book_index)
          :xionode_t(parent_object, thread_id,enum_xobject_type_vbook)
        {
            m_monitor_timer  = nullptr;
            m_book_index = book_index;
            m_book_combine_addr = (parent_object.get_ledger_id() << 10) | set_vledger_subaddr(book_index,0);
            memset(m_tables,0,sizeof(m_tables));
            
            //each book has own timer
            m_monitor_timer = get_thread()->create_timer((base::xtimersink_t*)this);
            m_monitor_timer->start(enum_timer_check_interval, enum_timer_check_interval); //check account by every 10 seconds
            
            xkinfo("xvbook_t::xvbook_t,book_index(%d) with full_addr(%d)",m_book_index,m_book_combine_addr);
        }
        xvbook_t::~xvbook_t()
        {
            //never destory & never release,should not enter here
            xerror("xvbook_t::destory,book_index(%d) with full_addr(%d)",m_book_index,m_book_combine_addr);
            clean_all(true);
            
            if(m_monitor_timer != nullptr)
            {
                m_monitor_timer->stop();
                m_monitor_timer->close();
                m_monitor_timer->release_ref();
            }
        }
        
        bool  xvbook_t::clean_all(bool force_clean)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vbook_has_tables_count; i++)
            {
                xvtable_t* exist_ptr = m_tables[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->clean_all(force_clean);
                }
            }
            return true;
        }
    
        bool  xvbook_t::on_process_close()//send process_close event to every objects
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vbook_has_tables_count; i++)
            {
                xvtable_t* exist_ptr = m_tables[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->on_process_close();
                }
            }
            return true;
        }
 
        xvtable_t*   xvbook_t::get_table(const xvid_t & account_id)
        {
            if(m_book_index != get_vledger_book_index(account_id))
            {
                xerror("xvbook_t::get_table,bad book index of account_id(%llu) vs m_book_index(%llu)",account_id,m_book_index);
                return NULL;
            }
            
            const uint32_t table_id = (uint32_t)get_vledger_table_index(account_id);
            xvtable_t* table_ptr = m_tables[table_id]; //index has been verifyed to not over the array
            if (NULL != table_ptr)
                return table_ptr;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if (NULL != m_tables[table_id])
                return m_tables[table_id];
            
            //shared same thread for book
            table_ptr = create_table_object(table_id);
            m_tables[table_id] = table_ptr;
            return table_ptr;
        }
        
        xvtable_t*   xvbook_t::create_table_object(const uint32_t table_index)
        {
            return new xvtable_t(*this,get_thread_id(),(uint8_t)table_index);
        }
        
        bool  xvbook_t::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            for (int i = 0; i < enum_vbook_has_tables_count; i++)
            {
                xvtable_t* table_ptr = m_tables[i];
                if(table_ptr != NULL)
                {
                    table_ptr->on_timer_fire(thread_id,timer_id,current_time_ms,start_timeout_ms);
                }
            }
            return true;
        }
        
        bool  xvbook_t::on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //attached into io-thread
        {
            return true;
        }
        
        bool  xvbook_t::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //detach means it detach
        {
            for (int i = 0; i < enum_vbook_has_tables_count; i++)
            {
                xvtable_t* table_ptr = m_tables[i];
                if(table_ptr != NULL)
                {
                    table_ptr->on_timer_stop(errorcode,thread_id,timer_id,cur_time_ms,timeout_ms);
                }
            }
            return true;
        }
        
        //---------------------------------------------xvledger_t---------------------------------------------//
        xvledger_t::xvledger_t(xvchain_t & parent_object,const int32_t thread_id,const uint16_t vledger_id)
            :xionode_t(parent_object,thread_id,enum_xobject_type_vledger)
        {
            m_ledger_id = vledger_id;
            memset(m_books,0,sizeof(m_books));
            
            xkinfo("xvledger_t::xvledger_t,ledger(%d)",m_ledger_id);
        }
    
        xvledger_t::~xvledger_t()
        {
            //never destory & never release,should not enter here
            xerror("xvledger_t::destory,ledger(%d)",m_ledger_id);
            clean_all(true);
        }
        
        bool    xvledger_t::clean_all(bool force_clean)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vbucket_has_books_count; i++)
            {
                xvbook_t* exist_ptr = m_books[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->clean_all(force_clean);
                }
            }
            return true;
        }
    
        bool  xvledger_t::on_process_close()//send process_close event to every objects
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vbucket_has_books_count; i++)
            {
                xvbook_t* exist_ptr = m_books[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->on_process_close();
                }
            }
            return true;
        }
     
        xvtable_t* xvledger_t::get_table(const std::string & account_address)//wrap function
        {
            xvaccount_t account_id(account_address);
            return get_table(account_id);
        }
    
        xvtable_t* xvledger_t::get_table(const xvid_t & account_id)
        {
            xvbook_t * _taget_book = get_book(account_id);
            if(_taget_book != NULL)
                return _taget_book->get_table(account_id);
            
            return NULL;
        }
    
        xauto_ptr<xvaccountobj_t>  xvledger_t::get_account(const std::string & account_address)
        {
            return get_table(account_address)->get_account(account_address);
        }
    
        xauto_ptr<xvaccountobj_t>  xvledger_t::get_account(const xvaccount_t & account_obj)
        {
            return get_table(account_obj.get_xvid())->get_account(account_obj.get_address());
        }
    
        xvbook_t*  xvledger_t::get_book(const xvid_t & account_id)
        {
            if( (m_ledger_id != 0) && (m_ledger_id != get_vledger_ledger_id(account_id)) )
            {
                xerror("xvledger_t::get_book,bad ledger id of account_id(%llu) vs m_ledger_id(%llu)",account_id,m_ledger_id);
                return NULL;
            }
            
            const uint32_t book_id = get_vledger_book_index(account_id);
            xvbook_t* book_ptr = m_books[book_id];
            if (NULL != book_ptr)
                return book_ptr;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if (NULL != m_books[book_id])
                return m_books[book_id];
            
            book_ptr = create_book_object(book_id);
            m_books[book_id] = book_ptr;
            return book_ptr;
        }
    
        xvbook_t*  xvledger_t::create_book_object(const uint32_t book_index)
        {
            return new xvbook_t(*this,get_thread_id(),(uint8_t)book_index);
        }
    
        //----------------------------------xvchain_t------------------------------------//
        xvchain_t * xvchain_t::__global_vchain_instance = NULL;
        xvchain_t &  xvchain_t::instance(const uint16_t chain_id)
        {
            if(__global_vchain_instance != NULL)
                return *__global_vchain_instance;
            
            //give dedicated thread for ledger and chain
             xiothread_t *_monitor_thread = base::xcontext_t::instance().find_thread(base::xiothread_t::enum_xthread_type_monitor, false);
            if(NULL == _monitor_thread)
                _monitor_thread = base::xiothread_t::create_thread(base::xcontext_t::instance(),base::xiothread_t::enum_xthread_type_monitor,-1);
            
            xassert(_monitor_thread != NULL);
            __global_vchain_instance = new xvchain_t(_monitor_thread->get_thread_id(),chain_id);
            return *__global_vchain_instance;
        }
 
        xvchain_t::xvchain_t(const int32_t thread_id,const uint16_t chain_id)
            :xionode_t(xcontext_t::instance(),thread_id,enum_xobject_type_vchain)
        {
            //initialize
            m_is_auto_prune = 0;//init as disabled status
            m_reserved_2 = 0;
            m_round_number = 0;
            m_chain_id = 0;
            m_current_node_roles = 0;
            m_current_process_id = 0;
            m_proces_start_time  = 0;
            //end of initialize
            
            m_chain_id = chain_id;
            memset(m_ledgers,0,sizeof(m_ledgers));
            m_current_process_id = base::xsys_utl::get_sys_process_id();
            m_proces_start_time = xtime_utl::gmttime_ms();
            
            //build default stores
            // xauto_ptr<xvstatestore_t> default_state_store(new xvstatestore_t());
            // set_xstatestore(default_state_store());
            
            // xauto_ptr<xvtxstore_t> default_txs_store(new xvtxstore_t());
            // set_xtxstore(default_txs_store());
            
            xauto_ptr<xvcontractstore_t> default_contract_store(new xvcontractstore_t());
            set_xcontractstore(default_contract_store());
            
            xvdrecycle_mgr * default_recycle_mgr = new xvdrecycle_mgr();
            set_xrecyclemgr(default_recycle_mgr);//may hold addtional reference
            default_recycle_mgr->release_ref();
            
            xkinfo("xvchain_t::xvchain_t,chain_id(%d)",m_chain_id);
        }
    
        xvchain_t::~xvchain_t()
        {
            //never destoryed
            xerror("xvchain_t::destroy,chain_id(%d)",m_chain_id);
            clean_all(true);
        }
    
        bool   xvchain_t::clean_all(bool force_clean)
        {
            xkinfo("xvchain_t::clean_all(),start!");
            const int64_t begin_time_ms = xtime_utl::time_now_ms();
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vchain_has_buckets_count; i++)
            {
                xvledger_t* exist_ptr = m_ledgers[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->clean_all(force_clean);//never destoryed,just do clean
                }
            }
            
            const int duration = (int)(xtime_utl::time_now_ms() - begin_time_ms);
            xkinfo("xvchain_t::clean_all(),finish after duration(%d)ms",duration);
            return true;
        }
    
        bool   xvchain_t::on_process_close() //save all unsaved data and meta etc
        {
            xkinfo("xvchain_t::on_process_close(),start!");
            const int64_t begin_time_ms = xtime_utl::time_now_ms();
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vchain_has_buckets_count; i++)
            {
                xvledger_t* exist_ptr = m_ledgers[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->on_process_close();//never destoryed,just do clean
                }
            }
            
            const int duration = (int)(xtime_utl::time_now_ms() - begin_time_ms);
            xkinfo("xvchain_t::on_process_close(),finish after duration(%d)ms",duration);
            return true;
        }
    
        bool   xvchain_t::save_all()
        {
            xkinfo("xvchain_t::save_all()");
            return on_process_close();
        }
        
        xvtable_t*  xvchain_t::get_table(const xvid_t & account_id)
        {
            xvledger_t * taget_ledger = get_ledger(account_id);
            if(taget_ledger != NULL)
                return taget_ledger->get_table(account_id);
            
            return NULL;
        }
    
        xvtable_t*  xvchain_t::get_table(const std::string & account_address)
        {
            xvaccount_t account_id(account_address);
            return get_table(account_id.get_xvid());
        }
    
        xauto_ptr<xvaccountobj_t> xvchain_t::get_account(const std::string & account_address)
        {
            return get_table(account_address)->get_account(account_address);
        }
    
        xauto_ptr<xvaccountobj_t> xvchain_t::get_account(const xvaccount_t & account_obj)
        {
            return get_table(account_obj.get_xvid())->get_account(account_obj.get_address());
        }
    
        xvledger_t*  xvchain_t::get_ledger(const xvid_t & account_id)
        {
            if((m_chain_id != 0) && (m_chain_id != get_vledger_chain_id(account_id)))
            {
                xerror("xvchain_t::get_ledger,bad chain id of account_id(%llu) vs m_chain_id(%llu)",account_id,m_chain_id);
                return NULL;
            }
            
            const uint32_t bucket_index = get_vledger_bucket_index(account_id);
            xvledger_t* bucket_ptr = m_ledgers[bucket_index];
            if (NULL != bucket_ptr)
                return bucket_ptr;
            
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if (NULL != m_ledgers[bucket_index])
                return m_ledgers[bucket_index];
            
            bucket_ptr = create_ledger_object(get_vledger_ledger_id(account_id));
            m_ledgers[bucket_index] = bucket_ptr;
            return bucket_ptr;
        }
    
        xvledger_t*   xvchain_t::create_ledger_object(const uint64_t ledger_id)
        {
            return new xvledger_t(*this,get_thread_id(),(uint16_t)ledger_id);
        }
    
        xvdbstore_t*    xvchain_t::get_xdbstore() //must be valid
        {
            xobject_t* target = m_plugins[enum_xvchain_plugin_kdb_store];
            //xassert(target != NULL);
            return (xvdbstore_t*)target;
        }
        
        xvtxstore_t*    xvchain_t::get_xtxstore() //must be valid
        {
            xobject_t* target = m_plugins[enum_xvchain_plugin_txs_store];
            xassert(target != NULL);
            return (xvtxstore_t*)target;
        }
    
        xvblockstore_t* xvchain_t::get_xblockstore() //must be valid
        {
            xobject_t* target = m_plugins[enum_xvchain_plugin_block_store];
            xassert(target != NULL);
            return (xvblockstore_t*)target;
        }
    
        // xvstatestore_t*    xvchain_t::get_xstatestore()//global shared statestore instance
        // {
        //     xobject_t* target = m_plugins[enum_xvchain_plugin_state_store];
        //     xassert(target != NULL);
        //     return (xvstatestore_t*)target;
        // }
    
        xvcontractstore_t*  xvchain_t::get_xcontractstore()//global shared statestore instance
        {
            xobject_t* target = m_plugins[enum_xvchain_plugin_contract_store];
            xassert(target != NULL);
            return (xvcontractstore_t*)target;
        }
    
        xveventbus_t*    xvchain_t::get_xevmbus() //global mbus object
        {
            xobject_t* target = m_plugins[enum_xvchain_plugin_event_mbus];
            xassert(target != NULL);
            return (xveventbus_t*)target;
        }
    
        xvdrecycle_mgr* xvchain_t::get_xrecyclemgr() //global recycler manager
        {
            xvdrecycle_mgr * mgr_obj = (xvdrecycle_mgr*)m_plugins[enum_xvchain_plugin_recycle_mgr];
            return mgr_obj;
        }
     
        xvdrecycle_t*  xvchain_t::get_xrecycler(enum_vdata_recycle_type type)
        {
            xvdrecycle_mgr * mgr_obj = (xvdrecycle_mgr*)m_plugins[enum_xvchain_plugin_recycle_mgr];
            return mgr_obj->get_recycler(type);
        }
    
        bool    xvchain_t::set_xdbstore(xvdbstore_t * new_store)
        {
            xassert(new_store != NULL);
            if(NULL == new_store)
                return false;
            return register_plugin(new_store,enum_xvchain_plugin_kdb_store);
        }
        
        bool    xvchain_t::set_xtxstore(xvtxstore_t * new_store)
        {
            xassert(new_store != NULL);
            if(NULL == new_store)
                return false;
            
            return register_plugin(new_store,enum_xvchain_plugin_txs_store);
        }
    
        bool    xvchain_t::set_xblockstore(xvblockstore_t * new_store)
        {
            xassert(new_store != NULL);
            if(NULL == new_store)
                return false;
            
            return register_plugin(new_store,enum_xvchain_plugin_block_store);
        }
    
        // bool    xvchain_t::set_xstatestore(xvstatestore_t* new_store)
        // {
        //     xassert(new_store != NULL);
        //     if(NULL == new_store)
        //         return false;
            
        //     return register_plugin(new_store,enum_xvchain_plugin_state_store);
        // }
    
        bool    xvchain_t::set_xcontractstore(xvcontractstore_t * new_store)
        {
            xassert(new_store != NULL);
            if(NULL == new_store)
                return false;
            
            return register_plugin(new_store,enum_xvchain_plugin_contract_store);
        }
    
        bool    xvchain_t::set_xevmbus(xveventbus_t * new_mbus)
        {
            xassert(new_mbus != NULL);
            if(NULL == new_mbus)
                return false;
            
            return register_plugin(new_mbus,enum_xvchain_plugin_event_mbus);
        }
    
        bool  xvchain_t::set_xrecyclemgr(xvdrecycle_mgr* new_mgr)
        {
            xassert(new_mgr != NULL);
            if(NULL == new_mgr)
                return false;
            
            xkinfo("xvchain_t::set_xrecyclemgr");
            return register_plugin(new_mgr,enum_xvchain_plugin_recycle_mgr);
        }
        
        bool  xvchain_t::set_data_dir_path(const std::string & dir_path)
        {
            if(m_data_dir_path.empty() == false)//not allow overwrited
                return false;
            
            m_data_dir_path = dir_path;
            return true;
        }
    
        void  xvchain_t::enable_auto_prune(bool enable)
        {
            if(enable)
                xkinfo("xvchain_t::enable_auto_prune,Enabled");
            else
                xkinfo("xvchain_t::enable_auto_prune,Disabled");
            
            if(enable)
                m_is_auto_prune = 1;
            else
                m_is_auto_prune = 0;
        }
        void xvchain_t::set_node_type(bool is_storage, bool has_other_node)
        {
            if (m_node_init && m_is_storage_node == is_storage && m_has_other_node == has_other_node) {
                return;
            }
            xkinfo("xvchain_t::set_node_type,is_storage=%d->%d,is_consensus=%d->%d",m_is_storage_node.load(),is_storage,m_has_other_node.load(),has_other_node);
            if (m_is_storage_node != is_storage) {
                m_is_storage_node = is_storage;
            }
            if (m_has_other_node != has_other_node) {
                m_has_other_node = has_other_node;
            }
            if (m_is_storage_node) {
                m_store_units = true;
                m_store_unitstates = false;
            } else {
                m_store_units = false;
                m_store_unitstates = true;
            }
            m_node_init = true;
        }

        bool xvchain_t::need_store_unitstate(int zone_index) const
        {
            // beacon,zec,relay always store history state, otherwise 
            if (zone_index == base::enum_chain_zone_beacon_index
                || zone_index == base::enum_chain_zone_zec_index
                || zone_index == base::enum_chain_zone_relay_index) {
                return false;
            }
            return m_store_unitstates.load();
        }
        bool xvchain_t::need_store_units(int zone_index) const
        {
            // beacon,zec,relay always store history state, otherwise 
            if (zone_index == base::enum_chain_zone_beacon_index
                || zone_index == base::enum_chain_zone_zec_index
                || zone_index == base::enum_chain_zone_relay_index) {
                return true;
            }
            return m_store_units.load();
        }
        void    xvchain_t::get_db_config_custom(std::vector<db::xdb_path_t> &extra_db_path, int &extra_db_kind)
        {
            int db_kind = top::db::xdb_kind_kvdb;
            std::vector<db::xdb_path_t> db_data_paths;
            std::string extra_config = get_data_dir_path();
            if(extra_config.empty()) {
                extra_config = ".extra_conf.json"; 
            } else {
                extra_config += "/.extra_conf.json"; 
            }

            std::ifstream keyfile(extra_config, std::ios::in);
            xinfo("xvchain_t:get_db_path  extra_config %s", extra_config.c_str());
            if (keyfile) {
                Json::Value key_info_js;
                std::stringstream buffer;
                buffer << keyfile.rdbuf();
                keyfile.close();
                std::string key_info = buffer.str();
                Json::Reader reader;
                reader.parse(key_info, key_info_js);

                //get db kind
                std::string db_compress = key_info_js["db_compress"].asString();
                if (db_compress == "high_compress") {
                    db_kind |= top::db::xdb_kind_high_compress;
                } else if (db_compress == "no_compress" ) {
                    db_kind |= top::db::xdb_kind_no_compress;
                } else if (db_compress == "bottom_compress" ) {
                    db_kind |= top::db::xdb_kind_bottom_compress;
                }
                //get db path
                if (key_info_js["db_path_num"] > 1) {   
                    int db_path_num  = key_info_js["db_path_num"].asInt();
                    for (int i = 0; i < db_path_num; i++) {
                        std::string key_db_path = "db_path_" + std::to_string(i+1);
                        std::string key_db_size = "db_path_size_" + std::to_string(i+1);
                        std::string db_path_result =  key_info_js[key_db_path].asString();
                        uint64_t db_size_result = key_info_js[key_db_size].asUInt64(); 
                        if (db_path_result.empty() || db_size_result < 1) {
                            db_path_num = 1;
                            xwarn("xvchain_t::read db %i path %s size %lld config failed!", i , db_path_result.c_str(), db_size_result);
                            break;
                        }
                        xinfo("xvchain_t::read db  %i path %s size %lld sucess!",i , db_path_result.c_str(), db_size_result);
                        db_data_paths.emplace_back(db_path_result, db_size_result);
                    }
                }
            }
            extra_db_path = db_data_paths;
            extra_db_kind = db_kind;
        }

    };//end of namespace of base
};//end of namespace of top
