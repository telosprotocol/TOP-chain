// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xbase/xhash.h"
#include "xbase/xcontext.h"
#include "xbase/xthread.h"
#include "xvblockhub.h"
#include "xvunithub.h"

namespace top
{
    namespace store
    {
        #define LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account_vid) \
            if(is_close())\
            {\
                xwarn_err("xvblockstore has closed at store_path=%s",m_store_path.c_str());\
                return nullptr;\
            }\
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account_vid.get_xvid()); \
            std::lock_guard<std::recursive_mutex> _dummy(target_table->get_lock()); \
            base::xauto_ptr<xblockacct_t> account_obj(get_block_account(target_table,account_vid.get_address())); \

        xvblockstore_impl::xvblockstore_impl(const std::string & blockstore_path,base::xcontext_t & _context,const int32_t target_thread_id)
            :base::xvblockstore_t(_context,target_thread_id)
        {
            m_raw_timer  = nullptr;

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

            if(m_raw_timer != nullptr)
            {
                m_raw_timer->stop();
                m_raw_timer->close();
            }
            return base::xiobject_t::on_object_close();
        }

        base::xauto_ptr<xblockacct_t>  xvblockstore_impl::get_block_account(base::xvtable_t * target_table,const std::string & account_address)
        {
            const xobject_t* exist_block_plugin = target_table->get_account_plugin_unsafe(account_address, base::enum_xvaccount_plugin_blockmgr);//exist_block_plugin has done add_ref by get_account_plugin_unsafe
            if(exist_block_plugin != NULL)
                return (xblockacct_t*)exist_block_plugin; //pass reference to xauto_ptr that release later

            xblockacct_t * new_plugin = new xchainacct_t(account_address,enum_account_idle_timeout_ms,m_store_path);//replace by new account address;
            new_plugin->init();

            const uint64_t _timenow = get_time_now();//note:x86 guanrentee it is atomic access for integer
            new_plugin->set_last_access_time(_timenow);
            //now ready to deliver to monitor thread
            {
                std::function<void(void*)> _add_new_plugin_job = [this](void* _account_plugin)->void{
                    xblockacct_t * _new_obj = (xblockacct_t*)_account_plugin;
                    m_monitor_expire.insert(std::multimap<uint64_t,xblockacct_t*>::value_type(_new_obj->get_idle_duration() + get_time_now(),_new_obj));
                };
                new_plugin->add_ref();//add reference to hold by m_monitor_expire
                send_call(_add_new_plugin_job,(void*)new_plugin);//send account ptr to store'thread to manage lifecycle
            }
            target_table->set_account_plugin(account_address, new_plugin, base::enum_xvaccount_plugin_blockmgr);
            return new_plugin;
        }

        /////////////////////////////////new api with better performance by passing base::xvaccount_t
        base::xvblock_t * xvblockstore_impl::load_block_from_index(xblockacct_t* target_account, base::xauto_ptr<base::xvbindex_t> target_index,const uint64_t target_height,bool ask_full_load)
        {
            ask_full_load = true;  // TODO(jimmy)
            if(!target_index)
            {
                if(target_height != 0)
                    xwarn("xvblockstore_impl load_block_from_index() fail load index at height(%llu) for account(%s) at store(%s)",target_height,target_account->get_address().c_str(),m_store_path.c_str());
                else
                    xwarn("xvblockstore_impl load_block_from_index() fail load index of latest for account(%s) at store(%s)",target_account->get_address().c_str(),m_store_path.c_str());
                return nullptr;
            }

            bool loaded_new_block = false;
            if(target_index->get_this_block() == NULL)
                loaded_new_block = target_account->load_block_object(target_index.get());

            if(ask_full_load)
            {
                target_account->load_index_input(target_index.get());
                target_account->load_index_output(target_index.get());
                target_account->load_index_offdata(target_index.get());
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
                    target_account->clean_caches(false);//light cleanup

                return raw_block_ptr;
            }
            //XTODO, add code to rebuild block from table block

            xassert(0);
            return nullptr;
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_genesis_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_genesis_index(),0,false);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_cert_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_cert_index(),0,false);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_locked_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_locked_index(),0,false);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_committed_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_committed_index(),0,false);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_executed_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_executed_index(),0,false);
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_connected_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_connected_index(),0,false);
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::get_latest_genesis_connected_block(const base::xvaccount_t & account)//block has connected to genesis
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_genesis_connected_index(),0,false);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_full_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_full_index(),0,false);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::get_latest_committed_full_block(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_latest_committed_full_index(),0,false);
        }

        //one api to get latest_commit/latest_lock/latest_cert for better performance
        base::xblock_mptrs  xvblockstore_impl::get_latest_blocks(const base::xvaccount_t & account)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);

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

        base::xblock_vector xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height)//might mutiple certs at same height
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
                            base::xvblock_t * block = load_block_from_index(account_obj.get(),base::xauto_ptr<base::xvbindex_t>(std::move(auto_index_ptr)),index->get_height(),false);
                            if(block != NULL)
                                block_list.push_back(block);//ptr will be released by xblock_vector later

                        }
                    }
                    return block_list;
                }
            }
            return base::xblock_vector();
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid,bool ask_full_load)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(height,viewid),height,ask_full_load);
        }
        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash,bool ask_full_load)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(height,blockhash),height,ask_full_load);
        }

        base::xauto_ptr<base::xvblock_t>    xvblockstore_impl::load_block_object(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block,bool ask_full_load)  //just return the highest viewid of matched flag
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->load_index(height,required_block),height,ask_full_load);
        }

        bool                xvblockstore_impl::load_block_input(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::load_block_input,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_block_input(block);//XTODO,add logic to extract from tabeblock
        }

        bool                xvblockstore_impl::load_block_output(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::load_block_output,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_block_output(block);//XTODO,add logic to extract from tabeblock
        }

        //load xvboffdata_t and set into xvblock_t
        bool                xvblockstore_impl::load_block_offdata(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::load_block_offdata,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_block_offdata(block);//XTODO,add logic to extract from tabeblock
        }

        bool    xvblockstore_impl::store_block(base::xauto_ptr<xblockacct_t> & container_account,base::xvblock_t * container_block) //store table/book blocks if they are
        {
            xdbg("jimmy xvblockstore_impl::store_block enter,store block(%s)", container_block->dump().c_str());
            //store it first at anyway
            if(!container_account->store_block(container_block))
            {
                xwarn("xvblockstore_impl::store_block,fail-store block(%s)", container_block->dump().c_str());
                return false;
            }
            container_account->try_execute_all_block();
            //then try extract for container if that is
            if(container_block->get_block_class() == base::enum_xvblock_class_light) //skip nil block
            {
                //add other container here if need
                if(container_block->get_header()->get_block_level() == base::enum_xvblock_level_table)
                {
                    //XTODO index add flag to avoiding repeat unpack unit
                    xassert(container_block->is_input_ready(true));
                    xassert(container_block->is_output_ready(true));

                    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
                    if(container_block->extract_sub_blocks(sub_blocks))
                    {
                        xinfo("xvblockstore_impl::store_block,table block(%s) carry unit num=%d", container_block->dump().c_str(), (int)sub_blocks.size());

                        for (auto & unit_block : sub_blocks)
                        {
                            base::xvaccount_t  unit_account(unit_block->get_account());
                            //XTODO,move set_parent_block into extract_sub_blocks
                            //unit_block->set_parent_block(container_block->get_account(),container_block->get_viewid(),xxx);

                            if(false == store_block(unit_account,unit_block.get())) //any fail resultin  re-unpack whole table again
                            {
                                xwarn("xvblockstore_impl::store_block,fail-store unit-block=%s from tableblock=%s",unit_block->dump().c_str(),container_block->dump().c_str());
                            }
                            else
                            {
                                xinfo("xvblockstore_impl::store_block,stored unit-block=%s from tableblock=%s",unit_block->dump().c_str(),container_block->dump().c_str());

                                on_block_stored(unit_block.get());//throw event for sub blocks
                            }
                        }
                    }
                    else
                    {
                        xerror("xvblockstore_impl::store_block,fail-extract_sub_blocks for table block(%s)", container_block->dump().c_str(), (int)sub_blocks.size());
                    }
                }
            }
            return true; //still return true since tableblock has been stored successful
        }

        bool                xvblockstore_impl::store_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            if(store_block(account_obj,block))
            {
                on_block_stored(block);
                return true;
            }
            return false;
        }

        bool                xvblockstore_impl::store_blocks(const base::xvaccount_t & account,std::vector<base::xvblock_t*> & batch_store_blocks)
        {
            if(batch_store_blocks.empty())
                return true;

            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            for(auto it : batch_store_blocks)
            {
                if((it != nullptr) && (it->get_account() == account_obj->get_address()) )
                {
                    if(store_block(account_obj,it))
                        on_block_stored(it);
                }
            }
            return  true;
        }

        bool                xvblockstore_impl::delete_block(const base::xvaccount_t & account,base::xvblock_t* block)
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::store_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->delete_block(block);
        }

        //note: block must be committed and connected
        bool                xvblockstore_impl::execute_block(const base::xvaccount_t & account,base::xvblock_t* block) //execute block and update state of acccount
        {
            if( (nullptr == block) || (account.get_account() != block->get_account()) )
            {
                xerror("xvblockstore_impl::execute_block,block NOT match account:%",account.get_account().c_str());
                return false;
            }
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->execute_block(block);
        }

        base::xvtransaction_store_ptr_t  xvblockstore_impl::query_tx(const std::string & txhash, base::enum_transaction_subtype type)
        {
            //XTODO:tx always not cache now
            std::string txkey;
            base::xvtransaction_store_ptr_t txstore = make_object_ptr<base::xvtransaction_store_t>();

            // std::string rawtxkey = base::xvdbkey_t::create_tx_key(txhash);
            auto raw_tx = base::xvchain_t::instance().get_xtxstore()->load_tx_obj(txhash);
            if(nullptr == raw_tx)
            {
                xwarn("xvblockstore_impl::query_tx tx content read from fail.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                return nullptr;
            }
            txstore->set_raw_tx(raw_tx.get());

            if(type == base::enum_transaction_subtype_all || type == base::enum_transaction_subtype_self || type == base::enum_transaction_subtype_send)
            {
                base::xvtxindex_ptr txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txhash, base::enum_transaction_subtype_send);
                if(!txindex)
                {
                    xwarn("xvblockstore_impl::query_tx send tx not find.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                    return nullptr;
                }
                txstore->set_send_unit_info(txindex);
                if(txindex->is_self_tx())
                {
                    xdbg("xvblockstore_impl::query_tx self tx");  //self tx no need query more
                    txstore->set_recv_unit_info(txindex);
                    txstore->set_confirm_unit_info(txindex);
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
                txstore->set_recv_unit_info(txindex);
            }
            if(type == base::enum_transaction_subtype_all || type == base::enum_transaction_subtype_confirm)
            {
                base::xvtxindex_ptr txindex = base::xvchain_t::instance().get_xtxstore()->load_tx_idx(txhash, base::enum_transaction_subtype_confirm);
                if(!txindex)
                {
                    xwarn("xvblockstore_impl::query_tx confirm tx not find.tx=%s", base::xstring_utl::to_hex(txhash).c_str());
                    return (type == base::enum_transaction_subtype_all) ? txstore : nullptr;
                }
                txstore->set_confirm_unit_info(txindex);
            }
            return txstore;
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height, const uint64_t viewid)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->query_index(height,viewid),height,false);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height, const std::string & blockhash)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->query_index(height,blockhash),height,false);
        }

        base::xauto_ptr<base::xvblock_t>  xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block) //just return the highest viewid of matched flag
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return load_block_from_index(account_obj.get(),account_obj->query_index(height,required_block),height,false);
        }

        base::xblock_vector xvblockstore_impl::query_block(const base::xvaccount_t & account,const uint64_t height)//might mutiple certs at same height
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
                            base::xvblock_t * block = load_block_from_index(account_obj.get(),base::xauto_ptr<base::xvbindex_t>(std::move(auto_index_ptr)),index->get_height(),false);
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

        base::xvbindex_vector   xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            if(account_obj->load_index(height)) //load first
                return base::xvbindex_vector(account_obj->query_index(height));//then query

            xwarn("xvblockstore_impl load_block_index(height) fail to load block(%llu) for account(%s) at store(%s)",height,account.get_address().c_str(),m_store_path.c_str());
            return base::xvbindex_vector();
        }

        base::xauto_ptr<base::xvbindex_t>  xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,const uint64_t viewid)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_index(height,viewid);
        }

        base::xauto_ptr<base::xvbindex_t>  xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,const std::string & blockhash)
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_index(height,blockhash);
        }

        base::xauto_ptr<base::xvbindex_t>  xvblockstore_impl::load_block_index(const base::xvaccount_t & account,const uint64_t height,base::enum_xvblock_flag required_block)//just return the highest viewid of matched flag
        {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            return account_obj->load_index(height,required_block);
        }

        //clean unsed caches of account to recall memory. notes: clean caches not affect the persisten data of account
        bool                xvblockstore_impl::clean_caches(const base::xvaccount_t & account)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account.get_xvid());
            std::lock_guard<std::recursive_mutex> _dummy(target_table->get_lock());
            base::xauto_ptr<xblockacct_t> target_account(get_block_account(target_table,account.get_address()));
            return target_account->clean_caches(true);
        }

        //clean all cached blocks after reach max idle duration(as default it is 60 seconds)
        bool                xvblockstore_impl::reset_cache_timeout(const base::xvaccount_t & account,const uint32_t max_idle_time_ms)
        {
            if(base::xvblockstore_t::is_close())
            {
                xwarn_err("xvblockstore_impl has closed at store_path=%s",m_store_path.c_str());
                return false;
            }
            base::xvtable_t * target_table = base::xvchain_t::instance().get_table(account.get_xvid());
            std::lock_guard<std::recursive_mutex> _dummy(target_table->get_lock());
            base::xauto_ptr<xblockacct_t> target_account(get_block_account(target_table,account.get_address()));
            return target_account->reset_cache_timeout(max_idle_time_ms);
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

                xblockacct_t* _test_for_plugin = expire_it->second;
                if(_test_for_plugin != nullptr)
                {
                    if( (false == _test_for_plugin->is_live(current_time_ms)) || (total_active_acounts > enum_max_active_acconts) ) //force to remove most less-active account while too much caches
                    {
                        base::xauto_ptr<base::xvaccountobj_t> target_account_container = base::xvchain_t::instance().get_account(*_test_for_plugin);
                        //always use same lock for same account
                        std::lock_guard<std::recursive_mutex> _dummy(target_account_container->get_lock());

                        if( (false == _test_for_plugin->is_live(current_time_ms)) || (total_active_acounts > enum_max_active_acconts) ) //force to remove most less-active account while too much caches
                        {
                            _test_for_plugin->close(); //mark to close first
                            _test_for_plugin->release_ref(); //now release last reference hold by m_monitor_expire

                            target_account_container->set_plugin(NULL, base::enum_xvaccount_plugin_blockmgr);
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
        
        bool      xvblockstore_impl::exist_genesis_block(const base::xvaccount_t & account) {
            LOAD_BLOCKACCOUNT_PLUGIN(account_obj,account);
            base::xvbindex_t* target_block = account_obj->query_index(0, 0);
#if (defined DEBUG)
            if(target_block != NULL) {//the ptr has been add reference by query_index
                xdbg("xvblockstore_impl::exist_genesis_block target_block not null");
            } else {
                xdbg("xvblockstore_impl::exist_genesis_block target_block null");
            }
#endif
            return (NULL == target_block) ? false : true;
        }
    };//end of namespace of vstore
};//end of namespace of top
