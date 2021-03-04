// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xvblockhub.h"
#include "xvtcaccount.h"
#include "xvunithub.h"

// #ifndef __MAC_PLATFORM__
    #include "xdata/xnative_contract_address.h"
// #endif

namespace top
{
    namespace store
    {
        bool  xvblockstore_impl::init_store(base::xcontext_t &_context)//do initialize for all store objects
        {
            xblockacct_t::init_account(_context);//let account has chance to initialize
            return true;
        }

        xvblockstore_impl::xvblockstore_impl(const std::string & blockstore_path,xstore_face_t & _persist_db,base::xcontext_t & _context,const int32_t target_thread_id)
            :base::xiobject_t(_context,target_thread_id,base::enum_xobject_type_vbstore)
        {
            m_persist_db = nullptr;
            m_raw_timer  = nullptr;

            _persist_db.add_ref();
            m_persist_db = &_persist_db;

            m_store_path = blockstore_path;
            xkinfo("xvblockstore_impl::create,store=%s,at thread=%d",m_store_path.c_str(),target_thread_id);
            m_raw_timer = get_thread()->create_timer((base::xtimersink_t*)this);
            m_raw_timer->start(enum_account_idle_check_interval, enum_account_idle_check_interval); //check account by every 10 seconds
        }

        xvblockstore_impl::~xvblockstore_impl()
        {
            if(m_raw_timer != nullptr)
            {
                m_raw_timer->release_ref();
            }
            m_persist_db->release_ref();
            xkinfo("xvblockstore_impl::destroy,store=%s",m_store_path.c_str());
        }

        bool    xvblockstore_impl::close(bool force_async) //must call close before release object,otherwise object never be cleanup
        {
            base::xvblockstore_t::close(force_async);//mark close flag first
            base::xiobject_t::close(force_async); //since mutiple base class has close(),we need call seperately

            xkinfo("xvblockstore_impl::close,store=%s",m_store_path.c_str());
            return true;
        }

        //on_object_close be called when close command processed at host thread,logic flow: Caller(Thread#A)->Close()->Wake this object thread(B)->clean and then execute: on_object_close
        bool    xvblockstore_impl::on_object_close() //notify the subclass the object is closed
        {
            xkinfo("xvblockstore_impl::on_object_close,store=%s",m_store_path.c_str());

            for(int i = 0; i < enum_units_group_count; ++i)
            {
                m_group_locks[i].lock();

                std::map<std::string,xblockacct_t*> & units = m_group_units[i];
                for(auto it = units.begin(); it != units.end(); ++it)
                {
                    it->second->close();
                    it->second->release_ref();
                }
                units.clear();
                m_group_locks[i].unlock();
            }
            if(m_raw_timer != nullptr)
            {
                m_raw_timer->stop();
                m_raw_timer->close();
            }
            return base::xiobject_t::on_object_close();
        }

        uint32_t   xvblockstore_impl::cal_group_index_from_account(const std::string & account)
        {
            return base::xvaccount_t::get_ledgersubaddr_from_account(account) % enum_units_group_count;
            //return base::xvaccount_t::get_index_from_account(account)% enum_units_group_count;
        }

        uint32_t   xvblockstore_impl::cal_group_index_from_account(const base::xvaccount_t & account)
        {
            return account.get_ledger_subaddr() % enum_units_group_count;
            // return base::xvaccount_t::get_ledgersubaddr_from_account(account.get_address()) % enum_units_group_count;
            //return  account.get_account_index()% enum_units_group_count;
        }

        xblockacct_t*     xvblockstore_impl::get_block_account(const uint32_t group_index,const std::string & account)
        {
            bool _create_new_account_obj = false;
            std::map<std::string,xblockacct_t*>& units = m_group_units[group_index];
            xblockacct_t* & _target_account = units[account];
            if( (nullptr == _target_account) || (_target_account->is_close()) )
            {
                xblockacct_t * old_account_ptr = _target_account; //backup old one
                if (account == sys_contract_beacon_timer_addr)
                {
                    _target_account = new xtcaccount_t(account,enum_account_idle_timeout_ms,m_store_path,*m_persist_db,*this);//replace by new account address
                }
                else
                {
                    _target_account = new xchainacct_t(account,enum_account_idle_timeout_ms,m_store_path,*m_persist_db,*this);//replace by new account address
                }
                _target_account->init();

                if(old_account_ptr != nullptr) //after idle than 60 seconds
                    old_account_ptr->release_ref();

                _create_new_account_obj = true;
            }
            const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
            _target_account->set_last_access_time(_timenow);
            if(_create_new_account_obj) //now ready to deliver to monitor thread
            {
                std::function<void(void*)> _add_new_account_job = [this](void* _account)->void{
                    xblockacct_t * _new_account_obj = (xblockacct_t*)_account;
                    m_monitor_expire.insert(std::multimap<uint64_t,xblockacct_t*>::value_type(_new_account_obj->get_idle_duration() + get_time_now(),_new_account_obj));
                };
                _target_account->add_ref();//add reference to hold
                send_call(_add_new_account_job,(void*)_target_account);//send account ptr to store'thread to manage lifecycle
            }
            return _target_account;
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_genesis_block(const std::string & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);//always use same lock for same account
            return get_block_account(index,account)->get_genesis_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_cert_block(const std::string & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_cert_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_locked_block(const std::string & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_locked_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_committed_block(const std::string & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_committed_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_executed_block(const std::string & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_executed_block();
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_connected_block(const std::string & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_connected_block();
        }
        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_full_block(const std::string & account)//block has full state,genesis is a full block
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_full_block();
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_current_block(const std::string & account, bool ask_full_load)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->get_latest_current_block(ask_full_load);
        }

        //just load vblock object but not load header and body those need load seperately if need. create a new one if not found
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const std::string & account,const uint64_t height,bool ask_full_load)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index =  cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account)->load_block_object(height,ask_full_load);
        }

        bool  xvblockstore_impl::load_block_input(base::xvblock_t* block)     //load and assign input data into  xvblock_t
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if(nullptr == block)
                return false;

            const uint32_t index =  cal_group_index_from_account(block->get_account());
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,block->get_account())->load_block_input(block);
        }

        bool  xvblockstore_impl::load_block_output(base::xvblock_t* block)   //load and assign output data into xvblock_t
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if(nullptr == block)
                return false;

            const uint32_t index =  cal_group_index_from_account(block->get_account());
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,block->get_account())->load_block_output(block);
        }

        bool  xvblockstore_impl::store_block(base::xvblock_t* block)    //update old one or insert as new
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if(nullptr == block)
                return false;

            const uint32_t index =  cal_group_index_from_account(block->get_account());
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,block->get_account())->store_block(block);
        }

        bool  xvblockstore_impl::delete_block(base::xvblock_t* block)  //return error code indicate what is result
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if(nullptr == block)
                return false;

            const uint32_t index =  cal_group_index_from_account(block->get_account());
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,block->get_account())->delete_block(block);
        }
        /////////////////////////////////new api with better performance by passing base::xvaccount_t
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_genesis_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_genesis_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_cert_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_cert_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_locked_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_locked_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_committed_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_committed_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_executed_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_executed_block();
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_connected_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_connected_block();
        }
        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_full_block(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_full_block();
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_current_block(const base::xvaccount_t & account, bool ask_full_load)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_latest_current_block(ask_full_load);
        }

        //one api to get latest_commit/latest_lock/latest_cert for better performance
        base::xblock_mptrs  xvblockstore_impl::get_latest_blocks(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return base::xblock_mptrs();
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);

            base::xvblock_t*  cert_block    = nullptr;
            base::xvblock_t*  lock_block    = nullptr;
            base::xvblock_t*  commit_block  = nullptr;
            get_block_account(index,account.get_account())->get_latest_blocks_list(cert_block,lock_block,commit_block);
            return base::xblock_mptrs(cert_block,lock_block,commit_block);//move reference to xblock_mptrs
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,bool ask_full_load)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->load_block_object(height,ask_full_load);
        }
        bool                xvblockstore_impl::store_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->store_block(block);
        }

        bool                xvblockstore_impl::store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks)
        {
            if(batch_store_blocks.empty())
                return true;

            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }

            for(auto it : batch_store_blocks)
            {
                if( (it != nullptr) && (it->get_account() != account.get_account()) )
                {
                    xerror("xvblockstore_impl::store_blocks,block NOT match account:%",account.get_account().c_str());
                    return false;
                }
            }

            //note:here skip account check and leave xblockacct_t check whether has correct account address later
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->store_blocks(batch_store_blocks);
        }

        bool                xvblockstore_impl::delete_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->delete_block(block);
        }

        //execute block and update state of acccount,note: block must be committed and connected
        bool                xvblockstore_impl::execute_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::execute_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->execute_block(block);
        }

        bool                xvblockstore_impl::load_block_input(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->load_block_input(block);
        }
        bool                xvblockstore_impl::load_block_output(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->load_block_output(block);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height, const uint64_t viewid)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_block(height,viewid);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height, const std::string & blockhash)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return nullptr;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_block(height,blockhash);
        }

        base::xblock_vector xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height)//might mutiple certs at same height
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return base::xblock_vector();
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->get_blocks(height);
        }

        //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
        bool                xvblockstore_impl::clean_caches(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->clean_caches();
        }

        //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
        bool                xvblockstore_impl::reset_cache_timeout(const base::xvaccount_t & account,const uint32_t max_idle_time_ms)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            const uint32_t index = cal_group_index_from_account(account);
            std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[index]);
            return get_block_account(index,account.get_account())->reset_cache_timeout(max_idle_time_ms);
        }

        bool  xvblockstore_impl::on_timer_fire(const int32_t thread_id,const int64_t timer_id,const int64_t current_time_ms,const int32_t start_timeout_ms,int32_t & in_out_cur_interval_ms)
        {
            std::deque<xblockacct_t*> _remonitor_list;

            int   expired_items_count = 0;
            auto  expire_it = m_monitor_expire.begin();
            while(expire_it != m_monitor_expire.end())
            {
                const int total_active_acounts = (int)m_monitor_expire.size();
                if(total_active_acounts < enum_max_active_acconts)
                {
                    //map or multiplemap are sorted as < operation as default
                    if(current_time_ms < (int64_t)expire_it->first )
                        break;
                    else if(expired_items_count > enum_max_expire_check_count) //not clean too much at each loop
                        break;
                }
                
                xblockacct_t* _test_for_account = expire_it->second;
                if(_test_for_account != nullptr)
                {
                    if( (false == _test_for_account->is_live(current_time_ms)) || (total_active_acounts > enum_max_active_acconts) ) //force to remove most less-active account while too much caches
                    {
                        const uint32_t group_index = cal_group_index_from_account(*_test_for_account);
                        std::lock_guard<std::recursive_mutex> _dummy(m_group_locks[group_index]);//always use same lock for same account

                        if( (false == _test_for_account->is_live(current_time_ms)) || (total_active_acounts > enum_max_active_acconts) ) //force to remove most less-active account while too much caches
                        {
                            _test_for_account->close(); //mark to close first

                            std::map<std::string,xblockacct_t*>& units = m_group_units[group_index];
                            auto target_it = units.find(_test_for_account->get_account());
                            if(target_it != units.end())
                            {
                                xblockacct_t * to_release = target_it->second;
                                units.erase(target_it);
                                if(to_release != nullptr)
                                {
                                    to_release->close();//double ensure it is closed
                                    to_release->release_ref();//release reference hold by units
                                }
                            }

                            _test_for_account->release_ref(); //now release last reference hold by m_monitor_expire
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
                m_monitor_expire.erase(old);
                ++expired_items_count;
            }

            for(auto it : _remonitor_list)
            {
                m_monitor_expire.insert(std::multimap<uint64_t,xblockacct_t*>::value_type(it->get_idle_duration() + current_time_ms,it));
            }
            return true;
        }

        bool  xvblockstore_impl::on_timer_start(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //attached into io-thread
        {
            return true;
        }
        bool  xvblockstore_impl::on_timer_stop(const int32_t errorcode,const int32_t thread_id,const int64_t timer_id,const int64_t cur_time_ms,const int32_t timeout_ms,const int32_t timer_repeat_ms)   //detach means it detach
        {
            for(auto it  = m_monitor_expire.begin(); it != m_monitor_expire.end(); ++it)
            {
                if(it->second != nullptr)
                {
                    it->second->release_ref();
                }
            }
            m_monitor_expire.clear();

            xkinfo("xvblockstore_impl::on_timer_stop,store=%s",m_store_path.c_str());
            return true;
        }
        int32_t  xvblockstore_impl::do_write(base::xstream_t & stream)    //write whole object to binary
        {
            return 0;
        }
        int32_t  xvblockstore_impl::do_read(base::xstream_t & stream)      //read from binary and regeneate content of xdataobj_t
        {
            return 0;
        }

    };//end of namespace of vstore
};//end of namespace of top
