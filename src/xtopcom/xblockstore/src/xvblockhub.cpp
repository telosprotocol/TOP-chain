// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xutl.h"
#include "xbase/xcontext.h"
#if defined(ENABLE_METRICS)
#include "xmetrics/xmetrics.h"
#endif
#include "xvblockhub.h"
#include "xvgenesis.h"
#include "xvledger/xvdbkey.h"

#ifdef __ALLOW_FORK_LOCK__
    #undef __ALLOW_FORK_LOCK__  // XTODO always allow store multi lock blocks
#endif

#ifndef __ALLOW_TABLE_MORE_CACHE_SIZE__
    #define __ALLOW_TABLE_MORE_CACHE_SIZE__
#endif

namespace top
{
    namespace store
    {
        xacctmeta_t*  xacctmeta_t::load(const std::string & meta_serialized_data)
        {
            if(meta_serialized_data.empty()) //check first
                return NULL;

            xacctmeta_t* meta_ptr = new xacctmeta_t();
            if(meta_ptr->serialize_from_string(meta_serialized_data) <= 0)
            {
                xerror("xacctmeta_t::load,bad meta_serialized_data that not follow spec");
                meta_ptr->release_ref();
                return NULL;
            }
            return meta_ptr;
        }

        xacctmeta_t::xacctmeta_t()
            :base::xdataobj_t(base::xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            #ifdef ENABLE_METRICS
            XMETRICS_GAUGE(metrics::dataobject_xacctmeta_t, 1);
            #endif

            _reserved_u16 = 0;
            _block_level  = (uint8_t)-1; //init to 255(that ensure is not allocated)
            _meta_spec_version = 1;     //version #1 now
            _highest_cert_block_height     = 0;
            _highest_lock_block_height     = 0;
            _highest_commit_block_height   = 0;
            _highest_execute_block_height  = 0;
            _highest_connect_block_height  = 0;
            _highest_full_block_height     = 0;
            _highest_genesis_connect_height= 0;
            _highest_sync_height           = 0;
        }

        xacctmeta_t::~xacctmeta_t()
        {
            #ifdef ENABLE_METRICS
            XMETRICS_GAUGE(metrics::dataobject_xacctmeta_t, -1);
            #endif
        }

        std::string xacctmeta_t::dump() const
        {
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"{meta:height for cert=%" PRIu64 ",lock=%" PRIu64 ",commit=%" PRIu64 ",execute=%" PRIu64 ",connected=%" PRIu64 ",full=%" PRIu64 ",g_connected=%" PRIu64 "}",(int64_t)_highest_cert_block_height,(int64_t)_highest_lock_block_height,(int64_t)_highest_commit_block_height,(int64_t)_highest_execute_block_height,(int64_t)_highest_connect_block_height,_highest_full_block_height,_highest_genesis_connect_height);

            return std::string(local_param_buf);
        }

        //caller respond to cast (void*) to related  interface ptr
        void*   xacctmeta_t::query_interface(const int32_t _enum_xobject_type_)
        {
            if(_enum_xobject_type_ == base::xdataunit_t::enum_xdata_type_vaccountmeta)
                return this;

            return base::xdataobj_t::query_interface(_enum_xobject_type_);
        }

        int32_t   xacctmeta_t::do_write(base::xstream_t & stream)//serialize whole object to binary
        {
            const int32_t begin_size = stream.size();

            stream << _highest_cert_block_height;
            stream << _highest_lock_block_height;
            stream << _highest_commit_block_height;
            stream << _highest_execute_block_height;
            stream << _highest_full_block_height;
            stream << _highest_connect_block_height;
            stream.write_tiny_string(_highest_connect_block_hash);
            stream.write_tiny_string(_highest_execute_block_hash);
            stream << _highest_genesis_connect_height;
            stream.write_tiny_string(_highest_genesis_connect_hash);
            stream << _highest_sync_height;

            //from here we introduce version control for meta
            stream << _meta_spec_version;
            stream << _block_level;
            stream << _reserved_u16;
            stream << _lowest_genesis_connect_height;

            return (stream.size() - begin_size);
        }

        int32_t   xacctmeta_t::do_read(base::xstream_t & stream)//serialize from binary and regeneate content
        {
            const int32_t begin_size = stream.size();

            stream >> _highest_cert_block_height;
            stream >> _highest_lock_block_height;
            stream >> _highest_commit_block_height;
            stream >> _highest_execute_block_height;
            stream >> _highest_full_block_height;
            stream >> _highest_connect_block_height;
            stream.read_tiny_string(_highest_connect_block_hash);
            stream.read_tiny_string(_highest_execute_block_hash);
            stream >> _highest_genesis_connect_height;
            stream.read_tiny_string(_highest_genesis_connect_hash);
            stream >> _highest_sync_height;

            stream >> _meta_spec_version;
            stream >> _block_level;
            stream >> _reserved_u16;
            stream >> _lowest_genesis_connect_height;

            if(stream.size() > 0) //still have data to read
            {
            }

            return (begin_size - stream.size());
        }

        //serialize vheader and certificaiton,return how many bytes is writed/read
        int32_t   xacctmeta_t::serialize_to_string(std::string & bin_data)   //wrap function fo serialize_to(stream)
        {
            base::xautostream_t<1024> _stream(base::xcontext_t::instance());
            const int result = xdataobj_t::serialize_to(_stream);
            if(result > 0)
                bin_data.assign((const char*)_stream.data(),_stream.size());
            return result;
        }

        int32_t   xacctmeta_t::serialize_from_string(const std::string & bin_data) //wrap function fo serialize_from(stream)
        {
            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)bin_data.data(),(uint32_t)bin_data.size());
            const int result = xdataobj_t::serialize_from(_stream);
            return result;
        }

        std::string  xblockacct_t::get_meta_path(base::xvaccount_t & _account)
        {
            std::string meta_path;
            meta_path.reserve(256);
            meta_path += base::xstring_utl::tostring(_account.get_chainid());
            meta_path += "/";
            meta_path += _account.get_account();
            meta_path += "/meta";

            return meta_path;
        }

        base::xvdbstore_t* xblockacct_t::get_xdbstore()
        {
            return m_xvdb_ptr;
        }

        xblockacct_t::xblockacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,base::xvdbstore_t* xvdb_ptr)
            :base::xobject_t(base::enum_xobject_type_vaccount),
             base::xvaccount_t(account_addr)
        {
#ifdef ENABLE_METRICS
            XMETRICS_GAUGE(metrics::dataobject_xblockacct_t, 1);
#endif
            m_meta = NULL;
            m_xvdb_ptr = NULL;

            m_xvdb_ptr = xvdb_ptr;//ptr never released,so here just keep it

            //need keep it unchanged forever as compatible consdieration
            m_blockstore_path = blockstore_path;
            if(m_blockstore_path.find_last_of('/') != (m_blockstore_path.size() - 1)) //if dont carry '/' at end
                m_blockstore_path.append("/");

            m_last_access_time_ms = 0;
            m_idle_timeout_ms     = timeout_ms;
        }

        xblockacct_t::~xblockacct_t()
        {
            xinfo("xblockacct_t::destroy,account=%s at blockstore=%s,objectid=% " PRId64 " ",
                  get_address().c_str(),get_blockstore_path().c_str(),
                  (int64_t)get_obj_id());
#ifdef ENABLE_METRICS
            XMETRICS_GAUGE(metrics::dataobject_xblockacct_t, -1);
#endif
            close_blocks();
            if(m_meta != nullptr)
                m_meta->release_ref();
        }

        std::string xblockacct_t::dump() const  //just for debug purpose
        {
            // execute height fall behind check, should be deleted eventually
            const int64_t distance = m_meta->_highest_full_block_height - m_meta->_highest_execute_block_height;
            uint32_t warn_level = (uint32_t)(distance >> 7);  // fall_num = 128;
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"{warn_meta=%d,distance=%" PRId64 ",account_id(%" PRIu64 "),account_addr=%s ->latest height for full=%" PRId64 ",genesis_connect=%" PRId64 ", connect=%" PRId64 ",commit=%" PRId64 ",execute=%" PRId64 " < lock=%" PRId64 " < cert=%" PRId64 "; at store(%s)}",
                warn_level, distance, get_xvid(), get_address().c_str(),m_meta->_highest_full_block_height,m_meta->_highest_genesis_connect_height,m_meta->_highest_connect_block_height,m_meta->_highest_commit_block_height,m_meta->_highest_execute_block_height,m_meta->_highest_lock_block_height,m_meta->_highest_cert_block_height,get_blockstore_path().c_str());

            return std::string(local_param_buf);
        }

        bool  xblockacct_t::init()
        {
            //first load meta data from xdb/xstore
            m_last_save_vmeta_bin.clear();
            const std::string full_meta_path = get_blockstore_path() + get_meta_path(*this);
            const std::string meta_content = load_value_by_path(full_meta_path);
            m_meta = xacctmeta_t::load(meta_content);
            if(nullptr == m_meta)
            {
                m_meta = new xacctmeta_t();
                xinfo("xblockacct_t::init,account=%s at blockstore=%s,objectid=% " PRId64 ",empty meta",
                      dump().c_str(),m_blockstore_path.c_str(),get_obj_id());
            }
            else
            {
                //pre-load latest execution block
                if(load_index(m_meta->_highest_execute_block_height) == 0)
                {
                    xwarn_err("xblockacct_t::init(),fail-load highest execution block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_execute_block_height,get_account().c_str(),get_blockstore_path().c_str());
                }

                //pre-load latest commit block
                if(load_index(m_meta->_highest_commit_block_height) == 0)
                {
                    xwarn_err("xblockacct_t::init(),fail-load highest commited block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_commit_block_height,get_account().c_str(),get_blockstore_path().c_str());
                }
                //pre-load latest lock block
                if(load_index(m_meta->_highest_lock_block_height) == 0)
                {
                    xwarn_err("xblockacct_t::init(),fail-load highest locked block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_lock_block_height,get_account().c_str(),get_blockstore_path().c_str());
                }
                if(load_index(m_meta->_highest_cert_block_height) == 0)
                {
                    xwarn_err("xblockacct_t::init(),fail-load highest cert block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_cert_block_height,get_account().c_str(),get_blockstore_path().c_str());
                }
                xinfo("xblockacct_t::init,account=%s at blockstore=%s,objectid=% " PRId64 ",meta=%s",
                      dump().c_str(),m_blockstore_path.c_str(),
                      get_obj_id(),m_meta->base::xobject_t::dump().c_str());
            }
            return true;
        }

        bool  xblockacct_t::save_meta()
        {
            std::string vmeta_bin;
            m_meta->serialize_to_string(vmeta_bin);
            if (m_last_save_vmeta_bin != vmeta_bin)
            {
                const std::string meta_path = get_blockstore_path() + get_meta_path(*this);
                store_value_by_path(meta_path, vmeta_bin);
                m_last_save_vmeta_bin = vmeta_bin;
            }
            return true;
        }

        bool  xblockacct_t::close(bool force_async)
        {
            if(is_close() == false)
            {
                base::xobject_t::close(force_async); //mark close status first
                xkinfo("xblockacct_t::close,account=%s",dump().c_str());

                //then clean all blocks at memory
                close_blocks();

                //finally save meta data of account
                save_meta();

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
            #ifdef __ALLOW_TABLE_MORE_CACHE_SIZE__
            if(base::enum_xvblock_level_table == m_meta->_block_level)
                return (enum_max_cached_blocks << 2);//allow cache to max 128 block
            #endif

            return enum_max_cached_blocks;
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
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end();)//search from lowest hight to higher
                {
                    if((int)m_all_blocks.size() <= keep_blocks_count)//clean enough
                        break;

                    auto old_height_it = height_it; //copy first
                    ++height_it;                    //move next in advance

                    if(old_height_it->second.empty()) //clean empty first if have
                    {
                        m_all_blocks.erase(old_height_it);
                        #ifdef ENABLE_METRICS
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                        #endif
                        continue;
                    }

                    if(   (old_height_it->first != m_meta->_highest_full_block_height)    //keep latest_full_block
                       && (old_height_it->first != m_meta->_highest_execute_block_height) //keep latest_executed block
                       && (old_height_it->first <  m_meta->_highest_commit_block_height)  //keep latest_committed block
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
                            if(it->second->check_modified_flag()) //store any modified blocks again
                                write_index_to_db(it->second);//push event to mbus if need

                            xdbg_info("xblockacct_t::clean_caches,index=%s",it->second->dump().c_str());
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
                        #ifdef ENABLE_METRICS
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1 * erase_count);
                        #endif
                    }
                    else //clean raw block for those reserved index
                    {
                        auto & view_map = old_height_it->second;
                        for(auto it = view_map.begin(); it != view_map.end(); ++it)
                        {
                            it->second->reset_this_block(NULL);
                        }
                    }
                }
                // try to save meta when clean blocks
                save_meta();
            }
            else if(force_release_unused_block) //force release block that only hold by internal
            {
                for(auto height_it = m_all_blocks.begin(); height_it != m_all_blocks.end();)//search from lowest hight to higher
                {
                    auto old_height_it = height_it; //copy first
                    ++height_it;                    //move next in advance
                    if(old_height_it->second.empty()) //clean empty first if have
                    {
                        m_all_blocks.erase(old_height_it);
                        #ifdef ENABLE_METRICS
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                        #endif
                        continue;
                    }

                    auto & view_map = old_height_it->second;
                    for(auto it = view_map.begin(); it != view_map.end(); ++it)
                    {
                        if(it->second->get_this_block() != NULL) //clean any block that just reference by index only
                        {
                            if(it->second->get_this_block()->get_refcount() == 1)//no any other hold
                            {
                                it->second->reset_this_block(NULL);
                                xdbg_info("xblockacct_t::clean_caches,block=%s",it->second->dump().c_str());
                            }
                        }
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

                        //at entry of quit we need make sure everything is consist
                        update_meta_metric(view_it->second);  //udate other meta and connect info
                        if(view_it->second->check_modified_flag()) //has changed since last store
                            write_index_to_db(view_it->second);//save_block but disable trigger event

                        xdbg_info("xblockacct_t::close_blocks,block=%s",view_it->second->dump().c_str());

                        view_it->second->close();//disconnect from prev-block and next-block,if have
                        view_it->second->release_ref();
                    }
                }
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1 * m_all_blocks.size());
                #endif
                m_all_blocks.clear();
            }
        }

        const int xblockacct_t::get_cache_size()
        {
            return (int)m_all_blocks.size();
        }

        //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
        bool  xblockacct_t::reset_cache_timeout(const uint32_t max_idle_time_ms)
        {
            m_idle_timeout_ms = max_idle_time_ms;
            return true;
        }

        void  xblockacct_t::set_last_access_time(const uint64_t last_access_time)
        {
            if(m_last_access_time_ms < last_access_time)
                m_last_access_time_ms = last_access_time;
        }

        bool xblockacct_t::is_live(const uint64_t timenow_ms)
        {
            uint64_t _idle_timeout_ms = m_idle_timeout_ms;
            if(m_meta->_block_level > base::enum_xvblock_level_unit)
                _idle_timeout_ms = _idle_timeout_ms << 4; //scale 16 times * m_idle_timeout_ms for table ,book etc

            if( timenow_ms > (_idle_timeout_ms + m_last_access_time_ms) )
                return false;
            return true;
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
                if(m_meta->_highest_cert_block_height > 0) //fallback
                {
                    m_meta->_highest_cert_block_height -= 1;
                    load_index(m_meta->_highest_cert_block_height);
                }
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
                if(m_meta->_highest_lock_block_height > 0)
                {
                    m_meta->_highest_lock_block_height -= 1;
                    load_index(m_meta->_highest_lock_block_height);
                }
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
                if(m_meta->_highest_commit_block_height > 0)
                {
                    m_meta->_highest_commit_block_height -= 1;
                    load_index(m_meta->_highest_commit_block_height);
                }
            }
            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_flag_committed);
            if(result != nullptr)//query_latest_index has been return a added-reference ptr
                return result;

            return load_genesis_index();
        }

        base::xvbindex_t*    xblockacct_t::load_latest_executed_index()
        {
            load_index(m_meta->_highest_execute_block_height);

            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_flag_executed);
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
            //connected block must be committed as well
            base::xvbindex_t* result = query_latest_index(base::enum_xvblock_flag_connected);
            if(result != nullptr)
                return result;

            return load_genesis_index();
        }

        //note:load_latest_genesis_connected_index is dedicated for sync-module only
        base::xvbindex_t*  xblockacct_t::load_latest_genesis_connected_index(bool ask_full_search) //block has connected to genesis
        {
            if(load_index(m_meta->_highest_genesis_connect_height) == 0)//load first
            {
                xwarn("xblockacct_t::load_latest_genesis_connected_index,fail load block at height(%" PRIu64 ") of account(%s)",m_meta->_highest_genesis_connect_height,get_address().c_str());
                for(uint64_t i = 1; i <= 3; ++i)//try forwarded 3 blocks
                {
                    if(m_meta->_highest_genesis_connect_height > i)
                    {
                        base::xvbindex_t* alternative = load_index(m_meta->_highest_genesis_connect_height - i, base::enum_xvblock_flag_committed);
                        if(alternative != NULL)//load_index has been return a added-reference ptr
                        {
                            m_meta->_highest_genesis_connect_height = alternative->get_height();
                            m_meta->_highest_genesis_connect_hash   = alternative->get_block_class();
                            return alternative;
                        }
                    }
                }
            }
            //note:when ask_full_search is true ,here may do heavy job to search all blocks until highest one
            if(ask_full_search)
            {
                const uint64_t old_highest_genesis_connect_height = m_meta->_highest_genesis_connect_height;
                for(uint64_t h = m_meta->_highest_genesis_connect_height + 1; h <= m_meta->_highest_commit_block_height; ++h)
                {
                    const uint64_t try_height = m_meta->_highest_genesis_connect_height + 1;
                    if(load_index(try_height) == 0) //missed block
                        break;

                    base::xauto_ptr<base::xvbindex_t> next_commit(query_index(try_height, base::enum_xvblock_flag_committed));
                    if(!next_commit) //dont have commited block
                        break;

                    if( (0 == m_meta->_highest_genesis_connect_height) && m_meta->_highest_genesis_connect_hash.empty())
                    {
                        //could be exception case that not event inited yet,so makeup
                        m_meta->_highest_genesis_connect_height = next_commit->get_height();
                        m_meta->_highest_genesis_connect_hash   = next_commit->get_block_hash();
                    }
                    else if(   (next_commit->get_height() == (m_meta->_highest_genesis_connect_height + 1))
                       && (next_commit->get_last_block_hash() == m_meta->_highest_genesis_connect_hash) )
                    {
                        m_meta->_highest_genesis_connect_height = next_commit->get_height();
                        m_meta->_highest_genesis_connect_hash   = next_commit->get_block_hash();
                    }
                    else
                    {
                        break;
                    }
                }

                if(m_meta->_highest_genesis_connect_height > (old_highest_genesis_connect_height + 64))
                    xwarn("xblockacct_t::load_latest_genesis_connected_index,navigate big step(%d) to new height(%" PRIu64 ") vs commit-height(%" PRIu64 ")  of account(%s)",(int)(m_meta->_highest_genesis_connect_height - old_highest_genesis_connect_height) ,m_meta->_highest_genesis_connect_height,m_meta->_highest_commit_block_height,get_address().c_str());
                else if(m_meta->_highest_genesis_connect_height > old_highest_genesis_connect_height)
                    xinfo("xblockacct_t::load_latest_genesis_connected_index,navigate small step(%d) to new height(%" PRIu64 ") vs commit-height(%" PRIu64 ")  of account(%s)",(int)(m_meta->_highest_genesis_connect_height - old_highest_genesis_connect_height) ,m_meta->_highest_genesis_connect_height,m_meta->_highest_commit_block_height,get_address().c_str());
            }
            else
            {
                xinfo("xblockacct_t::load_latest_genesis_connected_index,load org height(%" PRIu64 ") vs commit-height(%" PRIu64 ")  of account(%s)",m_meta->_highest_genesis_connect_height,m_meta->_highest_commit_block_height,get_address().c_str());
            }

            //connected block must be committed as well
            base::xvbindex_t* result = query_index(m_meta->_highest_genesis_connect_height,base::enum_xvblock_flag_committed);
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
            xdbg_info("xblockacct_t::load_latest_index_list succ retry. account=%s,cert=%s,lock=%s",
                      get_account().c_str(), cert_block->dump().c_str(), lock_block->dump().c_str());
            return true;
        }

        //load every index of block at target_height into cache layer
        int   xblockacct_t::load_index(const uint64_t target_height)
        {
            auto it = m_all_blocks.find(target_height);
            if(it == m_all_blocks.end())//load all at certain height
            {
                std::vector<base::xvbindex_t*> _indexes(read_index_from_db(target_height));
                if(_indexes.empty() == false) //found index at db
                {
                    for(auto it = _indexes.begin(); it != _indexes.end(); ++it)
                    {
                        cache_index(*it);      //cache it -> link-neighbor->mark-connect->update meta

                        //at entry of load, check connected_flag and meta info
                        update_meta_metric(*it); //update other meta and connect info

                        (*it)->release_ref();   //release ptr that reference added by read_index_from_db
                    }
                    #ifdef ENABLE_METRICS
                    XMETRICS_TIME_RECORD_KEY("blockstore_load_block_time", get_account() + ":" + std::to_string(target_height));
                    #endif
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
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
                #endif
                std::vector<base::xvbindex_t*> _indexes(read_index_from_db(target_height));
                if(_indexes.empty() == false) //found index at db
                {
                    for(auto it = _indexes.begin(); it != _indexes.end(); ++it)
                    {
                        cache_index(*it);      //cache it -> link-neighbor->mark-connect->update meta

                        //at entry of load, check connected_flag and meta info
                        update_meta_metric(*it); //update other meta and connect info

                        (*it)->release_ref();   //release ptr that reference added by read_index_from_db
                    }
                    #ifdef ENABLE_METRICS
                    XMETRICS_TIME_RECORD_KEY("blockstore_load_block_time", get_account() + ":" + std::to_string(target_height));
                    #endif
                    return (int)_indexes.size();
                }
                //genesis block but dont have data at DB, create it ondemand
                if(0 == target_height)
                {
                    xwarn("xblockacct_t::load_index(),fail found index for addr=%s at height=%" PRIu64 "", get_account().c_str(), target_height);
                    return 0;
                }
            }
            #ifdef ENABLE_METRICS
            XMETRICS_GAUGE(metrics::blockstore_index_load, 1);
            #endif
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
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 1);
                #endif
                return target_block;//found at cache layer
            }
            else
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
                #endif
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
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 1);
                #endif
                return target_block;//found at cache layer
            }
            else
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
                #endif
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
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 1);
                #endif
                return target_block;//found at cache layer
            }
            else
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                XMETRICS_GAUGE(metrics::blockstore_index_load, 0);
                #endif
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

        bool    xblockacct_t::load_block_object(base::xvbindex_t* index_ptr, const int atag)
        {
            if(NULL == index_ptr)
                return false;
            xdbg("xblockacct_t::load_block_object,target index(%s)",index_ptr->dump().c_str());

            if(index_ptr->get_this_block() != NULL)
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 1);
                #endif
                return true;
            }
            else
            {
                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)atag, 0);
                #endif
            }

            return read_block_object_from_db(index_ptr);
        }

        bool    xblockacct_t::load_index_input(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;

            if(index_ptr->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            xdbg("xblockacct_t::load_index_input,target index(%s)",index_ptr->dump().c_str());
            if(index_ptr->get_this_block() == NULL)
                read_block_object_from_db(index_ptr);

            if(index_ptr->get_this_block() == NULL) //check again
                return false;

            return  read_block_input_from_db(index_ptr);
        }

        bool    xblockacct_t::load_index_output(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;

            if(index_ptr->get_block_class() == base::enum_xvblock_class_nil)
                return true;
            xdbg("xblockacct_t::load_index_output,target index(%s)",index_ptr->dump().c_str());
            if(index_ptr->get_this_block() == NULL)
                read_block_object_from_db(index_ptr);

            if(index_ptr->get_this_block() == NULL) //check again
                return false;

            return  read_block_output_from_db(index_ptr);
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
#if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::store_block_call, 1);
#endif

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
                base::xauto_ptr<base::xvbindex_t> genesis_index(query_index(0, 0));
                if(!genesis_index)//if not existing at cache
                {
                    base::xauto_ptr<base::xvblock_t> generis_block(xgenesis_block::create_genesis_block(get_account()));
                    store_block(generis_block.get());
                }
            }

            xdbg("xblockacct_t::store_block,prepare for block=%s,cache_size:%zu,dump=%s",new_raw_block->dump().c_str(), m_all_blocks.size(), dump().c_str());
            #ifdef ENABLE_METRICS
            XMETRICS_TIME_RECORD_KEY("blockstore_store_block_time", new_raw_block->get_account() + ":" + std::to_string(new_raw_block->get_height()));
            #endif

            //#1:cache_block() ->link neighbor -> mark_connect_flag() -> update metric
            //#2:connect_block() ->process_block()
            //#3:save_index_to_db() -->save index-entry to db
            //#4:save_block_to_db() -->save raw-block to db

            base::xvbindex_t * final_cached_index = new_index(new_raw_block);//final_cached_ptr is a raw ptr that just valid at this moment
            if(final_cached_index != nullptr) //insert/update successful,or find duplicated one
            {
                update_meta_metric(final_cached_index); //update other meta and connect info

                //write_block_to_db may do double-check whether raw block not stored yet
                write_block_to_db(final_cached_index,new_raw_block);

                //try save index finally
                if(final_cached_index->check_modified_flag()) //if has anything changed
                {
                    write_index_to_db(final_cached_index); //save index then
                }

                //update block_level for first block
                if(m_meta->_block_level == (uint8_t)-1)
                {
                    m_meta->_block_level = new_raw_block->get_block_level();
                }

                xinfo("xblockacct_t::store_block,done for block,cache_size:%zu,new_raw_block=%s,dump=%s",m_all_blocks.size(), new_raw_block->dump().c_str(), dump().c_str());
                return true;
            }
            else if( (exist_cert) && (exist_cert->check_block_flag(base::enum_xvblock_flag_stored) == false) )
            {
                //give chance to store raw block even found duplicated indexx
                write_block_to_db(exist_cert(),new_raw_block);
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
#if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::store_block_delete, 1);
#endif
            xkinfo("xblockacct_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());

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
                        delete_block_from_db(view_it->second);

                        //remove entry then
                        view_it->second->close();
                        view_it->second->release_ref();
                        view_map.erase(view_it);
                    }
                    if(view_map.empty())
                    {
                        m_all_blocks.erase(height_it);
                        #ifdef ENABLE_METRICS
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                        #endif
                    }

                }
            }
            return true;
        }

        bool    xblockacct_t::delete_block(const uint64_t height)//return error code indicate what is result
        {
            if(height == 0)
                return false; //not allow delete genesis block

            xkinfo("xblockacct_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ") at store(%s)",get_chainid(),get_account().c_str(),height,get_blockstore_path().c_str());

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
                        #ifdef ENABLE_METRICS
                        XMETRICS_GAUGE(metrics::blockstore_cache_block_total, -1);
                        #endif
                    }
                    else
                    {
                        for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it)
                        {
                            //delete data at DB first
                            delete_block_from_db(view_it->second);
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

        bool    xblockacct_t::load_block_input(base::xvblock_t* target_block)
        {
            if(NULL == target_block)
                return false;

            if(target_block->get_block_class() == base::enum_xvblock_class_nil)
                return true;

            xdbg("xblockacct_t::load_block_input,target block(%s)",target_block->dump().c_str());
            return read_block_input_from_db(target_block);
        }

        bool    xblockacct_t::load_block_output(base::xvblock_t* target_block)
        {
            if(NULL == target_block)
                return false;

            if(target_block->get_block_class() == base::enum_xvblock_class_nil)
                return true;

            xdbg("xblockacct_t::load_block_output,target block(%s)",target_block->dump().c_str());
            return read_block_output_from_db(target_block);
        }

        bool   xblockacct_t::load_block_flags(base::xvblock_t* block_ptr)//update block'flags
        {
            if(NULL == block_ptr)
                return false;

            base::xauto_ptr<base::xvbindex_t> target_index(load_index(block_ptr->get_height(), block_ptr->get_block_hash()));
            if(!target_index)
            {
                xerror("xblockacct_t::load_block_flags,not found associated index for block(%s)",block_ptr->dump().c_str());
                return false;
            }

            //update raw block 'flag based on index
            const int index_block_flags    = target_index->get_block_flags();
            const int raw_block_flags      = block_ptr->get_block_flags();
            if((index_block_flags & base::enum_xvblock_flags_high4bit_mask) > (raw_block_flags & base::enum_xvblock_flags_high4bit_mask)
               ) //outdated one try to overwrite newer one,abort it
            {
                block_ptr->reset_block_flags(raw_block_flags | (index_block_flags & base::enum_xvblock_flags_high4bit_mask));//merge flags(just for high4bit)
                xdbg("xblockacct_t::load_block_flags,updated target block(%s)",block_ptr->dump().c_str());
            }
            return true;
        }

        bool   xblockacct_t::execute_block(base::xvblock_t* block_ptr) //execute block and update state of acccount
        {
            if(block_ptr == nullptr)
            {
                xassert(0); //should not pass nullptr
                return false;
            }
            xdbg("xblockacct_t::execute_block(block),enter block=%s",block_ptr->dump().c_str());
            store_block(block_ptr); //stored block if it not yet

            //then try load
            base::xauto_ptr<base::xvbindex_t> target_index(load_index(block_ptr->get_height(), block_ptr->get_block_hash()));
            if(!target_index)
            {
                xerror("xblockacct_t::execute_block,not found associated index for block(%s)",block_ptr->dump().c_str());
                return false;
            }

            return execute_block(target_index.get(),block_ptr);
        }

        bool   xblockacct_t::execute_block(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr) //execute block and update state of acccount
        {
            if(index_ptr == nullptr)
            {
                xassert(0); //should not pass nullptr
                return false;
            }
            if(block_ptr  == nullptr)
            {
                xassert(0);
                return false;
            }
            xdbg("xblockacct_t::execute_block(index),enter block=%s",index_ptr->dump().c_str());

            if(false == index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
            {
                xerror("xblockacct_t::execute_block(index), a non-committed block block=%s",index_ptr->dump().c_str());
                return false;
            }

            if(index_ptr->check_block_flag(base::enum_xvblock_flag_executed)) //did executed already
            {
                if (m_meta->_highest_execute_block_height < index_ptr->get_height())
                {
                    xwarn("xblockacct_t::execute_block(index) highest execute block height %" PRIu64 " less than block=%s", m_meta->_highest_execute_block_height, index_ptr->dump().c_str());
                    m_meta->_highest_execute_block_height = index_ptr->get_height();  //update meta info for executed
                    m_meta->_highest_execute_block_hash   = index_ptr->get_block_hash();
                }
                return true;
            }

            bool  is_ready_to_executed = false;
            if(  (0 == m_meta->_highest_execute_block_height)
               &&(index_ptr->get_height() == 0) ) //allow executed genesis block
            {
                is_ready_to_executed = true;
            }
            else if(   (index_ptr->get_height() == (m_meta->_highest_execute_block_height + 1))
                    && (index_ptr->get_last_block_hash() == m_meta->_highest_execute_block_hash) ) //restrict matched
            {
                is_ready_to_executed = true;
            }
            else if(   (index_ptr->get_height() > m_meta->_highest_execute_block_height)
                    && (index_ptr->get_block_class() == base::enum_xvblock_class_full) ) //any full block is eligibal to executed
            {
                //full-block need check whether state of offchain ready or not
                is_ready_to_executed = block_ptr->is_full_state_block();
            }

            if(is_ready_to_executed)
            {
                const bool executed_result =  get_xdbstore()->execute_block(block_ptr);
                if(executed_result)
                {
                    index_ptr->set_block_flag(base::enum_xvblock_flag_executed); //update flag of index
                    block_ptr->set_block_flag(base::enum_xvblock_flag_executed); //update raw block as well
                    xdbg_info("xblockacct_t::execute_block(index),successful-exectued block=%s based on height=%" PRIu64 "  ",index_ptr->dump().c_str(),index_ptr->get_height());

                    //note:store_block may update m_meta->_highest_execute_block_height as well
                    update_meta_metric(index_ptr);
                    write_index_to_db(index_ptr);
                    return true;
                }
                else
                {
                    xwarn("xblockacct_t::execute_block(index),fail-exectue for block=%s",index_ptr->dump().c_str());
                }
            }
            else
            {
                xwarn("xblockacct_t::execute_block(index),fail-ready to execute for block=%s highest_execute_block_height=%lld",index_ptr->dump().c_str(), m_meta->_highest_execute_block_height);
            }
            return false;
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
                        xerror("xblockacct_t::cache_index,fail-hash changed for block with exist height(%" PRIu64 ") and view#=%" PRIu64 " vs new block=%s at store(%s)",this_block->get_height(),existing_block->get_viewid(),this_block->dump().c_str(),get_blockstore_path().c_str());
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

                    #ifdef ENABLE_METRICS
                    XMETRICS_GAUGE(metrics::blockstore_cache_block_total, 1);
                    #endif
                    xdbg("xblockacct_t::cache_index,finally update block=%s of account=%s", this_block->dump().c_str(), m_meta->dump().c_str());

                    return existing_block;//indicate at least has changed flags
                }
                return nullptr; //nothing changed
            }
            else //insert a brand new entry
            {
                this_block->add_ref();  //hold reference now
                auto view_map_pos = target_height_map.emplace(this_block->get_viewid(),this_block);
                xassert(view_map_pos.second); //insert successful

                link_neighbor(this_block); //link as neighbor first

                #ifdef ENABLE_METRICS
                XMETRICS_GAUGE(metrics::blockstore_cache_block_total, 1);
                #endif
                xdbg("xblockacct_t::cache_index,finally cached block=%s of account=%s", this_block->dump().c_str(), m_meta->dump().c_str());
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

            if((0 == this_block_height) && (0 == m_meta->_highest_connect_block_height))
            {
                m_meta->_highest_connect_block_height = this_block_height;
                m_meta->_highest_connect_block_hash   = this_block->get_block_hash();
                this_block->set_block_flag(base::enum_xvblock_flag_connected);
            }

            if(false == this_block->check_block_flag(base::enum_xvblock_flag_connected))
            {
                //full-block must be a connected block
                if(   (this_block_height <= m_meta->_highest_connect_block_height)
                   || (this_block->get_block_class() == base::enum_xvblock_class_full)
                    )
                {
                    this_block->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status
                }
                else if(   (this_block_height == (m_meta->_highest_connect_block_height + 1)) //regular check
                        && (m_meta->_highest_connect_block_hash  == this_block->get_last_block_hash()))
                {
                    this_block->set_block_flag(base::enum_xvblock_flag_connected);
                }
                else if( (this_block->get_prev_block() != NULL) && (this_block->get_prev_block()->check_block_flag(base::enum_xvblock_flag_connected)) )//quick path
                {
                    xassert(this_block->get_prev_block()->check_block_flag(base::enum_xvblock_flag_committed));//must be true
                    this_block->set_block_flag(base::enum_xvblock_flag_connected);
                }
                else if(0 == this_block_height) //force to add it if not
                    this_block->set_block_flag(base::enum_xvblock_flag_connected);
            }

            if((0 == this_block_height) && (0 == m_meta->_highest_genesis_connect_height))
            {
                m_meta->_highest_genesis_connect_height = this_block_height;
                m_meta->_highest_genesis_connect_hash   = this_block->get_block_hash();
            }

            bool  logic_connect_more  = true;//logic connection that just ask connect to all the way to any fullblock
            bool  geneis_connect_more = true;//geneis connection that ask connect connect to all the way to geneis block
            std::vector<base::xauto_ptr<base::xvbindex_t>> fire_stored_events;
            fire_stored_events.reserve(16);//resever some sapce first

            //update record of _highest_connect_block_height/hash now
            if(this_block->check_block_flag(base::enum_xvblock_flag_connected))
            {
                //covered case of genesis block
                if(   ((0 == this_block_height) && (0 == m_meta->_highest_connect_block_height))
                   || (this_block_height > m_meta->_highest_connect_block_height) )
                {
                    m_meta->_highest_connect_block_height = this_block_height;
                    m_meta->_highest_connect_block_hash   = this_block->get_block_hash();

                    this_block->add_ref();//fire_stored_events need hold a reference to construct xauto_ptr object
                    fire_stored_events.emplace_back(base::xauto_ptr<base::xvbindex_t>(this_block));
                }
            }

            //heavy job to search from current height to m_meta->_highest_commit_block_height
            if(geneis_connect_more) //search more
            {
                const uint64_t old_highest_genesis_connect_height = m_meta->_highest_genesis_connect_height;
                for(uint64_t h = m_meta->_highest_genesis_connect_height + 1; h <= m_meta->_highest_commit_block_height; ++h)
                {
                    const uint64_t try_height = m_meta->_highest_genesis_connect_height + 1;
                    if(load_index(try_height) == 0) //missed block
                        break;
                    
                    base::xauto_ptr<base::xvbindex_t> next_commit(query_index(try_height, base::enum_xvblock_flag_committed));
                    if(!next_commit) //dont have commited block
                        break;
                    
                    if( (0 == m_meta->_highest_genesis_connect_height) && m_meta->_highest_genesis_connect_hash.empty())
                    {
                        //could be exception case that not event inited yet,so makeup
                        m_meta->_highest_genesis_connect_height = next_commit->get_height();
                        m_meta->_highest_genesis_connect_hash   = next_commit->get_block_hash();
                    }
                    else if(   (next_commit->get_height() == (m_meta->_highest_genesis_connect_height + 1))
                            && (next_commit->get_last_block_hash() == m_meta->_highest_genesis_connect_hash) )
                    {
                        m_meta->_highest_genesis_connect_height = next_commit->get_height();
                        m_meta->_highest_genesis_connect_hash   = next_commit->get_block_hash();
                    }
                    else
                    {
                        break;
                    }
                }
                
                const int  geneis_connect_step = (int)(m_meta->_highest_genesis_connect_height - old_highest_genesis_connect_height);
                xdbg("xblockacct_t::full_connect_to,navigate step(%d) to _highest_genesis_connect_height=%" PRIu64 " ",geneis_connect_step,m_meta->_highest_genesis_connect_height);
            }

            if(logic_connect_more) //search more
            {
                const uint64_t old_highest_connect_block_height = m_meta->_highest_connect_block_height;
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
                        
                        next_commit->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status,and save later
                        fire_stored_events.emplace_back(std::move(next_commit));//note:never use cur_commit anymore
                    }
                    else if(  (next_commit->get_height() == (m_meta->_highest_connect_block_height + 1))
                            && (next_commit->get_last_block_hash() == m_meta->_highest_connect_block_hash) )
                    {
                        m_meta->_highest_connect_block_height = next_commit->get_height();
                        m_meta->_highest_connect_block_hash   = next_commit->get_block_hash();
                        
                        next_commit->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status,and save later
                        fire_stored_events.emplace_back(std::move(next_commit));//note:never use cur_commit anymore
                    }
                    else
                    {
                        break;
                    }
                }
                const int  block_connect_step  = (int)(m_meta->_highest_connect_block_height - old_highest_connect_block_height);
                xdbg("xblockacct_t::full_connect_to,navigate step(%d) to _highest_connect_block_height=%" PRIu64 "  ",block_connect_step,m_meta->_highest_connect_block_height);
            }
            
            //finally send out all events.
            for(auto & index : fire_stored_events)
            {
                //XTODO,it might be good to just send latest one instead every events
                on_block_stored(index());
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
                xdbg_info("xblockacct_t::update_meta_metric,at store(%s) account=%s,commit block=%s",get_blockstore_path().c_str(), get_account().c_str(), new_block_ptr->dump().c_str());

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

                if(  (new_block_height >= m_meta->_highest_execute_block_height)
                   &&(new_block_ptr->check_block_flag(base::enum_xvblock_flag_executed)) )
                {
                    m_meta->_highest_execute_block_height = new_block_height;
                    m_meta->_highest_execute_block_hash   = new_block_ptr->get_block_hash();
                }

            }
            else if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_locked))
            {
                //update meta information now
                if(new_block_height > m_meta->_highest_cert_block_height) //committed block must also a cert block
                    m_meta->_highest_cert_block_height = new_block_height;

                if(new_block_height > m_meta->_highest_lock_block_height)
                    m_meta->_highest_lock_block_height = new_block_height;

                xdbg_info("xblockacct_t::update_meta_metric,at store(%s) lock block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());
            }
            else if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_authenticated)) //xstore should only store commit&locked block
            {
                //update meta information now
                if(new_block_height > m_meta->_highest_cert_block_height)
                    m_meta->_highest_cert_block_height = new_block_height;

                xdbg_info("xblockacct_t::update_meta_metric,at store(%s) cert block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());
            }
            return true;
        }

        bool  xblockacct_t::store_txs_to_db(base::xvbindex_t* index_ptr)
        {
            xassert(false);  // TODO(jimmy) delete later
            if(nullptr == index_ptr)
                return false;

            if(false == index_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                return false;

            if( (index_ptr->get_block_class() == base::enum_xvblock_class_light)
               && (index_ptr->get_block_level() == base::enum_xvblock_level_unit) )
            {
                if(!index_ptr->check_store_flag(base::enum_index_store_flag_transactions))
                {
                    if(load_block_object(index_ptr) == false)
                    {
                        xerror("xblockacct_t::store_txs_to_db,fail to load block object");
                        return false;
                    }
                    load_block_input(index_ptr->get_this_block());
                    load_block_output(index_ptr->get_this_block());
                    auto ret = base::xvchain_t::instance().get_xtxstore()->store_txs(index_ptr->get_this_block(),true);
                    if(ret)
                    {
                        index_ptr->set_store_flag(base::enum_index_store_flag_transactions);
                    }
                    return ret;
                }
            }
            return true;
        }

        bool    xblockacct_t::write_block_to_db(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;

            return write_block_to_db(index_ptr,index_ptr->get_this_block());
        }

        bool    xblockacct_t::write_block_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if( (NULL == index_ptr) || (NULL == block_ptr) )
                return false;

            if (index_ptr->has_parent_store())
            {
                xassert(block_ptr->get_height() != 0);
                index_ptr->set_block_flag(base::enum_xvblock_flag_stored); //mark as stored everything
                return true;
            }

            if(write_block_object_to_db(index_ptr,block_ptr) == false)
                return false;

            //new version(>=1) of block may serialize seperately
            if(block_ptr->get_block_class() == base::enum_xvblock_class_nil)
            {
                index_ptr->set_block_flag(base::enum_xvblock_flag_stored); //mark as stored everything
                return true;
            }

            //check stored input or not
            if(block_ptr->get_input() != NULL)
            {
                write_block_input_to_db(index_ptr,block_ptr);
            }

            //check stored output or not
            if(block_ptr->get_output() != NULL)
            {
                write_block_output_to_db(index_ptr,block_ptr);
            }

            const uint32_t everything_flags = base::enum_index_store_flag_mini_block | base::enum_index_store_flag_input_entity | base::enum_index_store_flag_input_resource | base::enum_index_store_flag_output_entity| base::enum_index_store_flag_output_resource;
            if(index_ptr->check_store_flags(everything_flags))
            {
                index_ptr->set_block_flag(base::enum_xvblock_flag_stored); //mark as stored everything
            }
            return true;
        }

        bool    xblockacct_t::write_block_object_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return false;
            xassert(!index_ptr->has_parent_store());

            //raw block not stored header yet
            if(index_ptr->check_store_flag(base::enum_index_store_flag_mini_block) == false)
            {
#if defined(ENABLE_METRICS)
                if (block_ptr->get_block_level() == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_table_write, 1);
                } else if (block_ptr->get_block_level() == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_unit_write, 1);
                } else {
                    XMETRICS_GAUGE(metrics::store_block_other_write, 1);
                }
#endif
                std::string blockobj_bin;
                block_ptr->serialize_to_string(blockobj_bin);
                const std::string blockobj_key = base::xvdbkey_t::create_block_object_key(*this,block_ptr->get_block_hash());
                if(get_xdbstore()->set_value(blockobj_key, blockobj_bin))
                {
                    //has stored entity of input/output inside of block
                    index_ptr->set_store_flag(base::enum_index_store_flag_input_entity);
                    index_ptr->set_store_flag(base::enum_index_store_flag_output_entity);
                    index_ptr->set_store_flag(base::enum_index_store_flag_mini_block);
                    xdbg("xblockacct_t::write_block_object_to_db,store object to DB for block(%s),bin_size=%zu",index_ptr->dump().c_str(), blockobj_bin.size());
                }
                else
                {
                    xerror("xblockacct_t::write_block_object_to_db,fail to store header for block(%s)",index_ptr->dump().c_str());
                    return false;
                }
            }
            return true;
        }

        bool    xblockacct_t::read_block_object_from_db(base::xvbindex_t* index_ptr)
        {
            xassert(!index_ptr->has_parent_store());
            if(index_ptr->get_this_block() == NULL)
            {
#if defined(ENABLE_METRICS)
                if (index_ptr->get_block_level() == base::enum_xvblock_level_table) {
                    XMETRICS_GAUGE(metrics::store_block_table_read, 1);
                } else if (index_ptr->get_block_level() == base::enum_xvblock_level_unit) {
                    XMETRICS_GAUGE(metrics::store_block_unit_read, 1);
                } else {
                    XMETRICS_GAUGE(metrics::store_block_other_read, 1);
                }
#endif
                const std::string blockobj_key = base::xvdbkey_t::create_block_object_key(*this,index_ptr->get_block_hash());
                const std::string blockobj_bin = get_xdbstore()->get_value(blockobj_key);
                if(blockobj_bin.empty())
                {
                    if(index_ptr->check_store_flag(base::enum_index_store_flag_mini_block)) //has stored header and cert
                        xerror("xblockacct_t::read_block_object_from_db,fail to find item at DB for key(%s)",blockobj_key.c_str());
                    else
                        xwarn("xblockacct_t::read_block_object_from_db,NOT stored block-object yet,index(%s) ",index_ptr->dump().c_str());

                    return false;
                }

                base::xauto_ptr<base::xvblock_t> new_block_ptr(base::xvblock_t::create_block_object(blockobj_bin));
                if(!new_block_ptr)
                {
                    xerror("xblockacct_t::read_block_object_from_db,bad data at DB for key(%s)",blockobj_key.c_str());
                    return false;
                }

                new_block_ptr->reset_block_flags(index_ptr->get_block_flags());
                index_ptr->reset_this_block(new_block_ptr.get());//link to raw block for index
            }
            return (index_ptr->get_this_block() != NULL);
        }

        bool    xblockacct_t::write_block_input_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return false;

            if(index_ptr->check_store_flag(base::enum_index_store_flag_input_entity) == false)
            {
                std::string input_bin;
                block_ptr->get_input()->serialize_to_string(input_bin);
                const std::string input_key = base::xvdbkey_t::create_block_input_key(*this,block_ptr->get_block_hash());
                if(get_xdbstore()->set_value(input_key, input_bin))
                {
                    index_ptr->set_store_flag(base::enum_index_store_flag_input_entity);
                    xdbg("xblockacct_t::write_block_input_to_db,store input entity to DB for block(%s),bin_size=%zu",index_ptr->dump().c_str(), input_bin.size());
                }
                else
                {
                    xerror("xblockacct_t::write_block_input_to_db,fail to store input entity for block(%s)",index_ptr->dump().c_str());
                    return false;
                }
            }

            if(index_ptr->check_store_flag(base::enum_index_store_flag_input_resource) == false)
            {
                if(block_ptr->get_input()->get_resources_hash().empty() == false)
                {
                    const std::string input_res_bin = block_ptr->get_input()->get_resources_data();
                    if(input_res_bin.empty() == false)
                    {
#if defined(ENABLE_METRICS)
                        XMETRICS_GAUGE(metrics::store_block_input_write, 1);
#endif
                        const std::string input_res_key = base::xvdbkey_t::create_block_input_resource_key(*this,block_ptr->get_block_hash());
                        if(get_xdbstore()->set_value(input_res_key, input_res_bin))
                        {
                            index_ptr->set_store_flag(base::enum_index_store_flag_input_resource);
                            xdbg("xblockacct_t::write_block_input_to_db,store input resource to DB for block(%s),bin_size=%zu",index_ptr->dump().c_str(), input_res_bin.size());
                        }
                        else
                        {
                            xerror("xblockacct_t::write_block_input_to_db,fail to store input resource for block(%s)",index_ptr->dump().c_str());
                            return false;
                        }
                    }
                    else //fail to found resource data for input of block
                    {
                        xerror("xblockacct_t::write_block_input_to_db,fail to found input resource for block(%s)",index_ptr->dump().c_str());
                        return false;
                    }
                }
                else //set flag for empty resource
                {
                    index_ptr->set_store_flag(base::enum_index_store_flag_input_resource);
                }
            }
            return true;
        }

        bool    xblockacct_t::read_block_input_from_db(base::xvbindex_t* index_ptr)
        {
            if(read_block_object_from_db(index_ptr))
            {
                base::xvblock_t * block_ptr = index_ptr->get_this_block();
                return read_block_input_from_db(block_ptr);
            }
            return false;
        }

        bool    xblockacct_t::read_block_input_from_db(base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return false;

            if(block_ptr->get_input() != NULL) //now has valid input
            {
                if(  (block_ptr->get_input()->get_resources_hash().empty() == false) //link resoure data
                   &&(block_ptr->get_input()->has_resource_data() == false) ) //but dont have resource avaiable now
                {
#if defined(ENABLE_METRICS)
                    XMETRICS_GAUGE(metrics::store_block_input_read, 1);
#endif
                    //which means resource are stored at seperatedly
                    const std::string input_resource_key = base::xvdbkey_t::create_block_input_resource_key(*this,block_ptr->get_block_hash());

                    const std::string input_resource_bin = get_xdbstore()->get_value(input_resource_key);
                    if(input_resource_bin.empty()) //that possible happen actually
                    {
                        xwarn_err("xblockacct_t::read_block_input_from_db,fail to read resource from db for path(%s)",input_resource_key.c_str());
                        return false;
                    }
                    if(block_ptr->set_input_resources(input_resource_bin) == false)
                    {
                        xerror("xblockacct_t::read_block_input_from_db,load bad input-resource for key(%s)",input_resource_key.c_str());
                        return false;
                    }
                    xdbg("xblockacct_t::read_block_input_from_db,read block-input resource,block(%s) ",block_ptr->dump().c_str());
                }
            }
            return (block_ptr->get_input() != NULL);
        }

        bool    xblockacct_t::write_block_output_to_db(base::xvbindex_t* index_ptr,base::xvblock_t * block_ptr)
        {
            if(block_ptr == NULL)
                return false;

            if(index_ptr->check_store_flags(base::enum_index_store_flag_output_entity) == false)
            {
                std::string output_bin;
                block_ptr->get_output()->serialize_to_string(output_bin);
                const std::string output_key = base::xvdbkey_t::create_block_output_key(*this,block_ptr->get_block_hash());
                if(get_xdbstore()->set_value(output_key, output_bin))
                {
                    index_ptr->set_store_flag(base::enum_index_store_flag_output_entity);
                    xdbg("xblockacct_t::write_block_output_to_db,store output entity to DB for block(%s),,bin_size=%zu",index_ptr->dump().c_str(), output_bin.size());
                }
                else
                {
                    xerror("xblockacct_t::write_block_output_to_db,fail to store output entity for block(%s)",index_ptr->dump().c_str());
                    return false;
                }
            }

            if(index_ptr->check_store_flag(base::enum_index_store_flag_output_resource) == false)
            {
                if(block_ptr->get_output()->get_resources_hash().empty() == false)
                {
#if defined(ENABLE_METRICS)
                    XMETRICS_GAUGE(metrics::store_block_output_write, 1);
#endif
                    const std::string output_res_bin = block_ptr->get_output()->get_resources_data();
                    if(output_res_bin.empty() == false)
                    {
                        const std::string output_res_key = base::xvdbkey_t::create_block_output_resource_key(*this,block_ptr->get_block_hash());
                        if(get_xdbstore()->set_value(output_res_key, output_res_bin))
                        {
                            index_ptr->set_store_flag(base::enum_index_store_flag_output_resource);
                            xdbg("xblockacct_t::write_block_output_to_db,store output resource to DB for block(%s),bin_size=%zu",index_ptr->dump().c_str(), output_res_bin.size());
                        }
                        else
                        {
                            xerror("xblockacct_t::write_block_output_to_db,fail to store output resource for block(%s)",index_ptr->dump().c_str());
                            return false;
                        }
                    }
                    else
                    {
                        xerror("xblockacct_t::write_block_output_to_db,fail to find output resource for block(%s)",index_ptr->dump().c_str());
                        return false;
                    }
                }
                else //set flag for empty resource
                {
                    index_ptr->set_store_flag(base::enum_index_store_flag_output_resource);
                }
            }
            return true;
        }

        bool    xblockacct_t::read_block_output_from_db(base::xvbindex_t* index_ptr)
        {
            if(read_block_object_from_db(index_ptr))
            {
                base::xvblock_t * block_ptr = index_ptr->get_this_block();
                return read_block_output_from_db(block_ptr);
            }
            return false;
        }

        bool    xblockacct_t::read_block_output_from_db(base::xvblock_t * block_ptr)
        {
            if(NULL == block_ptr)
                return false;

            if(block_ptr->get_output() != NULL) //now has valid output
            {
                if(  (block_ptr->get_output()->get_resources_hash().empty() == false) //link resoure data
                   &&(block_ptr->get_output()->has_resource_data() == false) ) //but dont have resource avaiable now
                {
#if defined(ENABLE_METRICS)
                    XMETRICS_GAUGE(metrics::store_block_output_read, 1);
#endif
                    //which means resource are stored at seperatedly
                    const std::string output_resource_key = base::xvdbkey_t::create_block_output_resource_key(*this,block_ptr->get_block_hash());

                    const std::string output_resource_bin = get_xdbstore()->get_value(output_resource_key);
                    if(output_resource_bin.empty()) //that possible happen actually
                    {
                        xwarn_err("xblockacct_t::read_block_output_from_db,fail to read resource from db for path(%s)",output_resource_key.c_str());
                        return false;
                    }
                    if(block_ptr->set_output_resources(output_resource_bin) == false)
                    {
                        xerror("xblockacct_t::read_block_output_from_db,read bad output-resource for key(%s)",output_resource_key.c_str());
                        return false;
                    }
                    xdbg("xblockacct_t::read_block_output_from_db,read output resource,block(%s) ",block_ptr->dump().c_str());
                }
            }
            return (block_ptr->get_output() != NULL);
        }

        bool    xblockacct_t::delete_block_from_db(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;

            //step#1: remove index first
            {
                if(index_ptr->check_store_flag(base::enum_index_store_flag_main_entry))
                {
                    const std::string index_key = base::xvdbkey_t::create_block_index_key(*this,index_ptr->get_height());
                    get_xdbstore()->delete_value(index_key);
                }
                else
                {
                    const std::string index_key = base::xvdbkey_t::create_block_index_key(*this,index_ptr->get_height(),index_ptr->get_viewid());
                    get_xdbstore()->delete_value(index_key);
                }
            }
            //step#2: remove raw block at db
            {
                const std::string block_obj_key = base::xvdbkey_t::create_block_object_key(*this,index_ptr->get_block_hash());
                get_xdbstore()->delete_value(block_obj_key);
            }
            //delete input
            {
                const std::string block_input_key = base::xvdbkey_t::create_block_input_key(*this,index_ptr->get_block_hash());
                get_xdbstore()->delete_value(block_input_key);

                const std::string input_resource_key = base::xvdbkey_t::create_block_input_resource_key(*this,index_ptr->get_block_hash());
                get_xdbstore()->delete_value(input_resource_key);
            }
            //delete output
            {
                const std::string block_output_key = base::xvdbkey_t::create_block_output_key(*this,index_ptr->get_block_hash());
                get_xdbstore()->delete_value(block_output_key);

                const std::string output_resource_key = base::xvdbkey_t::create_block_output_resource_key(*this,index_ptr->get_block_hash());
                get_xdbstore()->delete_value(output_resource_key);
            }

            //step#3: remove offdata
            {
                const std::string offdata_key = base::xvdbkey_t::create_block_offdata_key(*this, index_ptr->get_block_hash());
                get_xdbstore()->delete_value(offdata_key);
            }
            return true;
        }

        //return bool indicated whether has anything writed into db
        bool xblockacct_t::write_index_to_db(const uint64_t target_height)
        {
            auto it = m_all_blocks.find(target_height);
            if(it != m_all_blocks.end())
            {
                return write_index_to_db(it->second);
            }
            return false;
        }

        bool xblockacct_t::write_index_to_db(std::map<uint64_t,base::xvbindex_t*> & indexes)
        {
            if(indexes.empty())
                return false;

            for(auto it = indexes.begin(); it != indexes.end(); ++it)
            {
                write_index_to_db(it->second);//store entry really
            }
            return true;
        }

        //return bool indicated whether has anything writed into db
        bool   xblockacct_t::write_index_to_db(base::xvbindex_t* index_obj)
        {
            if(NULL == index_obj)
            {
                xassert(index_obj != NULL);
                return false;
            }
            if(index_obj->get_account_id() != get_xvid())//should not happen,but double check before save to db
            {
                xerror("xblockacct_t::write_index_to_db,passin wrong index(%" PRIu64 ") that not belong to this account(%s)",index_obj->get_account_id(),get_account().c_str());
                return false;
            }

            if(index_obj->get_height() == 0) //genesis block
                index_obj->set_store_flag(base::enum_index_store_flag_main_entry);//force to set

            bool exist_modified_flag = index_obj->check_modified_flag();
            index_obj->reset_modify_flag(); //clear the flag of modification before save

            std::string index_bin;
            if(index_obj->serialize_to(index_bin) <= 0)
            {
                if(exist_modified_flag)
                    index_obj->set_modified_flag();//restore flag as fail

                xerror("xblockacct_t::write_index_to_db,fail to serialize_to,index'dump(%s)",index_obj->dump().c_str());
                return false;
            }

            bool is_stored_db_successful = false;
            if(index_obj->check_store_flag(base::enum_index_store_flag_main_entry)) //main index for this height
            {
                const std::string key_path = base::xvdbkey_t::create_block_index_key(*this,index_obj->get_height());
                is_stored_db_successful = get_xdbstore()->set_value(key_path,index_bin);
            }
            else
            {
                const std::string key_path = base::xvdbkey_t::create_block_index_key(*this,index_obj->get_height(),index_obj->get_viewid());
                is_stored_db_successful = get_xdbstore()->set_value(key_path,index_bin);
            }

#if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::store_block_index_write, 1);
#endif

            if(false == is_stored_db_successful)
            {
                if(exist_modified_flag)
                    index_obj->set_modified_flag();//restore flag as fail

                xerror("xblockacct_t::write_index_to_db,fail to writed into db,index dump(%s)",index_obj->dump().c_str());
                return false;
            }
            return true;
        }
        void      xblockacct_t::try_execute_all_block(base::xvblock_t * target_block)
        {
            if (m_meta->_highest_execute_block_height >= m_meta->_highest_commit_block_height) {
                return;
            }
            // XTODO only tabletable need execute immediately after stored
            if (target_block->get_block_level() != base::enum_xvblock_level_table) {
                return;
            }

            xdbg("xblockacct_t::try_execute_all_block enter. block=%s,meta=%s", target_block->dump().c_str(), dump().c_str());

            // try to check and execute from target block firstly, maybe target block include snapshot by syncing
            if (m_meta->_highest_execute_block_height < target_block->get_height()  // target height not executed
                && target_block->get_block_class() == base::enum_xvblock_class_full  // must be full block
                && target_block->is_full_state_block()  // must meet execute ready condition
                && m_meta->_highest_commit_block_height >= target_block->get_height())  // must be commit block
            {
                // double check if committed block same with target block
                base::xauto_ptr<base::xvbindex_t> _execute_index = load_index(target_block->get_height(), base::enum_xvblock_flag_committed);
                if (_execute_index != nullptr
                    &&  _execute_index->get_block_hash() == target_block->get_block_hash())
                {
                    xinfo("xblockacct_t::try_execute_all_block try execute full block. %s,block=%s", dump().c_str(), target_block->dump().c_str());
                    if(false == execute_block(_execute_index.get(),target_block))
                    {
                        xerror("xblockacct_t::try_execute_all_block fail-execute full block,at block=%s",_execute_index->dump().c_str());
                    }
                    else
                    {
                        xassert(m_meta->_highest_execute_block_height >= target_block->get_height());
                        xassert(m_meta->_highest_full_block_height >= target_block->get_height());
                    }
                }
                else
                {
                    xwarn("xblockacct_t::try_execute_all_block fail-execute full block for not committed. %s,block=%s", dump().c_str(), target_block->dump().c_str());
                }
            }


            // TODO(jimmy)
            uint64_t max_count = 32;
            do
            {
                xobject_ptr_t<base::xvbindex_t> _execute_index = nullptr;
                // try to jump execute from full height
                if (m_meta->_highest_execute_block_height < m_meta->_highest_full_block_height  // check if need jump
                    && target_block->get_block_level() != base::enum_xvblock_level_table)  // full-table can't jump without snapshot
                {
                    base::xauto_ptr<base::xvbindex_t> _full_bindex(load_index(m_meta->_highest_full_block_height, base::enum_xvblock_flag_committed));
                    if(_full_bindex == nullptr)
                    {
                        xwarn("xblockacct_t::try_execute_all_block no load full index. %s", dump().c_str());
                        return;
                    }
                    _execute_index = _full_bindex;
                }

                // if can't jump to full height, then try to execute next execute height
                if (_execute_index == nullptr)
                {
                    uint64_t _query_height = (m_meta->_highest_execute_block_height == 0 && m_meta->_highest_execute_block_hash.empty()) ? 0 : m_meta->_highest_execute_block_height + 1;
                    if (_query_height > m_meta->_highest_commit_block_height)
                    {
                        xinfo("xblockacct_t::try_execute_all_block no next committed block to execute. %s", dump().c_str());
                        return;
                    }
                    base::xauto_ptr<base::xvbindex_t> _query_bindex(load_index(_query_height, base::enum_xvblock_flag_committed));
                    if(_query_bindex == nullptr)
                    {
                        xwarn("xblockacct_t::try_execute_all_block no find committed block. %s, query_height=%ld", dump().c_str(), _query_height);
                        return;
                    }
                    _execute_index = _query_bindex;
                }

                if(false == read_block_object_from_db(_execute_index.get()))
                {
                    xerror("xblockacct_t::try_execute_all_block fail-read block,at account=%s,block=%s",get_account().c_str(),_execute_index->dump().c_str());
                    return;
                }
                // load_index_input(_execute_index.get());  // execute no need load input
                load_index_output(_execute_index.get());
                //note:to avoid block ptr is release by reentery from execute_block,here have to hold ptr first
                base::auto_reference<base::xvblock_t> auto_hold_block_ptr(_execute_index->get_this_block());
                if(false == execute_block(_execute_index.get(),_execute_index->get_this_block()))
                {
                    xwarn("xblockacct_t::try_execute_all_block fail-execute block,at block=%s",_execute_index->dump().c_str());
                    return;
                }
            }
            while(max_count-- > 0);

            xdbg("xblockacct_t::try_execute_all_block finish, %s", dump().c_str());
        }

        //return map sorted by viewid from lower to high,caller respond to release ptr later
        std::vector<base::xvbindex_t*>   xblockacct_t::read_index_from_db(const uint64_t target_height)
        {
            std::vector<base::xvbindex_t*> all_blocks_at_height;

            const std::string main_entry_key = base::xvdbkey_t::create_block_index_key(*this,target_height);
            base::xvbindex_t* index_entry = read_index_from_db(main_entry_key);
            if(index_entry == NULL) //main entry
            {
                xdbg("xblockacct_t::read_index_from_db,dont find main entry for height(%" PRIu64 ")",target_height);
                return all_blocks_at_height;
            }
            if(index_entry->check_store_flag(base::enum_index_store_flag_main_entry) == false)
            {
                xerror("xblockacct_t::read_index_from_db,a bad index that lost flag main_entry,index(%s)",index_entry->dump().c_str());
                index_entry->set_store_flag(base::enum_index_store_flag_main_entry); //mark as main entry again
                index_entry->set_modified_flag(); //force to write later
            }
            all_blocks_at_height.push_back(index_entry);//transfer owner to vector
            xdbg("xblockacct_t::read_index_from_db,read main index(%s)",index_entry->dump().c_str());

            while(index_entry->get_next_viewid_offset() != 0) //check whether has other view entry
            {
                const std::string other_entry_key = base::xvdbkey_t::create_block_index_key(*this,target_height,index_entry->get_next_viewid());
                index_entry = read_index_from_db(other_entry_key);
                if(index_entry == NULL)
                    break;

                all_blocks_at_height.push_back(index_entry);//transfer owner to vector
                xdbg("xblockacct_t::read_index_from_db,read a index(%s)",index_entry->dump().c_str());
            }
            return all_blocks_at_height;
        }

        base::xvbindex_t*   xblockacct_t::read_index_from_db(const std::string & index_db_key_path)
        {
#if defined(ENABLE_METRICS)
            XMETRICS_GAUGE(metrics::store_block_index_read, 1);
#endif
            const std::string index_bin = get_xdbstore()->get_value(index_db_key_path);
            if(index_bin.empty())
            {
                xdbg("xblockacct_t::read_index_from_db,fail to read from db for path(%s)",index_db_key_path.c_str());
                return NULL;
            }

            base::xvbindex_t * new_index_obj = new base::xvbindex_t();
            if(new_index_obj->serialize_from(index_bin) <= 0)
            {
                xerror("xblockacct_t::read_index_from_db,fail to serialize from db for path(%s)",index_db_key_path.c_str());
                new_index_obj->release_ref();
                return NULL;
            }

            if(new_index_obj->check_modified_flag())
            {
                xerror("xblockacct_t::read_index_from_db,dirty index from db for path(%s)",index_db_key_path.c_str());
                new_index_obj->reset_modify_flag(); //should not happen,but add exception for incase
            }
            return new_index_obj;
        }

        //compatible for old version,e.g read meta and other stuff
        const std::string   xblockacct_t::load_value_by_path(const std::string & full_path_as_key)
        {
            return get_xdbstore()->get_value(full_path_as_key);
        }
        bool                xblockacct_t::delete_value_by_path(const std::string & full_path_as_key)
        {
            return get_xdbstore()->delete_value(full_path_as_key);
        }
        bool                xblockacct_t::store_value_by_path(const std::string & full_path_as_key,const std::string & value)
        {
            if(value.empty())
                return true;

            return get_xdbstore()->set_value(full_path_as_key,value);
        }

        bool      xblockacct_t::on_block_stored(base::xvbindex_t* index_ptr)
        {
            xdbg("jimmy xblockacct_t::on_block_stored,at account=%s,index=%s",get_account().c_str(),index_ptr->dump().c_str());
            if(index_ptr->get_height() == 0) //ignore genesis block
                return true;
            const int block_flags = index_ptr->get_block_flags();
            if((block_flags & base::enum_xvblock_flag_executed) != 0)
            {
                //here notify execution event if need
            }
            if( ((block_flags & base::enum_xvblock_flag_committed) != 0) && ((block_flags & base::enum_xvblock_flag_connected) != 0) )
            {
                base::xveventbus_t * mbus = base::xvchain_t::instance().get_xevmbus();
                xassert(mbus != NULL);
                if(mbus != NULL)
                {
                    if(index_ptr->get_height() != 0)
                    {
                        mbus::xevent_ptr_t event = mbus->create_event_for_store_index_to_db(index_ptr);
                        if (event != nullptr) {
                            mbus->push_event(event);
                        }
                    }
                    xdbg_info("xblockacct_t::on_block_stored,done at store(%s)-> block=%s",get_blockstore_path().c_str(),index_ptr->dump().c_str());
                }
            }
            return true;
        }

        bool      xblockacct_t::on_block_committed(base::xvbindex_t* index_ptr)
        {
            if(NULL == index_ptr)
                return false;
            xdbg("xblockacct_t::on_block_committed,at account=%s,index=%s",get_account().c_str(),index_ptr->dump().c_str());
            if(index_ptr->get_block_flags() & base::enum_xvblock_flag_committed
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
                    xdbg_info("xblockacct_t::on_block_committed,done at store(%s)-> block=%s",get_blockstore_path().c_str(),index_ptr->dump().c_str());
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
            m_events_queue.emplace_back(xblockevent_t(type,target));
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
            for(auto it = target_height_map.begin(); it != target_height_map.end();)
            {
                auto cur_it = it;
                auto next_it = ++it;

                //setup main-entry first
                if(cur_it == target_height_map.begin())//first force as main-entry
                {
                    if(cur_it->second->check_store_flag(base::enum_index_store_flag_main_entry) == false)
                    {
                        cur_it->second->set_store_flag(base::enum_index_store_flag_main_entry);
                        cur_it->second->set_modified_flag();
                    }
                }
                else //force it as second-entry
                {
                    if(cur_it->second->check_store_flag(base::enum_index_store_flag_main_entry))//used to be main-entry
                    {
                        cur_it->second->remove_store_flag(base::enum_index_store_flag_main_entry);
                        cur_it->second->set_modified_flag();
                    }
                }

                //setup linked viewid-offset
                int32_t new_view_offset = 0;
                if(next_it != target_height_map.end()) //has next one
                    new_view_offset = (int32_t)(((int64_t)next_it->second->get_viewid()) - ((int64_t)cur_it->second->get_viewid()));

                if(new_view_offset != cur_it->second->get_next_viewid_offset() ) //if changed
                {
                    cur_it->second->reset_next_viewid_offset(new_view_offset); //link view
                    cur_it->second->set_modified_flag(); //mark modified flag
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
                            xerror("xblockacct_t::rebase_chain_at_height,error-found forked commiteded block(%s) at store(%s)",cur_it->second->dump().c_str(),get_blockstore_path().c_str());

                            //exception handle, force reset weight to 0
                            weight = 0;

                            //exception handle, force keep the oldest one
                            base::xvbindex_t * index_to_remove = cur_it->second;
                            //erase from map first
                            target_height_map.erase(cur_it);
                            //delete data at DB then
                            delete_block_from_db(index_to_remove);
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
                        xinfo("xblockacct_t::rebase_chain_at_height,remove existing lower-weight' block(%s) < cur_max_weight(%" PRIu64 ") at store(%s)",cur_it->second->dump().c_str(),cur_max_weight,get_blockstore_path().c_str());

                        base::xvbindex_t * index_to_remove = cur_it->second;
                        //erase from map first
                        target_height_map.erase(cur_it);
                        //delete data at DB then
                        delete_block_from_db(index_to_remove);
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
                                xerror("xblockacct_t::precheck_new_index,error-try fork by new block(%s) vs existing commit block(%s) at store(%s)",new_index->dump().c_str(),it->second->dump().c_str(),get_blockstore_path().c_str());
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

            base::xauto_ptr<base::xvbindex_t > new_idx(new base::xvbindex_t(*new_raw_block));
            if(0 != new_idx->get_height())
            {
                //just keep low 4bit flags about unpack/store/connect etc
                new_idx->reset_block_flags(new_idx->get_block_flags() & base::enum_xvblock_flags_low4bit_mask);//reset all status flags and redo it from authenticated status
                new_idx->set_block_flag(base::enum_xvblock_flag_authenticated);//init it as  authenticated
                new_idx->reset_modify_flag(); //remove modified flag to avoid double saving

                load_index(new_idx->get_height()); //always load index first for non-genesis block
            }

            //note: emplace return a pair<iterator,bool>, value of bool indicate whether inserted or not, value of iterator point to inserted it
            auto height_map_pos  = m_all_blocks.emplace(new_idx->get_height(),std::map<uint64_t,base::xvbindex_t*>());
            auto & height_view_map = height_map_pos.first->second;//hight_map_pos.first->first is height, and hight_map_pos.first->second is viewmap

            //pre-check whether accept this new index
            if(precheck_new_index(new_idx(),height_view_map) == false)
            {
                xinfo("xblockacct_t::new_index,failed-precheck for block(%s) at store(%s)",new_idx->dump().c_str(),get_blockstore_path().c_str());

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
                cached_index_ptr->set_modified_flag(); //force set flag to store later
                //connect as chain,and check connected_flag and meta
                connect_index(cached_index_ptr);//here may change index'status
                //rebase forked blocks if have ,after connect_index
                rebase_chain_at_height(height_view_map);
                if(height_view_map.find(cached_index_ptr->get_viewid()) == height_view_map.end())
                {
                    xdbg("xblockacct_t::new_index,failed-new index (%s) erased after rebase at store(%s)",new_idx->dump().c_str(),get_blockstore_path().c_str());
                    return nullptr;
                }
            }

            return cached_index_ptr;
        }

        xchainacct_t::xchainacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,base::xvdbstore_t* xvdb_ptr)
            :xblockacct_t(account_addr,timeout_ms,blockstore_path,xvdb_ptr)
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
                xdbg("xchainacct_t::process_index lock at store=%s,lock-qc=%s",get_blockstore_path().c_str(),prev_block->dump().c_str());

                prev_block->add_ref();//hold it to avoid be released by rebase_chain_at_height
                {
                    prev_block->set_block_flag(base::enum_xvblock_flag_locked);

                    rebase_chain_at_height(prev_block->get_height()); //resolve other block of lower-weight thans this
                    if(prev_block->is_close() == false)//prev_block is still valid to use
                    {
                        update_meta_metric(prev_block);//update meta since block has change status
                        write_index_to_db(prev_block); //not send db event
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
                xdbg("xchainacct_t::process_index commit at store=%s,commit-qc=%s",get_blockstore_path().c_str(),prev_prev_block->dump().c_str());

                prev_prev_block->set_block_flag(base::enum_xvblock_flag_locked);//change to locked status
                prev_prev_block->set_block_flag(base::enum_xvblock_flag_committed);//change to commit status

                prev_prev_block->add_ref(); //hold it to avoid be released by rebase_chain_at_height
                rebase_chain_at_height(prev_prev_block->get_height()); //resolve other block of lower-weight thans this
                if(prev_prev_block->is_close() == false) //prev_prev_block is still valid to use
                {
                    update_meta_metric(prev_prev_block);//update meta since block has change status

                    push_event(enum_blockstore_event_committed,prev_prev_block);//fire event for commit
                    write_index_to_db(prev_prev_block); //trigger db event here if need
                }
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
            if(this_block_height > 0)
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

    };//end of namespace of vstore
};//end of namespace of top
