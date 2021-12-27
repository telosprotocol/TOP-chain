// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xvunithub.h"
#include "xvledger/xvdrecycle.h"
#include "xvledger/xunit_proof.h"
#include "xdata/xgenesis_data.h"

#include "xmetrics/xmetrics.h"
#define METRICS_TAG(tag, val) XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)tag, val)


namespace top
{
    namespace store
    {
        auto_xblockacct_ptr::auto_xblockacct_ptr(std::recursive_mutex & locker,xvblockstore_impl * store_ptr)
            :base_class(nullptr),
             m_mutex(locker)
        {
            m_store_ptr = store_ptr;//no need hold reference since auto_xblockacct_ptr onlyl use inside of xvblockstore_impl
            m_mutex.lock();
        }

        auto_xblockacct_ptr::~auto_xblockacct_ptr()
        {
            //first process all pending events at this moment
            if(m_raw_ptr != NULL)
            {
                const std::deque<xblockevent_t> block_events(m_raw_ptr->move_events());
                m_raw_ptr->process_events(block_events);

                //now unithub has chance to konw which block has commit or not
                for(auto & event : block_events)
                {
                    if(event.get_index() != NULL) //still valid
                    {
                        if(enum_blockstore_event_committed == event.get_type())
                        {
                            m_store_ptr->on_block_committed(m_raw_ptr, event.get_index());
                        }
                    }
                }

                // TODO(jimmy) here may delete future for performance
                if (m_raw_ptr->get_cached_size() >= (m_raw_ptr->get_max_cache_size() * 2))
                {
                    m_raw_ptr->clean_caches(false,false);//light cleanup
                }

                //then release raw ptr
                xblockacct_t * old_ptr = m_raw_ptr;
                m_raw_ptr = NULL;
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                //finally unlock it
                m_mutex.unlock();
                
                //handle events after unlock
                for(auto & event : block_events)
                {
                    if(event.get_index() != NULL) //still valid
                    {
                        if(enum_blockstore_event_committed == event.get_type())
                        {
                            m_store_ptr->on_block_committed(event);
                        }
                    }
                }
            }
            else
            {
                //then release raw ptr
                xblockacct_t * old_ptr = m_raw_ptr;
                m_raw_ptr = NULL;
                if(old_ptr != NULL)
                    old_ptr->release_ref();

                //finally unlock it
                m_mutex.unlock();
            }
        }

        //transfer owner to auto_xblockacct_ptr from raw_ptr
        void  auto_xblockacct_ptr::transfer_owner(xblockacct_t * raw_ptr)
        {
            xblockacct_t * old_ptr = m_raw_ptr;
            m_raw_ptr = raw_ptr;
            if(raw_ptr != NULL)
                raw_ptr->add_ref();
            
            if(old_ptr != NULL)
                old_ptr->release_ref();
        }

        #define LOAD_ACCOUNT_OBJECT(account_obj,account_vid) \
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid()); \
            if (target_table == nullptr) { \
                xwarn_err("invalid account=%s",account_vid.get_address().c_str());\
                return nullptr;\
            }\
            base::xauto_ptr<base::xvaccountobj_t> account_obj = target_table->get_account(account_address);\

        #define LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account_vid) \
            if(is_close())\
            {\
                xwarn_err("xvblockstore has closed at store_path=%s",m_store_path.c_str());\
                return nullptr;\
            }\
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid()); \
            if (target_table == nullptr) { \
                xwarn_err("xvblockstore invalid account=%s",account_vid.get_address().c_str());\
                return nullptr;\
            }\
            auto_xblockacct_ptr account_obj(target_table->get_lock(),this); \
            get_block_account(target_table,account_vid.get_address(),account_obj); \

        #define LOAD_BLOCKACCOUNT_PLUGIN2(account_obj,account_vid) \
            if(is_close())\
            {\
                xwarn_err("xvblockstore has closed at store_path=%s",m_store_path.c_str());\
                return 0;\
            }\
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid()); \
            if (target_table == nullptr) { \
                xwarn_err("xvblockstore invalid account=%s",account_vid.get_address().c_str());\
                return 0;\
            }\
            auto_xblockacct_ptr account_obj(target_table->get_lock(),this); \
            get_block_account(target_table,account_vid.get_address(),account_obj); \

        xvblockstore_impl::xvblockstore_impl(base::xcontext_t & _context,const int32_t target_thread_id,base::xvdbstore_t* xvdb_ptr)
            :base::xvblockstore_t(_context,target_thread_id)
        {
            m_xvblockdb_ptr   = nullptr;
            m_xvblockdb_ptr = new xvblockdb_t(xvdb_ptr);
            m_store_path    = xvdb_ptr->get_store_path();
            xkinfo("xvblockstore_impl::create,store=%s,at thread=%d",m_store_path.c_str(),target_thread_id);
        }

        xvblockstore_impl::~xvblockstore_impl()
        {
            m_xvblockdb_ptr->release_ref();
            xkinfo("xvblockstore_impl::destroy,store=%s",m_store_path.c_str());
        }

        bool    xvblockstore_impl::close(bool force_async) //must call close before release object,otherwise object never be cleanup
        {
            base::xiobject_t::close(force_async); //since mutiple base class has close(),we need call seperately

            xkinfo("xvblockstore_impl::close,store=%s",m_store_path.c_str());
            return true;
        }

        //on_object_close be called when close command processed at host thread,logic flow: Caller(Thread#A)->Close()->Wake this object thread(B)->clean and then execute: on_object_close
        bool    xvblockstore_impl::on_object_close() //notify the subclass the object is closed
        {
            xkinfo("xvblockstore_impl::on_object_close,store=%s",m_store_path.c_str());
            return base::xiobject_t::on_object_close();
        }

        bool  xvblockstore_impl::get_block_account(base::xvtable_t * target_table,const std::string & account_address,auto_xblockacct_ptr & inout_account_obj)
        {
            base::xauto_ptr<base::xvaccountobj_t> auto_account_ptr(target_table->get_account(account_address));
            
            base::xauto_ptr<base::xvactplugin_t> auto_plugin_ptr(auto_account_ptr->get_plugin( base::enum_xvaccount_plugin_blockmgr));
            if(auto_plugin_ptr)
            {
                inout_account_obj.transfer_owner((xblockacct_t*)auto_plugin_ptr.get());
                return true;
            }
            
            uint64_t timeout_for_block_plugin = base::enum_plugin_idle_timeout_ms;
            if(auto_account_ptr->is_table_address())
            {
                timeout_for_block_plugin = (uint32_t)-1; //table object keep plugin forever
            }
            
            #ifdef __new_plugin_by_lambda__
            #else
            
            xblockacct_t * new_plugin = NULL;
            if(!auto_account_ptr->is_table_address())
                new_plugin =  new xunitbkplugin(*auto_account_ptr,timeout_for_block_plugin,m_xvblockdb_ptr);
            else
                new_plugin =  new xtablebkplugin(*auto_account_ptr,timeout_for_block_plugin,m_xvblockdb_ptr);
  
            base::xauto_ptr<base::xvactplugin_t> final_ptr(auto_account_ptr->get_set_plugin(new_plugin));
            inout_account_obj.transfer_owner((xblockacct_t*)final_ptr.get());
            new_plugin->release_ref();
            #endif
            
            return true;
        }

        std::string get_parent_table_account_from_unit_account(const base::xvaccount_t & account)
        {
            return base::xvaccount_t::make_table_account_address(account);
        }

        /////////////////////////////////new api with better performance by passing base::xvaccount_t
        base::xvblock_t * xvblockstore_impl::load_block_from_index(xblockacct_t* target_account, base::xauto_ptr<base::xvbindex_t> target_index,const uint64_t target_height,bool ask_full_load, const int atag)
        {
            return load_block_from_index_for_raw_index(target_account, target_index.get(), target_height, ask_full_load, atag);
        }
        base::xvblock_t * xvblockstore_impl::load_block_from_index_for_raw_index(xblockacct_t* target_account, base::xvbindex_t* target_index,const uint64_t target_height,bool ask_full_load, const int atag)
        {
            if(!target_index)
            {
                if(target_height != 0)
                    xdbg("xvblockstore_impl::load_block_from_index fail-invalid para at height(%llu) for account(%s) ",target_height,target_account->get_address().c_str());
                else
                    xwarn("xvblockstore_impl::load_block_from_index fail-invalid para for account(%s)",target_account->get_address().c_str());
                return nullptr;
            }

            // firstly, check self index if has block stored
            {
                bool loaded_new_block = false;
                if(target_index->get_this_block() == NULL) {  // load from db
                  
                    XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                    XMETRICS_GAUGE(metrics::blockstore_blk_load, 0);
                    loaded_new_block = get_blockdb_ptr()->load_block_object(target_index, atag);
                } else {  // load from cache
                  
                    XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                    XMETRICS_GAUGE(metrics::blockstore_blk_load, 1);
                    
                }

                if(ask_full_load)
                {
                    get_blockdb_ptr()->load_block_input(target_index);
                    get_blockdb_ptr()->load_block_output(target_index);
                }
                if(target_index->get_this_block() != NULL)
                {
                    //transfer block flags from index to raw block here
    //                xassert(target_index->get_block_flags() == target_index->get_this_block()->get_block_flags());  // TODO(jimmy)
                    //target_index->get_this_block()->reset_block_flags(target_index->get_block_flags());

                    //must addreference first before clean_caches(),otherwise it might be reset by clean_caches
                    base::xvblock_t * raw_block_ptr = target_index->get_this_block();
                    raw_block_ptr->add_ref();//add reference before return

                    if(loaded_new_block) //try to keep balance when one new block loaded,so trigger lightly cleanup
                    {
                        if(target_index->get_block_level() == base::enum_xvblock_level_table)
                            target_account->clean_caches(false,false);//cache raw block londer for table with better performance
                        else
                            target_account->clean_caches(false,true);//light cleanup
                    }

                    return raw_block_ptr;
                }
            }
            
            if (target_index->has_parent_store() // secondly, if has parent block, try to load from parent block.
                && (false == target_index->get_extend_cert().empty()) 
                && (false == target_index->get_extend_data().empty()) )
            {
                // TODO(jimmy)
                // 1.load on-demand by ask-full future
                // 2.unit extend cert and extend data should be set after proved if has parent store
                base::xvaccount_t parent_account(get_parent_table_account_from_unit_account(*target_account->get_account_obj()));
                base::xauto_ptr<base::xvblock_t> parent_block(load_block_object(parent_account, target_index->get_parent_block_height(), target_index->get_parent_block_viewid(), true, atag));
                if(!parent_block)
                {
                    // parent block may deleted for fork block
                    xwarn("xvblockstore_impl::load_block_from_index fail-load parent block from unit(%s) at store(%s)",target_index->dump().c_str(),get_store_path().c_str());
                    return nullptr;
                }
#if 0  // TODO(jimmy) unpack all sub blocks take more time, so not use this mode
                std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
                if(parent_block->extract_sub_blocks(sub_blocks))
                {
                    for (auto & sub_block : sub_blocks)
                    {
                        if(  (sub_block->get_height()      == target_index->get_height())
                            &&(sub_block->get_viewid()      == target_index->get_viewid())
                            &&(sub_block->get_viewtoken()   == target_index->get_viewtoken())
                            &&(sub_block->get_account()     == target_index->get_account()) )
                        {
                            xdbg("xvblockstore_impl::load_block_from_index succ from parent.unit=%s,parent=%s",
                                    target_index->dump().c_str(),parent_block->dump().c_str());
                            sub_block->reset_block_flags(target_index->get_block_flags()); // copy bindex flags to block
                            // target_index->reset_this_block(sub_block.get()); // TODO(jimmy) always not cache unit block
                            return sub_block.detach();//transfer ownership to caller
                        }                        
                    }
                    xerror("xvblockstore_impl::load_block_from_index,fail-found unit(%s) from table block(%s)", target_index->dump().c_str(),parent_block->dump().c_str());
                }
                else
                {
                    xerror("xvblockstore_impl::load_block_from_index,fail-extract_sub_blocks unit(%s) from table block(%s)", target_index->dump().c_str(),parent_block->dump().c_str());
                }
#else
                xobject_ptr_t<base::xvblock_t> sub_block;
                if(parent_block->extract_one_sub_block(target_index->get_parent_block_entity(), target_index->get_extend_cert(), target_index->get_extend_data(), sub_block))
                {
                    if(  (sub_block->get_height()      == target_index->get_height())
                        &&(sub_block->get_viewid()      == target_index->get_viewid())
                        &&(sub_block->get_viewtoken()   == target_index->get_viewtoken())
                        &&(sub_block->get_account()     == target_index->get_account()) )
                    {
                        xdbg("xvblockstore_impl::load_block_from_index succ from parent.unit=%s,parent=%s",
                                target_index->dump().c_str(),parent_block->dump().c_str());
                        sub_block->reset_block_flags(target_index->get_block_flags()); // copy bindex flags to block
                        target_index->reset_this_block(sub_block.get());
                        return sub_block.detach();//transfer ownership to caller
                    }
                    xerror("xvblockstore_impl::load_block_from_index,fail-found unit(%s) from table block(%s)", target_index->dump().c_str(),parent_block->dump().c_str());
                }
                else
                {
                    xerror("xvblockstore_impl::load_block_from_index,fail-extract_sub_blocks unit(%s) from table block(%s)", target_index->dump().c_str(),parent_block->dump().c_str());
                }
#endif        
            }

            xerror("xvblockstore_impl::load_block_from_index fail load block object(%s) at store(%s)",target_index->dump().c_str(),m_store_path.c_str());
            return nullptr;
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_genesis_block(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_genesis_index(),0,false, atag);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_cert_block(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_cert_index(),0,false, atag);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_locked_block(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_locked_index(),0,false, atag);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_committed_block(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_committed_index(),0,false, atag);
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_connected_block(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_connected_index(),0,false, atag);
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_genesis_connected_block(const base::xvaccount_t & account,bool ask_full_search,const int atag)//block has connected to genesis
        {
            const uint64_t latest_genesis_height = get_latest_genesis_connected_block_height(account);
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(latest_genesis_height,0),0,false, atag);
        }

        base::xauto_ptr<base::xvbindex_t> xvblockstore_impl::get_latest_genesis_connected_index(const base::xvaccount_t & account,bool ask_full_search,const int atag) //block has connected to genesis
        {
            const uint64_t latest_genesis_height = get_latest_genesis_connected_block_height(account);
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            return account_obj->load_index(latest_genesis_height,0);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_committed_full_block(const base::xvaccount_t & account,const int atag)
        {
            auto connect_block = get_latest_connected_block(account, atag);
            if(connect_block != nullptr)
            {
                if(connect_block->get_block_class() == base::enum_xvblock_class_full)
                {
                    return connect_block;
                }
                auto latest_committed_full_height = connect_block->get_last_full_block_height();
                auto latest_committed_full_hash = connect_block->get_last_full_block_hash();
                if (latest_committed_full_height == 0) {
                    return load_block_object(account, latest_committed_full_height, 0, false, atag);
                } else {
                    return load_block_object(account, latest_committed_full_height, latest_committed_full_hash, false, atag);
                }
            }

            return nullptr;
        }

        uint64_t xvblockstore_impl::get_latest_committed_block_height(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN2(account_obj,account);
            METRICS_TAG(atag, 1);
            return account_obj->get_latest_committed_block_height();
        }

        uint64_t xvblockstore_impl::get_latest_connected_block_height(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN2(account_obj,account);
            METRICS_TAG(atag, 1);
            // XTODO use load_latest_connected_index for invoke update meta
            base::xauto_ptr<base::xvbindex_t> _bindex = account_obj->load_latest_connected_index();
            if (_bindex != nullptr) {
                return _bindex->get_height();
            }
            return 0;
        }

        uint64_t xvblockstore_impl::get_latest_genesis_connected_block_height(const base::xvaccount_t & address,const int atag)
        {
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(address.get_xvid());
            if (target_table == nullptr) {
                xwarn_err("invalid account=%s",address.get_address().c_str());
                return 0;
            }
            base::xauto_ptr<base::xvaccountobj_t> account_obj = target_table->get_account(address);
            METRICS_TAG(atag, 1);
            return account_obj->get_sync_meta()._highest_genesis_connect_height;
        }

        uint64_t xvblockstore_impl::get_latest_executed_block_height(const base::xvaccount_t & address,const int atag)
        {
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(address.get_xvid());
            if (target_table == nullptr) {
                xwarn_err("invalid account=%s",address.get_address().c_str());
                return 0;
            }
            base::xauto_ptr<base::xvaccountobj_t> account_obj = target_table->get_account(address);
            METRICS_TAG(atag, 1);
            return account_obj->get_latest_executed_block_height();
        }

        bool xvblockstore_impl::set_latest_executed_info(const base::xvaccount_t & account,uint64_t height,const std::string & blockhash,const int atag)
        {
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account.get_xvid());
            if (target_table == nullptr) {
                xwarn_err("invalid account=%s",account.get_address().c_str());
                return false;
            }
            base::xauto_ptr<base::xvaccountobj_t> account_obj = target_table->get_account(account);
            METRICS_TAG(atag, 1);
            return account_obj->set_latest_executed_block(height, blockhash);
        }

        //one api to get latest_commit/latest_lock/latest_cert for better performance
        base::xblock_mptrs  xvblockstore_impl::get_latest_blocks(const base::xvaccount_t & account,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            base::xvbindex_t*  cert_index    = nullptr;
            base::xvbindex_t*  lock_index    = nullptr;
            base::xvbindex_t*  commit_index  = nullptr;
            //note:cert_index/lock_index/commit_index move to load_block_from_index that may release finally
            if(account_obj->load_latest_index_list(cert_index,lock_index,commit_index) == false)
                return base::xblock_mptrs(nullptr,nullptr,nullptr);//move reference to xblock_mptrs

            //note:cert_block/lock_block/commit_block move to xblock_mptrs that may release finally
            base::xvblock_t*  cert_block    = load_block_from_index(account_obj.get(),cert_index,0,false);
            base::xvblock_t*  lock_block    = load_block_from_index(account_obj.get(),lock_index,0,false);
            base::xvblock_t*  commit_block  = load_block_from_index(account_obj.get(),commit_index,0,false);
            return base::xblock_mptrs(cert_block,lock_block,commit_block);//move reference to xblock_mptrs
        }

        base::xblock_vector xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,const int atag)//might mutiple certs at same height
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            if(account_obj->load_index(height) > 0) //load first
            {
                std::vector<base::xvbindex_t*> index_list = account_obj->query_index(height);//returned ptr with added reference
                if(index_list.empty() == false)
                {
                    std::vector<base::xvblock_t*> block_list;
                    for(auto & index : index_list)
                    {
                        if(index != NULL)
                        {
                            //load_index returned ptr with added reference,here move into auto_ptr to manage reference
                            base::xauto_ptr<base::xvbindex_t> auto_index_ptr(index);
                            base::xvblock_t * block = load_block_from_index(account_obj.get(),base::xauto_ptr<base::xvbindex_t>(std::move(auto_index_ptr)),index->get_height(),false,atag);
                            if(block != NULL)
                                block_list.push_back(block);//ptr will be released by xblock_vector later

                        }
                    }
                    return block_list;
                }
            }
            return base::xblock_vector();
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(height,viewid),height,ask_full_load,atag);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(height,blockhash),height,ask_full_load,atag);
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,bool ask_full_load,const int atag)  //just return the highest viewid of matched flag
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(height,required_block),height,ask_full_load,atag);
        }

        std::vector<base::xvblock_ptr_t> xvblockstore_impl::load_block_object(const std::string & tx_hash,const base::enum_transaction_subtype type,const int atag)
        {
            base::xvtxindex_ptr txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(tx_hash, type);
            if(!txindex)
            {
                xwarn("xvblockstore_impl::load_block_object tx not find.tx=%s, type=%u", base::xstring_utl::to_hex(tx_hash).c_str(), type);
                return std::vector<base::xvblock_ptr_t>{};
            }

            // get unit block in which stores the tx, plus trailing unit blocks(only height +1 & +2)
            const base::xvaccount_t account(txindex->get_block_addr());
            const uint64_t height = txindex->get_block_height();
            std::vector<std::vector<base::xvblock_ptr_t>> blocks_vv;
            const uint8_t max_fork_height = 2;
            for(uint64_t i = 0; i <= max_fork_height; ++i)
            {
                auto blks = load_block_object(account, height + i,atag);
                std::vector<base::xvblock_t*> blks_ptr = blks.get_vector();
                // if block not committed, return directly
                if((i == 0 && blks_ptr.size() > 1) || (blks_ptr.size() == 0))
                {
                    xdbg("xvblockstore_impl::load_block_object tx=%s, type=%u, height=%u, i=%u, blk size: %zu",
                         base::xstring_utl::to_hex(tx_hash).c_str(), type, height, i, blks_ptr.size());
                    return std::vector<base::xvblock_ptr_t>{};
                }

                std::vector<base::xvblock_ptr_t> blocks_v;
                for (uint32_t j = 0; j < blks_ptr.size(); j++)
                {
                    blks_ptr[j]->add_ref();
                    base::xvblock_ptr_t bp;
                    bp.attach(blks_ptr[j]);
                    blocks_v.push_back(bp);
                }
                blocks_vv.push_back(blocks_v);
            }

            // select one legal 3-blocks chain
            std::vector<base::xvblock_ptr_t> result(3, nullptr);
            result[0] = blocks_vv[0][0];
            for(uint16_t i = 0; i < blocks_vv[1].size(); ++i)
            {
                if(blocks_vv[1][i]->get_last_block_hash() != result[0]->get_block_hash())
                    continue;

                result[1] = blocks_vv[1][i];
                for(uint16_t j = 0; j < blocks_vv[2].size(); ++j)
                {
                    if(blocks_vv[2][j]->get_last_block_hash() == result[1]->get_block_hash())
                    {
                        result[2] = blocks_vv[2][j];
                        return result;
                    }
                }
            }

            return std::vector<base::xvblock_ptr_t>{};
        }

        bool                xvblockstore_impl::load_block_input(const base::xvaccount_t & account,base::xvblock_t* block,const int atag)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::load_block_input,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            if( block->get_block_class() == base::enum_xvblock_class_nil  // nil block has no input
               || block->get_input()->get_resources_hash().empty() //resources hash empty means has no resoure data
               || block->get_input()->has_resource_data() )  //already has resource data
            {
                return true;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            
            base::xauto_ptr<base::xvbindex_t> existing_index(account_obj->load_index(block->get_height(), block->get_block_hash()));
            if(existing_index)
            {
                return get_blockdb_ptr()->load_block_input(existing_index(),block);
                //XTODO,add logic to extract from tabeblock
            }
            return false;
        }

        bool                xvblockstore_impl::load_block_output(const base::xvaccount_t & account,base::xvblock_t* block,const int atag)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::load_block_output,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            if( block->get_block_class() == base::enum_xvblock_class_nil  // nil block has no input
               || block->get_output()->get_resources_hash().empty() //resources hash empty means has no resoure data
               || block->get_output()->has_resource_data() )  //already has resource data
            {
                return true;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            
            base::xauto_ptr<base::xvbindex_t> existing_index(account_obj->load_index(block->get_height(), block->get_block_hash()));
            if(existing_index)
            {
                return get_blockdb_ptr()->load_block_output(existing_index(),block);
                //XTODO,add logic to extract from tabeblock
            }
            return false;
        }

        bool    xvblockstore_impl::store_committed_unit_block(const base::xvaccount_t & account, base::xvblock_t * container_block) {
            xdbg("xvblockstore_impl::store_committed_unit_block enter store block(%s)", container_block->dump().c_str());
            {
                LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
                account_obj->store_committed_unit_block(container_block);
            }
            return true;
        }

        bool    xvblockstore_impl::store_block(base::xauto_ptr<xblockacct_t> & container_account,base::xvblock_t * container_block,bool execute_block) //store table/book blocks if they are
        {
            xdbg("jimmy xvblockstore_impl::store_block enter,store block(%s)", container_block->dump().c_str());

            //first do store block
            bool ret = container_account->store_block(container_block);
            if(!ret)
            {
                xwarn("xvblockstore_impl::store_block,fail-store block(%s)", container_block->dump().c_str());
                // return false;
            }

            bool did_stored = ret;//inited as false
            //then try extract for container if that is
            if(  (container_block->get_block_class() == base::enum_xvblock_class_light) //skip nil block
               &&(container_block->get_block_level() == base::enum_xvblock_level_table)
               &&(container_block->get_height() != 0) )
            {
                base::xauto_ptr<base::xvbindex_t> existing_index(container_account->load_index(container_block->get_height(), container_block->get_block_hash()));
                if(existing_index && (existing_index->get_block_flags() & base::enum_xvblock_flag_unpacked) == 0) //unpacked yet
                {
                    #ifdef DEBUG
                    xassert(container_block->is_input_ready(true));
                    xassert(container_block->is_output_ready(true));
                    #else
                    xassert(container_block->is_input_ready(false));
                    xassert(container_block->is_output_ready(false));
                    #endif

                    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
                    if(container_block->extract_sub_blocks(sub_blocks))
                    {
                        xdbg("xvblockstore_impl::store_block,table block(%s) carry unit num=%d", container_block->dump().c_str(), (int)sub_blocks.size());

                        bool table_extract_all_unit_successful = true;
                        for (auto & unit_block : sub_blocks)
                        {
                            base::xvaccount_t  unit_account(unit_block->get_account());

                            // // test: not store unit for test
                            // if (!data::is_sys_contract_address(common::xaccount_address_t{unit_account.get_account()})) {
                            //     uint64_t rand_num = rand();
                            //     if (rand_num % 8 == 0) {
                            //         xwarn("test drop unit=%s", unit_block->dump().c_str());
                            //         continue;
                            //     }  
                            // }

                            if(false == store_block(unit_account,unit_block.get())) //any fail resultin  re-unpack whole table again
                            {
                                //table_extract_all_unit_successful = false;//reset to false for any failure of unit  // TODO(jimmy) always true if stored
                                xwarn("xvblockstore_impl::store_block,fail-store unit-block=%s",unit_block->dump().c_str());
                            }
                            else
                            {
                                xdbg("xvblockstore_impl::store_block,stored unit-block=%s",unit_block->dump().c_str());

                                on_block_stored(unit_block.get());//throw event for sub blocks
                            }
                        }

                        //update to block'flag acccording table_extract_all_unit_successful
                        if(table_extract_all_unit_successful)
                        {
                            existing_index->set_block_flag(base::enum_xvblock_flag_unpacked);
                            xinfo("xvblockstore_impl::store_block,extract_sub_blocks done for table block, %s", container_block->dump().c_str());
                        }
                    }
                    else
                    {
                        xerror("xvblockstore_impl::store_block,fail-extract_sub_blocks for table block(%s)", container_block->dump().c_str(), (int)sub_blocks.size());
                    }
                }
                else
                {
                    did_stored = true;
                }
            }

            if(did_stored)
            {
                //move clean logic here to reduce risk of reenter process that might clean up some index too early
                if(container_block->get_block_level() == base::enum_xvblock_level_table)
                    container_account->clean_caches(false,false);//cache raw block londer for table with better performance
                else
                    container_account->clean_caches(false,true);
            }
#if 0  // TODO(jimmy)
            if(execute_block)
            {
                container_account->try_execute_all_block(container_block);  // try to push execute block, ignore store result
            }
#endif
            return true; //still return true since tableblock has been stored successful
        }

        bool                xvblockstore_impl::store_block(const base::xvaccount_t & account,base::xvblock_t* block,const int atag)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }

            if(block->check_block_flag(base::enum_xvblock_flag_authenticated) == false
            || (block->get_height() == 0 && block->check_block_flag(base::enum_xvblock_flag_committed) == false))
            {
                xerror("xvblockstore_impl::store_block,unauthorized block(%s)",block->dump().c_str());
                return false;
            }

            bool ret = false;
            {
                LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
                METRICS_TAG(atag, 1);
                ret = store_block(account_obj,block);
            }

            // XTODO only tabletable need execute immediately after stored
            if (block->get_block_level() == base::enum_xvblock_level_table) {
                // TODO(jimmy) commit tableblock try to update table state
                base::auto_reference<base::xvblock_t> auto_hold_block_ptr(block);
                base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->execute_block(block, metrics::statestore_access_from_blockstore);
            }

            return ret;
        }

        bool                xvblockstore_impl::store_block_but_not_execute(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block_but_not_execute,block NOT match account:%",account.get_account().c_str());
                return false;
            }

            if(block->check_block_flag(base::enum_xvblock_flag_authenticated) == false)
            {
                xerror("xvblockstore_impl::store_block_but_not_execute,unauthorized block(%s)",block->dump().c_str());
                return false;
            }

            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            if(store_block(account_obj,block,false))//force to not execute anymore
            {
                return true;
            }
            return false;
        }

        bool                xvblockstore_impl::store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks,const int atag)
        {
            if(batch_store_blocks.empty())
                return true;

            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            for(auto it : batch_store_blocks)
            {
                if((it != nullptr) && (it->get_account() == account_obj->get_address()) )
                {
                    if(store_block(account_obj,it,atag))
                        on_block_stored(it);
                }
            }
            return  true;
        }

        bool                xvblockstore_impl::delete_block(const base::xvaccount_t & account,base::xvblock_t* block,const int atag)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            return account_obj->delete_block(block);
        }

        bool xvblockstore_impl::try_update_account_index(const base::xvaccount_t & account, uint64_t height, uint64_t viewid, bool update_pre_block) {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->try_update_account_index(height, viewid, update_pre_block);
        }

        base::xvtransaction_store_ptr_t  xvblockstore_impl::query_tx(const std::string & txhash, base::enum_transaction_subtype type,const int atag)
        {
            //XTODO:tx always not cache now
            base::xvtransaction_store_ptr_t txstore = make_object_ptr<base::xvtransaction_store_t>();
            METRICS_TAG(atag, 1);

            if(type == base::enum_transaction_subtype_all || type == base::enum_transaction_subtype_self || type == base::enum_transaction_subtype_send)
            {
                base::xvtxindex_ptr send_txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txhash, base::enum_transaction_subtype_send);
                if(nullptr == send_txindex)
                {
                    xwarn("xvblockstore_impl::query_tx fail-send tx index not find.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                    return nullptr;
                }
                base::xvaccount_t _vaddr(send_txindex->get_block_addr());
                base::xauto_ptr<base::xvblock_t> unit_block = load_block_object(_vaddr, send_txindex->get_block_height(), send_txindex->get_block_hash(), true); // TODO(jimmy) false+input
                if (nullptr == unit_block)
                {
                    xwarn("xvblockstore_impl::query_tx fail-send unit not find.account=%s,tx=%s", send_txindex->get_block_addr().c_str(), base::xstring_utl::to_hex(txhash).c_str());
                    return nullptr;
                }
                std::string orgtx_bin = unit_block->get_input()->query_resource(txhash);
                if (orgtx_bin.empty())
                {
                    xerror("xvblockstore_impl::query_tx fail-query tx from send unit.account=%s,tx=%s", send_txindex->get_block_addr().c_str(), base::xstring_utl::to_hex(txhash).c_str());
                    return nullptr;
                }
                base::xauto_ptr<base::xdataunit_t> raw_tx = base::xdataunit_t::read_from(orgtx_bin);
                if(nullptr == raw_tx)
                {
                    xerror("xvblockstore_impl::query_tx fail-tx content read from fail.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                    return nullptr;
                }
                txstore->set_raw_tx(raw_tx.get());
                txstore->set_send_block_info(send_txindex);
                if(send_txindex->is_self_tx())
                {
                    xdbg("xvblockstore_impl::query_tx self tx");  //self tx no need query more
                    txstore->set_recv_block_info(send_txindex);
                    txstore->set_confirm_block_info(send_txindex);
                    return txstore;
                }
            }
            if(type == base::enum_transaction_subtype_all || type == base::enum_transaction_subtype_recv)
            {
                base::xvtxindex_ptr txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txhash, base::enum_transaction_subtype_recv);
                if(!txindex)
                {
                    xwarn("xvblockstore_impl::query_tx recv tx not find.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                    return (type == base::enum_transaction_subtype_all) ? txstore : nullptr;
                }
                txstore->set_recv_block_info(txindex);
            }
            if(type == base::enum_transaction_subtype_all || type == base::enum_transaction_subtype_confirm)
            {
                base::xvtxindex_ptr txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txhash, base::enum_transaction_subtype_confirm);
                if(!txindex)
                {
                    xwarn("xvblockstore_impl::query_tx confirm tx not find.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                    return (type == base::enum_transaction_subtype_all) ? txstore : nullptr;
                }
                txstore->set_confirm_block_info(txindex);
            }

            return txstore;
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height, const uint64_t viewid,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->query_index(height,viewid),height,false,atag);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height, const std::string & blockhash,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->query_index(height,blockhash),height,false,atag);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,const int atag) //just return the highest viewid of matched flag
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->query_index(height,required_block),height,false,atag);
        }

        base::xblock_vector xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height,const int atag)//might mutiple certs at same height
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            if(account_obj->load_index(height) > 0) //check and load from db even for query_block
            {
                std::vector<base::xvbindex_t*> index_list(account_obj->query_index(height));//return raw ptr with added reference
                if(index_list.empty() == false)
                {
                    std::vector<base::xvblock_t*> block_list;
                    for(auto & index : index_list)
                    {
                        if(index != NULL)
                        {
                            //query_index return raw ptr with added reference,so here move into auto_ptr to relase it
                            base::xauto_ptr<base::xvbindex_t> auto_index_ptr(index);
                            base::xvblock_t * block = load_block_from_index(account_obj.get(),base::xauto_ptr<base::xvbindex_t>(std::move(auto_index_ptr)),index->get_height(),false,atag);
                            if(block != NULL)
                                block_list.push_back(block); //ptr will be released by xblock_vector later
                        }
                    }
                    return block_list;
                }
            }

            xwarn("xvblockstore_impl query_block(height) fail to load block(%llu) for account(%s) at store(%s)",height,account.get_address().c_str(),m_store_path.c_str());
            return nullptr;
        }

        base::xvbindex_vector   xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            int index_size = account_obj->load_index(height);
            if(index_size > 0) //load first
                return base::xvbindex_vector(account_obj->query_index(height));//then query

            xwarn("xvblockstore_impl load_block_index(height) fail to load block(%llu) for account(%s) at store(%s)",height,account.get_address().c_str(),m_store_path.c_str());
            return base::xvbindex_vector();
        }

        base::xauto_ptr<base::xvbindex_t>  xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_index(height,viewid,atag);
        }

        base::xauto_ptr<base::xvbindex_t>  xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,const int atag)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_index(height,blockhash,atag);
        }

        base::xauto_ptr<base::xvbindex_t>  xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,const int atag)//just return the highest viewid of matched flag
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_index(height,required_block,atag);
        }

        //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
        bool                xvblockstore_impl::clean_caches(const base::xvaccount_t & account,const int atag)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            return account_obj->clean_caches(true,true);
        }
        
        bool      xvblockstore_impl::on_block_committed(const xblockevent_t & event)
        {
            base::xvbindex_t* index_ptr = event.get_index();
            if(NULL == index_ptr)
                return false;
            
            xdbg("xvblockstore_impl::on_block_committed,at account=%s,index=%s",index_ptr->get_account().c_str(),index_ptr->dump().c_str());
            
            #ifdef __FIRE_MBUS_EVENT_ON_UNITHUB_LAYER__
            if(index_ptr->get_block_level() == base::enum_xvblock_level_table
               && index_ptr->get_block_flags() & base::enum_xvblock_flag_committed
               && index_ptr->get_height() != 0)
            {
                base::xveventbus_t * mbus = base::xvchain_t::instance().get_xevmbus();
                xassert(mbus != NULL);
                if(mbus != NULL)
                {
                    if(index_ptr->get_height() != 0)
                    {
                        mbus::xevent_ptr_t event = mbus->create_event_for_store_committed_block(index_ptr);
                        if (event != nullptr) {
                            mbus->push_event(event);
                        }
                    }
                    xdbg_info("xvblockstore_impl::on_block_committed,done at store(%s)-> block=%s",get_store_path().c_str(),index_ptr->dump().c_str());
                }
            }
            #endif //end of __FIRE_MBUS_EVENT_ON_UNITHUB_LAYER__
            
            base::xblockrecycler_t* recycler = base::xvchain_t::instance().get_xrecyclemgr()->get_block_recycler();
            if(recycler != NULL)
            {
                base::xblockmeta_t clone_meta(event.get_meta());
                if(recycler->recycle(*index_ptr, clone_meta))
                {
                    event.get_plugin()->set_latest_deleted_block_height(clone_meta._highest_deleted_block_height);
                }
            }
            return true;
        }

        bool      xvblockstore_impl::on_block_stored(base::xvblock_t* this_block_ptr)
        {
            //we have enable event at xblockacct_t layer,so disable following code
            /*
            if(this_block_ptr->get_height() == 0) //ignore genesis block
                return true;

            const int block_flags = this_block_ptr->get_block_flags();
            if((block_flags & base::enum_xvblock_flag_executed) != 0)
            {
                //here notify execution event if need
            }
            else if( ((block_flags & base::enum_xvblock_flag_committed) != 0) && ((block_flags & base::enum_xvblock_flag_connected) != 0) )
            {
                base::xveventbus_t * mbus = base::xvchain_t::instance().get_xevmbus();
                #ifndef __ENABLE_MOCK_XSTORE__
                xassert(mbus != NULL);
                #endif
                if(mbus != NULL)
                {
                    xdbg_info("xvblockstore_impl::on_block_stored,at store(%s)-> block=%s",m_store_path.c_str(),this_block_ptr->dump().c_str());

                    mbus::xevent_ptr_t event = mbus->create_event_for_store_block_to_db(this_block_ptr);
                    mbus->push_event(event);
                }
            }
            */
            return true;
        }

        bool  xvblockstore_impl::on_block_committed(xblockacct_t* target_account,base::xvbindex_t* index_ptr)
        {
            if(nullptr == index_ptr)
                return false;

            if(false == index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                return false;

            bool ret = store_units_to_db(target_account, index_ptr);
            if (!ret) {
                return ret;
            }
            return store_txs_to_db(target_account, index_ptr);
        }

        bool  xvblockstore_impl::store_units_to_db(xblockacct_t* target_account,base::xvbindex_t* index_ptr)
        {
            // // test: not store commit unit for test
            // uint64_t rand_num = rand();
            // if (rand_num % 8 == 0) {
            //     xwarn("test not store units of table=%s", index_ptr->dump().c_str());
            //     return true;
            // }

            if( (index_ptr->get_block_class() == base::enum_xvblock_class_light)
               && (index_ptr->get_block_level() == base::enum_xvblock_level_table) )
            {
                base::xauto_ptr<base::xvblock_t> container_block = load_block_from_index_for_raw_index(target_account, index_ptr, index_ptr->get_height(), true);
                if (container_block == nullptr) {
                    xerror("xvblockstore_impl::store_units_to_db,fail-load tableblock.index=%s",index_ptr->dump().c_str());
                    return false;
                }

                auto cert_blocks = load_block_object(*index_ptr, index_ptr->get_height() + 2);
                if (cert_blocks.get_vector().empty()) {
                    xerror("xvblockstore_impl::store_units_to_db,fail-load cert tableblock.index=%s,cert height=%llu",index_ptr->dump().c_str(), index_ptr->get_height() + 2);
                    return false;
                }

                xassert(container_block->is_input_ready(true));
                xassert(container_block->is_output_ready(true));

                std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
                if(container_block->extract_sub_blocks(sub_blocks))
                {
                    xdbg("xvblockstore_impl::store_units_to_db,table block(%s) carry unit num=%d", container_block->dump().c_str(), (int)sub_blocks.size());

                    for (auto & unit_block : sub_blocks)
                    {
                        base::xvaccount_t  unit_account(unit_block->get_account());

                        // // test: not store unit for test
                        // if (!data::is_sys_contract_address(common::xaccount_address_t{unit_account.get_account()})) {
                        //     uint64_t rand_num = rand();
                        //     if (rand_num % 8 == 0) {
                        //         xwarn("test drop unit=%s", unit_block->dump().c_str());
                        //         return true;
                        //     }  
                        // }

                        unit_block->set_block_flag(base::enum_xvblock_flag_committed);
                        unit_block->set_block_flag(base::enum_xvblock_flag_locked);

                        if(false == store_committed_unit_block(unit_account,unit_block.get()))
                        {
                            xwarn("xvblockstore_impl::store_units_to_db,fail-store unit-block=%s",unit_block->dump().c_str());
                        }
                        else
                        {
                            xdbg("xvblockstore_impl::store_units_to_db,stored unit-block=%s",unit_block->dump().c_str());                                
                        }
                        // store corresponding table proof for latest commit unit block
                        base::xunit_proof_t unit_proof(unit_block->get_height(), unit_block->get_viewid(), cert_blocks.get_vector().at(0)->get_cert());
                        std::string unit_proof_str;
                        unit_proof.serialize_to(unit_proof_str);
                        if (!set_unit_proof(unit_account, unit_proof_str, unit_block->get_height())) {
                            xerror("xvblockstore_impl::store_units_to_db account %s,fail to writed into db,block=%s",unit_account.get_address().c_str(), unit_block->dump().c_str());
                            return false;
                        }
                    }

                    //update to block'flag acccording table_extract_all_unit_successful
                    index_ptr->set_block_flag(base::enum_xvblock_flag_unpacked);
                    xinfo("xvblockstore_impl::store_units_to_db,extract_sub_blocks done for table block, %s", container_block->dump().c_str());
                }
                else
                {
                    xerror("xvblockstore_impl::store_units_to_db,fail-extract_sub_blocks for table block(%s)", container_block->dump().c_str(), (int)sub_blocks.size());
                }
            }
            return true;
        }

        bool  xvblockstore_impl::store_txs_to_db(xblockacct_t* target_account,base::xvbindex_t* index_ptr)
        {
            if(nullptr == index_ptr)
                return false;

            if(false == index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                return false;

            if( (index_ptr->get_block_class() == base::enum_xvblock_class_light)
               && (index_ptr->get_block_level() == base::enum_xvblock_level_table) )
            {
                if(!index_ptr->check_store_flag(base::enum_index_store_flag_transactions))
                {
                    xdbg("xvblockstore_impl::store_txs_to_db,index=%s",index_ptr->dump().c_str());
                    base::xauto_ptr<base::xvblock_t> target_block = load_block_from_index_for_raw_index(target_account, index_ptr, index_ptr->get_height(), false); // TODO(jimmy) false
                    // target_account->load_block_object(index_ptr);
                    // target_account->load_block_input(index_ptr->get_this_block());
                    // target_account->load_block_output(index_ptr->get_this_block());
                    auto ret = base::xvchain_t::instance().get_xtxstore()->store_txs(target_block.get());
                    if(ret)
                    {
                        index_ptr->set_store_flag(base::enum_index_store_flag_transactions);
                    }
                    return ret;
                }
            }
            return true;
        }

        bool      xvblockstore_impl::exist_genesis_block(const base::xvaccount_t & account,const int atag) {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            METRICS_TAG(atag, 1);
            base::xvbindex_t* target_block = account_obj->query_index(0, 0);
            if (target_block != NULL) {
                xdbg("xvblockstore_impl::exist_genesis_block target_block not null");
                target_block->release_ref();
                return true;
            }
            if (account_obj->load_index_by_height(0) > 0) {
                target_block = account_obj->query_index(0, 0); //found existing ones
            }
#if (defined DEBUG)
            if(target_block != NULL) {//the ptr has been add reference by query_index
                xdbg("xvblockstore_impl::exist_genesis_block target_block not null after load");
            } else {
                xdbg("xvblockstore_impl::exist_genesis_block target_block null after load");
            }
#endif
            if(target_block != NULL)
                target_block->release_ref();

            return (nullptr != target_block);
        }

        bool xvblockstore_impl::set_genesis_height(const base::xvaccount_t & account, const std::string &height)
        {
            const std::string key_path = base::xvdbkey_t::create_account_span_genesis_height_key(account);
            if (!base::xvchain_t::instance().get_xdbstore()->set_value(key_path, height))
            {
                xerror("xvblockstore_impl::set_genesis_height key %s,fail to writed into db,index dump(%s)",key_path.c_str(), height.c_str());
                return false;
            }
            return true;
        }
        
        const std::string xvblockstore_impl::get_genesis_height(const base::xvaccount_t & account)
        {
            const std::string key_path = base::xvdbkey_t::create_account_span_genesis_height_key(account);
            return base::xvchain_t::instance().get_xdbstore()->get_value(key_path);
        }
        
        bool xvblockstore_impl::set_block_span(const base::xvaccount_t & account, const uint64_t height,  const std::string& span)
        {
            const std::string key_path = base::xvdbkey_t::create_account_span_key(account, height);
            if (!base::xvchain_t::instance().get_xdbstore()->set_value(key_path, span))
            {
                xerror("xvblockstore_impl::set_block_span key %s,fail to writed into db,index dump(%s)",key_path.c_str(), span.c_str());
                return false;
            }
            return true;
        }
        
        bool xvblockstore_impl::delete_block_span(const base::xvaccount_t & account, const uint64_t height)
        {
            const std::string key_path = base::xvdbkey_t::create_account_span_key(account, height);
            if (!base::xvchain_t::instance().get_xdbstore()->delete_value(key_path))
            {
                xerror("xvblockstore_impl::delete_block_span key %s,fail to delete from db",key_path.c_str());
                return false;
            }
            return true;
        }
        
        const std::string xvblockstore_impl::get_block_span(const base::xvaccount_t & account, const uint64_t height)
        {
            const std::string key_path = base::xvdbkey_t::create_account_span_key(account, height);
            return base::xvchain_t::instance().get_xdbstore()->get_value(key_path);
        }

        bool xvblockstore_impl::set_unit_proof(const base::xvaccount_t & account, const std::string& unit_proof, uint64_t height){
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->set_unit_proof(unit_proof, height);
        }
        
        const std::string xvblockstore_impl::get_unit_proof(const base::xvaccount_t & account, uint64_t height){
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->get_unit_proof(height);
        }
    };//end of namespace of vstore
};//end of namespace of top
