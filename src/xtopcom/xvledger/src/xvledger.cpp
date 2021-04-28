// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "../xvledger.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"

namespace top
{
    namespace base
    {
        xvaccountobj_t::xvaccountobj_t(xvtable_t & parent_object,const std::string & account_address)
             :xobject_t(enum_xobject_type_vaccount),
              xvaccount_t(account_address),
              m_ref_table(parent_object)//note:table object never be release/deleted,so here just hold reference
        {
            memset(m_plugins,0,sizeof(m_plugins));
            xinfo("xvaccountobj_t::xvaccountobj_t,acccount(%s)-xvid(%llu)",get_address().c_str(),get_xvid());
        }
    
        xvaccountobj_t::~xvaccountobj_t()
        {
            xinfo("xvaccountobj_t::destroy,acccount(%s)-xvid(%llu)",get_address().c_str(),get_xvid());
            for(int i = 0; i < enum_xvaccount_plugin_max; ++i)
            {
                xobject_t* old_ptr = m_plugins[i];
// TODO(jimmy)               xassert(old_ptr == NULL); //MUST did clean before destruction
                if(old_ptr != NULL)//catch exception case if have
                    xcontext_t::instance().delay_release_object(old_ptr);
            }
        }
    
        bool xvaccountobj_t::close(bool force_async)
        {
            xinfo("xvaccountobj_t::close,acccount(%s)",get_address().c_str());
            if(is_close() == false)
            {
                xobject_t::close(true); //force at async mode
                
                //clean it fromt table first,since base::on_object_close may clean up parent node information
                // TODO(jimmy) m_ref_table.close_account(get_address());
            }
            return true;
        }
    
        std::recursive_mutex&  xvaccountobj_t::get_lock()
        {
            return m_ref_table.get_lock();
        }
    
        xobject_t*    xvaccountobj_t::get_plugin(enum_xvaccount_plugin_type plugin_type)
        {
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            return get_plugin_unsafe(plugin_type);
        }
    
        //locked by table or account self in advance
        xobject_t*    xvaccountobj_t::get_plugin_unsafe(enum_xvaccount_plugin_type plugin_type)
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
                
            xobject_t* plugin_ptr = m_plugins[plugin_type];
            if(plugin_ptr != nullptr)
            {
                if(plugin_ptr->is_close() == false)
                {
                    plugin_ptr->add_ref();//note:add reference before return, caller need release it
                    return plugin_ptr;
                }
                xatomic_t::xreset(m_plugins[plugin_type]);
                plugin_ptr->release_ref();//release reference of the closed plugin
            }
            return nullptr;//converted to xauto_ptr automatically
        }
    
        //return to indicate setup successful or not
        bool  xvaccountobj_t::set_plugin(xobject_t * plugin_obj,enum_xvaccount_plugin_type plugin_type)
        {
            if(NULL == plugin_obj)
                return false;
            
            std::lock_guard<std::recursive_mutex> locker(get_lock());
            return set_plugin_unsafe(plugin_obj,plugin_type);
        }
    
        //locked by table or account self in advance
        bool  xvaccountobj_t::set_plugin_unsafe(xobject_t * new_plugin_obj,enum_xvaccount_plugin_type plugin_type)
        {
            if(NULL == new_plugin_obj)
                return false;
            
            if(is_close()) //object not avaiable
            {
                if(new_plugin_obj != nullptr)
                    xerror("xvaccountobj_t::set_plugin,closed account(%s)",get_address().c_str());
                return false;
            }
            if(plugin_type >= enum_xvaccount_plugin_max)
            {
                xerror("xvaccountobj_t::set_plugin,bad plugin_type(%d) >= enum_max_plugins_count(%d)",plugin_type,enum_xvaccount_plugin_max);
                return false;
            }
            
            if(new_plugin_obj != nullptr) //setup
            {
                xobject_t* existing_plugin_ptr = xatomic_t::xload(m_plugins[plugin_type]);
                if(existing_plugin_ptr == new_plugin_obj) //same one
                    return true;
                
                //xassert(nullptr == existing_plugin_ptr);
                if(existing_plugin_ptr != nullptr)
                {
                    if(existing_plugin_ptr->is_close() == false)
                        return false;//not allow to replace by different ptr
                }
            }
            //replace existing one by new ptr
            if(NULL != new_plugin_obj)
                new_plugin_obj->add_ref();
            
            xobject_t* old_ptr = xatomic_t::xexchange(m_plugins[plugin_type],new_plugin_obj);
            if(NULL != old_ptr)//successful
                xcontext_t::instance().delay_release_object(old_ptr);
            
            if(nullptr == new_plugin_obj) //check whether it is ok to close account object
            {
                for(int i = enum_xvaccount_plugin_start; i <= enum_xvaccount_plugin_end; ++i)
                {
                    xobject_t* item = m_plugins[i];
                    if( (item != nullptr) && (item->is_close() == false) )
                        return true; //indicate setup successful
                }
                //close this account after every plugin are reset
                close(true);//force to close it
            }
            //indicate setup successful
            return true;
        }
    
        bool  xvaccountobj_t::handle_event(const xvevent_t & ev)
        {
            //add your code here
            return false;
        }
    
        //---------------------------------------------xvtable_t---------------------------------------------//
        xvtable_t::xvtable_t(xvbook_t & parent_object,const int32_t thread_id,const uint8_t table_index)
            :xionode_t(parent_object, thread_id,enum_xobject_type_vtable)
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
            std::lock_guard<std::recursive_mutex> locker(m_lock);
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
                for(auto it = m_accounts.begin(); it != m_accounts.end(); ++it)
                {
                    auto old = it; //just copy the old value
                    ++it;
                    if(old->second->is_close())
                    {
                        old->second->release_ref();
                        m_accounts.erase(it);
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
    
        xauto_ptr<xvaccountobj_t>   xvtable_t::get_account(const std::string & account_address)
        {
            return get_account_unsafe(account_address);
        }
        
        xvaccountobj_t*   xvtable_t::get_account_unsafe(const std::string & account_address)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            auto & exist_account_ptr = m_accounts[account_address];
            if( (exist_account_ptr != NULL) && (exist_account_ptr->is_close() == false) ) //valid account object
            {
                exist_account_ptr->add_ref();
                return exist_account_ptr;
            }
            
            xvaccountobj_t * old_account_obj = exist_account_ptr; //backup first
            xvaccountobj_t * new_account_obj = new xvaccountobj_t(*this,account_address);
            exist_account_ptr = new_account_obj;
            if(old_account_obj != NULL)//it must has been closed
                old_account_obj->release_ref();
            
            new_account_obj->add_ref(); //add reference for xauto_ptr
            return new_account_obj;
        }
    
        xauto_ptr<xobject_t>  xvtable_t::get_account_plugin(const std::string & account_address,enum_xvaccount_plugin_type plugin_type)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            auto & exist_account_ptr = m_accounts[account_address];
            if( (exist_account_ptr != NULL) && (exist_account_ptr->is_close() == false) ) //valid account object
                return exist_account_ptr->get_plugin_unsafe(plugin_type);
            
            xvaccountobj_t * old_account_obj = exist_account_ptr; //backup first
            xvaccountobj_t * new_account_obj = new xvaccountobj_t(*this,account_address);
            exist_account_ptr = new_account_obj;
            if(old_account_obj != NULL)
                old_account_obj->release_ref();
            
            return new_account_obj->get_plugin_unsafe(plugin_type);
        }
    
        //return raw ptr that has been add_ref,caller need manually release it
        xobject_t*    xvtable_t::get_account_plugin_unsafe(const std::string & account_address,enum_xvaccount_plugin_type plugin_type)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            auto & exist_account_ptr = m_accounts[account_address];
            if( (exist_account_ptr != NULL) && (exist_account_ptr->is_close() == false) ) //valid account object
                return exist_account_ptr->get_plugin_unsafe(plugin_type);
            
            xvaccountobj_t * old_account_obj = exist_account_ptr; //backup first
            xvaccountobj_t * new_account_obj = new xvaccountobj_t(*this,account_address);
            exist_account_ptr = new_account_obj;
            if(old_account_obj != NULL)
                old_account_obj->release_ref();
            
            return new_account_obj->get_plugin_unsafe(plugin_type);
        }
    
        bool  xvtable_t::set_account_plugin(const std::string & account_address,xobject_t * plugin_obj,enum_xvaccount_plugin_type plugin_type)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            if(nullptr != plugin_obj)
            {
                auto & exist_account_ptr = m_accounts[account_address];
                if( (exist_account_ptr != NULL) && (exist_account_ptr->is_close() == false) ) //valid account object
                    return exist_account_ptr->set_plugin_unsafe(plugin_obj,plugin_type);

                xvaccountobj_t * old_account_obj = exist_account_ptr; //backup first
                xvaccountobj_t * new_account_obj = new xvaccountobj_t(*this,account_address);
                exist_account_ptr = new_account_obj;
                if(old_account_obj != NULL)
                    old_account_obj->release_ref();
                
                return exist_account_ptr->set_plugin_unsafe(plugin_obj,plugin_type);
            }
            else //reset case
            {
                auto it = m_accounts.find(account_address);
                if(it != m_accounts.end())//found exsiting one
                {
                    if(it->second->is_close() == false) //still be valid account object
                        it->second->set_plugin_unsafe(plugin_obj,plugin_type);
                    
                    if(it->second->is_close()) //check again after reset
                    {
                        it->second->release_ref();
                        m_accounts.erase(it);
                    }
                }
                return true;
            }
        }
    
        bool xvtable_t::close_account(const std::string & account_address)
        {
            std::lock_guard<std::recursive_mutex> locker(m_lock);
            
            auto it = m_accounts.find(account_address);
            if(it != m_accounts.end())
            {
                if(it->second->is_close()) //protection: not allow close with any valid plugin
                {
                    it->second->release_ref();
                    m_accounts.erase(it);
                    return true;
                }
                return false; //close fail
            }
            return true;
        }
 
        //---------------------------------------------xvbook_t---------------------------------------------//
        xvbook_t::xvbook_t(xvledger_t & parent_object,const int32_t thread_id,const uint8_t book_index)
          :xionode_t(parent_object, thread_id,enum_xobject_type_vbook)
        {
            m_book_index = book_index;
            m_book_combine_addr = (parent_object.get_ledger_id() << 10) | set_vledger_subaddr(book_index,0);
            memset(m_tables,0,sizeof(m_tables));
            
            xkinfo("xvbook_t::xvbook_t,book_index(%d) with full_addr(%d)",m_book_index,m_book_combine_addr);
        }
        xvbook_t::~xvbook_t()
        {
            //never destory & never release,should not enter here
            xerror("xvbook_t::destory,book_index(%d) with full_addr(%d)",m_book_index,m_book_combine_addr);
            clean_all(true);
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
    
        //the returned ptr has done add_ref before return,so release it when nolonger need manually
        //it is multiple thread safe
        xvaccountobj_t*   xvledger_t::get_account_unsafe(const std::string & account_address)
        {
            return get_table(account_address)->get_account_unsafe(account_address);
        }
        xvaccountobj_t*   xvledger_t::get_account_unsafe(const xvaccount_t & account_obj)
        {
            return get_table(account_obj.get_xvid())->get_account_unsafe(account_obj.get_address());
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
        xvchain_t &  xvchain_t::instance(const uint16_t chain_id)
        {
            static xvchain_t * __global_vchain_instance = NULL;
            if(__global_vchain_instance != NULL)
                return *__global_vchain_instance;
            
            xiothread_t *_monitor_thread = xcontext_t::instance().find_thread(xiothread_t::enum_xthread_type_monitor, false);
            if(NULL == _monitor_thread)
                _monitor_thread = xiothread_t::create_thread(base::xcontext_t::instance(),base::xiothread_t::enum_xthread_type_monitor,-1);
            
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
            set_xstatestore(default_state_store.get());
            
            xauto_ptr<xvtxstore_t> default_txs_store(new xvtxstore_t());
            set_xtxstore(default_txs_store.get());
            
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
    
        //the returned ptr has done add_ref before return,so release it when nolonger need manually
        //it is multiple thread safe
        xvaccountobj_t*  xvchain_t::get_account_unsafe(const std::string & account_address)
        {
            return get_table(account_address)->get_account_unsafe(account_address);
        }
        xvaccountobj_t*  xvchain_t::get_account_unsafe(const xvaccount_t & account_obj)
        {
            return get_table(account_obj.get_xvid())->get_account_unsafe(account_obj.get_address());
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
    
        bool    xvchain_t::set_xevmbus(xveventbus_t * new_mbus)
        {
            xassert(new_mbus != NULL);
            if(NULL == new_mbus)
                return false;
            
            return register_plugin(new_mbus,enum_xvchain_plugin_event_mbus);
        }
    };//end of namespace of base
};//end of namespace of top
