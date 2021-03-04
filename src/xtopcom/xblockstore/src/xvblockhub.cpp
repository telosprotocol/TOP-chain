// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "xbase/xcontext.h"
#include "xvblockhub.h"
#include "xvgenesis.h"
#include "xdata/xnative_contract_address.h"
#include "xstore/xstore_face.h"
#include "xdata/xgenesis_data.h"

// #ifndef __MAC_PLATFORM__
    #include "xconfig/xconfig_register.h"
    #include "xdata/xtableblock.h"
    #include "xconfig/xpredefined_configurations.h"
    #include "xmbus/xevent_store.h"
    #include "xmetrics/xmetrics.h"
// #endif

namespace top
{
    namespace store
    {
        xacctmeta_t*  xacctmeta_t::load(const std::string & meta_serialized_data)
        {
            if(meta_serialized_data.empty()) //check first
                return NULL;

            base::xstream_t _stream(base::xcontext_t::instance(),(uint8_t*)meta_serialized_data.data(),(uint32_t)meta_serialized_data.size());
            xdataunit_t*  _data_obj_ptr = base::xdataunit_t::read_from(_stream);
            if(nullptr == _data_obj_ptr)
            {
                xerror("xacctmeta_t::load,bad meta_serialized_data that not follow spec");
                return NULL;
            }
            xacctmeta_t* meta_ptr = (xacctmeta_t*)_data_obj_ptr->query_interface(base::xdataunit_t::enum_xdata_type_vaccountmeta);
            if(nullptr == meta_ptr)
            {
                xerror("xacctmeta_t::load,,bad object type for meta_serialized_data,but for type:%d",_data_obj_ptr->get_obj_type());

                _data_obj_ptr->release_ref();
                return NULL;
            }
            return meta_ptr;
        }

        xacctmeta_t::xacctmeta_t()
            :base::xdataobj_t(base::xdataunit_t::enum_xdata_type_vaccountmeta)
        {
            _highest_cert_block_height     = 0;
            _highest_lock_block_height     = 0;
            _highest_commit_block_height   = 0;
            _highest_execute_block_height  = 0;
            _highest_connect_block_height  = 0;
            _highest_full_block_height     = 0;
        }

        xacctmeta_t::~xacctmeta_t()
        {
        }

        std::string xacctmeta_t::dump() const
        {
            char local_param_buf[128];
            xprintf(local_param_buf,sizeof(local_param_buf),"{meta:height for cert=%" PRIu64 ",lock=%" PRIu64 ",commit=%" PRIu64 ",execute=%" PRIu64 ",connected=%" PRIu64 ",full=%" PRIu64 "}",(int64_t)_highest_cert_block_height,(int64_t)_highest_lock_block_height,(int64_t)_highest_commit_block_height,(int64_t)_highest_execute_block_height,(int64_t)_highest_connect_block_height,_highest_full_block_height);

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

        bool  xblockacct_t::init_account(base::xcontext_t &_context)//do initialize for all accounts
        {
            base::auto_new_registor<xacctmeta_t>::_register(_context); //register xacctmeta_t to dynamic create for read_from
            return true;
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

        xblockacct_t::xblockacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore)
            :base::xobject_t(base::enum_xobject_type_vaccount),
             base::xvaccount_t(account_addr)
        {
            m_meta = NULL;
            m_persist_db = NULL;
            m_blockstore = NULL;

            _persist_db.add_ref();
            m_persist_db = &_persist_db;

            _blockstore.add_ref();
            m_blockstore = &_blockstore;

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

            close_blocks();
            if(m_meta != nullptr)
                m_meta->release_ref();

            m_persist_db->release_ref();
            m_blockstore->release_ref();
        }

        std::string xblockacct_t::dump() const  //just for debug purpose
        {
            char local_param_buf[256];
            xprintf(local_param_buf,sizeof(local_param_buf),"{account=%s ->latest height for full=%" PRId64 ",connect=%" PRId64 ",commit=%" PRId64 ",execute=%" PRId64 " < lock=%" PRId64 " < cert=%" PRId64 "; at store(%s)}",get_address().c_str(),m_meta->_highest_full_block_height,m_meta->_highest_connect_block_height,m_meta->_highest_commit_block_height,m_meta->_highest_execute_block_height,m_meta->_highest_lock_block_height,m_meta->_highest_cert_block_height,get_blockstore_path().c_str());

            return std::string(local_param_buf);
        }

        bool  xblockacct_t::init()
        {
            //first load meta data from xdb/xstore
            const std::string full_meta_path = get_blockstore_path() + get_meta_path(*this);
            const std::string meta_content = load_value_by_path(full_meta_path);
            m_meta = xacctmeta_t::load(meta_content);
            if(nullptr == m_meta)
                m_meta = new xacctmeta_t();

            check_meta();
            //first load latest connected block that should >= _highest_full_block_height
            base::xauto_ptr<base::xvblock_t> latest_connected_block(load_block_object(m_meta->_highest_connect_block_height,true));
            if(latest_connected_block == nullptr)
            {
                xwarn_err("xblockacct_t::init(),fail-load highest connected block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_connect_block_height,get_account().c_str(),get_blockstore_path().c_str());

                m_meta->_highest_connect_block_hash.clear(); //clean hash first
                int max_try_reload_count = 256;
                while( (m_meta->_highest_connect_block_height > 0) && (max_try_reload_count > 0) )//error handling for db corrupt issue
                {
                    --max_try_reload_count;
                    --m_meta->_highest_connect_block_height;
                    base::xvblock_t * loaded_block = load_block(m_meta->_highest_connect_block_height,1);
                    if(NULL != loaded_block)
                    {
                        m_meta->_highest_connect_block_hash = loaded_block->get_block_hash();
                        xkinfo("xblockacct_t::init(),failover and reloaded highest connected block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_connect_block_height,get_account().c_str(),get_blockstore_path().c_str());
                        break;
                    }
                }
            }
            else //exception protect in case DB corrupt
            {
                if(m_meta->_highest_connect_block_hash.empty())
                    m_meta->_highest_connect_block_hash = latest_connected_block->get_block_hash();
                else
                    xassert(m_meta->_highest_connect_block_hash == latest_connected_block->get_block_hash());
            }

            //then load the latest full-bock
            base::xauto_ptr<base::xvblock_t> latest_full_block(load_block_object(m_meta->_highest_full_block_height,true));
            if(latest_full_block == nullptr)
            {
                xwarn_err("xblockacct_t::init(),fail-load highest full-block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_full_block_height,get_account().c_str(),get_blockstore_path().c_str());

                m_meta->_highest_full_block_height = 0;//error handling for db corrupt issue
            }

            //pre-load lastest executed block and full-block pointed
            base::xauto_ptr<base::xvblock_t> latest_execute_block(load_block_object(m_meta->_highest_execute_block_height,true));
            if(latest_execute_block == nullptr)
            {
                xwarn_err("xblockacct_t::init(),fail-load highest executed block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_execute_block_height,get_account().c_str(),get_blockstore_path().c_str());
            }
            else if(latest_execute_block->get_height() != 0)
            {
                base::xauto_ptr<base::xvblock_t> dummy(load_block_object(latest_execute_block->get_last_full_block_height(),true));
            }
            //pre-load latest commit block and full-block pointed
            base::xauto_ptr<base::xvblock_t> latest_commit_block(load_block_object(m_meta->_highest_commit_block_height,true));
            if(latest_commit_block == nullptr)
            {
                xwarn_err("xblockacct_t::init(),fail-load highest commited block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_commit_block_height,get_account().c_str(),get_blockstore_path().c_str());

                m_meta->_highest_commit_block_height   = 0; //error handling for db corrupt issue
            }
            else if(latest_commit_block->get_height() != 0)
            {
                base::xauto_ptr<base::xvblock_t> dummy(load_block_object(latest_commit_block->get_last_full_block_height(),true));
            }
            //pre-load latest lock block and full-block pointed
            base::xauto_ptr<base::xvblock_t> latest_lock_block(load_block_object(m_meta->_highest_lock_block_height,true));
            if(latest_lock_block == nullptr)
            {
                xwarn_err("xblockacct_t::init(),fail-load highest locked block at height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_lock_block_height,get_account().c_str(),get_blockstore_path().c_str());
                m_meta->_highest_lock_block_height = m_meta->_highest_commit_block_height;//error handling for db corrupt issue
            }
            else if(latest_lock_block->get_height() != 0)
            {
                base::xauto_ptr<base::xvblock_t> dummy(load_block_object(latest_lock_block->get_last_full_block_height(),true));
            }

            //then load latest cert block
            if(m_meta->_highest_cert_block_height > m_meta->_highest_lock_block_height)
            {
                const std::string  _last_cert_block_path  = full_meta_path + "/lastcert/";
                base::xauto_ptr<base::xvblock_t> new_block_ptr(load_block_by_path(_last_cert_block_path));
                if( (new_block_ptr != nullptr) && (new_block_ptr->get_height() == m_meta->_highest_cert_block_height) )
                {
                    store_block(new_block_ptr.get()); //put into memory
                }
            }

            xinfo("xblockacct_t::init,account=%s at blockstore=%s,objectid=% " PRId64 ",meta=%s",
                    dump().c_str(),m_blockstore_path.c_str(),
                    get_obj_id(),m_meta->base::xobject_t::dump().c_str());
            return true;
        }

        void  xblockacct_t::check_meta()
        {
            xdbg_info("xblockacct_t::check_meta load %s metadata=%s", get_account().c_str(), m_meta->dump().c_str());

            // then try load latest cert block
            const std::string full_meta_path = get_blockstore_path() + get_meta_path(*this);
            const std::string  _last_cert_block_path  = full_meta_path + "/lastcert/";
            base::xauto_ptr<base::xvblock_t> new_block_ptr(load_block_by_path(_last_cert_block_path));
            if (new_block_ptr != nullptr)
            {
                if (new_block_ptr->get_height() >  m_meta->_highest_cert_block_height)
                {
                    xwarn("xblockacct_t::check_meta load %s metadata=%s highqc height diff %lu", get_account().c_str(), m_meta->dump().c_str(), new_block_ptr->get_height());

                    m_meta->_highest_cert_block_height = new_block_ptr->get_height();

                    //try pre-load latest lock block since lock block is not executed
                    uint64_t try_lock_height = m_meta->_highest_cert_block_height - 1;
                    base::xauto_ptr<base::xvblock_t> latest_lock_block(load_block_object(try_lock_height,true));
                    if(latest_lock_block != nullptr && m_meta->_highest_lock_block_height < try_lock_height)
                    {
                        m_meta->_highest_lock_block_height = try_lock_height;
                    }

                    uint64_t try_commit_height = try_lock_height - 1;
                    base::xauto_ptr<base::xvblock_t> latest_commit_block(load_block_object(try_commit_height,true));
                    if(latest_commit_block != nullptr && m_meta->_highest_commit_block_height < try_commit_height)
                    {
                        m_meta->_highest_commit_block_height = try_commit_height;
                    }

                    xwarn("xblockacct_t::check_meta load %s metadata update %lu %lu %lu", get_account().c_str(), m_meta->_highest_commit_block_height, m_meta->_highest_lock_block_height, m_meta->_highest_cert_block_height);
                }
            }

            // force to load execute height from xstore blockchain, the highqc height will be missed
            base::xauto_ptr<xblockchain2_t> blockchain = m_persist_db->clone_account(get_account());
            if (blockchain != nullptr)
            {
                xassert(m_meta->_highest_execute_block_height <= blockchain->get_last_height());
                if (m_meta->_highest_execute_block_height < blockchain->get_last_height())
                {
                    xkinfo("xblockacct_t::init(),failover and reloaded highest execute block, old height(%" PRId64 ") new height(%" PRId64 ") of account(%s) at store(%s)",m_meta->_highest_execute_block_height,blockchain->get_last_height(),get_account().c_str(),get_blockstore_path().c_str());
                    m_meta->_highest_execute_block_height = blockchain->get_last_height();
                    m_meta->_highest_execute_block_hash = blockchain->get_last_block_hash();

                    if (m_meta->_highest_commit_block_height < m_meta->_highest_execute_block_height)
                    {
                        m_meta->_highest_commit_block_height = m_meta->_highest_execute_block_height;
                    }

                    if (m_meta->_highest_lock_block_height < m_meta->_highest_commit_block_height)
                    {
                        m_meta->_highest_lock_block_height = m_meta->_highest_commit_block_height;
                    }
                    //try pre-load latest lock block since lock block is not executed
                    uint64_t try_lock_height = m_meta->_highest_commit_block_height + 1;
                    base::xauto_ptr<base::xvblock_t> latest_lock_block(load_block_object(try_lock_height,true));
                    if(latest_lock_block != nullptr && m_meta->_highest_lock_block_height < try_lock_height)
                    {
                        m_meta->_highest_lock_block_height = try_lock_height;
                    }

                    if (m_meta->_highest_cert_block_height < m_meta->_highest_lock_block_height)
                    {
                        m_meta->_highest_cert_block_height = m_meta->_highest_lock_block_height;
                    }
                }
            }
        }

        bool  xblockacct_t::save_meta()
        {
            std::string vmeta_bin;
            m_meta->serialize_to_string(vmeta_bin);
            if (m_last_save_vmeta_bin != vmeta_bin) {
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

                const std::string full_meta_path = get_blockstore_path() + get_meta_path(*this);
                if(false == m_all_blocks.empty()) //save latest cert block to persist storage with fixed location
                {
                    auto  last_height_it = m_all_blocks.rbegin();
                    auto  last_view_it  = last_height_it->second.rbegin();
                    if(last_view_it != last_height_it->second.rend())
                    {
                        const int block_flags = last_view_it->second->get_block_flags();
                        if((block_flags & (base::enum_xvblock_flag_locked | base::enum_xvblock_flag_committed)) == 0)//cert-only block
                        {
                            const std::string  _last_cert_block_path  = full_meta_path + "/lastcert/";;
                            base::xvblock_t *  _latest_cert_block = last_view_it->second;
                            store_block_by_path(_last_cert_block_path,_latest_cert_block);
                        }
                    }
                }
                //then clean all blocks at memory
                close_blocks();

                //finally save meta data of account
                save_meta();

                //TODO, retore following assert check after full_block enable
                //xassert(m_meta->_highest_full_block_height    <= m_meta->_highest_connect_block_height);
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
            if( timenow_ms > (m_idle_timeout_ms + m_last_access_time_ms) )
                return false;
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
                        const uint64_t this_block_height = view_it->second->get_height();
                        const int      this_block_flags  = view_it->second->get_block_flags();

                        //here go through every stored block to maintain connect status
                        if( (this_block_flags & base::enum_xvblock_flag_stored) != 0)//did stored before
                        {
                            if((this_block_flags & base::enum_xvblock_flag_connected) != 0)
                            {
                                if(this_block_height > m_meta->_highest_connect_block_height)//recheck
                                {
                                    m_meta->_highest_connect_block_height = this_block_height;
                                    m_meta->_highest_connect_block_hash   = view_it->second->get_block_hash();
                                }
                            }
                            else  if(  (this_block_height == (m_meta->_highest_connect_block_height + 1))
                                     &&((this_block_flags & base::enum_xvblock_flag_committed) != 0) )//only allow commit block
                            {
                                if(m_meta->_highest_connect_block_hash  == view_it->second->get_last_block_hash())
                                {
                                    m_meta->_highest_connect_block_height = this_block_height;
                                    m_meta->_highest_connect_block_hash   = view_it->second->get_block_hash();
                                    view_it->second->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status
                                }
                            }
                        }

                        if(view_it->second->get_modified_count() > 0) //has changed since last store
                            save_block(view_it->second);//save_block may drop cert-only block

                        xdbg_info("xblockacct_t::close_blocks,block=%s",view_it->second->dump().c_str());

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

        base::xvblock_t*   xblockacct_t::get_genesis_block()
        {
            return load_block_object(0,true);
        }

        base::xvblock_t*   xblockacct_t::get_latest_cert_block()
        {
            base::xvblock_t* result = query_latest_block(base::enum_xvblock_flag_authenticated);
            if(result != nullptr)
            {
                result->add_ref();
                return result;
            }
            return get_genesis_block();
        }

        base::xvblock_t*    xblockacct_t::get_latest_locked_block()
        {
            base::xvblock_t* result = query_latest_block(base::enum_xvblock_flag_locked);
            if(result != nullptr)
            {
                result->add_ref();
                return result;
            }
            return get_genesis_block();
        }

        base::xvblock_t*   xblockacct_t::get_latest_committed_block()
        {
            base::xvblock_t* result = query_latest_block(base::enum_xvblock_flag_committed);
            if(result != nullptr)
            {
                result->add_ref();
                return result;
            }
            return get_genesis_block();
        }

        base::xvblock_t*    xblockacct_t::get_latest_executed_block()
        {
            base::xvblock_t* result = query_latest_block(base::enum_xvblock_flag_executed);
            if(result != nullptr)
            {
                result->add_ref();
                return result;
            }
            return get_genesis_block();
        }

        base::xvblock_t*  xblockacct_t::get_latest_connected_block() //block has connected to genesis or latest full-block
        {
            base::xvblock_t* result = query_latest_block(base::enum_xvblock_flag_connected);
            if(result != nullptr)
            {
                result->add_ref();
                return result;
            }
            return get_genesis_block();
        }

        base::xvblock_t*  xblockacct_t::get_latest_full_block()
        {
            //load from _highest_full_block_height first
            {
                base::xvblock_t* _latest_full_block = load_block_object(m_meta->_highest_full_block_height,true);
                xassert(_latest_full_block != nullptr);
                if(_latest_full_block != nullptr)
                    return _latest_full_block;//load_block_object already add reference,so here just return
            }
            //then fail-handling
            {
                base::xvblock_t* latest_commit_block = query_latest_block(base::enum_xvblock_flag_committed);
                xassert(latest_commit_block != nullptr);
                if(latest_commit_block != nullptr)
                {
                    if( (latest_commit_block->get_block_class() == base::enum_xvblock_class_full) || (latest_commit_block->get_height() == 0) )
                    {
                        latest_commit_block->add_ref();//added reference before return
                        return latest_commit_block;//note:genesis is qualifed as definition of full block
                    }
                    return load_block_object(latest_commit_block->get_last_full_block_height(),true);
                }
            }
            //bottom line from genesis block
            return get_genesis_block();
        }

        base::xvblock_t*  xblockacct_t::get_latest_current_block(bool ask_full_load)
        {
            if (m_meta->_highest_cert_block_height <= 1)
            {
                base::xvblock_t* _latest_current_block = get_latest_cert_block();
                if(_latest_current_block != nullptr)
                {
                    return _latest_current_block;
                }
            }
            // normal case
            if ((m_meta->_highest_commit_block_height == m_meta->_highest_connect_block_height)
                && (m_meta->_highest_lock_block_height == m_meta->_highest_commit_block_height + 1)
                && (m_meta->_highest_cert_block_height == m_meta->_highest_lock_block_height + 1))
            {
                base::xvblock_t* _latest_current_block = get_latest_cert_block();
                if(_latest_current_block != nullptr)
                {
                    return _latest_current_block;
                }
            }

            // abnornal case will try to load block from highest connect height
            base::xvblock_t* connect_block = query_block(m_meta->_highest_connect_block_height,base::enum_xvblock_flag_authenticated);
            if (connect_block != nullptr)
            {
                connect_block->add_ref();
            }
            else
            {
                connect_block = load_block_object(m_meta->_highest_connect_block_height,ask_full_load);
            }
            // return genesis block if not find connect block
            if (connect_block == nullptr)
            {
                xerror("xblockacct_t::get_latest_current_block not find connect block. meta=%s", m_meta->dump().c_str());
                return get_genesis_block();
            }

            // find connect + 1 block
            base::xvblock_t* connect_next_block = query_block(m_meta->_highest_connect_block_height + 1,base::enum_xvblock_flag_authenticated);
            if (connect_next_block != nullptr)
            {
                connect_next_block->add_ref();
            }
            else
            {
                connect_next_block = load_block_object(m_meta->_highest_connect_block_height + 1,ask_full_load);
            }
            // return connect block if not find connect + 1 block
            if (connect_next_block == nullptr)
            {
                xwarn("xblockacct_t::get_latest_current_block return connect block. meta=%s,block=%s", m_meta->dump().c_str(), connect_block->dump().c_str());
                return connect_block;
            }
            if (false == connect_next_block->check_block_flag(base::enum_xvblock_flag_locked)) {
                xwarn("xblockacct_t::get_latest_current_block return connect + 1 block. meta=%s,block=%s", m_meta->dump().c_str(), connect_next_block->dump().c_str());
                connect_block->release_ref();
                return connect_next_block;
            }

            // find connect + 2 block
            base::xvblock_t* connect_next_next_block = query_block(m_meta->_highest_connect_block_height + 2,base::enum_xvblock_flag_authenticated);
            if (connect_next_next_block != nullptr)
            {
                connect_next_next_block->add_ref();
            }
            else
            {
                connect_next_next_block = load_block_object(m_meta->_highest_connect_block_height + 2,ask_full_load);
            }
            if (connect_next_next_block == nullptr)
            {
                xwarn("xblockacct_t::get_latest_current_block return connect+1 block for not find connect+2 block. meta=%s,block=%s", m_meta->dump().c_str(), connect_next_block->dump().c_str());
                connect_block->release_ref();
                return connect_next_block;
            }

            // if locked block is diverged, return connect block, need sync connect + 1 block.
            if (connect_next_block->get_block_hash() != connect_next_next_block->get_last_block_hash())
            {
                if ((!connect_next_block->check_block_flag(base::enum_xvblock_flag_committed) && connect_next_next_block->check_block_flag(base::enum_xvblock_flag_committed))
                  ||(!connect_next_block->check_block_flag(base::enum_xvblock_flag_locked) && connect_next_next_block->check_block_flag(base::enum_xvblock_flag_locked)))
                {
                    xwarn("xblockacct_t::get_latest_current_block return connect block for last block hash of connect+2 block is not connect+1 block . meta=%s, connect+1 block=%s,connect+2 block=%s",
                        m_meta->dump().c_str(), connect_next_block->dump().c_str(), connect_next_next_block->dump().c_str());
                    connect_next_block->release_ref();
                    connect_next_next_block->release_ref();
                    return connect_block;
                }
            }

            // find connect + 3 block, check if block is forked
            base::xvblock_t* connect_next_next_next_block = query_block(m_meta->_highest_connect_block_height + 3,base::enum_xvblock_flag_authenticated);
            if (connect_next_next_next_block != nullptr)
            {
                connect_next_next_next_block->add_ref();
            }
            else
            {
                connect_next_next_next_block = load_block_object(m_meta->_highest_connect_block_height + 3,ask_full_load);
            }
            if (connect_next_next_next_block != nullptr)
            {
                if (connect_next_next_block->get_block_hash() != connect_next_next_next_block->get_last_block_hash()
                 && connect_next_next_next_block->check_block_flag(base::enum_xvblock_flag_committed))
                {
                    xwarn("xblockacct_t::get_latest_current_block return connect+1 block for last block hash of connect+3 block is not connect+2 block . meta=%s, connect+2 block=%s,connect+3 block=%s",
                        m_meta->dump().c_str(), connect_next_next_block->dump().c_str(), connect_next_next_next_block->dump().c_str());
                    connect_block->release_ref();
                    connect_next_next_block->release_ref();
                    connect_next_next_next_block->release_ref();
                    return connect_next_block;
                }
                connect_next_next_next_block->release_ref();
            }

            xwarn("xblockacct_t::get_latest_current_block return connect+2 block for find connect+2 block. meta=%s,block=%s", m_meta->dump().c_str(), connect_next_next_block->dump().c_str());
            connect_block->release_ref();
            connect_next_block->release_ref();
            return connect_next_next_block;
        }

        base::xvblock_t*  xblockacct_t::get_block(const uint64_t height, const uint64_t viewid)
        {
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    for(auto view_it = view_map.rbegin(); view_it != view_map.rend(); ++view_it) //search from highest view#
                    {
                        if( (0 == viewid) || (view_it->second->get_viewid() == viewid))
                        {
                            view_it->second->add_ref();
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }

        base::xvblock_t*  xblockacct_t::get_block(const uint64_t height, const std::string & blockhash)
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

        //query all blocks at target height, it might have mutiple certs at target height
        base::xblock_vector xblockacct_t::get_blocks(const uint64_t height)
        {
            std::vector<base::xvblock_t*> all_blocks_at_height;
            if(false == m_all_blocks.empty())
            {
                auto height_it = m_all_blocks.find(height);
                if(height_it != m_all_blocks.end())
                {
                    auto & view_map  = height_it->second;
                    all_blocks_at_height.reserve(view_map.size()); //just reserved intead of resizing
                    for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it) //search from lower view#
                    {
                        view_it->second->add_ref();//add reference for result
                        all_blocks_at_height.push_back(view_it->second);
                    }
                }
            }
            return base::xblock_vector(all_blocks_at_height);
        }

        //one api to get latest_commit/latest_lock/latest_cert for better performance
        bool               xblockacct_t::get_latest_blocks_list(base::xvblock_t* & cert_block,base::xvblock_t* & lock_block,base::xvblock_t* & commit_block)
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
                            cert_block = view_it->second;
                            cert_block->add_ref();
                        }
                        if( (lock_block == nullptr) && (view_it->second->check_block_flag(base::enum_xvblock_flag_locked)) )
                        {
                            lock_block = view_it->second;
                            lock_block->add_ref();
                        }
                        if( (commit_block == nullptr) && (view_it->second->check_block_flag(base::enum_xvblock_flag_committed)) )
                        {
                            commit_block = view_it->second;
                            commit_block->add_ref();
                        }

                        if( (cert_block != nullptr) && (lock_block != nullptr) && (commit_block != nullptr) )
                            break;
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

        //just load vblock object ,but not load header and body those need load seperately if need
        base::xvblock_t*     xblockacct_t::load_block_object(const uint64_t target_height,bool ask_full_load)
        {
            base::xvblock_t* loaded_block = query_block(target_height,base::enum_xvblock_flag_locked);//only load locked or committed block
            if(loaded_block == nullptr)//try load from db if not found at cache
                loaded_block = load_block(target_height,1);

            if(loaded_block != nullptr)
            {
                loaded_block->add_ref();//add reference before return
                if(ask_full_load) //ensure has input,output as well
                {
                    load_block_input(loaded_block);
                    load_block_output(loaded_block);
                }
            }
            return loaded_block;
        }

        bool    xblockacct_t::load_block_input(base::xvblock_t* block_ptr) //load and assign body data into  xvblock_t
        {
            xassert(block_ptr != NULL);
            if(nullptr == block_ptr)
                return false;

            if( (block_ptr->get_block_class() == base::enum_xvblock_class_nil) || block_ptr->is_input_ready(true) )
                return true;

            if(block_ptr->check_block_flag(base::enum_xvblock_flag_committed) || block_ptr->check_block_flag(base::enum_xvblock_flag_locked) ) //only stored locked or commited blocks at db
            {
                if(m_persist_db->get_vblock_input(get_account(),block_ptr))
                {
                    xdbg_info("xblockacct_t::load_block_input,loaded it for block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());
                    return true;
                }
                else
                {
                    xerror("xblockacct_t::load_block_input,fail-load it for block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());
                    return false;
                }
            }
            return true;
        }

        bool    xblockacct_t::load_block_output(base::xvblock_t* block_ptr) //load and assign body data into  xvblock_t
        {
            xassert(block_ptr != NULL);
            if(nullptr == block_ptr)
                return false;

            if( (block_ptr->get_block_class() == base::enum_xvblock_class_nil) || block_ptr->is_output_ready(true) )
                return true;

            if( block_ptr->check_block_flag(base::enum_xvblock_flag_committed) || block_ptr->check_block_flag(base::enum_xvblock_flag_locked) ) //only stored locked or commited blocks at db
            {
                if(m_persist_db->get_vblock_output(get_account(),block_ptr))
                {
                    xdbg_info("xblockacct_t::load_block_output,loaded it for block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());
                    return true;
                }
                else
                {
                    xerror("xblockacct_t::load_block_output,fail-load it for block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());
                    return false;
                }
            }
            return true;
        }

        bool   xblockacct_t::store_blocks(std::vector<base::xvblock_t*> & batch_store_blocks) //better performance
        {
            //std::sort(batch_store_blocks.begin(),batch_store_blocks.end(),base::less_by_block_height());
            for(auto it : batch_store_blocks)
            {
                if((it != nullptr) && (it->get_account() == get_account()) )//here do account check since xvblockstore_impl not do batch check
                    store_block(it);
            }
            return true;
        }

        bool    xblockacct_t::delete_block(base::xvblock_t* block_ptr)//return error code indicate what is result
        {
            if(nullptr == block_ptr)
                return false;

            xkinfo("xblockacct_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ")->viewid(%" PRIu64 ") at store(%s)",block_ptr->get_chainid(),block_ptr->get_account().c_str(),block_ptr->get_height(),block_ptr->get_viewid(),get_blockstore_path().c_str());

            //only allow delete cert-only blocks
            if(false == block_ptr->check_block_flag(base::enum_xvblock_flag_locked))
            {
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
            }
            return true;
        }

        bool    xblockacct_t::delete_block(uint64_t height)//return error code indicate what is result
        {
            xkinfo("xblockacct_t::delete_block,delete block:[chainid:%u->account(%s)->height(%" PRIu64 ") at store(%s)",get_chainid(),get_account().c_str(),height,get_blockstore_path().c_str());
#ifndef __MAC_PLATFORM__
            if (get_account() == sys_contract_beacon_timer_addr)
            {
                //allow delete outdated blocks
                if(false == m_all_blocks.empty())
                {
                    auto height_it = m_all_blocks.find(height);
                    if(height_it != m_all_blocks.end())
                    {
                        auto & view_map  = height_it->second;
                        for(auto view_it = view_map.begin(); view_it != view_map.end(); ++view_it)
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
            }
#endif
            return true;
        }

        bool   xblockacct_t::execute_block(base::xvblock_t* block) //execute block and update state of acccount
        {
            if( (block == nullptr) || (false == block->check_block_flag(base::enum_xvblock_flag_committed)) || (false == block->check_block_flag(base::enum_xvblock_flag_connected)) )
                return false;

            if(block->check_block_flag(base::enum_xvblock_flag_executed)) //did executed already
            {
                if (m_meta->_highest_execute_block_height < block->get_height())
                {
                    xwarn("xblockacct_t::execute_block highest execute block height %ld less than block=%s", m_meta->_highest_execute_block_height, block->dump().c_str());
                    m_meta->_highest_execute_block_height = block->get_height();  //update meta info for executed
                    m_meta->_highest_execute_block_hash   = block->get_block_hash();
                }
                return true;
            }

            if(false == unload_subblock_from_container(block)) //check and unload tableblock etc first
            {
                return false;
            }

            bool  is_ready_to_executed = false;
            if(  (0 == m_meta->_highest_execute_block_height)
               &&(block->get_height() == 0) ) //allow executed genesis block
            {
                is_ready_to_executed = true;
            }
            else if( (block->get_height() == (m_meta->_highest_execute_block_height + 1))
               && (block->get_last_block_hash() == m_meta->_highest_execute_block_hash) ) //restrict matched
            {
                is_ready_to_executed = true;
            }
            else if(   (block->get_height() > m_meta->_highest_execute_block_height)
                    && (block->get_block_class() == base::enum_xvblock_class_full) ) //any full block is eligibal to executed
            {
                //full_property_attached to full-block
                is_ready_to_executed = true;
            }

            if(is_ready_to_executed)
            {
                const bool executed_result = m_persist_db->execute_block(block);
                if(executed_result)
                {
                    block->set_block_flag(base::enum_xvblock_flag_executed); //update flag of block
                    bool store_db_result = save_to_xdb(block);
                    if (!store_db_result) {
                        m_blockstore->remove_block_flag(block,base::enum_xvblock_flag_executed);
                        xerror("xblockacct_t::execute_block,store to xdb fail block=%s",block->dump().c_str());
                        return false;
                    }

                    const uint64_t old_executed_height = m_meta->_highest_execute_block_height;
                    m_meta->_highest_execute_block_height = block->get_height();  //update meta info for executed
                    m_meta->_highest_execute_block_hash   = block->get_block_hash();
                    xinfo("xblockacct_t::execute_block,successful-exectued block=%s based on height=%" PRIu64 "  ",block->dump().c_str(),old_executed_height);

//                    auto heigh_it = m_all_blocks.find(m_meta->_highest_execute_block_height + 1); //search more
//                    if(heigh_it != m_all_blocks.end())
//                    {
//                        if(   (heigh_it->first == (m_meta->_highest_execute_block_height + 1))
//                           && (false == heigh_it->second.empty()) )
//                        {
//                            auto view_it = heigh_it->second.begin();
//                            if(view_it->second->check_block_flag(base::enum_xvblock_flag_committed))
//                            {
//                                execute_block(view_it->second);
//                            }
//                        }
//                    }
                    return true;
                }
                else
                {
                    xwarn("xblockacct_t::execute_block,fail-exectue for block=%s",block->dump().c_str());
                }
            }
            else
            {
                xwarn("xblockacct_t::execute_block,fail-ready to execute for block=%s highest_execute_block_height=%lld",block->dump().c_str(), m_meta->_highest_execute_block_height);
            }
            return false;
        }

        bool   xblockacct_t::unload_subblock_from_container(base::xvblock_t* container_block)
        {
            const int block_flags = container_block->get_block_flags();
            if( (container_block->get_header()->get_block_class() != base::enum_xvblock_class_nil)//for non-nil container
              &&((block_flags & base::enum_xvblock_flag_committed) != 0) //only handle committed container
              &&((block_flags & base::enum_xvblock_flag_unpacked) == 0) //and not unpacked yet
               )
            {
                bool unpack_result = false;
                if(container_block->get_header()->get_block_level() == base::enum_xvblock_level_table) //add other container here if need
                {
                    #if defined(__MAC_PLATFORM__) && defined(__ENABLE_MOCK_XSTORE__)
                    // do nothing
                    #else
                    xassert(container_block->is_input_ready(true));
                    xassert(container_block->is_output_ready(true));

                    auto tableblock = dynamic_cast<data::xtable_block_t*>(container_block);
                    xassert(tableblock != nullptr);
                    if (tableblock != nullptr)
                    {
                        auto & units = tableblock->get_tableblock_units(true);
                        if(false == units.empty()) //must be have at least one
                            unpack_result = true;//now ok to set true
                        else
                            xassert(!units.empty());
                        xinfo("xblockacct_t::unload_subblock_from_container,table block(%s) unit num=%d,tx num=%d", tableblock->dump().c_str(), units.size(), tableblock->get_txs_count());
                        for (auto & unitblock : units)
                        {
                            if(false == m_blockstore->store_block(unitblock.get())) //any fail resultin  re-unpack whole table again
                            {
                                xerror("xblockacct_t::unload_subblock_from_container,fail-store unitblock=%s from tableblock=%s",unitblock->dump().c_str(),tableblock->dump().c_str());
                                return false;
                            }
                            else
                            {
                                xinfo("xblockacct_t::unload_subblock_from_container,stored unitblock=%s from tableblock=%s",unitblock->dump().c_str(),tableblock->dump().c_str());
                            }
                        }
                    }
                    #endif
                }
                if(unpack_result) //finally check and set flag of enum_xvblock_flag_unpacked
                {
                    container_block->set_block_flag(base::enum_xvblock_flag_unpacked);
                }
            }
            return true;
        }

        bool xblockacct_t::is_replace_existing_block(base::xvblock_t* existing_block, base::xvblock_t* this_block)
        {
            if (!existing_block->check_block_flag(base::enum_xvblock_flag_committed))
            {
                base::xauto_ptr<base::xvblock_t> next_block(load_block_object(existing_block->get_height() + 1, true));
                if (next_block != nullptr && next_block->check_block_flag(base::enum_xvblock_flag_committed) && next_block->get_last_block_hash() == this_block->get_block_hash())
                {
                    xwarn("xblockacct_t::is_replace_existing_block,block forked from height(%" PRIu64 ") and view#=%" PRIu64 ",new block=%s connected with next commit block=%s,replace existing block=%s at store(%s)",
                            this_block->get_height(),existing_block->get_viewid(),this_block->dump().c_str(),next_block->dump().c_str(),existing_block->dump().c_str(),get_blockstore_path().c_str());
                    return true;
                }
            }
            return false;
        }

        /* 3 rules for managing cache
            #1. clean blocks of lower stage when higher stage coming. stage include : cert, lock and commit
            #2. only allow one block at same height for the locked or committed block,in other words it allow mutiple cert-only blocks
            #3. not allow overwrite block with newer/more latest block at same height and same stage
         */
        bool    xblockacct_t::store_block(base::xvblock_t* this_block)
        {
            if(nullptr == this_block)
                return false;

#ifdef ENABLE_METRICS
            XMETRICS_TIME_RECORD_KEY("blockstore_store_block_time", this_block->get_account() + ":" + std::to_string(this_block->get_height()));
#endif
            if(   (false == this_block->is_input_ready(true))
               || (false == this_block->is_output_ready(true))
               || (false == this_block->is_deliver(false)) )//must have full valid data and has mark as enum_xvblock_flag_authenticated
            {
                xerror("xblockacct_t::store_block,undevlier block=%s,input_ready=%d and output_ready=%d",this_block->dump().c_str(),this_block->is_input_ready(true),this_block->is_output_ready(true));
                return false;
            }

            const uint64_t this_block_height = this_block->get_height();
#if 0
            if(this_block_height <= m_meta->_highest_execute_block_height)//not allow change any more once executed
            {
                xinfo("xblockacct_t::store_block,at store(%s) drop block=%s <= executed-height(%" PRIu64 ") ",get_blockstore_path().c_str(),this_block->dump().c_str(),m_meta->_highest_execute_block_height);
                return true; //block is outdate and behind than executed block
            }
            else
#endif
            {
                xdbg("xblockacct_t::store_block,prepare for block=%s,cache_size:%zu",this_block->dump().c_str(), m_all_blocks.size());
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
                if(  (this_block_height <= m_meta->_highest_commit_block_height)
                   ||(this_block_height <= m_meta->_highest_execute_block_height)
                   ||(this_block_height <= m_meta->_highest_connect_block_height) )
                {
                    load_block(this_block_height, 0);//force load block first
                }
            }

            auto existing_view_iterator = view_map.end();
            //apply rule#1 clean blocks of lower stage when higher stage coming. stage include : cert, lock and commit
            if(this_block->check_block_flag(base::enum_xvblock_flag_committed))   //commit block
            {
                xassert(this_block->check_block_flag(base::enum_xvblock_flag_locked)); //must be true
                for(auto it = view_map.begin(); it != view_map.end();)
                {
                    auto old_it = it;
                    ++it;

                    if(old_it->second->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        existing_view_iterator = old_it; //found existing commit block,then reuse this slot
                    }
                    else //apply rule#1: clean any cert,lock blocs since a commit block occupy this slot
                    {
                        xdbg("xblockacct_t::store_block,new-commit one clean existing block=%s",old_it->second->dump().c_str());

                        old_it->second->close();
                        old_it->second->release_ref();//old_it->second might be same as this_block
                        view_map.erase(old_it);
                    }
                }
            }
            else if(this_block->check_block_flag(base::enum_xvblock_flag_locked)) //lock-only block
            {
                for(auto it = view_map.begin(); it != view_map.end();)
                {
                    auto old_it = it;
                    ++it;

                    if(old_it->second->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        xwarn("xblockacct_t::store_block,lock-only block conflict with existing committed block,block=%s",this_block->dump().c_str());
                        return true; //apply rule#2,not allow put lower stage one
                    }
                    else if(old_it->second->check_block_flag(base::enum_xvblock_flag_locked))
                    {
                        existing_view_iterator = old_it; //found existing lock block,then reuse this slot
                    }
                    else //clean any cert-only block
                    {
                        xdbg("xblockacct_t::store_block,new-lock one clean existing block=%s",old_it->second->dump().c_str());

                        old_it->second->close();
                        old_it->second->release_ref();
                        view_map.erase(old_it);
                    }
                }
            }
            else if(this_block->check_block_flag(base::enum_xvblock_flag_authenticated))//cert-only
            {
                for(auto it = view_map.begin(); it != view_map.end();)
                {
                    auto old_it = it;
                    ++it;

                    if(  (old_it->second->check_block_flag(base::enum_xvblock_flag_locked))
                       ||(old_it->second->check_block_flag(base::enum_xvblock_flag_committed)) )
                    {
                        xwarn("xblockacct_t::store_block,cert-only block conflict with existing locked/committed block,block=%s",this_block->dump().c_str());
                        return true; //apply rule#2,not allow put lower stage one
                    }
                    if(old_it->second->get_viewid() == this_block->get_viewid())
                        existing_view_iterator = old_it; //found existing cert block,then reuse this slot
                }
            }
            if(existing_view_iterator != view_map.end())//apple rule#2 by reuse existing iterator and replace by new value
            {
                xdbg("xblockacct_t::store_block, find same viewid block, block=%s",this_block->dump().c_str());
                base::xvblock_t* existing_block = existing_view_iterator->second;
                if(this_block == existing_block) //found same one but modified
                {
                    if(0 == this_block->get_modified_count())//nothing changed,just return
                        return true;

                    save_block(this_block);

                    on_block_changed(this_block); //throw event
                }
                else //ensure only one commit in the map
                {
                    bool is_replace_block = false;
                    if(existing_block->get_block_hash() != this_block->get_block_hash())//safety check
                    {
                        // Block chain is forked! If new block can connect with next block which is committed, replace existing block with new block.
                        if (!is_replace_existing_block(existing_block, this_block))
                        {
                            xerror("xblockacct_t::store_block,fail-hash changed for block with exist height(%" PRIu64 ") and view#=%" PRIu64 " vs new block=%s at store(%s)",this_block->get_height(),existing_block->get_viewid(),this_block->dump().c_str(),get_blockstore_path().c_str());
                            return false;
                        }
                        is_replace_block = true;
                    }

                    //apply rule#3. not allow overwrite block with newer/more latest block at same height and same stage
                    const int existing_block_flags = existing_block->get_block_flags();
                    const int new_block_flags      = this_block->get_block_flags();
                    if(!is_replace_block
                       && ((existing_block_flags == new_block_flags)
                         ||(existing_block_flags & base::enum_xvblock_flags_high4bit_mask) > (new_block_flags & base::enum_xvblock_flags_high4bit_mask)
                         ||(existing_block_flags & base::enum_xvblock_flags_low4bit_mask)  > (new_block_flags & base::enum_xvblock_flags_low4bit_mask))
                       ) //outdated one try to overwrite newer one,abort it
                    {
                        xwarn("xblockacct_t::store_block,warn-try to overwrite newer block with flags(0x%x) by outdated block=%s at store(%s)",existing_block->get_unit_flags(),this_block->dump().c_str(),get_blockstore_path().c_str());
                        return true;
                    }

                    this_block->reset_prev_block(existing_block->get_prev_block());      //transfer pre_block ptr to this_block
                    if(existing_block->check_block_flag(base::enum_xvblock_flag_stored)) //transfer stored flag to this_block
                        this_block->set_block_flag(base::enum_xvblock_flag_stored);

                    this_block->add_ref();                       //hold reference for map
                    existing_view_iterator->second = this_block; //replace ptr in the map
                    existing_block->close();                     //close first to release any linked reference
                    existing_block->release_ref();               //now safe to release existing one

                    //redo connection since change of object ptr
                    auto it_next  = hight_map_pos.first;         //copy first to avoid change hight_map_pos
                    ++it_next;
                    if( (it_next != m_all_blocks.end()) && (it_next->first == (this_block_height + 1)) ) //if found next set
                    {
                        for(auto it = it_next->second.rbegin(); it != it_next->second.rend(); ++it)//search from higher view
                        {
                            it->second->reset_prev_block(this_block);//this_block  <---next block successful
                        }
                    }
                    save_block(this_block);
                    on_block_changed(this_block); //throw event
                }
            }
            else
            {
                auto view_map_pos = view_map.emplace(this_block->get_viewid(),this_block);
                if(view_map_pos.second) //insert successful
                {
                    this_block->add_modified_count(); //force to add amout before call connect_block
                    if(connect_block(this_block,hight_map_pos.first,1)) //might did save_block()
                    {
                        this_block->add_ref(); //now ok to hold reference for map

                        if(this_block->get_modified_count() != 0)//not save_block yet
                            save_block(this_block);

                        clean_blocks(enum_max_cached_blocks); //as limited cached memory, clean the oldest one if need
                    }
                    else //a bad cert block when connect_block fail
                    {
                        xwarn("xblockacct_t::store_block,a bad block=%s of account=%s", this_block->dump().c_str(), m_meta->dump().c_str());

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

#ifndef __MAC_PLATFORM__
#if 0
            if (this_block->get_account() == sys_contract_beacon_timer_addr)
            {
                uint16_t min_tailor_height = XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_clock_tailor_height);
                uint16_t step_tailor_height = XGET_ONCHAIN_GOVERNANCE_PARAMETER(step_clock_tailor_height);
                if ((m_meta->_highest_connect_block_height - m_meta->_lowest_store_block_height) > (min_tailor_height + step_tailor_height)) {
                    bool has_input_output = false;
                    if (this_block->get_block_class() != base::enum_xvblock_class_nil)
                    {
                        has_input_output = true;
                    }
                    uint64_t lowest_tailor_height = m_meta->_lowest_store_block_height > 0 ? m_meta->_lowest_store_block_height : 1;
                    for (uint64_t i = lowest_tailor_height; i < (lowest_tailor_height + step_tailor_height); ++i) {
                        delete_block(i);
                        m_persist_db->delete_block_by_path(get_blockstore_path(),get_account(),i,has_input_output);
                    }
                    m_meta->_lowest_store_block_height += step_tailor_height;
                }
            }
#endif
#endif
            xdbg("xblockacct_t::store_block,finally cached block=%s of account=%s", this_block->dump().c_str(), m_meta->dump().c_str());
#ifdef ENABLE_METRICS
            XMETRICS_COUNTER_INCREMENT("blockstore_store_block", 1);
#endif
            return true;
        }

        //internal use only: query_block just check at cache layer and return raw ptr without added reference, so caller need use careful
        base::xvblock_t*    xblockacct_t::query_block(const uint64_t target_height,base::enum_xvblock_flag request_flag)
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
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }

        //internal use only: query_block just check at cache layer and return raw ptr without added reference, so caller need use careful
        base::xvblock_t*    xblockacct_t::query_latest_block(base::enum_xvblock_flag request_flag)
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
                            return view_it->second;
                        }
                    }
                }
            }
            return nullptr;
        }

        bool xblockacct_t::save_to_xdb(base::xvblock_t* this_block)
        {
            const uint64_t this_block_height = this_block->get_height();
            const int      this_block_flags  = this_block->get_block_flags();

            //step#1: mark connected status for committed block
            if((this_block_flags & base::enum_xvblock_flag_committed) != 0)
            {
                if((this_block_flags & base::enum_xvblock_flag_connected) == 0)
                {
                    if(this_block_height <= m_meta->_highest_connect_block_height) //block is skiped before,makeup it
                    {
                        this_block->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status
                    }
                    else if(this_block->get_block_class() == base::enum_xvblock_class_full) //full-block must be a connected block
                    {
                        this_block->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status
                    }
                    else if( (this_block->get_prev_block() != NULL) && (this_block->get_prev_block()->check_block_flag(base::enum_xvblock_flag_connected)) )//quick path
                    {
                        xassert(this_block->get_prev_block()->check_block_flag(base::enum_xvblock_flag_committed));//must be true
                        this_block->set_block_flag(base::enum_xvblock_flag_connected);
                    }
                    else if(   (this_block_height == (m_meta->_highest_connect_block_height + 1)) //regular check
                            && (m_meta->_highest_connect_block_hash  == this_block->get_last_block_hash()))
                    {
                        this_block->set_block_flag(base::enum_xvblock_flag_connected);
                    }
                }
            }
            else if((this_block_flags & base::enum_xvblock_flag_locked) == 0)//not save for block of non-locked and non-committed
            {
                if(this_block_height > m_meta->_highest_cert_block_height)//update _highest_cert_block_height event not persist db
                {
                    m_meta->_highest_cert_block_height = this_block_height;
                    xinfo("xblockacct_t::save_block,at store(%s) persistently save cert-only block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                    const std::string full_meta_path = get_blockstore_path() + get_meta_path(*this);
                    const std::string  _last_cert_block_path  = full_meta_path + "/lastcert/";;
                    store_block_by_path(_last_cert_block_path,this_block);
                }
                return true;
            }

            bool       now_stored_result = false;
            //step#2: do persist store for block
            if( (this_block_flags & base::enum_xvblock_flag_stored) == 0) //first time to store db fully
            {
                this_block->set_block_flag(base::enum_xvblock_flag_stored); //set flag stored first
                now_stored_result  = m_persist_db->set_vblock(get_blockstore_path(),this_block); //store fully
                if(false == now_stored_result)//removed flag since stored fail
                {
                    m_blockstore->remove_block_flag(this_block,base::enum_xvblock_flag_stored);
                    xerror("xblockacct_t::save_block,at store(%s) fail-store full block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
                else //make sure store persistently first
                {
                    this_block->reset_modified_count(); //count everything already,just remove status of changed
                    xdbg_info("xblockacct_t::save_block,at store(%s) store full block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
            }
            else //has changed something of header/cert
            {
                if (this_block->get_modified_count() == 0)
                {
                    xinfo("xblockacct_t::save_block,at store(%s) unchanged block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                    return true;
                }
                now_stored_result = m_persist_db->set_vblock_header(get_blockstore_path(),this_block);//store header only
                if(now_stored_result)
                {
                    this_block->reset_modified_count(); //count everything already,just remove status of changed
                    xdbg_info("xblockacct_t::save_block,at store(%s) store portion of block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
                else
                {
                    xerror("xblockacct_t::save_block,at store(%s) fail-store portion of block=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
                }
            }
            //step#3: after finish persisten store, then ready to update the meta information
            if(now_stored_result) //just search and match once for each commit block
            {
                if((this_block_flags & base::enum_xvblock_flag_authenticated) != 0)
                {
                    if(this_block_height > m_meta->_highest_cert_block_height)
                        m_meta->_highest_cert_block_height = this_block_height;
                }

                if((this_block_flags & base::enum_xvblock_flag_locked) != 0)
                {
                    if(this_block_height > m_meta->_highest_lock_block_height)
                        m_meta->_highest_lock_block_height = this_block_height;
                }

                if((this_block_flags & base::enum_xvblock_flag_committed) != 0)
                {
                    if(this_block_height > m_meta->_highest_lock_block_height)//commit block must already include locked status
                        m_meta->_highest_lock_block_height = this_block_height;

                    if(this_block_height > m_meta->_highest_commit_block_height)//has test above for base::enum_xvblock_flag_committed
                        m_meta->_highest_commit_block_height = this_block_height;

                    if(  (this_block_height > m_meta->_highest_full_block_height)
                       &&(this_block->get_block_class() == base::enum_xvblock_class_full) )
                    {
                        m_meta->_highest_full_block_height = this_block_height;
                    }

                    if( (this_block->check_block_flag(base::enum_xvblock_flag_connected)) && (this_block_height > m_meta->_highest_connect_block_height) )
                    {
                        m_meta->_highest_connect_block_height = this_block_height;
                        m_meta->_highest_connect_block_hash   = this_block->get_block_hash();

                        auto heigh_it = m_all_blocks.find(m_meta->_highest_connect_block_height + 1); //search more
                        for(;heigh_it != m_all_blocks.end();++heigh_it)
                        {
                            if(   (heigh_it->first == (m_meta->_highest_connect_block_height + 1))
                               && (false == heigh_it->second.empty()) )
                            {
                                auto view_it = heigh_it->second.begin();
                                if(  (view_it->second->check_block_flag(base::enum_xvblock_flag_committed))
                                   &&(view_it->second->get_last_block_hash() == m_meta->_highest_connect_block_hash) )
                                {
                                    m_meta->_highest_connect_block_height = view_it->second->get_height();
                                    m_meta->_highest_connect_block_hash   = view_it->second->get_block_hash();
                                    view_it->second->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status,and save later
                                }
                                else
                                {
                                    break;
                                }
                            }
                            else
                            {
                                break;
                            }
                        }
                    }
                }
            }
            //step#4: give chance to trigger db event
            if(now_stored_result)
                on_block_stored_to_xdb(this_block);

            return now_stored_result;
        }

        bool xblockacct_t::save_block(base::xvblock_t* this_block)
        {
            const uint64_t this_block_height = this_block->get_height();
            const int      this_block_flags  = this_block->get_block_flags();
            if( (this_block_height == 1) && ((this_block_flags & base::enum_xvblock_flag_committed) != 0) )//ensure to store genesis block
            {
                base::xvblock_t* genesis_block = query_block(0,base::enum_xvblock_flag_committed);
                if( (genesis_block != nullptr) && (false == genesis_block->check_block_flag(base::enum_xvblock_flag_stored)) )
                {
                    save_block(genesis_block);//store genesis block first
                }
            }

            if(save_to_xdb(this_block))
            {
                // try to execute this block first
                execute_block(this_block);//execute_block may call save_to_xdb as well

                // then, try to execute history block
                if (m_meta->_highest_execute_block_height < m_meta->_highest_connect_block_height) {
                    for (uint64_t i = m_meta->_highest_execute_block_height + 1; i <= m_meta->_highest_connect_block_height; i++)
                    {
                        base::xauto_ptr<base::xvblock_t> next_execute_block(load_block_object(m_meta->_highest_execute_block_height + 1,true));
                        if (next_execute_block != nullptr)
                        {
                            if (false == execute_block(next_execute_block.get()))
                            {
                                xwarn("xblockacct_t::execute_block,fail-try to execute next execute height, block=%s",next_execute_block->dump().c_str());
                                return false;
                            }
                            else
                            {
                                xinfo("xblockacct_t::execute_block,success-try to execute next execute height, block=%s",next_execute_block->dump().c_str());
                            }
                        }
                    }
                }

                return true;
            }

            return false;
        }

        //note:the returned ptr NOT increased reference, so use carefully and only use internally
        //to connect prev block, load_block may call load_block again to get prev-block, reenter_allow_count decide how many times can reenter
        base::xvblock_t*     xblockacct_t::load_block(const uint64_t target_height,int reenter_allow_count)
        {
            xinfo("xblockacct_t::load_block enter addr=%s,height=%ld,reenter_allow_count=%d", get_account().c_str(), target_height, reenter_allow_count);
            //xassert(target_height <= m_meta->_highest_lock_block_height);
            //load it from db now
#ifdef ENABLE_METRICS
            XMETRICS_TIME_RECORD_KEY("blockstore_load_block_time", get_account() + ":" + std::to_string(target_height));
#endif
            base::xvblock_t* new_block_ptr = m_persist_db->get_vblock(get_blockstore_path(),get_account(),target_height);//added reference when return by get_vblock
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
                    xerror("xblockacct_t::load_block,Exception-load unmatched block of height=%" PRIu64 ",but ask height=%" PRIu64 "from db",new_block_ptr->get_height(),target_height);

                    new_block_ptr->release_ref();
                    new_block_ptr = nullptr;
                    return nullptr;
                }
                if(target_height != 0)
                    xassert(new_block_ptr->check_block_flag(base::enum_xvblock_flag_stored));

                #ifdef DEBUG
                if(false == new_block_ptr->is_deliver(false))//must have full valid data and has mark as enum_xvblock_flag_authenticated
                {
                    xerror("xblockacct_t::load_block,undevlier block=%s",new_block_ptr->dump().c_str());
                }
                #endif

                //note: emplace return a pair<iterator,bool>, value of bool indicate whether inserted or not, value of iterator point to inserted it
                auto hight_map_pos  = m_all_blocks.emplace(new_block_ptr->get_height(),std::map<uint64_t,base::xvblock_t*>());
#ifdef ENABLE_METRICS
                if (hight_map_pos.second) {
                    XMETRICS_COUNTER_INCREMENT("blockstore_cache_block_total", 1);
                }
#endif
                auto & view_map     = hight_map_pos.first->second;//hight_map_pos.first->first is height, and hight_map_pos.first->second is viewmap

                auto insert_result = view_map.emplace(new_block_ptr->get_viewid(),new_block_ptr); //new_block_ptr has added reference
                if(insert_result.second)//insered succssful
                {
                    //connect to prev/next block
                    connect_block(new_block_ptr,hight_map_pos.first,reenter_allow_count);

                    //update meta information per this block
                    if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        xdbg_info("xblockacct_t::load_commit_block,at store(%s) commit block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());

                        //update meta information now
                        if(target_height > m_meta->_highest_cert_block_height) //committed block must also a cert block
                            m_meta->_highest_cert_block_height = target_height;

                        if(target_height > m_meta->_highest_lock_block_height) //committed block must also a locked block
                            m_meta->_highest_lock_block_height = target_height;

                        if(target_height > m_meta->_highest_commit_block_height)
                            m_meta->_highest_commit_block_height = target_height;

                        if(  (target_height > m_meta->_highest_full_block_height)
                           &&(new_block_ptr->get_block_class() == base::enum_xvblock_class_full) )
                        {
                            m_meta->_highest_full_block_height = target_height;
                        }

                        //mark connected status
                        if(false == new_block_ptr->check_block_flag(base::enum_xvblock_flag_connected)) //genesis block must be connected-status
                        {
                            if(target_height <= m_meta->_highest_connect_block_height) //block is skiped before,makeup it
                            {
                                new_block_ptr->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status
                            }
                            else if(new_block_ptr->get_block_class() == base::enum_xvblock_class_full) //full-block must be a connected block
                            {
                                new_block_ptr->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status
                            }
                            else if( (new_block_ptr->get_prev_block() != NULL) && (new_block_ptr->get_prev_block()->check_block_flag(base::enum_xvblock_flag_connected)) )//quick path
                            {
                                xassert(new_block_ptr->get_prev_block()->check_block_flag(base::enum_xvblock_flag_committed));//must be true
                                new_block_ptr->set_block_flag(base::enum_xvblock_flag_connected);
                            }
                            else if(   (target_height == (m_meta->_highest_connect_block_height + 1)) //regular check
                                    && (m_meta->_highest_connect_block_hash  == new_block_ptr->get_last_block_hash()))
                            {
                                new_block_ptr->set_block_flag(base::enum_xvblock_flag_connected);
                            }
                        }
                        //update record of _highest_connect_block_height/hash now
                        if( (new_block_ptr->check_block_flag(base::enum_xvblock_flag_connected)) && (target_height > m_meta->_highest_connect_block_height) )
                        {
                            m_meta->_highest_connect_block_height = target_height;
                            m_meta->_highest_connect_block_hash   = new_block_ptr->get_block_hash();

                            auto heigh_it = m_all_blocks.find(m_meta->_highest_connect_block_height + 1); //search more
                            for(;heigh_it != m_all_blocks.end();++heigh_it)
                            {
                                if(   (heigh_it->first == (m_meta->_highest_connect_block_height + 1))
                                   && (false == heigh_it->second.empty()) )
                                {
                                    auto view_it = heigh_it->second.begin();
                                    if(  (view_it->second->check_block_flag(base::enum_xvblock_flag_committed))
                                       &&(view_it->second->get_last_block_hash() == m_meta->_highest_connect_block_hash) )
                                    {
                                        m_meta->_highest_connect_block_height = view_it->second->get_height();
                                        m_meta->_highest_connect_block_hash   = view_it->second->get_block_hash();
                                        view_it->second->set_block_flag(base::enum_xvblock_flag_connected); //mark connected status,and save later
                                    }
                                    else
                                    {
                                        break;
                                    }
                                }
                                else
                                {
                                    break;
                                }
                            }

                        }
                    }
                    else if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_locked))
                    {
                        //update meta information now
                        if(target_height > m_meta->_highest_cert_block_height) //committed block must also a cert block
                            m_meta->_highest_cert_block_height = target_height;

                        if(target_height > m_meta->_highest_lock_block_height)
                            m_meta->_highest_lock_block_height = target_height;

                        xdbg_info("xblockacct_t::load_lock_block,at store(%s) lock block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());
                    }
                    else if(new_block_ptr->check_block_flag(base::enum_xvblock_flag_authenticated)) //xstore should only store commit&locked block
                    {
                        //update meta information now
                        if(target_height > m_meta->_highest_cert_block_height)
                            m_meta->_highest_cert_block_height = target_height;

                        xdbg_info("xblockacct_t::load_cert_block,at store(%s) cert block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());
                    }
                    else //clean it for all other cases
                    {
                        new_block_ptr->release_ref();
                        new_block_ptr = nullptr;
                    }
                }
                else
                {
                    xwarn("xblockacct_t::load_block,at store(%s) duplicated block=%s",get_blockstore_path().c_str(),new_block_ptr->dump().c_str());
                    new_block_ptr->release_ref();
                    new_block_ptr = nullptr;
                }
            }
            else
            {
                xwarn("xblockacct_t::load_block,warn-not found block at height:%" PRIu64 " of account(%s) at store(%s)",(int64_t)target_height,get_account().c_str(),get_blockstore_path().c_str());

                on_block_miss_at_xdb(target_height); //throw event to notify
            }
#ifdef ENABLE_METRICS
            XMETRICS_COUNTER_INCREMENT("blockstore_load_block", 1);
#endif
            return new_block_ptr;
        }

        //clean unsed caches of account to recall memory
        bool xblockacct_t::clean_caches()
        {
            return clean_blocks(0);//try all possible blocks
        }

        bool xblockacct_t::clean_blocks(const size_t keep_blocks_count)
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

                    if(   (old_height_it->first != 0) //keep genesis
                       && (old_height_it->first != m_meta->_highest_full_block_height)    //keep latest_full_block
                       && (old_height_it->first != m_meta->_highest_execute_block_height) //keep latest_executed block
                       && (old_height_it->first <  m_meta->_highest_commit_block_height)  //keep latest_committed block
                       && (old_height_it->first != m_meta->_highest_lock_block_height)    //keep latest_lock_block
                       && (old_height_it->first != m_meta->_highest_connect_block_height))//keep latest_connect_block
                    {
                        auto & view_map = old_height_it->second;
                        if(   (old_height_it->first < m_meta->_highest_connect_block_height)  //must be old commit than connect
                           || (view_map.begin()->second->check_block_flag(base::enum_xvblock_flag_committed)) )//clean committed first
                        {
                            for(auto it = view_map.begin(); it != view_map.end(); ++it)
                            {
                                if(it->second->get_modified_count() > 0) //store any modified blocks again
                                    save_block(it->second);

                                xdbg_info("xblockacct_t::clean_blocks,block=%s",it->second->dump().c_str());

                                on_block_removing_from_cache(it->second);

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

                // try to save meta when clean blocks
                save_meta();
            }
            return true;
        }

        //there might be mutiple candidates(blocks) at same height,so clean unqualified ones
        bool xblockacct_t::clean_candidates(base::xvblock_t* compare_by_block)
        {
            auto it  = m_all_blocks.find(compare_by_block->get_height());
            if(it != m_all_blocks.end())
            {
                auto & view_map = it->second;
                const int filter_by_block_flags = compare_by_block->get_block_flags();
                return clean_candidates(view_map,filter_by_block_flags);
            }
            return false;
        }

        bool xblockacct_t::clean_candidates(std::map<uint64_t,base::xvblock_t*> & view_map,const int filter_by_block_flags)
        {
            if( (filter_by_block_flags & base::enum_xvblock_flag_committed) != 0)
            {
                for(auto it = view_map.begin(); it != view_map.end();)
                {
                    auto old_it = it;
                    ++it;

                    if(false == old_it->second->check_block_flag(base::enum_xvblock_flag_committed))
                    {
                        xdbg_info("xblockacct_t::clean_candidates,for non-commit block=%s",old_it->second->dump().c_str());

                        old_it->second->close();
                        old_it->second->release_ref();
                        view_map.erase(old_it);//apply rule#1: clean any other candidated than committed one
                    }
                }
            }
            else if( (filter_by_block_flags & base::enum_xvblock_flag_locked) != 0) //lock-only block
            {
                for(auto it = view_map.begin(); it != view_map.end();)
                {
                    auto old_it = it;
                    ++it;

                    if(false == old_it->second->check_block_flag(base::enum_xvblock_flag_locked))//clean any cert-only block
                    {
                        xdbg_info("xblockacct_t::clean_candidates,for non-lock block=%s",old_it->second->dump().c_str());

                        old_it->second->close();
                        old_it->second->release_ref();
                        view_map.erase(old_it);//apply rule#1: clean any other candidated than locked/committed one
                    }
                }
            }
            return true;
        }

        //to connect prev block, load_block may call load_block again to get prev-block, reload_allow_count decide how many times can reenter
        bool    xblockacct_t::connect_block(base::xvblock_t* this_block,int reload_allow_count)
        {
            auto it = m_all_blocks.find(this_block->get_height());
            if(it == m_all_blocks.end())
                return true;

            return connect_block(this_block,it,reload_allow_count);
        }

        bool    xblockacct_t::connect_block(base::xvblock_t* this_block,const std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> >::iterator & this_block_height_it,int reload_allow_count)
        {
            if(this_block_height_it == m_all_blocks.end())//protection
                return true;

            if(this_block->get_height() > 0)
            {
                if( (this_block->get_prev_block() == nullptr) || (this_block->get_prev_block()->is_close()) )
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
                                    it->second->reset_next_block(this_block);//previous block ---> this_block
                                    break;
                                }
                            }
                            if(this_block->get_prev_block() == nullptr) //not connect any prev_block,it is a bad cert
                            {
                                xwarn("xblockacct_t::connect_block,fail-block not connect any prev-block for block=%s",this_block->dump().c_str());
                            }
                        }
                    }
                    if( (this_block->get_prev_block() == nullptr) || (this_block->get_prev_block()->is_close()) ) //check again
                    {
                        if(reload_allow_count > 0)
                        {
                            base::xvblock_t* previouse_block = load_block(this_block->get_height() - 1,--reload_allow_count);
                            if(previouse_block != nullptr)
                            {
                                if(this_block->reset_prev_block(previouse_block) == false)  //previouse block <-- this_block_ptr
                                {
                                    xwarn("xblockacct_t::connect_block,fail-block not connect prev-lock-block=%s for block=%s",previouse_block->dump().c_str(),this_block->dump().c_str());
                                }
                                else
                                {
                                    previouse_block->reset_next_block(this_block);//previous block ---> this_block
                                }
                            }
                        }
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
                    this_block->reset_next_block(it->second);//this_block  --->next block
                }
            }
            return true;
        }

        bool    xblockacct_t::store_block_by_path(const std::string & block_base_path, base::xvblock_t* block_ptr)
        {
            if( (nullptr == block_ptr) || (block_base_path.empty()) )
                return false;

            if(block_ptr->get_block_class() != base::enum_xvblock_class_nil)
            {
                const std::string _input_path   = block_base_path + base::xvblock_t::get_input_name();
                store_value_by_path(_input_path,block_ptr->get_input()->get_resources_data());

                const std::string _output_path  = block_base_path + base::xvblock_t::get_output_name();
                store_value_by_path(_output_path,block_ptr->get_output()->get_resources_data());
            }
            const std::string _header_cert_path  = block_base_path + base::xvblock_t::get_header_name();
            std::string vheader_cert_bin;
            block_ptr->serialize_to_string(vheader_cert_bin);
            return store_value_by_path(_header_cert_path,vheader_cert_bin);
        }

        base::xvblock_t*    xblockacct_t::load_block_by_path(const std::string & block_base_path, bool ask_full_load)
        {
            const std::string _header_cert_path    = block_base_path + base::xvblock_t::get_header_name();
            const std::string _header_cert_content = load_value_by_path(_header_cert_path);
            if(_header_cert_content.empty())
            {
                xwarn("xblockacct_t::load_block_by_path,not found block object at %s",_header_cert_path.c_str());
                return nullptr;
            }
            base::xvblock_t* new_block_ptr = base::xvblockstore_t::create_block_object(_header_cert_content);
            if(new_block_ptr != NULL && ask_full_load)
            {
                if(new_block_ptr->get_block_class() != base::enum_xvblock_class_nil)
                {
                    const std::string  _input_path    = block_base_path + base::xvblock_t::get_input_name();
                    const std::string  _input_content = load_value_by_path(_input_path);
                    if(false == _input_content.empty())
                    {
                        new_block_ptr->set_input_resources(_input_content);
                    }

                    const std::string  _output_path    = block_base_path + base::xvblock_t::get_output_name();
                    const std::string  _output_content = load_value_by_path(_output_path);
                    if(false == _output_content.empty())
                    {
                        new_block_ptr->set_output_resources(_output_content);
                    }
                }
            }
            return new_block_ptr;
        }

        const std::string   xblockacct_t::load_value_by_path(const std::string & full_path_as_key)
        {
            return m_persist_db->get_value(full_path_as_key);
        }

        bool                xblockacct_t::delete_value_by_path(const std::string & full_path_as_key)
        {
            return m_persist_db->delete_value(full_path_as_key);
        }

        bool                xblockacct_t::store_value_by_path(const std::string & full_path_as_key,const std::string & value)
        {
            return m_persist_db->set_value(full_path_as_key, value);
        }

        void  xblockacct_t::notify_commit_store_event(base::xvblock_t* this_block_ptr)
        {
            #ifndef __MAC_PLATFORM__
            xassert(this_block_ptr != nullptr);
            // send mbus event
            if (m_persist_db->get_mbus() && this_block_ptr->get_height() > 0)
            {
                xblock_t* block = dynamic_cast<data::xblock_t*>(this_block_ptr);
                xassert(block != nullptr);
                block->add_ref();
                xblock_ptr_t obj;
                obj.attach(block);

                mbus::xevent_ptr_t event = std::make_shared<mbus::xevent_store_block_to_db_t>(obj, obj->get_account(), true);
                m_persist_db->get_mbus()->push_event(event);
                xdbg_info("xblockaccount_t::notify_event,at store(%s) block=%s",get_blockstore_path().c_str(),this_block_ptr->dump().c_str());
            }
            #endif
        }

        bool      xblockacct_t::on_block_changed(base::xvblock_t* _block)           //stage change from cert->lock->commit->executed ...
        {
            return true; //do nothing as default
        }
        bool      xblockacct_t::on_block_stored_to_xdb(base::xvblock_t* _block)       //persist stored
        {
            if(is_close()) //trigger when closed,just ignore it
                return true;

            if(_block->get_height() == 0) //ignore genesis block
                return true;

            const int block_flags = _block->get_block_flags();
            if((block_flags & base::enum_xvblock_flag_executed) != 0)
            {
                //here notify execution event if need
            }
            else if( ((block_flags & base::enum_xvblock_flag_committed) != 0) && ((block_flags & base::enum_xvblock_flag_connected) != 0) )
            {
                notify_commit_store_event(_block);
            }
            return true;
        }
        bool      xblockacct_t::on_block_miss_at_xdb(const uint64_t target_height)
        {
            return true; //do nothing as default
        }
        bool      xblockacct_t::on_block_removing_from_cache(base::xvblock_t* _block) //remove from cache
        {
            return true; //do nothing as default
        }

        xchainacct_t::xchainacct_t(const std::string & account_addr,const uint64_t timeout_ms,const std::string & blockstore_path,xstore_face_t & _persist_db,base::xvblockstore_t& _blockstore)
            :xblockacct_t(account_addr,timeout_ms,blockstore_path,_persist_db,_blockstore)
        {
            _lowest_commit_block_height = 0;
        }

        xchainacct_t::~xchainacct_t()
        {
        }

        bool  xchainacct_t::process_block(base::xvblock_t* this_block)
        {
            base::xvblock_t* prev_block = this_block->get_prev_block();
            if(nullptr == prev_block)
            {
                xwarn("xchainacct_t::process_block no prev block. block=%s", this_block->dump().c_str());
                return false;
            }

            //xdbg("xchainacct_t::process_block at store=%s,high-qc=%s",get_blockstore_path().c_str(),this_block->dump().c_str());
            //transfer to locked status
            //if(false == prev_block->check_block_flag(base::enum_xvblock_flag_locked))
            if(false == prev_block->check_block_flag(base::enum_xvblock_flag_locked)
               || m_meta->_highest_lock_block_height < prev_block->get_height())  // xbft may already set locked flag without update meta
            {
                xdbg("xchainacct_t::process_block at store=%s,lock-qc=%s",get_blockstore_path().c_str(),prev_block->dump().c_str());

                prev_block->set_block_flag(base::enum_xvblock_flag_locked);
                if(this_block->check_block_flag(base::enum_xvblock_flag_locked))//if this_block already been locked status
                {
                    prev_block->set_block_flag(base::enum_xvblock_flag_committed);//convert prev_block directly to commit

                    if(this_block->get_next_block() != nullptr) //find highqc
                        prev_block->set_next_next_cert(this_block->get_next_block()->get_cert());
                }
                prev_block->reset_next_block(this_block);//do forward connection

                save_block(prev_block);
                clean_candidates(prev_block);

                on_block_changed(prev_block); //throw event
            }
            else if(prev_block->get_next_block() == nullptr)
            {
                prev_block->reset_next_block(this_block);//do forward connection
            }

            base::xvblock_t* prev_prev_block = prev_block->get_prev_block();
            if(nullptr == prev_prev_block)
            {
                if (this_block->get_height() > 1) {
                    xwarn("xchainacct_t::process_block no prev prev block. block=%s", this_block->dump().c_str());
                }
                return true;
            }
            //transfer to commit status
            //if(false == prev_prev_block->check_block_flag(base::enum_xvblock_flag_committed))
            if(false == prev_prev_block->check_block_flag(base::enum_xvblock_flag_committed)
               || m_meta->_highest_commit_block_height < prev_prev_block->get_height()  // xbft may already set commit flag without update meta
               || m_meta->_highest_execute_block_height < prev_prev_block->get_height())  // xbft may already set prev prev block commit flag and save_block(prev_block) may already update _highest_commit_block_height, here should try to save prev prev block
            {
                xdbg("xchainacct_t::process_block at store=%s,commit-qc=%s",get_blockstore_path().c_str(),prev_prev_block->dump().c_str());

                prev_prev_block->set_block_flag(base::enum_xvblock_flag_committed);//change to commit status
                prev_prev_block->reset_next_block(prev_block);//do forward connection
                prev_prev_block->set_next_next_cert(this_block->get_cert());

                save_block(prev_prev_block);
                clean_candidates(prev_prev_block);

                on_block_changed(prev_prev_block);//throw event
            }
            else if(prev_prev_block->get_next_block() == nullptr)
            {
                prev_prev_block->reset_next_block(prev_block);//do forward connection
            }
            if(prev_prev_block->get_next_next_cert() == nullptr)
                prev_prev_block->set_next_next_cert(this_block->get_cert());
            return true;
        }

        bool    xchainacct_t::connect_block(base::xvblock_t* this_block,const std::map<uint64_t,std::map<uint64_t,base::xvblock_t*> >::iterator & this_block_height_it,int reenter_allow_count)
        {
            if(this_block_height_it == m_all_blocks.end())//protection
                return true;

            if(this_block->get_height() > 0)
            {
                if(this_block != nullptr) //always true to force to recheck
                {
                    //try to link to previous block: previous block <--- this_block
                    if(this_block_height_it != m_all_blocks.begin()) //must check first
                    {
                        auto  it_prev_height  = this_block_height_it;   //copy first
                        --it_prev_height;       //modify iterator of it_prev_height
                        if(this_block->get_height() == (it_prev_height->first + 1)) //found previous map
                        {
                            for(auto it = it_prev_height->second.rbegin(); it != it_prev_height->second.rend(); ++it)//search from higher view
                            {
                                if(this_block->reset_prev_block(it->second))//previous block <--- this_block successful
                                {
                                    process_block(this_block); //start process from current block,and let previous block->this_block
                                    break;
                                }
                            }
                            if(this_block->get_prev_block() == nullptr) //not connect any prev_block,it is a bad cert
                            {
                                xwarn("xchainacct_t::connect_block,fail-block not connect any prev-block for block=%s",this_block->dump().c_str());
                            }
                        }
                    }
                    if( (this_block->get_prev_block() == nullptr) || (this_block->get_prev_block()->is_close()) ) //check again
                    {
                        if(reenter_allow_count > 0)
                        {
                            base::xvblock_t* previouse_block = load_block(this_block->get_height() - 1,--reenter_allow_count);
                            if(previouse_block != nullptr)
                            {
                                if(this_block->reset_prev_block(previouse_block)) //previouse block <-- this_block_ptr
                                    process_block(this_block); //start process from current block,and let previous block->this_block
                                else //prev-block is locked or commited, but this-block not connected it
                                {
                                    xwarn("xchainacct_t::connect_block,fail-block not connect prev-lock-block=%s for block=%s",previouse_block->dump().c_str(),this_block->dump().c_str());
                                }
                            }
                        }
                    }
                    // check if connect to any prev_block
                    if(this_block->get_prev_block() == nullptr)
                    {
                        xwarn("xchainacct_t::connect_block,fail-block not connect any prev-block finally, for block=%s",this_block->dump().c_str());
                    }
                }
                else
                {
                    process_block(this_block); //start process from current block,and let previous block->this_block
                }
            }

            auto  it_next_height  = this_block_height_it;//copy first
            ++it_next_height; //directly modify iterator
            if(it_next_height != m_all_blocks.end())//found next one
            {
                if(it_next_height->first == (this_block->get_height() + 1))//try to link to next block: this_block  <---next block
                {
                    for(auto it = it_next_height->second.rbegin(); it != it_next_height->second.rend(); ++it)//search from higher view
                    {
                        if(it->second->reset_prev_block(this_block))//this_block  <---next block successful
                        {
                            process_block(it->second); //start process from here
                        }
                    }

                    auto  it_next_next = it_next_height; //try to find next and next one if existing
                    ++it_next_next; //calculated first
                    if( (it_next_next != m_all_blocks.end()) && (it_next_next->first == (this_block->get_height() + 2)) )//if found next_next set
                    {
                        //note: xbft using 3-chain mode, so it request process next and next one
                        for(auto it = it_next_next->second.rbegin(); it != it_next_next->second.rend(); ++it)//search from higher view
                        {
                            process_block(it->second); //start process from here
                        }
                    }
                }
            }
            return true;
        }

    };//end of namespace of vstore
};//end of namespace of top
