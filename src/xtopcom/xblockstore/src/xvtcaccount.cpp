// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xcontext.h"
#include "xvtcaccount.h"
#include "xvgenesis.h"
#include "xdata/xgenesis_data.h"
#include "xdata/xnative_contract_address.h"
#include "xstore/xstore_face.h"

// #ifndef __MAC_PLATFORM__
    #include "xconfig/xconfig_register.h"
    #include "xconfig/xpredefined_configurations.h"
    #include "xmbus/xevent_store.h"
    #include "xmetrics/xmetrics.h"
// #endif

namespace top
{
    namespace store
    {


        xtcaccount_t::xtcaccount_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore)
            : xchainacct_t(account_addr,timeout_ms,blockstore_path,_persist_db,_blockstore)

        {

        }

        xtcaccount_t::~xtcaccount_t()
        {

        }

        bool  xtcaccount_t::init()
        {
            //first load meta data from xdb/xstore
            const std::string full_meta_path = get_blockstore_path() + get_meta_path(*this);
            const std::string meta_content = load_value_by_path(full_meta_path);
            m_meta = xacctmeta_t::load(meta_content);
            if(nullptr == m_meta)
            {
                m_meta = new xacctmeta_t();
            }
            xdbg("xtcaccount_t::init load metadata=%s", m_meta->dump().c_str());

            //first load latest commit block that should == _highest_full_block_height
            base::xauto_ptr<base::xvblock_t> latest_committed_block(load_block_object(m_meta->_highest_commit_block_height,true));
            if(latest_committed_block == nullptr)
            {
                xwarn_err("xtcaccount_t::init(),fail-load highest connected block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_connect_block_height,get_account().c_str(),get_blockstore_path().c_str());

                return false;
            }
            else //exception protect in case DB corrupt
            {
                xinfo("xtcaccount_t::init(), latest commit block=%s", latest_committed_block->dump().c_str());
                if (latest_committed_block->get_height() != 0)
                {
                    xassert(m_meta->_highest_execute_block_hash == latest_committed_block->get_block_hash());
                }
            }

            xkinfo("xtcaccount_t::init,account=%s",dump().c_str());
            return true;
        }

        bool  xtcaccount_t::close(bool force_async)
        {
            if(is_close() == false)
            {
                base::xobject_t::close(force_async); //mark close status first
                xkinfo("xtcaccount_t::close,account=%s",dump().c_str());

                //then clean all blocks at memory
                close_blocks();

                //finally save meta data of account
                std::string vmeta_bin;
                const std::string meta_path = get_blockstore_path() + get_meta_path(*this);
                m_meta->serialize_to_string(vmeta_bin);
                store_value_by_path(meta_path,vmeta_bin);

                //TODO, retore following assert check after full_block enable
                //xassert(m_meta->_highest_full_block_height    == m_meta->_highest_connect_block_height);
                xassert(m_meta->_highest_connect_block_height == m_meta->_highest_commit_block_height);
                xassert(m_meta->_highest_commit_block_height  == m_meta->_highest_lock_block_height);
                xassert(m_meta->_highest_lock_block_height    == m_meta->_highest_cert_block_height);

                //debug test (commit-lock-connect) should be a reasonable range
                //TODO, once let syn module save block through blockstore, need using below condition to replace above one
            }
            return true;
        }

        void  xtcaccount_t::close_blocks()
        {
            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end(); ++height_it)//search from lower height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it) //search from lower view#
                    {
                        const uint64_t this_block_height = view_it->second->get_height();
                        const int      this_block_flags  = view_it->second->get_block_flags();

                        if(view_it->second->get_modified_count() > 0) //has changed since last store
                            save_block(view_it->second);//save_block may drop cert-only block

                        xdbg_info("xtcaccount_t::close_blocks,block=%s",view_it->second->dump().c_str());

                        view_it->second->close();//disconnect from prev-block and next-block,if have
                        view_it->second->release_ref();
                    }
                }
#ifdef ENABLE_METRICS
                XMETRICS_COUNTER_DECREMENT("blockstore_cache_block_total", m_all_blocks.size());
#endif
                m_all_blocks.clear();
            }
        }

        //one api to get latest_commit/latest_lock/latest_cert for better performance
        bool               xtcaccount_t::get_latest_blocks_list(base::xvblock_t* & cert_block,base::xvblock_t* & lock_block,base::xvblock_t* & commit_block)
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
                        if(commit_block == nullptr)
                        {
                            commit_block = view_it->second;
                            commit_block->add_ref();
                            break;
                        }
                    }
                }
            }
            if(commit_block == nullptr)
                commit_block = get_genesis_block();//return ptr that already hold/added reference

            if(lock_block == nullptr)
            {
                lock_block   = commit_block;
                lock_block->add_ref();//hold reference before return
            }
            if(cert_block == nullptr)
            {
                cert_block   = lock_block;
                cert_block->add_ref();//hold reference before return
            }
            return true;
        }

        bool   xtcaccount_t::store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks) //better performance
        {
            //std::sort(batch_store_blocks.begin(),batch_store_blocks.end(),base::less_by_block_height());
            for(auto it : batch_store_blocks)
            {
                if((it != nullptr) && (it->get_account() == get_account()) )//here do account check since xvblockstore_impl not do batch check
                    store_block(it);
            }
            return true;
        }

        bool    xtcaccount_t::delete_block(base::xvblock_t* block_ptr)//return error code indicate what is result
        {
            if(nullptr == block_ptr)
                return false;

            xkinfo("xtcaccount_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());

            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(block_ptr->get_height());
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    auto view_it = view_map.find(block_ptr->get_viewid());
                    if(view_it != view_map.end())
                    {
                        view_it->second->close();
                        view_it->second->release_ref();
                        view_map.erase(view_it);
                    }
                    if(view_map.empty())
                    {
                        m_all_blocks.erase(height_it);
#ifdef ENABLE_METRICS
                        XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", -1);
#endif
                    }
                }
            }

            return true;
        }

        void xtcaccount_t::set_meta_info(base::xvblock_t* this_block)
        {
            if (this_block == nullptr)
            {
                return;
            }
            xinfo("xtcaccount_t::set_meta_info block=%s", this_block->dump().c_str());
            set_meta_info(this_block->get_height(), this_block->get_block_hash());
        }

        void xtcaccount_t::set_meta_info(uint64_t height, const std::string& hash)
        {
            if (height >= m_meta->_highest_execute_block_height)
            {
                m_meta->_highest_execute_block_hash = hash;
                m_meta->_highest_connect_block_hash = hash;

                m_meta->_highest_commit_block_height = height;
                m_meta->_highest_execute_block_height = m_meta->_highest_commit_block_height;
                m_meta->_highest_connect_block_height = m_meta->_highest_commit_block_height;
                m_meta->_highest_full_block_height = m_meta->_highest_commit_block_height;
                m_meta->_highest_lock_block_height = m_meta->_highest_commit_block_height;
                m_meta->_highest_cert_block_height = m_meta->_highest_commit_block_height;
            }
        }

        void xtcaccount_t::set_block_flag(base::xvblock_t* this_block)
        {
            if (this_block == nullptr)
            {
                return;
            }

            this_block->set_block_flag(base::enum_xvblock_flag_committed);
            this_block->set_block_flag(base::enum_xvblock_flag_locked);
            this_block->set_block_flag(base::enum_xvblock_flag_authenticated);
            this_block->set_block_flag(base::enum_xvblock_flag_connected);
            this_block->set_block_flag(base::enum_xvblock_flag_executed);
        }


        bool    xtcaccount_t::delete_block(uint64_t height)//return error code indicate what is result
        {
            xkinfo("xtcaccount_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ") at store(%s)",get_chainid(),get_account().c_str(),height,get_blockstore_path().c_str());
            return true;
        }

        bool   xtcaccount_t::execute_block(base::xvblock_t* block) //execute block and update state of acccount
        {
            std::string vmeta_bin;
            const std::string meta_path = get_blockstore_path() + get_meta_path(*this);
            m_meta->serialize_to_string(vmeta_bin);
            store_value_by_path(meta_path,vmeta_bin);

            xinfo("xtcaccount_t::execute_block,successful-exectued block=%s based on height=%" PRIu64 "  ",block->dump().c_str(), m_meta->_highest_commit_block_height);
            return true;
        }


        /* 3 rules for managing cache
            #1. clean blocks of lower stage when higher stage coming. stage include : cert, lock and commit
            #2. only allow one block at same height for the locked or committed block,in other words it allow mutiple cert-only blocks
            #3. not allow overwrite block with newer/more latest block at same height and same stage
         */
        bool    xtcaccount_t::store_block(base::xvblock_t* this_block)
        {
            if(nullptr == this_block)
                return false;

            xdbg("xtcaccount_t::store_block,prepare for block=%s,cache_size:%zu, metat:%s",this_block->dump().c_str(), m_all_blocks.size(), m_meta->dump().c_str());
            uint64_t this_block_height = this_block->get_height();

#ifdef ENABLE_METRICS
            XMETRICS_TIME_RECORD_KEY("blockstore_store_block_time", this_block->get_account() + ":" + std::to_string(this_block->get_height()));
#endif
            if(   (false == this_block->is_input_ready(true))
               || (false == this_block->is_output_ready(true))
               || (false == this_block->is_deliver(false)) )//must have full valid data and has mark as enum_xvblock_flag_authenticated
            {
                xerror("xtcaccount_t::store_block,undevlier block=%s",this_block->dump().c_str());
                return false;
            }

            base::auto_reference<base::xvblock_t> autohold(this_block);//avoid to release in advance

            //note: emplace return a pair<iterator,bool>, value of bool indicate whether inserted or not, value of iterator point to inserted it
            auto hight_map_pos  = m_all_blocks.emplace(this_block_height,std::map<uint64_t,base::xvblock_t*>());
#ifdef ENABLE_METRICS
            if (hight_map_pos.second) {
                XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", 1);
            }
#endif
            auto & view_map     = hight_map_pos.first->second;//hight_map_pos.first->first is height, and hight_map_pos.first->second is viewmap
            if(view_map.empty()) //no any existing block occupy
            {
                auto view_map_pos = view_map.emplace(this_block->get_viewid(), this_block);
                if(view_map_pos.second) //insert successful
                {
                    if(connect_block(this_block,hight_map_pos.first,1))
                    {
                        this_block->add_ref(); //now ok to hold reference for map
                        this_block->add_modified_count(); //force to add amout before call save_block
                        save_block(this_block);

                        clean_blocks(enum_max_cached_blocks); //as limited cached memory, clean the oldest one if need
                    }
                    else //a bad cert block when connect_block fail
                    {
                        xwarn("xtcaccount_t::store_block,a bad block=%s of account=%s", this_block->dump().c_str(), m_meta->dump().c_str());

                        view_map.erase(view_map_pos.first);
                        if(view_map.empty())
                        {
                            m_all_blocks.erase(hight_map_pos.first);
#ifdef ENABLE_METRICS
                            XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", -1);
#endif
                        }
                    }
                }
                else
                {
                    xassert(0);
                    return false;
                }
            }
            else
            {
                xdbg_info("xtcaccount_t::store_block,duplicate block=%s", this_block->dump().c_str(), m_meta->dump().c_str());
                return false;
            }
            xdbg("xtcaccount_t::store_block,finally cached block=%s of account=%s", this_block->dump().c_str(), m_meta->dump().c_str());
#ifdef ENABLE_METRICS
            XMETRICS_COUNTER_INCREMENT("blockstore_store_block", 1);
#endif
            return true;
        }

        //internal use only: query_block just check at cache layer and return raw ptr without added reference, so caller need use careful
        base::xvblock_t*    xtcaccount_t::query_latest_block(base::enum_xvblock_flag request_flag)
        {
            if(false == m_all_blocks.empty())
            {
                for(auto height_it = m_all_blocks.rbegin(); height_it != m_all_blocks.rend(); ++height_it)//search from highest height
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        // don't care the viewid and request_flag
                        return view_it->second;
                    }
                }
            }
            return nullptr;
        }

        bool xtcaccount_t::save_to_xdb(base::xvblock_t* this_block)
        {
            const uint64_t this_block_height = this_block->get_height();
            const int      this_block_flags  = this_block->get_block_flags();

            //step#1: mark connected status for committed block

            bool       now_stored_result = false;
            //step#2: do persist store for block
            if( (this_block_flags & base::enum_xvblock_flag_stored) == 0) //first time to store db fully
            {
                this_block->set_block_flag(base::enum_xvblock_flag_stored); //set flag stored first
                now_stored_result  = get_store()->set_vblock(get_blockstore_path(),this_block); //store fully
                if(false == now_stored_result)//removed flag since stored fail
                {
                    get_blockstore()->remove_block_flag(this_block,base::enum_xvblock_flag_stored);
                    xerror("xtcaccount_t::save_block,at store(%s) fail-store full block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
                else //make sure store persistently first
                {
                    this_block->reset_modified_count(); //count everything already,just remove status of changed
                    xdbg_info("xtcaccount_t::save_block,at store(%s) store full block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
            }
            else //has changed something of header/cert
            {
                if (this_block->get_modified_count() == 0)
                {
                    xinfo("xtcaccount_t::save_block,at store(%s) unchanged block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                    return true;
                }
                now_stored_result = get_store()->set_vblock_header(get_blockstore_path(),this_block);//store header only
                if(now_stored_result)
                {
                    this_block->reset_modified_count(); //count everything already,just remove status of changed
                    xdbg_info("xtcaccount_t::save_block,at store(%s) store portion of block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
                else
                {
                    xerror("xtcaccount_t::save_block,at store(%s) fail-store portion of block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
            }
            //step#3: after finish persisten store, then ready to update the meta information (in execute_block);
            set_meta_info(this_block);

            return now_stored_result;
        }

        bool xtcaccount_t::save_block(base::xvblock_t* this_block)
        {
            set_block_flag(this_block);
            if(save_to_xdb(this_block))
            {
                // try to execute this block first
                return execute_block(this_block);
            }

            return false;
        }

        //note:the returned ptr NOT increased reference, so use carefully and only use internally
        //to connect prev block, load_block may call load_block again to get prev-block, reenter_allow_count decide how many times can reenter
        base::xvblock_t*     xtcaccount_t::load_block(const uint64_t target_height,int reenter_allow_count)
        {
            //load it from db now
#ifdef ENABLE_METRICS
            XMETRICS_TIME_RECORD_KEY("blockstore_load_block", get_account() + ":" + std::to_string(target_height));
#endif
            base::xvblock_t* new_block_ptr = get_store()->get_vblock(get_blockstore_path(),get_account(),target_height);//added reference when return by get_vblock
            if(nullptr == new_block_ptr)
            {
                if(0 == target_height) //genesis block but dont have data at DB, create it ondemand
                    new_block_ptr = xgenesis_block::create_genesis_block(get_account());
            }
            if(new_block_ptr != nullptr)
            {
                new_block_ptr->reset_modified_count();//clear modified count after create block from db
                if(target_height != new_block_ptr->get_height())
                {
                    xerror("xtcaccount_t::load_block,Exception-load unmatched block of height=%" PRIu64 ",but ask height=%" PRIu64 "from db",new_block_ptr->get_height(),target_height);

                    new_block_ptr->release_ref();
                    new_block_ptr = nullptr;
                    return nullptr;
                }
                if(target_height != 0)
                    xassert(new_block_ptr->check_block_flag(base::enum_xvblock_flag_stored));

                #ifdef DEBUG
                if(false == new_block_ptr->is_deliver(false))//must have full valid data and has mark as enum_xvblock_flag_authenticated
                {
                    xerror("xtcaccount_t::load_block,undevlier block=%s",new_block_ptr->dump().c_str());
                }
                #endif

                //note: emplace return a pair<iterator,bool>, value of bool indicate whether inserted or not, value of iterator point to inserted it
                auto hight_map_pos  = m_all_blocks.emplace(new_block_ptr->get_height(),std::map<uint64_t,base::xvblock_t*>());
#ifdef ENABLE_METRICS
                if (hight_map_pos.second)
                {
                    XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", 1);
                }
#endif
                auto & view_map     = hight_map_pos.first->second;//hight_map_pos.first->first is height, and hight_map_pos.first->second is viewmap
                // viewid is not used, using 0 as a placeholder
                auto insert_result = view_map.emplace(new_block_ptr->get_viewid(), new_block_ptr); //new_block_ptr has added reference
                if(insert_result.second)//inserted succssful
                {
                    //connect to prev/next block
                    connect_block(new_block_ptr,hight_map_pos.first,reenter_allow_count);

                    xassert(new_block_ptr->check_block_flag(base::enum_xvblock_flag_committed));
                    xdbg_info("xtcaccount_t::load_block,at store(%s) commit block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());

                    //update meta information now
                    if(target_height > m_meta->_highest_commit_block_height)
                    {
                        set_meta_info(new_block_ptr);
                    }
                }
                else
                {
                    xwarn("xtcaccount_t::load_block,at store(%s) duplicated block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());
                    new_block_ptr->release_ref();
                    new_block_ptr = nullptr;
                }
            }
            else
            {
                xwarn("xtcaccount_t::load_block,warn-not found block at height:%" PRIu64 " of account(%s) at store(%s)",(int64_t)target_height,get_account().c_str(),get_blockstore_path().c_str());
            }
#ifdef ENABLE_METRICS
            XMETRICS_COUNTER_INCREMENT("blockstore_load_block", 1);
#endif
            return new_block_ptr;
        }

        bool xtcaccount_t::clean_blocks(const size_t keep_blocks_count)
        {
            if(m_all_blocks.size() > keep_blocks_count)
            {
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end();)//search from lowest hight to higher
                {
                    if(m_all_blocks.size() <= keep_blocks_count)//clean enough
                        break;

                    auto old_height_it = height_it; //copy first
                    ++height_it;                    //move next in advance

                    if(old_height_it->second.empty()) //clean empty first if have
                    {
                        m_all_blocks.erase(old_height_it);
#ifdef ENABLE_METRICS
                        XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", -1);
#endif
                        continue;
                    }

                    if( (old_height_it->first != 0) //keep genesis
                       && (old_height_it->first != m_meta->_highest_commit_block_height) )
                    {
                        auto & view_map = old_height_it->second;
                        xassert(view_map.size() == 1);

                        for(auto it = view_map.begin(); it != view_map.end(); ++it)
                        {
                            if(it->second->get_modified_count() > 0) //store any modified blocks again
                                save_block(it->second);

                            xdbg_info("xtcaccount_t::clean_blocks,block=%s",it->second->dump().c_str());

                            it->second->close();//disconnect from prev-block and next-block
                            it->second->release_ref();
                        }
                        m_all_blocks.erase(old_height_it);
#ifdef ENABLE_METRICS
                        XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", -1);
#endif
                    }
                }
            }
            return true;
        }

        bool    xtcaccount_t::connect_block(base::xvblock_t* this_block,const std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> >::iterator & this_block_height_it,int reload_allow_count)
        {
            // do nothing
            return true;
        }

    };//end of namespace of vstore
};//end of namespace of top
