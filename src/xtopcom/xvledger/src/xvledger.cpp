// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvledger.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        xvactplugin_t::xvactplugin_t(xvaccountobj_t & parent_obj,const uint64_t idle_timeout_ms,enum_xvaccount_plugin_type type)
        {
            m_last_access_time_ms = 0;
            m_idle_timeout_ms     = idle_timeout_ms;
            
            m_plugin_type = type;
            m_account_obj = &parent_obj;
            m_account_obj->add_ref();
            
            //XMETRICS_GAUGE(metrics::dataobject_xvactplugin_t, 1);
            xinfo("xvactplugin_t::xvactplugin_t,acccount(%s)-type(%d)",m_account_obj->get_address().c_str(),get_plugin_type());
        }
    
        xvactplugin_t::~xvactplugin_t()
        {
            m_account_obj->release_ref();
            
            //XMETRICS_GAUGE(metrics::dataobject_xvactplugin_t, -1);
            xinfo("xvactplugin_t::destroy,acccount(%s)-type(%d)",m_account_obj->get_address().c_str(),get_plugin_type());
        }
    
        bool   xvactplugin_t::close(bool force_async)
        {
            xinfo("xvactplugin_t::close,acccount(%s)-type(%d)",m_account_obj->get_address().c_str(),get_plugin_type());
            return xobject_t::close(force_async);
        }
    
        const std::string &  xvactplugin_t::get_account_address()  const
        {
            return m_account_obj->get_address();
        }
    
        void  xvactplugin_t::set_last_access_time(const uint64_t last_access_time)
        {
            if(m_last_access_time_ms < last_access_time)
                m_last_access_time_ms = last_access_time;
        }
        
        bool xvactplugin_t::is_live(const uint64_t timenow_ms)
        {
            if(is_close())
                return false;
            
            const uint64_t last_time_ms = m_last_access_time_ms;
            if( timenow_ms > (last_time_ms + m_idle_timeout_ms) )
                return false;
            
            return true;
        }
    
        xvaccountobj_t::xvaccountobj_t(xvtable_t & parent_object,const std::string & account_address)
             : xiobject_t(*parent_object.get_context(),parent_object.get_thread_id(),  enum_xobject_type_vaccount),
              xvaccount_t(account_address),
              m_ref_table(parent_object)//note:table object never be release/deleted,so here just hold reference
        {
            m_meta_ptr = NULL;
            m_last_saved_meta_hash = 0;
            m_is_idle = 1;  //init as idled status
            m_is_keep_forever = 0;
            memset(m_plugins,0,sizeof(m_plugins));
            m_idle_start_time_ms  = get_time_now();
            m_idle_timeout_ms     = enum_account_idle_timeout_ms;
            
            if(is_contract_address()) //keep contract account forever at memory
                m_is_keep_forever  = 1;
            
            xinfo("xvaccountobj_t::xvaccountobj_t,acccount(%s)-xvid(%llu)",get_address().c_str(),get_xvid());
            XMETRICS_GAUGE(metrics::dataobject_xvaccountobj, 1);
        }
    
        xvaccountobj_t::~xvaccountobj_t()
        {
            xinfo("xvaccountobj_t::destroy,acccount(%s)-xvid(%llu)",get_address().c_str(),get_xvid());
            for(int i = 0; i < enum_xvaccount_plugin_max; ++i)
            {
                xvactplugin_t* old_ptr = m_plugins[i];
                if(old_ptr != NULL)//catch exception case if have
                {
                    old_ptr->close();
                    old_ptr->release_ref();
                }
            }
            
            {
                std::lock_guard<std::recursive_mutex> locker(get_table_lock());//using book lock
                if(m_meta_ptr != NULL)
                {
                    if(m_meta_ptr->is_close() == false)
                    {
                        save_meta();
                        m_meta_ptr->close();
                    }
                    m_meta_ptr->release_ref();
                }
            }
            
            XMETRICS_GAUGE(metrics::dataobject_xvaccountobj, -1);
        }
        
        const uint64_t  xvaccountobj_t::get_idle_duration() const
        {
            return m_idle_timeout_ms;
        }
    
        //status-change  :  live <-> idle -> closed
        bool xvaccountobj_t::is_live(const uint64_t timenow_ms)
        {
            if(m_is_keep_forever != 0) //table/book keep forever
                return true;
            
            if(is_idle() == false)//still active
                return true;
            
            if(is_close()) //be closed
                return false;
            
            const uint64_t idle_start_time_ms = m_idle_start_time_ms;
            if( timenow_ms > (idle_start_time_ms + m_idle_timeout_ms) )
                return false;//time to close it since idle too long
    
            return true;//still watching
        }
    
        bool xvaccountobj_t::close(bool force_async)
        {
            xinfo("xvaccountobj_t::close,acccount(%s)",get_address().c_str());
            if(is_close() == false)
            {
                xobject_t::close(false); //force close
                
                for(int i = 0; i < enum_xvaccount_plugin_max; ++i)
                {
                    xvactplugin_t* old_ptr = m_plugins[i];
                    if(old_ptr != NULL)//catch exception case if have
                    {
                        old_ptr->close(false);
                    }
                }
                
                {
                    std::lock_guard<std::recursive_mutex> locker(get_table_lock());//using book lock
                    if(m_meta_ptr != NULL)
                    {
                        if(m_meta_ptr->is_close() == false)
                        {
                            save_meta();
                            m_meta_ptr->close(); //mark as closed status
                        }
                    }
                }
            }
            return true;
        }
    
        std::recursive_mutex&   xvaccountobj_t::get_table_lock()
        {
            return m_ref_table.get_lock();
        }
        
        std::recursive_mutex&   xvaccountobj_t::get_book_lock()
        {
            return m_ref_table.get_book().get_lock();
        }
             
        bool   xvaccountobj_t::set_block_meta(const xblockmeta_t & new_meta)
        {
             std::lock_guard<std::recursive_mutex> locker(get_table_lock());
             xvactmeta_t * meta_ptr = get_meta();
             if(meta_ptr->set_block_meta(new_meta))
             {
                 if(meta_ptr->get_modified_count() > enum_account_save_meta_interval)
                     save_meta();
                 return true;
             }
             return false;
        }
    
        bool   xvaccountobj_t::set_state_meta(const xstatemeta_t & new_meta)
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            xvactmeta_t * meta_ptr = get_meta();
            if(meta_ptr->set_state_meta(new_meta))
            {
                if(meta_ptr->get_modified_count() > enum_account_save_meta_interval)
                    save_meta();
                return true;
            }
            return false;
        }
    
        bool   xvaccountobj_t::set_latest_executed_block(const uint64_t height, const std::string & blockhash)
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());//using book lock
            xvactmeta_t * meta_ptr = get_meta();
            if(meta_ptr->set_latest_executed_block(height,blockhash))
            {
                if(meta_ptr->get_modified_count() > enum_account_save_meta_interval)
                    save_meta();
                
                return true;
            }
            return false;
        }
    
        bool   xvaccountobj_t::set_sync_meta(const xsyncmeta_t & new_meta)
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            xvactmeta_t * meta_ptr = get_meta();
            if(meta_ptr->set_sync_meta(new_meta))
            {
                if(meta_ptr->get_modified_count() > enum_account_save_meta_interval)
                    save_meta();
                
                return true;
            }
            return false;
        }
    
        bool  xvaccountobj_t::set_index_meta(const xindxmeta_t & new_meta)
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            xvactmeta_t * meta_ptr = get_meta();
            if(meta_ptr->set_index_meta(new_meta))
            {
                if(meta_ptr->get_modified_count() > enum_account_save_meta_interval)
                    save_meta();
                
                return true;
            }
            return false;
        }
        
        const xblockmeta_t  xvaccountobj_t::get_block_meta()
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            return get_meta()->get_block_meta();
        }
    
        const xstatemeta_t  xvaccountobj_t::get_state_meta()
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            return get_meta()->get_state_meta();
        }
    
        const xindxmeta_t   xvaccountobj_t::get_index_meta()
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            return get_meta()->get_index_meta();
        }
    
        const xsyncmeta_t   xvaccountobj_t::get_sync_meta()
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());
            return get_meta()->get_sync_meta();
        }
        
        xvactmeta_t*  xvaccountobj_t::get_meta()
        {
            if( (m_meta_ptr != NULL) && (m_meta_ptr->is_close() == false) )
                return m_meta_ptr;
                   
            XMETRICS_GAUGE(metrics::store_block_meta_read, 1);
            const std::string full_meta_path = xvactmeta_t::get_meta_path(*this);
            const std::string meta_content = xvchain_t::instance().get_xdbstore()->get_value(full_meta_path);
   
            xvactmeta_t* new_meta_ptr = xvactmeta_t::load(*this,meta_content);
            xvactmeta_t * old_ptr = base::xatomic_t::xexchange(m_meta_ptr, new_meta_ptr);
            if(old_ptr != NULL)
            {
                old_ptr->close(); //close first
                xinfo("xvaccountobj_t::meta->get_meta,new_meta(%s) vs old_meta(%s)",new_meta_ptr->dump().c_str(),old_ptr->dump().c_str());
                old_ptr->release_ref();//then release
            }
            else
            {
                xinfo("xvaccountobj_t::meta->get_meta,new_meta(%s)",new_meta_ptr->dump().c_str());
            }
            m_last_saved_meta_hash = xhash64_t::digest(meta_content);
            return m_meta_ptr;
        }
        
        bool  xvaccountobj_t::save_meta()
        {
            std::string vmeta_bin;
            uint64_t    new_meta_hash = 0;
            uint32_t    last_modified_count = 0;
            {
                if(m_meta_ptr != NULL)
                {
                    std::lock_guard<std::recursive_mutex> locker(get_table_lock());
                    if(m_meta_ptr != NULL)
                    {
                        last_modified_count = m_meta_ptr->get_modified_count();
                        if(last_modified_count > 0)
                        {
                            m_meta_ptr->serialize_to_string(vmeta_bin);
                            new_meta_hash = xhash64_t::digest(vmeta_bin);
                            if(new_meta_hash == m_last_saved_meta_hash)//if nothing changed
                                return true;
                            
                            //optimism handle first
                            m_last_saved_meta_hash = new_meta_hash;
                            m_meta_ptr->reset_modified_count();
                        }
                    }
                }
            }
            
            if(vmeta_bin.empty() == false)
            {
                XMETRICS_GAUGE(metrics::store_block_meta_write, 1);
                const std::string full_meta_path = xvactmeta_t::get_meta_path(*this);
                if(xvchain_t::instance().get_xdbstore()->set_value(full_meta_path,vmeta_bin))
                {
                    xinfo("xvaccountobj_t::meta->save_meta,meta(%s)",m_meta_ptr->dump().c_str());
                    return true;
                }
                else //failure handle
                {
                    std::lock_guard<std::recursive_mutex> locker(get_table_lock());
                    m_last_saved_meta_hash = 0;
                    if(m_meta_ptr != NULL)
                        m_meta_ptr->add_modified_count();

                    xerror("xvaccountobj_t::meta->save_meta,fail to write db for account(%s)",get_address().c_str());
                    return false;
                }
            }
            return true;
        }
    
        xauto_ptr<xvactplugin_t> xvaccountobj_t::get_plugin(enum_xvaccount_plugin_type plugin_type)
        {
            std::lock_guard<std::recursive_mutex> locker(get_table_lock());//using table lock
            xvactplugin_t * plugin_ptr = get_plugin_unsafe(plugin_type);
            if(plugin_ptr != NULL)
                plugin_ptr->add_ref();//note:add reference for xauto_ptr
            
            return plugin_ptr;
        }

        //locked by table or account self in advance
        xvactplugin_t*    xvaccountobj_t::get_plugin_unsafe(enum_xvaccount_plugin_type plugin_type)
        {
            if(is_close()) //object not avaiable
            {
                xerror("xvaccountobj_t::get_plugin,closed account(%s)",get_address().c_str());
                return nullptr;
            }
            if((int)plugin_type >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::get_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d)",plugin_type,enum_xvaccount_plugin_max);
                return nullptr;
            }
                
            xvactplugin_t* plugin_ptr = m_plugins[plugin_type];
            if(plugin_ptr != nullptr)
            {
                if(plugin_ptr->is_close() == false)
                {
                    const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
                    plugin_ptr->set_last_access_time(_timenow);
                    return plugin_ptr;
                }
                xwarn("xvaccountobj_t::get_plugin,closed plugin_type(%d) for account(%s) ",plugin_type,enum_xvaccount_plugin_max,get_address().c_str());
            }
            return nullptr;//converted to xauto_ptr automatically
        }
    
        bool  xvaccountobj_t::reset_plugin_unsafe(enum_xvaccount_plugin_type plugin_type)
        {
            if((int)plugin_type >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::reset_plugin_unsafe,bad plugin_type(%d) >= enum_max_plugins_count(%d)",plugin_type,enum_xvaccount_plugin_max);
                return false;
            }
 
            xvactplugin_t* old_ptr = xatomic_t::xexchange(m_plugins[plugin_type],(xvactplugin_t*)NULL);
            if(NULL != old_ptr)//successful
            {
                old_ptr->close(false);//force closed existing one
                old_ptr->release_ref();//quickly releasedd it
            }
            
            for(int i = enum_xvaccount_plugin_start; i <= enum_xvaccount_plugin_end; ++i)
            {
                xobject_t* item = m_plugins[i];
                if( (item != nullptr) && (item->is_close() == false) )
                    return true; //indicate setup successful
            }
            
            if(m_is_idle != 1)
            {
                m_is_idle = 1; //enter idling status
                m_idle_start_time_ms = get_time_now();
            }
            xinfo("xvaccountobj_t::reset_plugin_unsafe,enter idle status for account(%s)",get_address().c_str());
            return true;
        }
        
        bool  xvaccountobj_t::reset_plugin_unsafe(xvactplugin_t * target_plugin_obj)
        {
            if(NULL == target_plugin_obj)
                return false;
            
            if((int)target_plugin_obj->get_plugin_type() >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::reset_plugin_unsafe,bad plugin_type(%d) >= enum_max_plugins_count(%d)",target_plugin_obj->get_plugin_type(),enum_xvaccount_plugin_max);
                return false;
            }

            if(m_plugins[target_plugin_obj->get_plugin_type()] != target_plugin_obj)
            {
                xwarn("xvaccountobj_t::reset_plugin_unsafe,plugin has been renewed for type(%d) of account(%s)",target_plugin_obj->get_plugin_type(),get_address().c_str());
                return false;
            }
            
            xvactplugin_t* old_ptr = xatomic_t::xexchange(m_plugins[target_plugin_obj->get_plugin_type()],(xvactplugin_t*)NULL);
            if(NULL != old_ptr)//successful
            {
                old_ptr->close();//force closed existing one
                old_ptr->release_ref();//quickly releasedd it
            }
            
            for(int i = enum_xvaccount_plugin_start; i <= enum_xvaccount_plugin_end; ++i)
            {
                xobject_t* item = m_plugins[i];
                if( (item != nullptr) && (item->is_close() == false) )
                    return true; //indicate setup successful
            }
            
            if(m_is_idle != 1)
            {
                m_is_idle = 1; //enter idling status
                m_idle_start_time_ms = get_time_now();
            }
            xinfo("xvaccountobj_t::reset_plugin_unsafe,enter idle status for account(%s)",get_address().c_str());
            return true;
        }
    
        //locked by table or account self in advance
        bool  xvaccountobj_t::set_plugin_unsafe(xvactplugin_t * new_plugin_obj)
        {
            if(nullptr == new_plugin_obj) //valid check
            {
                xassert(0);
                return false;
            }
            if(is_close()) //object not avaiable
            {
                xerror("xvaccountobj_t::set_plugin,closed account(%s)",get_address().c_str());
                return false;
            }
            if(new_plugin_obj->is_close())
            {
                xerror("xvaccountobj_t::set_plugin,a closed plugin(%s)",new_plugin_obj->dump().c_str());
                return false;
            }
            if(new_plugin_obj->get_plugin_type() >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::set_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d)",new_plugin_obj->get_plugin_type(),enum_xvaccount_plugin_max);
                return false;
            }
            
            xvactplugin_t* existing_plugin_ptr = xatomic_t::xload(m_plugins[new_plugin_obj->get_plugin_type()]);
            if(existing_plugin_ptr == new_plugin_obj) //same one
                return true;
            
            //xassert(nullptr == existing_plugin_ptr);
            if(existing_plugin_ptr != nullptr)
            {
                if(existing_plugin_ptr->is_close() == false)
                    return false;//not allow to replace by different ptr
            }
            
            //replace existing one by new ptr
            if(NULL != new_plugin_obj)
            {
                new_plugin_obj->add_ref();
                const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
                new_plugin_obj->set_last_access_time(_timenow);
            }
            
            xvactplugin_t* old_ptr = xatomic_t::xexchange(m_plugins[new_plugin_obj->get_plugin_type()],new_plugin_obj);
            if(NULL != old_ptr)//successful
            {
                old_ptr->close(false);//force closed existing one
                old_ptr->release_ref();//quickly releasedd it
            }
            const uint8_t old_status = m_is_idle;
            m_is_idle = 0; //actived
            if(old_status != 0)
                xinfo("xvaccountobj_t::set_plugin_unsafe,reenter active status for account(%s)",get_address().c_str());
            return true;
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
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            if(force_clean)
            {
                for(auto it : m_accounts)
                {
                    it.second->close(); //force close first
                    it.second->release_ref();
                }
                m_accounts.clear();
            }
            else//just clean closed ones
            {
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
        
    #ifdef DEBUG //debug only purpose
        int32_t   xvtable_t::add_ref()
        {
            return xionode_t::add_ref();
        }
        int32_t   xvtable_t::release_ref()
        {
            return xionode_t::release_ref();
        }
    #endif
    
        void    xvtable_t::monitor_plugin(xvactplugin_t * plugin_obj)
        {
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
    
        xvaccountobj_t*     xvtable_t::create_account_unsafe(const std::string & account_address)
        {
            xvaccountobj_t * new_account_obj = new xvaccountobj_t(*this,account_address);
            //push to monitor queue
            monitor_account(new_account_obj);
            return new_account_obj;
        }
    
        xauto_ptr<xvaccountobj_t>   xvtable_t::get_account(const std::string & account_address)
        {
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            xvaccountobj_t * account_ptr = get_account_unsafe(account_address);
            account_ptr->add_ref(); //add reference to pair xauto_ptr
            return account_ptr;
        }
        
        xvaccountobj_t*   xvtable_t::get_account_unsafe(const std::string & account_address)
        {
            auto & exist_account_ptr = m_accounts[account_address];
            if( (exist_account_ptr != NULL) && (exist_account_ptr->is_close() == false) ) //valid account object
                return exist_account_ptr;

            xvaccountobj_t * old_account_obj = exist_account_ptr; //backup first
            xvaccountobj_t * new_account_obj = create_account_unsafe(account_address);
            exist_account_ptr = new_account_obj;//overwrite the referenced exist_account_ptr
            if(old_account_obj != NULL)//it must has been closed
            {
                old_account_obj->close(false);//force makr as closed
                old_account_obj->release_ref();//release it
            }
            return new_account_obj;
        }
    
        xauto_ptr<xvactplugin_t>  xvtable_t::get_account_plugin(const std::string & account_address,enum_xvaccount_plugin_type plugin_type)
        {
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            xvactplugin_t * plugin_ptr = get_account_plugin_unsafe(account_address,plugin_type);
            if(plugin_ptr != NULL)
                plugin_ptr->add_ref();//note:add reference for xauto_ptr
            
            return plugin_ptr;
        }
    
        xvactplugin_t*    xvtable_t::get_account_plugin_unsafe(const std::string & account_address,enum_xvaccount_plugin_type plugin_type)
        {
            return get_account_unsafe(account_address)->get_plugin_unsafe(plugin_type);
        }
    
        bool  xvtable_t::set_account_plugin(xvactplugin_t * plugin_obj)
        {
            if(nullptr == plugin_obj)
            {
                xassert(plugin_obj != nullptr);
                return false;
            }
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            if(plugin_obj->get_account_obj()->set_plugin_unsafe(plugin_obj))
            {
                monitor_plugin(plugin_obj);
                return true;
            }
            return false;
        }
    
        bool xvtable_t::reset_account_plugin(xvactplugin_t * target_plugin) //clean
        {
            if(NULL == target_plugin)
                return false;
            
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            bool res = target_plugin->get_account_obj()->reset_plugin_unsafe(target_plugin);
            {
                if(target_plugin->get_account_obj()->is_close())
                {
                    monitor_account(target_plugin->get_account_obj());//post to monitor list
                }
            }
            return res;
        }
    
        bool xvtable_t::reset_account_plugin(const std::string & account_address,enum_xvaccount_plugin_type plugin_type) //clean
        {
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            
            auto it = m_accounts.find(account_address);
            if(it != m_accounts.end())//found exsiting one
            {
                bool res = it->second->reset_plugin_unsafe(plugin_type);
                if(it->second->is_close()) //check again after reset
                {
                    monitor_account(it->second);//post to monitor list
                }
                return res;
            }
            return false;
        }
    
        bool xvtable_t::close_account(const std::string & account_address)
        {
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            return close_account_unsafe(account_address);
        }
    
        bool xvtable_t::close_account_unsafe(const std::string & account_address)
        {
            auto it = m_accounts.find(account_address);
            if(it != m_accounts.end())
            {
                if(it->second->is_close())
                {
                    it->second->release_ref();
                    m_accounts.erase(it);
                    return true;
                }
                return false; //still be using
            }
            return true;
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
                    if(_test_for_account->is_live(current_time_ms) == false)//idle too long
                    {
                        //remove most less-active account while too much caches
                        std::lock_guard<std::recursive_mutex> _dummy(get_lock());
                        if(_test_for_account->is_live(current_time_ms) == false)//check again
                        {
                            _test_for_account->close(); //mark closed status first
                            close_account_unsafe(_test_for_account->get_address()); //then remove from table
                            _test_for_account->release_ref();//release reference it finally
                        }
                        else
                        {
                            _remonitor_list.push_back(_test_for_account);//transfer to list
                        }
                    }
                    else
                    {
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
                    if(_test_for_plugin->is_live(current_time_ms) == false)//idle too long
                    {
                        //remove most less-active account while too much caches
                        std::lock_guard<std::recursive_mutex> _dummy(get_lock());
                        if(_test_for_plugin->is_live(current_time_ms) == false)//check again
                        {
                            _test_for_plugin->close(); //mark closed first
                            _test_for_plugin->get_account_obj()->reset_plugin_unsafe(_test_for_plugin);
                            _test_for_plugin->release_ref();//release reference it
                        }
                        else
                        {
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
            on_timer_for_plugins(thread_id, timer_id, current_time_ms, start_timeout_ms);
            on_timer_for_accounts(thread_id, timer_id, current_time_ms, start_timeout_ms);
            return true;
        }
    
        bool  xvtable_t::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms)
        {
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            //cleanup monitoring plugins
            {
                for(auto it  = m_monitor_plugins.begin(); it != m_monitor_plugins.end(); ++it)
                {
                    if(it->second != nullptr)
                    {
                        it->second->close();
                        it->second->release_ref();
                    }
                }
                m_monitor_plugins.clear();
            }

            //cleanup monitoring accounts
            {
                for(auto it  = m_monitor_accounts.begin(); it != m_monitor_accounts.end(); ++it)
                {
                    if(it->second != nullptr)
                    {
                        it->second->close();
                        it->second->release_ref();
                    }
                }
                m_monitor_accounts.clear();
            }
            
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
            m_monitor_timer->start(enum_plugin_idle_check_interval, enum_plugin_idle_check_interval); //check account by every 10 seconds
            
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
            m_chain_id = chain_id;
            memset(m_ledgers,0,sizeof(m_ledgers));
            
            //build default stores
            xauto_ptr<xvstatestore_t> default_state_store(new xvstatestore_t());
            set_xstatestore(default_state_store());
            
            xauto_ptr<xvtxstore_t> default_txs_store(new xvtxstore_t());
            set_xtxstore(default_txs_store());
            
            xauto_ptr<xvcontractstore_t> default_contract_store(new xvcontractstore_t());
            set_xcontractstore(default_contract_store());
            
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
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            for (int i = 0; i < enum_vchain_has_buckets_count; i++)
            {
                xvledger_t* exist_ptr = m_ledgers[i];
                if(exist_ptr != NULL)
                {
                    exist_ptr->clean_all(force_clean);//never destoryed,just do clean
                }
            }
            return true;
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
            xassert(target != NULL);
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
    
        xvstatestore_t*    xvchain_t::get_xstatestore()//global shared statestore instance
        {
            xobject_t* target = m_plugins[enum_xvchain_plugin_state_store];
            xassert(target != NULL);
            return (xvstatestore_t*)target;
        }
    
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
    
        bool    xvchain_t::set_xstatestore(xvstatestore_t* new_store)
        {
            xassert(new_store != NULL);
            if(NULL == new_store)
                return false;
            
            return register_plugin(new_store,enum_xvchain_plugin_state_store);
        }
    
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
    };//end of namespace of base
};//end of namespace of top
