// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#include "xmetrics/xmetrics.h"
#include "xvblockhub.h"
#include "xvgenesis.h"
#include "xvledger/xvdbkey.h"

#ifdef __ALLOW_FORK_LOCK__
    #undef __ALLOW_FORK_LOCK__  // XTODO always allow store multi lock blocks
#endif
 
#ifndef __PERSIST_SAVE_UNIT_WITHOUT_INDEX__
    // #define __PERSIST_SAVE_UNIT_WITHOUT_INDEX__
#endif

#ifndef __LAZY_SAVE_UNIT_BLOCK_UNTIL_COMMIT__
    //#define __LAZY_SAVE_UNIT_BLOCK_UNTIL_COMMIT__  // XTODO not enable this feature now
#endif

//XTODO,turn on __CACHE_RAW_BLOCK_PTR_AT_FIRST_PLACE__ to improve cache effect but need test memory usage first
#ifndef __CACHE_RAW_BLOCK_PTR_AT_FIRST_PLACE__
    //#define __CACHE_RAW_BLOCK_PTR_AT_FIRST_PLACE__
#endif

namespace top
{
    namespace store
    {
        xblockacct_t::xblockacct_t(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr)
            :xvblockplugin_t(parent_obj,timeout_ms)
        {
            m_meta = NULL;
            m_blockdb_ptr = NULL;

            m_blockdb_ptr = xvbkdb_ptr;
            xassert(xvbkdb_ptr != NULL);
            xvbkdb_ptr->add_ref();

            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xblockacct_t, 1);
        }

        xblockacct_t::~xblockacct_t()
        {
            xinfo("xblockacct_t::destroy,account=%s objectid=% " PRId64 " ",
                  get_address().c_str(),
                  (int64_t)get_obj_id());

            XMETRICS_GAUGE_DATAOBJECT(metrics::dataobject_xblockacct_t, -1);

            close_blocks();
            m_blockdb_ptr->release_ref();
        }

        std::string xblockacct_t::dump() const  //just for debug purpose
        {
            // execute height fall behind check, should be deleted eventually
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"{account_id(%" PRIu64 "),account_addr=%s ->latest height for full=%" PRId64 ",connect=%" PRId64 ",commit=%" PRId64 ",lock=%" PRId64 " < cert=%" PRId64 ";}",
                get_xvid(), get_address().c_str(),m_meta->_highest_full_block_height,m_meta->_highest_connect_block_height,m_meta->_highest_commit_block_height,m_meta->_highest_lock_block_height,m_meta->_highest_cert_block_height);

            return std::string(local_param_buf);
        }

        bool  xblockacct_t::init_meta(const base::xvactmeta_t & account_meta)
        {
            xvblockplugin_t::init_meta(account_meta);
            m_meta = (base::xblockmeta_t*)get_block_meta();
            recover_meta(account_meta);
            
            xinfo("xblockacct_t::init_meta,account=%s objectid=% " PRId64 ",this=% " PRId64 ",meta=%s",
                    dump().c_str(),
                    get_obj_id(),this,m_meta->ddump().c_str());
            return true;
        }
        
        bool   xblockacct_t::recover_meta(const base::xvactmeta_t & account_meta)//recover at plugin level if possible
        {
            if(  (0 == account_meta.get_meta_process_id())
               ||(account_meta.get_meta_process_id() == base::xvchain_t::instance().get_current_process_id())
               )
            {
                return false; //nothing need recover since there not reboot yet
            }
            
            if(0 == m_meta->_highest_cert_block_height)
                return false; //nothing need recover for genesis
            
            bool recovered_something = false;
            if(m_meta->_highest_cert_block_height > 0)
            {
                const int64_t min_recover_height = m_meta->_highest_cert_block_height;
                const int64_t max_recover_height = min_recover_height + base::enum_account_save_meta_interval + 1;
                for(int64_t i = min_recover_height; i <= max_recover_height; ++i)
                {
                    if(load_index(i) > 0)//load_index may update_meta as well
                    {
                        recovered_something = true;
                        xwarn("xblockacct_t::recover_meta,recover block at height=% " PRId64 " of account(%s)",i,get_address().c_str());
                    }
                    else//stop recover if not load any block
                    {
                        break;
                    }
                }
            }
            
            //try recover full_block from highest commit block
            if(m_meta->_highest_commit_block_height > 0)
            {
                load_index(m_meta->_highest_commit_block_height);
                base::xauto_ptr<base::xvbindex_t> latest_commit(query_latest_index(base::enum_xvblock_flag_committed));
                if(latest_commit)//query_latest_index has been return a added-reference ptr
                {
                    if(latest_commit->get_height() > 0)
                    {
                        if(load_index(latest_commit->get_last_full_block_height()) > 0)
                            xwarn("xblockacct_t::recover_meta,recover full_block at height=% " PRId64 " of account(%s)",latest_commit->get_last_full_block_height(),get_address().c_str());
                    }
                }
            }
            
            return recovered_something;
        }
 
        bool  xblockacct_t::close(bool force_async)
        {
            if(is_close() == false)
            {
                xvblockplugin_t::close(force_async); //mark close status first
                xkinfo("xblockacct_t::close,account=%s,objectid=% " PRId64 " and this=% " PRId64 "ptr",dump().c_str(),get_obj_id(),this);

                //then clean all blocks at memory
                close_blocks();

                //TODO, retore following assert check after full_block enable
                xassert(m_meta->_highest_connect_block_height <= m_meta->_highest_commit_block_height);
                xassert(m_meta->_highest_commit_block_height  <= m_meta->_highest_lock_block_height);
                xassert(m_meta->_highest_lock_block_height    <= m_meta->_highest_cert_block_height);

                //debug test (commit-lock-connect) should be a reasonable range
                // xassert((m_meta->_highest_lock_block_height - m_meta->_highest_commit_block_height) <  128);
                //TODO, once let syn module save block through blockstore, need using below condition to replace above one
                //xdbgassert( (m_meta->_highest_commit_block_height - m_meta->_highest_connect_block_height) < 128);
            }
            return true;
        }

        const int  xblockacct_t::get_max_cache_size() const
        {
            //note: place code first but  please enable it later
            if(base::enum_xvblock_level_table == m_meta->_block_level)
                return (enum_max_cached_blocks << 1);//cache to max 128 block

            if(base::enum_xvblock_level_unit == m_meta->_block_level)
                return (enum_max_cached_blocks >> 2);//cache to max 8 block for unit

            return enum_max_cached_blocks;
        }
    
        const int  xblockacct_t::get_cached_size() const
        {
            return (int)m_all_blocks.size();
        }

        //clean unsed caches of account to recall memory
        bool xblockacct_t::clean_caches(bool clean_all,bool force_release_unused_block)
        {
            return clean_blocks(clean_all ? 0 : get_max_cache_size(),force_release_unused_block);//try all possible clean_caches
        }

        bool xblockacct_t::clean_blocks(const int keep_blocks_count,bool force_release_unused_block)
        {
            if((int)m_all_blocks.size() > keep_blocks_count)
            {
                if ((int)m_all_blocks.size() > keep_blocks_count * 3) {
                    // XTODO need fix this issue
                    xwarn("xblockacct_t::clean_blocks account:%s cache size:%u is more than 2 times of upper limmit:%u", get_account().c_str(), m_all_blocks.size(), keep_blocks_count);
                }

                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end();)//search from lowest hight to higher
                {
                    if((int)m_all_blocks.size() <= keep_blocks_count)//clean enough
                        break;

                    auto old_height_it = height_it; //copy first
                    ++height_it;                    //move next in advance

                    if(old_height_it->second.empty()) //clean empty first if have
                    {
                        m_all_blocks.erase(old_height_it);
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                        continue;
                    }

                    if(   (old_height_it->first != m_meta->_highest_full_block_height)    //keep latest_full_block
                       && (old_height_it->first != m_meta->_highest_commit_block_height)  //keep latest_committed block
                       && (old_height_it->first != m_meta->_highest_lock_block_height)    //keep latest_lock_block
                       && (old_height_it->first != m_meta->_highest_cert_block_height)    //keep latest_cert block
                       && (old_height_it->first != m_meta->_highest_connect_block_height))//keep latest_connect_block
                    {
                        auto & view_map = old_height_it->second;
                        #ifdef ENABLE_METRICS
                        auto erase_count = view_map.size();
                        #endif
                        for(auto it = view_map.begin(); it != view_map.end(); ++it)
                        {
                            //at entry of quit we need make sure everything is consist
                            update_meta_metric(it->second);  //udate other meta and connect info
                            
                            //store any modified blocks again
                            //may determine whether need do real save
                            write_block(it->second);
                            write_index(it->second);
                            
                            base::xvblock_t * raw_block = it->second->get_this_block();
                            if(raw_block != NULL) //force to cleanup prev/next ptr if have
                            {
                                raw_block->reset_prev_block(NULL);
                                raw_block->reset_next_block(NULL);
                            }
                            xdbg_info("xblockacct_t::clean_caches,blockptr=%llx,index=%s",it->second,it->second->dump().c_str());
                            
                            it->second->close();//disconnect from prev-block and next-block
                            it->second->release_ref();
                        }
                        //force to clean all prev_pr of next height
                        if(height_it != m_all_blocks.end())
                        {
                            auto & view_map = height_it->second;
                            for(auto it = view_map.begin(); it != view_map.end(); ++it)
                                it->second->reset_prev_block(NULL);
                        }
                        //erase the this iterator finally
                        m_all_blocks.erase(old_height_it);

                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1 * erase_count);

                    }
                }
            }
            else if(force_release_unused_block) //force release block that only hold by internal
            {
                //just return if dont cache block at first place
                #ifndef __CACHE_RAW_BLOCK_PTR_AT_FIRST_PLACE__
                    return true;
                #endif
                
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end();)//search from lowest hight to higher
                {
                    auto old_height_it = height_it; //copy first
                    ++height_it;                    //move next in advance
                    if(old_height_it->second.empty()) //clean empty first if have
                    {
                        m_all_blocks.erase(old_height_it);
                        
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                        
                        continue;
                    }

                    bool cleaned_one = false;
                    if(   (old_height_it->first != m_meta->_highest_full_block_height)    //keep latest_full_block
                       && (old_height_it->first <  m_meta->_highest_commit_block_height)  //keep latest_committed block
                       && (old_height_it->first != m_meta->_highest_lock_block_height)    //keep latest_lock_block
                       && (old_height_it->first != m_meta->_highest_cert_block_height)    //keep latest_cert block
                       && (old_height_it->first != m_meta->_highest_connect_block_height))//keep latest_connect_block
                    {
                        auto & view_map = old_height_it->second;
                        for(auto it = view_map.begin(); it != view_map.end(); ++it)
                        {
                            if(it->second->check_store_flag(base::enum_index_store_flag_non_index) == false)
                            {
                                //normal case for block with index associated
                                if(it->second->get_this_block() != NULL) //clean any block that just reference by index only
                                {
                                    if(it->second->get_this_block()->get_refcount() == 1)//no any other hold
                                    {
                                        xdbg_info("xblockacct_t::clean_caches,block=%s",it->second->dump().c_str());
                                        
                                        //store any modified blocks again if need
                                        write_block(it->second);
                                        write_index(it->second);
                                        
                                        it->second->reset_this_block(NULL);
                                        cleaned_one = true;
                                    }
                                }
                            }
                        }
                    }
                    if(cleaned_one)
                        break;
                }
            }
            return true;
        }

	bool  xblockacct_t::save_data()//must be protected by table ' lock
        {
            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end(); ++height_it)//search from lower height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it) //search from lower view#
                    {
                        //at entry of close we need make sure everything is consist
                        update_meta_metric(view_it->second);  //udate other meta and connect info
                        
                        //at entry of quit we need make sure everything is consist
                        //may determine whether need do real save
                        write_block(view_it->second);
                        write_index(view_it->second);
                    }
                }
            }
            return true;
        }

        void  xblockacct_t::close_blocks()
        {
            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end(); ++height_it)//search from lower height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it) //search from lower view#
                    {
                        //const uint64_t this_block_height = view_it->second->get_height();
                        //const int      this_block_flags  = view_it->second->get_block_flags();
                        xdbg_info("xblockacct_t::close_blocks,block=%s",view_it->second->dump().c_str());
                        
                        //at entry of close we need make sure everything is consist
                        update_meta_metric(view_it->second);  //udate other meta and connect info
                        //may determine whether need do real save
                        write_block(view_it->second);
                        write_index(view_it->second);
                        

                        view_it->second->close();//disconnect from prev-block and next-block,if have
                        view_it->second->release_ref();
                    }
                }

                XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1 * m_all_blocks.size());

                m_all_blocks.clear();
            }
        }

        //one api to get latest_commit/latest_lock/latest_cert for better performance
        bool   xblockacct_t::query_latest_index_list(base::xvbindex_t* & cert_block,base::xvbindex_t* & lock_block,base::xvbindex_t* & commit_block)
        {
            cert_block   = nullptr;
            lock_block   = nullptr;
            commit_block = nullptr;

            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.rbegin(); height_it != m_all_blocks.rend(); ++height_it)//search from highest height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        if( (cert_block == nullptr) && (view_it->second->check_block_flag(base::enum_xvblock_flag_authenticated)) )
                        {
                            view_it->second->add_ref();
                            cert_block = view_it->second;
                        }
                        if( (lock_block == nullptr) && (view_it->second->check_block_flag(base::enum_xvblock_flag_locked)) )
                        {
                            view_it->second->add_ref();
                            lock_block = view_it->second;
                        }
                        if( (commit_block == nullptr) && (view_it->second->check_block_flag(base::enum_xvblock_flag_committed)) )
                        {
                            view_it->second->add_ref();
                            commit_block = view_it->second;
                        }

                        if( (cert_block != nullptr) && (lock_block != nullptr) && (commit_block != nullptr) )
                            break;
                    }
                }
            }
            if(commit_block == nullptr)
                commit_block = load_genesis_index();//return ptr that already hold/added reference

            if(lock_block == nullptr)
            {
                lock_block   = commit_block;
                lock_block->add_ref();
            }
            if(cert_block == nullptr)
            {
                cert_block   = lock_block;
                cert_block->add_ref();
            }
            return true;
        }

        //query all blocks at target height, it might have mutiple certs at target height
        std::vector<base::xvbindex_t*> xblockacct_t::query_index(const uint64_t height)
        {
            std::vector<base::xvbindex_t*> all_blocks_at_height;
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    all_blocks_at_height.reserve(view_map.size()); //just reserved intead of resizing
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from higher view#
                    {
                        view_it->second->add_ref();
                        all_blocks_at_height.push_back(view_it->second);
                    }
                }
            }
            return all_blocks_at_height;
        }

        base::xvbindex_t*  xblockacct_t::query_index(const uint64_t height, const uint64_t viewid)
        {
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    if(0 != viewid)
                    {
                        for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                        {
                            if(view_it->second->get_viewid() == viewid)
                            {
                                view_it->second->add_ref();
                                return view_it->second;
                            }
                        }
                    }
                    else if(view_map.empty() == false) //choose most suitable one
                    {
                        base::xvbindex_t* highest_commit = NULL;
                        base::xvbindex_t* highest_lock = NULL;
                        base::xvbindex_t* highest_cert = NULL;
                        for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                        {
                            if( (highest_commit == NULL) && (view_it->second->check_block_flag(base::enum_xvblock_flag_committed)) )
                            {
                                highest_commit = view_it->second;
                            }

                            if( (highest_lock == NULL) && (view_it->second->check_block_flag(base::enum_xvblock_flag_locked)) )
                            {
                                highest_lock = view_it->second;
                            }

                            if( (highest_cert == NULL) && (view_it->second->check_block_flag(base::enum_xvblock_flag_authenticated)) )
                            {
                                highest_cert = view_it->second;
                            }
                        }
                        if(highest_commit != NULL)
                        {
                            highest_commit->add_ref();
                            return highest_commit;
                        }
                        if(highest_lock != NULL)
                        {
                            highest_lock->add_ref();
                            return highest_lock;
                        }
                        if(highest_cert != NULL)
                        {
                            highest_cert->add_ref();
                            return highest_cert;
                        }
                    }
                }
            }
            return nullptr;
        }

        base::xvbindex_t*  xblockacct_t::query_index(const uint64_t height, const std::string & blockhash)
        {
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        if(blockhash == view_it->second->get_block_hash())
                        {
                            view_it->second->add_ref();
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }

        //internal use only: query_block just check at cache layer and return raw ptr without added reference, so caller need use careful
        base::xvbindex_t*    xblockacct_t::query_index(const uint64_t target_height,base::enum_xvblock_flag request_flag)
        {
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(target_height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        if(view_it->second->check_block_flag(request_flag))
                        {
                            view_it->second->add_ref();
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }


        //internal use only: query_block just check at cache layer and return raw ptr without added reference, so caller need use careful
        base::xvbindex_t*    xblockacct_t::query_latest_index(base::enum_xvblock_flag request_flag)
        {
            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.rbegin(); height_it != m_all_blocks.rend(); ++height_it)//search from highest height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        if(view_it->second->check_block_flag(request_flag))
                        {
                            view_it->second->add_ref();
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }

        //internal use only: query_block just check at cache layer and return raw ptr without added reference, so caller need use careful
        base::xvbindex_t*    xblockacct_t::query_latest_index(base::enum_xvblock_class request_class)
        {
            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.rbegin(); height_it != m_all_blocks.rend(); ++height_it)//search from highest height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        if(view_it->second->get_block_class() == request_class)
                        {
                            view_it->second->add_ref();
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }

        base::xvbindex_t*   xblockacct_t::load_genesis_index()
        {
            return load_index(0,0);//load from db into cache
        }

        base::xvbindex_t*   xblockacct_t::load_latest_cert_index()
        {
            if(load_index(m_meta->_highest_cert_block_height) == 0)//load first
            {
                xwarn_err("xblockacct_t::load_latest_cert_index,fail to load height(%d)",m_meta->_highest_cert_block_height);
                while(m_meta->_highest_cert_block_height > 1) //fallback
                {
                    m_meta->_highest_cert_block_height -= 1;
                    if(load_index(m_meta->_highest_cert_block_height) > 0)
                        break;
                }
                xwarn_err("xblockacct_t::load_latest_cert_index,fallback to load height(%d)",m_meta->_highest_cert_block_height);
            }
            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_flag_authenticated);//then query again
            if(result != nullptr)//query_latest_index has been return a added-reference ptr
                return result;

            return load_genesis_index();
        }

        base::xvbindex_t*    xblockacct_t::load_latest_locked_index()
        {
            if(load_index(m_meta->_highest_lock_block_height) == 0)//load first
            {
                xwarn_err("xblockacct_t::load_latest_locked_index,fail to load height(%d)",m_meta->_highest_lock_block_height);
                while(m_meta->_highest_lock_block_height > 1)
                {
                    m_meta->_highest_lock_block_height -= 1;
                    if(load_index(m_meta->_highest_lock_block_height) > 0)
                        break;
                }
                xwarn_err("xblockacct_t::load_latest_locked_index,fallback to load height(%d)",m_meta->_highest_lock_block_height);
            }
            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_flag_locked);
            if(result != nullptr)//query_latest_index has been return a added-reference ptr
                return result;

            return load_genesis_index();
        }

        base::xvbindex_t*   xblockacct_t::load_latest_committed_index()
        {
            if(load_index(m_meta->_highest_commit_block_height) == 0)//load first
            {
                xwarn_err("xblockacct_t::load_latest_committed_index,fail to load height(%d)",m_meta->_highest_commit_block_height);
                while(m_meta->_highest_commit_block_height > 1)
                {
                    m_meta->_highest_commit_block_height -= 1;
                    if(load_index(m_meta->_highest_commit_block_height) > 0)
                        break;
                }
                xwarn_err("xblockacct_t::load_latest_committed_index,fallback to load height(%d)",m_meta->_highest_commit_block_height);
            }
            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_flag_committed);
            if(result != nullptr)//query_latest_index has been return a added-reference ptr
                return result;

            return load_genesis_index();
        }

        //every connected block required committed
        base::xvbindex_t*  xblockacct_t::load_latest_connected_index() //block has connected to genesis or latest full-block
        {
            if(load_index(m_meta->_highest_connect_block_height) == 0)//load first
            {
                xwarn("xblockacct_t::load_latest_connected_index,fail load block at height(%" PRIu64 ") of account(%s)",m_meta->_highest_connect_block_height,get_address().c_str());

                load_index(m_meta->_highest_full_block_height);//full-block must be connected status
                for(uint64_t i = 1; i <= 3; ++i)//try forwarded 3 blocks
                {
                    if(m_meta->_highest_connect_block_height > i)
                    {
                        base::xvbindex_t* alternative = load_index(m_meta->_highest_connect_block_height - i, base::enum_xvblock_flag_committed);
                        if(alternative != NULL)//load_index has been return a added-reference ptr
                        {
                            m_meta->_highest_connect_block_height = alternative->get_height();
                            m_meta->_highest_connect_block_hash   = alternative->get_block_hash();
                            return alternative;
                        }
                    }
                }
            }
            else
            {
                base::xauto_ptr<base::xvbindex_t> connectindex(query_index(m_meta->_highest_connect_block_height, base::enum_xvblock_flag_committed));
                if (connectindex != nullptr
                    && m_meta->_highest_connect_block_height == connectindex->get_height()
                    && m_meta->_highest_connect_block_hash != connectindex->get_block_hash())
                {
                    m_meta->_highest_connect_block_hash = connectindex->get_block_hash();
                    xwarn("xblockacct_t::load_latest_connected_index,recover _highest_connect_block_hash,account=%s,height=%ld",get_account().c_str(), m_meta->_highest_connect_block_height);
                }
            }

            bool full_search_more = true;
            if(full_search_more) //search more
            {
                const uint64_t old_highest_connect_block_height = m_meta->_highest_connect_block_height;

                //try connect height jump to full height
                if (m_meta->_highest_connect_block_height < m_meta->_highest_full_block_height)
                {
                    if (load_index(m_meta->_highest_full_block_height) > 0)//full-block must be connected status
                    {
                        base::xauto_ptr<base::xvbindex_t> fullindex(query_index(m_meta->_highest_full_block_height, base::enum_xvblock_flag_committed));
                        if(fullindex != nullptr) //dont have commited block
                        {
                            m_meta->_highest_connect_block_height = fullindex->get_height();
                            m_meta->_highest_connect_block_hash   = fullindex->get_block_hash();
                            xinfo("xblockacct_t::load_latest_connected_index,account=%s,navigate to full,old_height=%" PRIu64 ",new_height=%" PRIu64 ",full_height=%" PRIu64 " ",
                                get_account().c_str(), old_highest_connect_block_height,m_meta->_highest_connect_block_height,m_meta->_highest_full_block_height);
                        }
                        else
                        {
                            xerror("xblockacct_t::load_latest_connected_index,account=%s,fail query fullindex,full_height=%" PRIu64 ",connnect_height=%" PRIu64 "",
                                get_account().c_str(), m_meta->_highest_full_block_height, m_meta->_highest_connect_block_height);
                        }
                    }
                    else
                    {
                        xerror("xblockacct_t::load_latest_connected_index,account=%s,fail load fullindex,full_height=%" PRIu64 ",connnect_height=%" PRIu64 "",
                            get_account().c_str(), m_meta->_highest_full_block_height, m_meta->_highest_connect_block_height);
                    }
                }

                for(uint64_t h = m_meta->_highest_connect_block_height + 1; h <= m_meta->_highest_commit_block_height; ++h)
                {
                    const uint64_t try_height = m_meta->_highest_connect_block_height + 1;
                    if(load_index(try_height) == 0) //missed block
                        break;

                    base::xauto_ptr<base::xvbindex_t> next_commit(query_index(try_height, base::enum_xvblock_flag_committed));
                    if(!next_commit) //dont have commited block
                        break;

                    if(next_commit->get_block_class() == base::enum_xvblock_class_full)
                    {
                        m_meta->_highest_connect_block_height = next_commit->get_height();
                        m_meta->_highest_connect_block_hash   = next_commit->get_block_hash();
                    }
                    else if(next_commit->get_height() == (m_meta->_highest_connect_block_height + 1))
                    {
                        if (next_commit->get_last_block_hash() == m_meta->_highest_connect_block_hash)
                        {
                            m_meta->_highest_connect_block_height = next_commit->get_height();
                            m_meta->_highest_connect_block_hash   = next_commit->get_block_hash();
                        }
                        else
                        {
                            xerror("xblockacct_t::load_latest_connected_index,account=%s,hash mismatch last=%s,commit=%s,connnect_height=%" PRIu64 "",
                                get_account().c_str(), m_meta->_highest_connect_block_hash.c_str(), next_commit->get_last_block_hash().c_str(), m_meta->_highest_connect_block_height);
                        }
                    }
                    else
                    {
                        break;
                    }
                }
                const int  block_connect_step  = (int)(m_meta->_highest_connect_block_height - old_highest_connect_block_height);
                if(block_connect_step > 0)
                    xinfo("xblockacct_t::load_latest_connected_index,account=%s,navigate step(%d) to old_height=%" PRIu64 ",new_height=%" PRIu64 ",commit_height=%" PRIu64 " ",
                        get_account().c_str(), block_connect_step,old_highest_connect_block_height,m_meta->_highest_connect_block_height,m_meta->_highest_commit_block_height);
            }

            //connected block must be committed as well
            base::xvbindex_t* result = query_index(m_meta->_highest_connect_block_height,base::enum_xvblock_flag_committed);
            if(result != nullptr)
            {
                return result;
            }
            return load_genesis_index();
        }

        base::xvbindex_t*  xblockacct_t::load_latest_full_index()
        {
            if(load_index(m_meta->_highest_full_block_height) == 0)//load first
                load_index(m_meta->_highest_commit_block_height);//fallback

            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_class_full);
            if(result != nullptr)
                return result;//query_latest_index has added reference,here just return ptr

            base::xauto_ptr<base::xvbindex_t> latest_commit(query_latest_index(base::enum_xvblock_flag_committed));
            if(latest_commit)//query_latest_index has been return a added-reference ptr
            {
                if(latest_commit->get_height() > 0)
                {
                    base::xvbindex_t* result = load_index(latest_commit->get_last_full_block_height(),0);
                    if(result != NULL)//load_index has been return a added-reference ptr
                    {
                        m_meta->_highest_full_block_height = result->get_height();
                        return result;
                    }
                }
            }
            //bottom line from genesis block
            return load_genesis_index();
        }

        base::xvbindex_t*  xblockacct_t::load_latest_committed_full_index()
        {
            load_index(m_meta->_highest_full_block_height);
            base::xvbindex_t* result = query_index(m_meta->_highest_full_block_height,base::enum_xvblock_flag_committed);
            if(result != nullptr)
                return result;

            //bottom line from genesis block
            return load_genesis_index();
        }

        //caller respond to release those returned ptr
        bool    xblockacct_t::load_latest_index_list(base::xvbindex_t* & cert_block,base::xvbindex_t* & lock_block,base::xvbindex_t* & commit_block)
        {
            cert_block      = NULL;
            lock_block      = NULL;
            commit_block    = NULL;

            //ptr has been an added-reference
            commit_block = load_latest_committed_index();
            lock_block   = load_latest_locked_index();
            cert_block   = load_latest_cert_index();

            if(cert_block->get_height() == (lock_block->get_height() + 1) )
            {
                if(cert_block->get_last_block_hash() != lock_block->get_block_hash())//rare case
                {
                    auto height_it = m_all_blocks.find(cert_block->get_height());
                    if(height_it != m_all_blocks.end())
                    {
                        auto & view_map  = height_it->second;
                        for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it)
                        {
                            if(view_it->second->get_last_block_hash() == lock_block->get_block_hash())
                            {
                                //release prev holded on
                                cert_block->release_ref();
                                cert_block = NULL;
                                //retake new one
                                view_it->second->add_ref();
                                cert_block = view_it->second;
                                break;
                            }
                        }
                    }
                    xassert(cert_block->get_last_block_hash() == lock_block->get_block_hash());
                }
            }
            xdbg_info("xblockacct_t::load_latest_index_list succ retry. account=%s,cert=%s,lock=%s vs meta(%s)",
                      get_account().c_str(), cert_block->dump().c_str(), lock_block->dump().c_str(),m_meta->ddump().c_str());
            
            if(m_meta->_highest_commit_block_height > 0)
            {
                xassert(cert_block->get_height() > 0);
                xassert(lock_block->get_height() > 0);
                xassert(commit_block->get_height() > 0);
            }
            else if(m_meta->_highest_lock_block_height > 0)
            {
                xassert(cert_block->get_height() > 0);
                xassert(lock_block->get_height() > 0);
            }
            else if(m_meta->_highest_cert_block_height > 0)
            {
                xassert(cert_block->get_height() > 0);
            }
            return true;
        }

        //load every index of block at target_height into cache layer
        int   xblockacct_t::load_index(const uint64_t target_height)
        {
            auto it = m_all_blocks.find(target_height);
            if(it == m_all_blocks.end())//load all at certain height
            {
                std::vector<base::xvbindex_t*> _indexes(read_index(target_height));
                if(_indexes.empty() == false) //found index at db
                {
                    for(auto it = _indexes.begin(); it != _indexes.end(); ++it)
                    {
                        cache_index(*it);      //cache it -> link-neighbor->mark-connect->update meta

                        //at entry of load, check connected_flag and meta info
                        update_meta_metric(*it); //update other meta and connect info

                        (*it)->release_ref();   //release ptr that reference added by read_index_from_db
                    }
                   
                    XMETRICS_TIME_RECORD_KEY("blockstore_load_block_time", get_account() + ":" + std::to_string(target_height));
                    
                    return (int)_indexes.size();
                }
                
                //genesis block but dont have data at DB, create it ondemand
                if(0 == target_height)
                {
                    base::xauto_ptr<base::xvblock_t> generis_block(xgenesis_block::create_genesis_block(get_account()));
                    store_block(generis_block.get());
                    return 1; //genesis always be 1 block at height(0)
                }
                xdbg("xblockacct_t::load_index(),fail found index for addr=%s at height=%" PRIu64 "", get_account().c_str(), target_height);
                return 0;
            }
            return (int)it->second.size(); //found existing ones
        }

        size_t   xblockacct_t::load_index_by_height(const uint64_t target_height)
        {
            auto it = m_all_blocks.find(target_height);
            if(it == m_all_blocks.end())//load all at certain height
            {
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
                std::vector<base::xvbindex_t*> _indexes(read_index(target_height));
                if(_indexes.empty() == false) //found index at db
                {
                    for(auto it = _indexes.begin(); it != _indexes.end(); ++it)
                    {
                        cache_index(*it);      //cache it -> link-neighbor->mark-connect->update meta

                        //at entry of load, check connected_flag and meta info
                        update_meta_metric(*it); //update other meta and connect info

                        (*it)->release_ref();   //release ptr that reference added by read_index_from_db
                    }
                    
                    XMETRICS_TIME_RECORD_KEY("blockstore_load_block_time", get_account() + ":" + std::to_string(target_height));
                    
                    return (int)_indexes.size();
                }
                //genesis block but dont have data at DB, create it ondemand
                if(0 == target_height)
                {
                    xwarn("xblockacct_t::load_index(),fail found index for addr=%s at height=%" PRIu64 "", get_account().c_str(), target_height);
                    return 0;
                }
            }

            XMETRICS_GAUGE(metrics::blockstore_index_load, 1);

            return (int)it->second.size(); //found existing ones
        }

        //load specific index of block with view_id
        base::xvbindex_t*     xblockacct_t::load_index(const uint64_t target_height,const uint64_t view_id, const int atag)
        {
            //#1: query_block() at cache layer
            //#2: check certain index with viewid at height
            //#3: load_index_from_db(target_height)
            //#4: for genesis block case
            base::xvbindex_t* target_block = query_index(target_height, view_id);
            if(target_block != NULL) //the ptr has been add reference by query_index
            {
              
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 1);
               
                return target_block;//found at cache layer
            }
            else
            {
              
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
              
            }
            if(load_index(target_height) > 0)//load from db
                target_block = query_index(target_height, view_id);//query again after loaded

            if(NULL == target_block)
                xwarn("xblockacct_t::load_index(viewid),faild to load index for addr=%s at height=%ld", get_account().c_str(), target_height);

            return target_block;
        }

        //load specific index of block with block hash
        base::xvbindex_t*     xblockacct_t::load_index(const uint64_t target_height,const std::string & block_hash, const int atag)
        {
            //#1: query_index() at cache layer
            //#2: check certain index with blockhash at height
            //#3: load_index_from_db(target_height)
            //#4: for genesis block case
            base::xvbindex_t* target_block = query_index(target_height, block_hash);
            if(target_block != NULL) //the ptr has been add reference by query_index
            {
               
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 1);

                return target_block;//found at cache layer
            }
            else
            {
                
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
               
            }

            if(load_index(target_height) > 0)//load from db
                target_block = query_index(target_height, block_hash);//query again after loaded

            if(NULL == target_block)
                xdbg("xblockacct_t::load_index(hash),faild to load index for addr=%s at height=%ld", get_account().c_str(), target_height);

            return target_block;
        }

        //load specific index of block with block hash
        base::xvbindex_t*     xblockacct_t::load_index(const uint64_t target_height,base::enum_xvblock_flag request_flag, const int atag)
        {
            //#1: query_index() at cache layer
            //#2: check certain index with flag at height
            //#3: load_index_from_db(target_height)
            //#4: for genesis block case
            base::xvbindex_t* target_block = query_index(target_height, request_flag);

            if(target_block != NULL)//the ptr has been add reference by query_index
            {
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 1);
              
                return target_block;//found at cache layer
            }
            else
            {
               
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
               
            }

            if(load_index(target_height) > 0)//load from db
                target_block = query_index(target_height, request_flag);//query again after loaded

            if(NULL == target_block)
                xdbg("xblockacct_t::load_index(flag),faild to load index for addr=%s at height=%ld", get_account().c_str(), target_height);

            return target_block;
        }

        std::vector<base::xvbindex_t*>  xblockacct_t::load_indexes(const uint64_t target_height)//load indexes from db for height
        {
            load_index(target_height); //load first
            return query_index(target_height);//then query
        }

        bool   xblockacct_t::store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks) //better performance
        {
            //std::sort(batch_store_blocks.begin(),batch_store_blocks.end(),base::less_by_block_height());
            for(auto it : batch_store_blocks)
            {
                //here do account check since xvblockstore_impl not do batch check
                if((it != nullptr) && (it->get_account() == get_account()) )
                    store_block(it);
            }
            return true;
        }

        bool    xblockacct_t::store_committed_unit_block(base::xvblock_t* new_raw_block)
        {
            base::xauto_ptr<base::xvbindex_t> exist_cert(load_index(new_raw_block->get_height(),new_raw_block->get_block_hash()));
            if(exist_cert) //found duplicated ones
            {
                if (!exist_cert->check_block_flag(base::enum_xvblock_flag_committed)) {
                    // check if pre block is committed, update it if not.
                    if (new_raw_block->get_height() > 1) {
                        base::xauto_ptr<base::xvbindex_t> pre_idx(load_index(new_raw_block->get_height() - 1, new_raw_block->get_last_block_hash()));
                        if (pre_idx != nullptr && !pre_idx->check_block_flag(base::enum_xvblock_flag_committed)) {
                            pre_idx->set_block_flag(base::enum_xvblock_flag_locked);
                            pre_idx->set_block_flag(base::enum_xvblock_flag_committed);
                            update_bindex(pre_idx.get());
                            xinfo("xblockacct_t::store_committed_unit_block update pre index,store_block,done for pre_idx(%s),dump:%s", pre_idx->dump().c_str(), dump().c_str());
                        }
                    }

                    exist_cert->set_block_flag(base::enum_xvblock_flag_locked);
                    exist_cert->set_block_flag(base::enum_xvblock_flag_committed);
                    update_bindex(exist_cert.get());
                    xinfo("xblockacct_t::store_committed_unit_block update index,store_block,done for block(%s),dump:%s", new_raw_block->dump().c_str(), dump().c_str());
                } else {
                    xwarn("xblockacct_t::store_committed_unit_block already committed,block(%s),dump:%s", new_raw_block->dump().c_str(), dump().c_str());
                }
                return true;
            }
            //first do store block
            bool ret = store_block(new_raw_block);
            if(!ret)
            {
                xwarn("xblockacct_t::store_committed_unit_block,fail-store block(%s)", new_raw_block->dump().c_str());
            }
            return true;
        }

        bool   xblockacct_t::try_update_account_index(uint64_t height, uint64_t viewid, bool update_pre_block)
        {
            base::xauto_ptr<base::xvbindex_t> exist_cert(load_index(height, viewid));
            if (exist_cert == nullptr) {
                xinfo("xblockacct_t::try_update_account_index index not found:account:%s,height:%llu,view:%llu", get_address().c_str(), height, viewid);
                return false;
            }

            bool ret = true;
            if (update_pre_block && height > 1) {
                base::xauto_ptr<base::xvbindex_t> exist_cert2(load_index(height - 1, exist_cert->get_last_block_hash()));
                if (exist_cert2 == nullptr) {
                    xinfo("xblockacct_t::try_update_account_index index not found:account:%s,height:%llu,hash:%s", get_address().c_str(), height - 1, exist_cert->get_last_block_hash().c_str());
                    ret = false;
                } else {
                    exist_cert2->set_block_flag(base::enum_xvblock_flag_locked);
                    exist_cert2->set_block_flag(base::enum_xvblock_flag_committed);

                    update_bindex(exist_cert2.get());
                    xinfo("xblockacct_t::try_update_account_index succ:account:%s,height:%llu,hash:%s", get_address().c_str(), height - 1, exist_cert->get_last_block_hash().c_str());
                }
            }

            exist_cert->set_block_flag(base::enum_xvblock_flag_locked);
            exist_cert->set_block_flag(base::enum_xvblock_flag_committed);
            update_bindex(exist_cert.get());
            xinfo("xblockacct_t::try_update_account_index succ:account:%s,height:%llu,view:%llu", get_address().c_str(), height, viewid);
            return ret;
        }

        void xblockacct_t::update_bindex(base::xvbindex_t* this_block)
        {
            rebase_chain_at_height(this_block->get_height()); //resolve other block of lower-weight thans this
            if(this_block->is_close() == false)
            {
                update_meta_metric(this_block);//update meta since block has change status

                //may double check whether need save again
                write_block(this_block);
                write_index(this_block);
                
                //push event at end
                push_event(enum_blockstore_event_committed, this_block);//fire event for commit
            }
        }

        //physical store and cache seperately
        /* 3 rules for managing cache
         #1. clean blocks of lower stage when higher stage coming. stage include : cert, lock and commit
         #2. only allow one block at same height for the locked or committed block,in other words it allow mutiple cert-only blocks
         #3. not allow overwrite block with newer/more latest block at same height and same stage
         */
        bool    xblockacct_t::store_block(base::xvblock_t* new_raw_block)
        {
            if(nullptr == new_raw_block)
                return false;

            XMETRICS_GAUGE(metrics::store_block_call, 1);


            base::xauto_ptr<base::xvbindex_t> exist_cert( query_index(new_raw_block->get_height(),new_raw_block->get_block_hash()));
            if(exist_cert) //found duplicated ones
            {
                if(exist_cert->check_block_flag(base::enum_xvblock_flag_stored))//if fully stored
                {
                    //transfer flag of unpacked to existing if need
                    if(new_raw_block->check_block_flag(base::enum_xvblock_flag_unpacked))
                        exist_cert->set_block_flag(base::enum_xvblock_flag_unpacked);

                    return false;
                }
            }

            #ifdef DEBUG
            if(   (false == new_raw_block->is_input_ready(true))
               || (false == new_raw_block->is_output_ready(true))
               || (false == new_raw_block->is_deliver(true)) )//must have full valid data and has mark as enum_xvblock_flag_authenticated
            {
                xwarn("xblockacct_t::store_block,undevlier block=%s",new_raw_block->dump().c_str());
                return false;
            }
            #else //quick check for release mode
            if(   (false == new_raw_block->is_input_ready(false))
               || (false == new_raw_block->is_output_ready(false))
               || (false == new_raw_block->is_deliver(true)) )//must have full valid data and has mark as enum_xvblock_flag_authenticated
            {
                xwarn("xblockacct_t::store_block,undevlier block=%s",new_raw_block->dump().c_str());
                return false;
            }
            #endif

            // TODO(jimmy) should store and execute genesis block
            if(new_raw_block->get_height() == 1 && m_meta->_highest_connect_block_hash.empty())
            {
                // meta is not 100% reliable, query to ensure the existence of genesis block
                std::vector<base::xvbindex_t*> _indexes(read_index(0));
                if(_indexes.empty())//if not existing at cache
                {
                    base::xauto_ptr<base::xvblock_t> generis_block(xgenesis_block::create_genesis_block(get_account()));
                    store_block(generis_block.get());
                } else {
                    for (auto it = _indexes.begin(); it != _indexes.end(); ++it) {
                        (*it)->release_ref();   //release ptr that reference added by read_index_from_db
                    }
                }
            }

            xdbg("xblockacct_t::store_block,prepare for block=%s,cache_size:%zu,dump=%s",new_raw_block->dump().c_str(), m_all_blocks.size(), dump().c_str());
          
            XMETRICS_TIME_RECORD_KEY("blockstore_store_block_time", new_raw_block->get_account() + ":" + std::to_string(new_raw_block->get_height()));
           

            //#1:cache_block() ->link neighbor -> mark_connect_flag() -> update metric
            //#2:connect_block() ->process_block()
            //#3:save_index_to_db() -->save index-entry to db
            //#4:save_block_to_db() -->save raw-block to db

            base::xvbindex_t * final_cached_index = new_index(new_raw_block);//final_cached_ptr is a raw ptr that just valid at this moment
            if(final_cached_index != nullptr) //insert/update successful,or find duplicated one
            {
                //update block_level for first block
                if(m_meta->_block_level == (uint8_t)-1)
                {
                    m_meta->_block_level = new_raw_block->get_block_level();
                }
                update_meta_metric(final_cached_index); //update other meta and connect info

                #ifdef __CACHE_RAW_BLOCK_PTR_AT_FIRST_PLACE__
                bool keep_raw_block_ptr = true; //init to cache raw block
                #else
                bool keep_raw_block_ptr = false;
                #endif
                if(!final_cached_index->is_table_address()) // table still has bindex
                {
                    //just store raw-block without index
                    if(final_cached_index->check_store_flag(base::enum_index_store_flag_non_index))
                    {
                        keep_raw_block_ptr = true; //force keep raw block for non_index case
                        if(final_cached_index->get_this_block() == NULL)
                            final_cached_index->reset_this_block(new_raw_block);//force to bind raw block ptr
                        
                        #ifdef __LAZY_SAVE_UNIT_BLOCK_UNTIL_COMMIT__
                        //lazy to save raw block unti committed or closing
                        if(final_cached_index->check_block_flag(base::enum_xvblock_flag_committed))
                        #endif
                        {
                            //write_block_to_db may do double-check whether raw block not stored yet
                            write_block(final_cached_index);
                            
                            //we not save index seperately
                            final_cached_index->reset_modify_flag(); //clean up modified flags
                            //NOTE: raw_block ptr must be valid at all time
                        }
                    }
                    else //model of store block and index
                    {
                        //write_block_to_db may do double-check whether raw block not stored yet
                        write_block(final_cached_index,new_raw_block);
                        
                        #ifdef __LAZY_SAVE_UNIT_BLOCK_UNTIL_COMMIT__
                        //lazy to save index unti committed or closing
                        if(final_cached_index->check_block_flag(base::enum_xvblock_flag_committed))
                        #endif
                        {
                            //try save index finally
                            write_index(final_cached_index); //save index then
                        }
                        //Note: reset_this_block(NULL) here if want save memory more and not care cache effect
                    }
                }
                else //instant save block and index for table/book blocks
                {
                    //write_block_to_db may do double-check whether raw block not stored yet
                    write_block(final_cached_index,new_raw_block);
                    write_index(final_cached_index); //save index then
                }

                xinfo("xblockacct_t::store_block,done for block,cache_size:%zu,new_raw_block=%s,dump=%s",m_all_blocks.size(), new_raw_block->dump().c_str(), dump().c_str());
                
                //update meta right now
                if(new_raw_block->get_height() != 0)//genesis block might be created by load_index
                    update_meta();
                
                if(false == keep_raw_block_ptr)
                    final_cached_index->reset_this_block(NULL);

                return true;
            }

            xinfo("xblockacct_t::store_block,cache index fail.block=%s", new_raw_block->dump().c_str());
            return false;
        }

        bool    xblockacct_t::delete_block(base::xvblock_t* block_ptr)//return error code indicate what is result
        {
            if(nullptr == block_ptr)
                return false;

            if(block_ptr->get_height() == 0)
                return false; //not allow delete genesis block

            XMETRICS_GAUGE(metrics::store_block_delete, 1);

            xkinfo("xblockacct_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ")",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid());

            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(block_ptr->get_height());
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    auto view_it = view_map.find(block_ptr->get_viewid());
                    if(view_it != view_map.end())
                    {
                        //delete data at DB first
                        get_blockdb_ptr()->delete_block(view_it->second);

                        //remove entry then
                        view_it->second->close();
                        view_it->second->release_ref();
                        view_map.erase(view_it);
                    }
                    if(view_map.empty())
                    {
                        m_all_blocks.erase(height_it);

                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);

                    }

                }
            }
            return true;
        }

        bool    xblockacct_t::delete_block(const uint64_t height)//return error code indicate what is result
        {
            if(height == 0)
                return false; //not allow delete genesis block

            xkinfo("xblockacct_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ")",get_account_obj()-> get_chainid(),get_account().c_str(),height);

            //allow delete outdated blocks
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    if(view_map.empty())
                    {
                        m_all_blocks.erase(height_it);
                        
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                      
                    }
                    else
                    {
                        for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it)
                        {
                            //delete data at DB first
                            get_blockdb_ptr()->delete_block(view_it->second);
                            //remove entry then
                            view_it->second->close();
                            view_it->second->release_ref();
                        }
                        m_all_blocks.erase(height_it);
                    }
                }
            }
            return true;
        }

        //return cached ptr if successful inserted into cache,otherwise return nullptr,which may bring better performance
        base::xvbindex_t*   xblockacct_t::cache_index(base::xvbindex_t* this_block)
        {
            if(nullptr == this_block)
                return nullptr;

            //note: emplace return a pair<iterator,bool>, value of bool indicate whether inserted or not, value of iterator point to inserted it
            auto height_map_pos  = m_all_blocks.emplace(this_block->get_height(),std::map<uint64_t,base::xvbindex_t*>());
            auto & target_height_map     = height_map_pos.first->second;//hight_map_pos.first->first is height, and hight_map_pos.first->second is viewmap
            return cache_index(this_block,target_height_map);
        }

        //return cached ptr if successful inserted into cache,otherwise return nullptr,which may bring better performance
        base::xvbindex_t*   xblockacct_t::cache_index(base::xvbindex_t* this_block,std::map<uint64_t,base::xvbindex_t*> & target_height_map)
        {
            if(nullptr == this_block)
                return nullptr;

            xdbg("xblockacct_t::cache_index,prepare for block,cache_size:%zu",m_all_blocks.size());

            auto existing_view_iterator = target_height_map.find(this_block->get_viewid());
            if(existing_view_iterator != target_height_map.end())//apple rule#4 by reuse existing iterator and replace by new value
            {
                xdbg("xblockacct_t::cache_index, find same viewid block(%s)",this_block->dump().c_str());
                base::xvbindex_t* existing_block = existing_view_iterator->second;
                if(this_block != existing_block) //found same one but modified
                {
                    //ensure only one valid in the map
                    if(existing_block->get_block_hash() != this_block->get_block_hash())//safety check
                    {
                        xerror("xblockacct_t::cache_index,fail-hash changed for block with exist height(%" PRIu64 ") and view#=%" PRIu64 " vs new block=%s",this_block->get_height(),existing_block->get_viewid(),this_block->dump().c_str());
                        return nullptr;
                    }

                    //apply rule#3. not allow overwrite block with newer/more latest block at same height and same stage
                    const int existing_block_flags = existing_block->get_block_flags();
                    const int new_block_flags      = this_block->get_block_flags();
                    if( (new_block_flags & base::enum_xvblock_flag_unpacked) != 0)//merge unpacket flag into existing one
                        existing_block->set_block_flag(base::enum_xvblock_flag_unpacked);

                    if(  (existing_block_flags == new_block_flags)
                       ||(existing_block_flags & base::enum_xvblock_flags_high4bit_mask) >= (new_block_flags & base::enum_xvblock_flags_high4bit_mask)
                       ) //outdated one try to overwrite newer one,abort it
                    {
                        if(existing_block_flags != new_block_flags)
                            xdbg_info("xblockacct_t::cache_index,warn-try to overwrite newer block with flags(0x%x) by outdated block=%s",existing_block->get_block_flags(),this_block->dump().c_str());

                        //since found the duplicated one, we need let caller know this fact,so tranfer flag of stored to new index
                        *this_block = *existing_block; //transfer all existing info into new one
                        return nullptr;
                    }
                    //now combine flags
                    existing_block->reset_block_flags(existing_block_flags | (new_block_flags & base::enum_xvblock_flags_high4bit_mask));//merge flags(just for high4bit)
                    existing_block->set_modified_flag(); //add flag since it merged flags

                    //since found the duplicated one, we need let caller know this fact,so tranfer flag of stored to new index
                    *this_block = *existing_block; //transfer all existing info into new one


                    XMETRICS_GAUGE(metrics::blockstore_cache_block_total, 1);

                    xdbg("xblockacct_t::cache_index,finally update block=%s of account=%s", this_block->dump().c_str(), m_meta->ddump().c_str());

                    return existing_block;//indicate at least has changed flags
                }
                return nullptr; //nothing changed
            }
            else //insert a brand new entry
            {
                this_block->add_ref();  //hold reference now
                auto view_map_pos = target_height_map.emplace(this_block->get_viewid(),this_block);
                xassert(view_map_pos.second); //insert successful
                
                this_block->set_modified_flag(); //add flag since it brand inserted
                if(target_height_map.size() == 1) //assign main-entry for first one
                {
                    this_block->set_store_flag(base::enum_index_store_flag_main_entry);
                }

                link_neighbor(this_block); //link as neighbor first

               
                XMETRICS_GAUGE(metrics::blockstore_cache_block_total, 1);
                
                xdbg("xblockacct_t::cache_index,finally cached block=%s of account=%s", this_block->dump().c_str(), m_meta->ddump().c_str());
                return this_block;
            }
        }

        //just connect prev and next index in the cache list
        bool    xblockacct_t::link_neighbor(base::xvbindex_t* this_block)
        {
            if(NULL == this_block)
                return false;

            auto this_block_height_it = m_all_blocks.find(this_block->get_height());
            if(this_block_height_it == m_all_blocks.end())
                return false;

            if(this_block->get_height() > 0)
            {
                if( (this_block->get_prev_block() == NULL) || this_block->get_prev_block()->is_close() )
                {
                    //try to link to previous block: previous block <--- this_block
                    if(this_block_height_it != m_all_blocks.begin()) //must check first
                    {
                        auto  it_prev_height = this_block_height_it;  //copy first
                        --it_prev_height;                             //modify iterator
                        if(this_block->get_height() == (it_prev_height->first + 1)) //found previous map
                        {
                            for(auto it = it_prev_height->second.rbegin(); it != it_prev_height->second.rend(); ++it)//search from higher view
                            {
                                if(this_block->reset_prev_block(it->second))//previous block <--- this_block successful
                                {
                                    break;
                                }
                            }
                        }
                    }
                    if(this_block->get_prev_block() == nullptr) //not connect any prev_block right now
                    {
                        xdbg("xblockacct_t::link_neighbor,block not connect the prev-one yet for block=%s",this_block->dump().c_str());
                    }
                }
            }

            //try to link to next block: this_block  <---next block
            auto it_next = this_block_height_it;  //copy first
            ++it_next;                            //modify iterator
            if( (it_next != m_all_blocks.end()) && (it_next->first == (this_block->get_height() + 1)) ) //if found next set
            {
                for(auto it = it_next->second.rbegin(); it != it_next->second.rend(); ++it)//search from higher view
                {
                    it->second->reset_prev_block(this_block);//this_block  <---next block successful
                }
            }
            return true;
        }

        //note: genesis block must has been  connected-status
        bool  xblockacct_t::full_connect_to(base::xvbindex_t* this_block)
        {
            if(NULL == this_block)
                return false;

            const uint64_t this_block_height = this_block->get_height();
            const int      this_block_flags  = this_block->get_block_flags();
            // only committed block can mark connected.
            if((this_block_flags & base::enum_xvblock_flag_committed) == 0)
                return false;

            const uint64_t old_highest_connect_block_height = m_meta->_highest_connect_block_height;
            if((0 == this_block_height) && (0 == m_meta->_highest_connect_block_height))
            {
                m_meta->_highest_connect_block_height = this_block_height;
                m_meta->_highest_connect_block_hash   = this_block->get_block_hash();
            }

            //full-block must be a connected block
            if(   (this_block_height > m_meta->_highest_connect_block_height)
               && (this_block->get_block_class() == base::enum_xvblock_class_full)
                )
            {
                m_meta->_highest_connect_block_height = this_block_height;
                m_meta->_highest_connect_block_hash   = this_block->get_block_hash();
            }
            else if(   (this_block_height == (m_meta->_highest_connect_block_height + 1)) //regular check
                    && (m_meta->_highest_connect_block_hash  == this_block->get_last_block_hash()))
            {
                m_meta->_highest_connect_block_height = this_block_height;
                m_meta->_highest_connect_block_hash   = this_block->get_block_hash();
            }

            bool  logic_connect_more  = true;//logic connection that just ask connect to all the way to any fullblock
            if(logic_connect_more) //search more
            {
                for(uint64_t h = m_meta->_highest_connect_block_height + 1; h <= m_meta->_highest_commit_block_height; ++h)
                {
                    const uint64_t try_height = m_meta->_highest_connect_block_height + 1;
                    if(load_index(try_height) == 0) //missed block
                        break;
                    
                    base::xauto_ptr<base::xvbindex_t> next_commit(query_index(try_height, base::enum_xvblock_flag_committed));
                    if(!next_commit) //dont have commited block
                        break;
                    
                    if(next_commit->get_block_class() == base::enum_xvblock_class_full)
                    {
                        m_meta->_highest_connect_block_height = next_commit->get_height();
                        m_meta->_highest_connect_block_hash   = next_commit->get_block_hash();
                    }
                    else if(  (next_commit->get_height() == (m_meta->_highest_connect_block_height + 1))
                            && (next_commit->get_last_block_hash() == m_meta->_highest_connect_block_hash) )
                    {
                        m_meta->_highest_connect_block_height = next_commit->get_height();
                        m_meta->_highest_connect_block_hash   = next_commit->get_block_hash();
                    }
                    else
                    {
                        break;
                    }
                }
                const int  block_connect_step  = (int)(m_meta->_highest_connect_block_height - old_highest_connect_block_height);
                xdbg("xblockacct_t::full_connect_to,navigate step(%d) to _highest_connect_block_height=%" PRIu64 "  ",block_connect_step,m_meta->_highest_connect_block_height);
            }
            
            return true;
        }

        //update other meta except connected info
        bool xblockacct_t::update_meta_metric(base::xvbindex_t* new_block_ptr )
        {
            if(NULL == new_block_ptr)
                return false;

            const uint64_t new_block_height = new_block_ptr->get_height();
            //update meta information per this block
            if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
            {
                xdbg_info("xblockacct_t::update_meta_metric,account=%s,commit block=%s", get_account().c_str(), new_block_ptr->dump().c_str());

                //update meta information now
                if(new_block_height > m_meta->_highest_cert_block_height) //committed block must also a cert block
                    m_meta->_highest_cert_block_height = new_block_height;

                if(new_block_height > m_meta->_highest_lock_block_height) //committed block must also a locked block
                    m_meta->_highest_lock_block_height = new_block_height;

                if(new_block_height > m_meta->_highest_commit_block_height)
                    m_meta->_highest_commit_block_height = new_block_height;

                if(  (new_block_height > m_meta->_highest_full_block_height)
                   &&(new_block_ptr->get_block_class() == base::enum_xvblock_class_full) )
                {
                    m_meta->_highest_full_block_height = new_block_height;
                }

            }
            else if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_locked))
            {
                //update meta information now
                if(new_block_height > m_meta->_highest_cert_block_height) //committed block must also a cert block
                    m_meta->_highest_cert_block_height = new_block_height;

                if(new_block_height > m_meta->_highest_lock_block_height)
                    m_meta->_highest_lock_block_height = new_block_height;

                xdbg_info("xblockacct_t::update_meta_metric,lock block=%s",new_block_ptr->dump().c_str());
            }
            else if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_authenticated)) //xstore should only store commit&locked block
            {
                //update meta information now
                if(new_block_height > m_meta->_highest_cert_block_height)
                    m_meta->_highest_cert_block_height = new_block_height;

                xdbg_info("xblockacct_t::update_meta_metric,cert block=%s",new_block_ptr->dump().c_str());
            }
            return true;
        }


        bool      xblockacct_t::on_block_committed(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;
            //xdbg("xblockacct_t::on_block_committed,at account=%s,index=%s",get_account().c_str(),index_ptr->dump().c_str());
            //if(index_ptr->get_block_level() == base::enum_xvblock_level_table
            xdbg("xblockacct_t::on_block_committed,at account=%s,index=%s,level=%d",get_account().c_str(),index_ptr->dump().c_str(), index_ptr->get_block_level());
            if((index_ptr->get_block_level() == base::enum_xvblock_level_table || index_ptr->get_block_level() == base::enum_xvblock_level_root)            
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
                    xdbg_info("xblockacct_t::on_block_committed,done block=%s",index_ptr->dump().c_str());
                }
            }
            //fully connect to geneis block or last full-block here
            full_connect_to(index_ptr);

            return true;
        }

        bool      xblockacct_t::on_block_revoked(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;

            if(index_ptr->get_block_class() == base::enum_xvblock_class_nil) //skip nil block
                return true;

            #ifdef __ENABLE_ASYNC_EVENT_HANDLE__
            if(   (index_ptr->get_block_level() == base::enum_xvblock_level_table)
               && (index_ptr->get_block_type()  == base::enum_xvblock_type_batch) )
            {
                //batch'table just treat as container, all drivered by unit self
                xdbg("xblockacct_t::on_block_revoked,ignore batch table for  account=%s,index=%s",get_account().c_str(),index_ptr->dump().c_str());
                return true;
            }

            xdbg("xblockacct_t::on_block_revoked,at account=%s,index=%s",get_account().c_str(),index_ptr->dump().c_str());

            base::xveventbus_t * mbus = base::xvchain_t::instance().get_xevmbus();
            xassert(mbus != NULL);
            if(mbus != NULL)
            {
                mbus::xevent_ptr_t event = mbus->create_event_for_revoke_index_to_db(index_ptr);
                if (event != nullptr)
                {
                    mbus->push_event(event);

                    xdbg_info("xblockacct_t::on_block_revoked,done at store(%s)-> block=%s",get_blockstore_path().c_str(),index_ptr->dump().c_str());
                    return true;
                }
            }
            #endif //end of __ENABLE_ASYNC_EVENT_HANDLE__
            return false;
        }

        bool   xblockacct_t::push_event(enum_blockstore_event type,base::xvbindex_t* target)
        {
            m_events_queue.emplace_back(xblockevent_t(type,target,this,*m_meta));
            return true;
        }

        const std::deque<xblockevent_t> xblockacct_t::move_events()
        {
             return std::move(m_events_queue);
        }

        bool   xblockacct_t::process_events()
        {
            const std::deque<xblockevent_t> copy_events(std::move(m_events_queue));
            return process_events(copy_events);
        }

        bool   xblockacct_t::process_events(const std::deque<xblockevent_t> & copy_events)
        {
            for(auto & event : copy_events)
            {
                if(event.get_index() != NULL) //still valid
                {
                    if(enum_blockstore_event_committed == event.get_type())
                    {
                        on_block_committed(event.get_index());
                    }
                    else if(enum_blockstore_event_revoke == event.get_type())
                    {
                        on_block_revoked(event.get_index());
                    }
                }
            }
            return true;
        }



        //base weight is just regarding block self
        const uint64_t xblockacct_t::cal_index_base_weight(base::xvbindex_t * index)
        {
            if(NULL == index)
                return 0;

            uint64_t weight = index->get_height();

            #ifdef __ALLOW_FORK_LOCK__
            if(index->check_block_flag(base::enum_xvblock_flag_committed))
                weight += base::enum_xvblock_flag_committed;//0x4000

            return weight;
            #endif //end of __ALLOW_FORK_LOCK__

        #ifdef __USE_HIGHEST_FLAG_FOR_WEIGHT
            if(index->check_block_flag(base::enum_xvblock_flag_committed))
                weight += base::enum_xvblock_flag_committed;//0x4000
            else if(index->check_block_flag(base::enum_xvblock_flag_locked))
                weight += base::enum_xvblock_flag_locked;   //0x2000
            else
                weight += base::enum_xvblock_flag_authenticated;//0x1000
        #else //using composed flags,which is more reasonable
            weight += (index->get_block_flags() & (base::enum_xvblock_flag_committed | base::enum_xvblock_flag_locked | base::enum_xvblock_flag_authenticated));
        #endif
            return weight;
        }

        //define weight system for block' weight = ([status]) + [prev-connected]
        //to speed up clean up any forked or useless block, let it allow store first then rebase it
        bool   xblockacct_t::rebase_chain_at_height(const uint64_t target_height)
        {
            auto height_map_pos = m_all_blocks.find(target_height);
            if(height_map_pos != m_all_blocks.end())
            {
                auto & target_height_map = height_map_pos->second;
                return rebase_chain_at_height(target_height_map);
            }
            return true;//nothing to rebase
        }

        bool   xblockacct_t::rebase_chain_at_height(std::map<uint64_t,base::xvbindex_t*> & target_height_map)
        {
            if(target_height_map.size() > 1) //no need handle if there is only 0/1 candidate block
            {
                bool     has_commit_already = false;
                uint64_t cur_max_weight = 0;//init to 0
                for(auto it = target_height_map.begin(); it != target_height_map.end();)//counting every blocks at this height
                {
                    auto cur_it = it;
                    ++it;

                    uint64_t weight = cal_index_base_weight(cur_it->second);
                    if(cur_it->second->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        if(false == has_commit_already)
                        {
                            has_commit_already = true;
                        }
                        else
                        {
                            xerror("xblockacct_t::rebase_chain_at_height,error-found forked commiteded block(%s)",cur_it->second->dump().c_str());

                            //exception handle, force reset weight to 0
                            weight = 0;

                            //exception handle, force keep the oldest one
                            base::xvbindex_t * index_to_remove = cur_it->second;
                            //erase from map first
                            target_height_map.erase(cur_it);
                            //delete data at DB then
                            get_blockdb_ptr()->delete_block(index_to_remove);
                            //close and release object
                            index_to_remove->close();
                            index_to_remove->release_ref();
                        }
                    }
                    cur_max_weight = std::max(cur_max_weight,weight);//pick bigger one
                }

                //resolve any blocks of lower weight than max one
                for(auto it = target_height_map.begin(); it != target_height_map.end();)
                {
                    auto cur_it = it;
                    ++it;

                    const uint64_t weight = cal_index_base_weight(cur_it->second);

                    if(weight < cur_max_weight) //remove lower one
                    {
                        xinfo("xblockacct_t::rebase_chain_at_height,remove existing lower-weight' block(%s) < cur_max_weight(%" PRIu64 ")",cur_it->second->dump().c_str(),cur_max_weight);

                        base::xvbindex_t * index_to_remove = cur_it->second;
                        //erase from map first
                        target_height_map.erase(cur_it);
                        //delete data at DB then
                        get_blockdb_ptr()->delete_block(index_to_remove);
                        //close and release object
                        index_to_remove->close();
                        index_to_remove->release_ref();
                    }
                }
                xassert(target_height_map.size() >= 1); //at least one block kept
            }
            //remark viewid offset for all existing blocks
            resort_index_of_store(target_height_map);
            return true;
        }

        bool   xblockacct_t::resort_index_of_store(const uint64_t target_height)
        {
            auto height_map_pos = m_all_blocks.find(target_height);
            if(height_map_pos != m_all_blocks.end())
            {
                auto & target_height_map = height_map_pos->second;
                return resort_index_of_store(target_height_map);
            }
            return true;//nothing to resort
        }
        
        bool   xblockacct_t::resort_index_of_store(std::map<uint64_t,base::xvbindex_t*> & target_height_map)
        {
            if(target_height_map.empty())//nothing need resor
                return true;
            
            if(target_height_map.size() == 1)
            {
                base::xvbindex_t * index_entry = target_height_map.begin()->second;
                if(index_entry->check_store_flag(base::enum_index_store_flag_main_entry) == false)
                {
                    index_entry->set_store_flag(base::enum_index_store_flag_main_entry);
                }
                return true;
            }
            
            std::deque<base::xvbindex_t*> resort_queue;
            //step#1: //sort main-entry as first one,then sort by view# from lower to higher
            {
                base::xvbindex_t * main_entry = NULL;
                for(auto it = target_height_map.begin(); it != target_height_map.end();++it)
                {
                    if(it->second->check_store_flag(base::enum_index_store_flag_main_entry))
                    {
                        main_entry = it->second;
                    }
                    else
                    {
                        resort_queue.push_back(it->second);
                    }
                }
                if(NULL != main_entry)
                {
                    resort_queue.push_front(main_entry);
                }
                else//exception case
                {
                    main_entry = *resort_queue.begin();
                    main_entry->set_store_flag(base::enum_index_store_flag_main_entry);
                }
            }
            
            //step#2: recal next-view-offset as queue sort
            {
                for(auto it = resort_queue.begin(); it != resort_queue.end();)
                {
                    auto cur_entry = *it;
                    auto next_it = ++it;
                    
                    //setup linked viewid-offset
                    int32_t new_view_offset = 0;
                    if(next_it != resort_queue.end()) //has next one
                        new_view_offset = (int32_t)(((int64_t)(*next_it)->get_viewid()) - ((int64_t)cur_entry->get_viewid()));
                    
                    if(new_view_offset != cur_entry->get_next_viewid_offset() ) //if changed
                    {
                        cur_entry->reset_next_viewid_offset(new_view_offset); //link view
                        cur_entry->set_modified_flag(); //mark modified flag
                    }
                }
            }
            return true;
        }
    
        bool   xblockacct_t::precheck_new_index(base::xvbindex_t * new_index)
        {
            if(NULL == new_index)
                return false;

            auto height_map_pos = m_all_blocks.find(new_index->get_height());
            if(height_map_pos != m_all_blocks.end())
            {
                auto & target_height_map = height_map_pos->second;
                return precheck_new_index(new_index,target_height_map);
            }
            return true;//nothing to conflict
        }

        bool   xblockacct_t::precheck_new_index(base::xvbindex_t * new_index,std::map<uint64_t,base::xvbindex_t*> & target_height_map)
        {
            if(NULL == new_index)
                return false;

            if(target_height_map.empty() == false)
            {
                const uint64_t new_weight = cal_index_base_weight(new_index);
                for(auto it = target_height_map.begin(); it != target_height_map.end();++it)
                {
                    if(it->second->get_height() == new_index->get_height()) //double check
                    {
                        const uint64_t exist_weight = cal_index_base_weight(it->second);
                        if(new_weight < exist_weight)
                        {
                            xdbg("xblockacct_t::precheck_new_index,an outofdate'block(%s) < exist_weight(%" PRIu64 ") ",new_index->dump().c_str(),exist_weight);
                            return false;
                        }

                        if(   (new_weight == exist_weight)
                           && (it->second->check_block_flag(base::enum_xvblock_flag_committed)) )
                        {
                            if(   (new_index->get_viewid() != it->second->get_viewid())
                               || (new_index->get_block_hash() != it->second->get_block_hash()) )
                            {
                                xerror("xblockacct_t::precheck_new_index,error-try fork by new block(%s) vs existing commit block(%s)",new_index->dump().c_str(),it->second->dump().c_str());
                                return false;
                            }
                        }
                    }
                }
            }
            return true;
        }

        //return new & cached index
        base::xvbindex_t*  xblockacct_t::new_index(base::xvblock_t* new_raw_block)
        {
            if(NULL == new_raw_block)
                return nullptr;

            if(new_raw_block->check_block_flag(base::enum_xvblock_flag_authenticated) == false)//safety protect
                return nullptr;

            base::xauto_ptr<base::xvbindex_t > new_idx(create_index(*new_raw_block));
            if(0 != new_idx->get_height())
            {
                //just keep low 4bit flags about unpack/store/connect etc
                new_idx->reset_block_flags(new_idx->get_block_flags() & base::enum_xvblock_flags_low4bit_mask);//reset all status flags and redo it from authenticated status
                new_idx->set_block_flag(base::enum_xvblock_flag_authenticated);//init it as  authenticated
                new_idx->reset_modify_flag(); //remove modified flag to avoid double saving

                if(new_idx->get_height() <= m_meta->_highest_cert_block_height)
                {
                    if(new_idx->get_height() > m_meta->_highest_deleted_block_height)
                       load_index(new_idx->get_height()); //always load index first for non-genesis block
                }
            }
            else//genesis block
            {
                new_idx->set_store_flag(base::enum_index_store_flag_main_entry);//force to set
            }

            //note: emplace return a pair<iterator,bool>, value of bool indicate whether inserted or not, value of iterator point to inserted it
            auto height_map_pos  = m_all_blocks.emplace(new_idx->get_height(),std::map<uint64_t,base::xvbindex_t*>());
            auto & height_view_map = height_map_pos.first->second;//hight_map_pos.first->first is height, and hight_map_pos.first->second is viewmap

            //pre-check whether accept this new index
            if(precheck_new_index(new_idx(),height_view_map) == false)
            {
                xinfo("xblockacct_t::new_index,failed-precheck for block(%s)",new_idx->dump().c_str());

                if( (new_idx->get_block_flags() & base::enum_xvblock_flag_unpacked) != 0)//still need merge flags of block
                {
                    for(auto it = height_view_map.begin(); it != height_view_map.end();++it)
                    {
                        if(   (it->second->get_height() == new_raw_block->get_height())
                           && (it->second->get_block_hash() == new_raw_block->get_block_hash())  )
                        {
                            it->second->set_block_flag(base::enum_xvblock_flag_unpacked);
                            break;
                        }
                    }
                }
                return nullptr;
            }

            //cached_index_ptr is a raw ptr that just valid at this moment
            base::xvbindex_t * cached_index_ptr = cache_index(new_idx(),height_view_map);
            if(cached_index_ptr != nullptr) //insert or updated successful
            {
                if(cached_index_ptr->check_store_flag(base::enum_index_store_flag_non_index))
                {
                    base::xauto_ptr<base::xvblock_t> cloned_block(new_raw_block->clone_block());
                    cloned_block->reset_modified_count();//clean modifed count as fresh start
                    
                    //sync flags from index to raw block
                    if(cached_index_ptr->check_block_flag(base::enum_xvblock_flag_locked))
                        cloned_block->set_block_flag(base::enum_xvblock_flag_locked);
                    
                    if(cached_index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                        cloned_block->set_block_flag(base::enum_xvblock_flag_committed);
                    
                    //force to bind raw block ptr until block to commit or saved
                    cached_index_ptr->reset_this_block(cloned_block());
                }
                else //directly link  raw block first
                {
                    cached_index_ptr->reset_this_block(new_raw_block);
                }
                cached_index_ptr->set_modified_flag(); //force set flag to store later
                
                //connect as chain,and check connected_flag and meta
                connect_index(cached_index_ptr);//here may change index'status
                //rebase forked blocks if have ,after connect_index
                rebase_chain_at_height(height_view_map);
                if(height_view_map.find(cached_index_ptr->get_viewid()) == height_view_map.end())
                {
                    xdbg("xblockacct_t::new_index,failed-new index (%s) erased after rebase",new_idx->dump().c_str());
                    return nullptr;
                }
            }

            return cached_index_ptr;
        }
        
       

        xchainacct_t::xchainacct_t(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr)
            :xblockacct_t(parent_obj,timeout_ms,xvbkdb_ptr)
        {
            _lowest_commit_block_height = 0;
        }

        xchainacct_t::~xchainacct_t()
        {
        }

        //XTODO,set next_next_cert at outside
        bool  xchainacct_t::process_index(base::xvbindex_t* this_block)
        {
            //xdbg("jimmy xchainacct_t::process_index enter account=%s,index=%s", get_account().c_str(), this_block->dump().c_str());
            base::xvbindex_t* prev_block = this_block->get_prev_block();
            if(nullptr == prev_block)
            {
                xdbg_info("xchainacct_t::process_index no prev block. block=%s", this_block->dump().c_str());
                return false;
            }

            //xdbg("xchainacct_t::process_block at store=%s,high-qc=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
            //transfer to locked status
            if(false == prev_block->check_block_flag(base::enum_xvblock_flag_locked))
            {
                xdbg("xchainacct_t::process_index lock-qc=%s",prev_block->dump().c_str());

                prev_block->add_ref();//hold it to avoid be released by rebase_chain_at_height
                {
                    prev_block->set_block_flag(base::enum_xvblock_flag_locked);

                    rebase_chain_at_height(prev_block->get_height()); //resolve other block of lower-weight thans this
                    if(prev_block->is_close() == false)//prev_block is still valid to use
                    {
                        update_meta_metric(prev_block);//update meta since block has change status
                        
                        if(prev_block->is_unit_address() == false)//update status for any other block on time
                        {
                            //may double check whether need save again
                            write_block(prev_block);
                            write_index(prev_block);
                        }
                    }
                }
                prev_block->release_ref();//safe release now
            }

            base::xvbindex_t* prev_prev_block = prev_block->get_prev_block();
            if(nullptr == prev_prev_block)
            {
                if (this_block->get_height() > 1) {
                    xdbg_info("xchainacct_t::process_index no prev prev block. block=%s", this_block->dump().c_str());
                }
                return true;
            }
            //transfer to commit status
            if(false == prev_prev_block->check_block_flag(base::enum_xvblock_flag_committed))
            {
                xdbg("xchainacct_t::process_index commit-qc=%s",prev_prev_block->dump().c_str());

                prev_prev_block->set_block_flag(base::enum_xvblock_flag_locked);//change to locked status
                prev_prev_block->set_block_flag(base::enum_xvblock_flag_committed);//change to commit status

                prev_prev_block->add_ref(); //hold it to avoid be released by rebase_chain_at_height
                update_bindex(prev_prev_block);
                prev_prev_block->release_ref(); //safe release now
            }
            return true;
        }

        bool    xchainacct_t::connect_index(base::xvbindex_t* this_block)
        {
            //xdbg("jimmy xchainacct_t::connect_index enter account=%s,index=%s", get_account().c_str(), this_block->dump().c_str());
            if(NULL == this_block)
                return false;

            if(this_block->check_block_flag(base::enum_xvblock_flag_committed)) //trace it as well
                push_event(enum_blockstore_event_committed,this_block);

            const uint64_t this_block_height = this_block->get_height();
            if(this_block_height > m_meta->_highest_deleted_block_height)
            {
                if(this_block_height >= 2)
                {
                    load_index(this_block_height - 1);//force load height of prev
                    load_index(this_block_height - 2);//force load height of prev_prev
                }
                else //height == 1
                {
                    load_index(this_block_height - 1);
                }
                process_index(this_block);//now ready process this block
            }
            
            update_meta_metric(this_block);
            if(this_block_height >= m_meta->_highest_cert_block_height)//just return as no further blocks
                return true;

            if(load_index(this_block_height + 1) > 0) //force this load next block
            {
                auto  it_next_height = m_all_blocks.find(this_block_height + 1);
                if(it_next_height != m_all_blocks.end())//found next one
                {
                    for(auto it = it_next_height->second.rbegin(); it != it_next_height->second.rend(); ++it)//search from higher view
                    {
                        process_index(it->second); //start process from here
                    }

                    load_index(this_block_height + 2);//force this load next_next block
                    auto  it_next_next = m_all_blocks.find(this_block_height + 2); //try to find next and next one if existing
                    if(it_next_next != m_all_blocks.end())//if found next_next set
                    {
                        //note: xbft using 3-chain mode, so it request process next and next one
                        for(auto it = it_next_next->second.rbegin(); it != it_next_next->second.rend(); ++it)//search from higher view
                        {
                            process_index(it->second); //start process from here
                        }
                    }
                }
            }
            return true;
        }
    
        xtablebkplugin::xtablebkplugin(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr)
            :xchainacct_t(parent_obj,timeout_ms,xvbkdb_ptr)
        {
        }
        
        xtablebkplugin::~xtablebkplugin()
        {
        }
    
        base::xvbindex_t*  xtablebkplugin::create_index(base::xvblock_t & new_raw_block)
        {
            base::xvbindex_t* new_index = new base::xvbindex_t(new_raw_block);
            return new_index;
        }
        
        std::vector<base::xvbindex_t*> xtablebkplugin::read_index(const uint64_t target_height)
        {
            return get_blockdb_ptr()->read_index_from_db(*get_account_obj(),target_height);
        }
        
        bool  xtablebkplugin::write_index(base::xvbindex_t* this_index)
        {
            if(this_index->check_modified_flag() == false)//nothing changed
                return true;
            
            return get_blockdb_ptr()->write_index_to_db(this_index);
        }
    
        bool  xtablebkplugin::write_block(base::xvbindex_t* this_index)
        {
            if(this_index->check_block_flag(base::enum_xvblock_flag_stored))
                return true;
            
            return write_block(this_index,this_index->get_this_block());
        }
        
        bool  xtablebkplugin::write_block(base::xvbindex_t* index_ptr,base::xvblock_t * new_block_ptr)
        {
            if(NULL == new_block_ptr)
            {
                return false;
            }           
            
            //sync flags from index to raw block if has any change in case
            if(index_ptr->check_block_flag(base::enum_xvblock_flag_locked))
                new_block_ptr->set_block_flag(base::enum_xvblock_flag_locked);
            
            if(index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                new_block_ptr->set_block_flag(base::enum_xvblock_flag_committed);
            
            const int _stored_flags = get_blockdb_ptr()->save_block(index_ptr, new_block_ptr);
            if(_stored_flags > 0)
            {
                const uint32_t new_total_stored_flags = _stored_flags | index_ptr->get_store_flags();
                index_ptr->reset_store_flags(new_total_stored_flags);
     
                const uint32_t everything_flags = base::enum_index_store_flag_mini_block | base::enum_index_store_flag_input_entity | base::enum_index_store_flag_input_resource | base::enum_index_store_flag_output_entity| base::enum_index_store_flag_output_resource;
                
                if(index_ptr->check_store_flags(everything_flags))
                {
                    index_ptr->set_block_flag(base::enum_xvblock_flag_stored); //mark as stored everything
                }
                return true;
            }
            return false;
        }
        
        bool  xtablebkplugin::store_block(base::xvblock_t* new_raw_block)
        {
            return xchainacct_t::store_block(new_raw_block);
        }
    
        xunitbkplugin::xunitbkplugin(base::xvaccountobj_t & parent_obj,const uint64_t timeout_ms,xvblockdb_t * xvbkdb_ptr)
            :xchainacct_t(parent_obj,timeout_ms,xvbkdb_ptr)
        {
        }
        
        xunitbkplugin::~xunitbkplugin()
        {
        }
     
        base::xvbindex_t*  xunitbkplugin::create_index(base::xvblock_t & new_raw_block)
        {
            base::xvbindex_t* new_index = new base::xvbindex_t(new_raw_block);
            #ifdef __PERSIST_SAVE_UNIT_WITHOUT_INDEX__
            //force to save raw block only to DB by adding enum_index_store_flag_non_index
            new_index->set_store_flag(base::enum_index_store_flag_non_index);
            #endif
            return new_index;
        }
        
        //unit raw block must save fullly(dont split input/output/resource)
        std::vector<base::xvbindex_t*> xunitbkplugin::read_index(const uint64_t target_height)
        {
            #ifdef __PERSIST_SAVE_UNIT_WITHOUT_INDEX__
            std::vector<base::xvblock_t*> target_blocks(get_blockdb_ptr()->read_prunable_block_object_from_db(*get_account_obj(),target_height));
            
            std::vector<base::xvbindex_t*>  index_list;
            for(auto block_ptr : target_blocks)
            {
                base::xvbindex_t * target_index = create_index(*block_ptr);
                
                //recover flags back to index
                const uint32_t everything_flags = base::enum_index_store_flag_mini_block | base::enum_index_store_flag_input_entity | base::enum_index_store_flag_input_resource | base::enum_index_store_flag_output_entity| base::enum_index_store_flag_output_resource;
                target_index->reset_store_flags(everything_flags);
                target_index->set_block_flag(base::enum_xvblock_flag_stored);
                if(block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                {
                    //committed block always stored at height'key to improve I/O
                    target_index->set_store_flag(base::enum_index_store_flag_main_entry);
                }
                //force to save raw block only to DB by adding enum_index_store_flag_non_index
                target_index->set_store_flag(base::enum_index_store_flag_non_index);
                block_ptr->reset_modified_count(); //clean flag of modification if have
                target_index->reset_this_block(block_ptr); //hook to raw block within index
                block_ptr->release_ref(); //release it since reset_this_block has hold reference now
                
                target_index->reset_modify_flag(); //clean flag of modified
                index_list.push_back(target_index); //transfer ownership of target_index to here
            }
            return index_list;
            #else //new key-format
            return get_blockdb_ptr()->read_index_from_db(*get_account_obj(),target_height);
            #endif
        }
        
        bool  xunitbkplugin::write_index(base::xvbindex_t* this_index)
        {   
            if(this_index->check_store_flag(base::enum_index_store_flag_non_index))//just store raw-block
            {
                return true;//do nothing
            }
            else if(this_index->check_modified_flag()) //still store index and raw block both
            {
                return get_blockdb_ptr()->write_index_to_db(this_index);
            }
            return true;
        }
    
        bool  xunitbkplugin::write_block(base::xvbindex_t* this_index)
        {            
            bool reset_modified_flag_after_saved = false;
            base::xvblock_t * raw_block_ptr = this_index->get_this_block();
            if(this_index->check_store_flag(base::enum_index_store_flag_non_index))//just store raw-block
            {
                //handle 3 cases:
                /*
                    case#1: Index has changed
                    case#2: raw block not saved yet
                    case#3: raw block has changed
                    */
                if(  (this_index->check_modified_flag())
                    ||(this_index->check_store_flag(base::enum_index_store_flag_mini_block) == false)
                    ||((raw_block_ptr != NULL) && (raw_block_ptr->get_modified_count() > 0))
                    )
                {
                    xassert(raw_block_ptr != NULL);//always keep raw block for non_index case
                    if(raw_block_ptr == NULL)
                    {
                        get_blockdb_ptr()->load_block_object(this_index);
                        raw_block_ptr = this_index->get_this_block();
                    }
                    reset_modified_flag_after_saved = true;//reset index since it not save
                }
                else //nothing need save
                {
                    return true;
                }
            }
            else //new key-format but still have index
            {
                if(this_index->check_store_flag(base::enum_index_store_flag_mini_block))
                    return true;//nothing need save
            }
            
            xassert(raw_block_ptr != NULL);
            if(raw_block_ptr != NULL) //sync index to raw block
            {
                if(write_block(this_index, raw_block_ptr))
                {
                    if(reset_modified_flag_after_saved)
                        this_index->reset_modify_flag(); //clean up modified flags
                    
                    return true; //here just return as we not save index for unit account for new version
                }
            }
            return false;
        }
    
        bool  xunitbkplugin::write_block(base::xvbindex_t* index_ptr,base::xvblock_t * new_block_ptr)
        {           
            //sync flags from index to raw block if has any change in case
            if(index_ptr->check_block_flag(base::enum_xvblock_flag_locked))
                new_block_ptr->set_block_flag(base::enum_xvblock_flag_locked);
            
            if(index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                new_block_ptr->set_block_flag(base::enum_xvblock_flag_committed);
 
            const int _stored_flags = get_blockdb_ptr()->save_block(index_ptr, new_block_ptr);
            if(_stored_flags > 0)
            {
                const uint32_t new_total_stored_flags = _stored_flags | index_ptr->get_store_flags();
                index_ptr->reset_store_flags(new_total_stored_flags);
                
                const uint32_t everything_flags = base::enum_index_store_flag_mini_block | base::enum_index_store_flag_input_entity | base::enum_index_store_flag_input_resource | base::enum_index_store_flag_output_entity| base::enum_index_store_flag_output_resource;
                
                if(index_ptr->check_store_flags(everything_flags))
                {
                    index_ptr->set_block_flag(base::enum_xvblock_flag_stored); //mark as stored everything
                }
                new_block_ptr->reset_modified_count();//force to reset it
                return true;
            }
            return false;
        }
    
        bool  xunitbkplugin::store_block(base::xvblock_t* new_raw_block)
        {
            return xchainacct_t::store_block(new_raw_block);
        }

        bool        xblockacct_t::set_unit_proof(const std::string& unit_proof, uint64_t height){
            const std::string key_path = base::xvdbkey_t::create_prunable_unit_proof_key(*get_account_obj(), height);
            if (!base::xvchain_t::instance().get_xdbstore()->set_value(key_path, unit_proof)) {
                xerror("xblockacct_t::set_block_span key %s,fail to writed into db,index dump(%s)",key_path.c_str(), unit_proof.c_str());            
                return false;
            }

            return true;
        }

        const std::string xblockacct_t::get_unit_proof(uint64_t height){
            const std::string key_path = base::xvdbkey_t::create_prunable_unit_proof_key(*get_account_obj(), height);
            return base::xvchain_t::instance().get_xdbstore()->get_value(key_path);
        }
    };//end of namespace of vstore
};//end of namespace of top
