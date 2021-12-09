// Copyright (c) 2018-2020 Telos Foundation & contributors
// taylor.wei@topnetwork.org
// Licensed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <cinttypes>
#include "../xvstatestore.h"
#include "../xvledger.h"
#include "../xvdbkey.h"
#include "xmetrics/xmetrics.h"

namespace top
{
    namespace base
    {
        //----------------------------------------xvblkstate_api-------------------------------------//
        xvblkstatestore_t::xvblkstatestore_t() : m_table_state_cache(enum_max_table_bstate_lru_cache_max), m_unit_state_cache(enum_max_unit_bstate_lru_cache_max)
        {

        }

        bool   xvblkstatestore_t::write_state_to_db(const xvaccount_t & target_account, xvbstate_t & target_state,const std::string & target_block_hash)
        {
            if(target_block_hash.empty())
            {
                xerror("xvblkstatestore_t::write_state_to_db,nil block hash for state(%s)",target_state.dump().c_str());
                return false;
            }
            if (target_state.get_block_level() == enum_xvblock_level_table) {
                XMETRICS_GAUGE(metrics::store_state_table_write, 1);
            } else if (target_state.get_block_level() == enum_xvblock_level_unit) {
                XMETRICS_GAUGE(metrics::store_state_unit_write, 1);
            }
            
            const std::string state_db_key = xvdbkey_t::create_prunable_state_key(target_account,target_state.get_block_height(),target_block_hash);

            std::string state_db_bin;
            if(target_state.serialize_to_string(state_db_bin))
            {
                if(xvchain_t::instance().get_xdbstore()->set_value(state_db_key,state_db_bin)) {
                    xinfo("xvblkstatestore_t::write_state_to_db succ.state=%s",target_state.dump().c_str());
                    return true;
                }

                xerror("xvblkstatestore_t::write_state_to_db,fail to write into db for path(%s)",state_db_key.c_str());
                return false;
            }
            xerror("xvblkstatestore_t::write_state_to_db,fail to serialize for state of block(%s)",target_state.dump().c_str());
            return false;
        }

        xvbstate_t*     xvblkstatestore_t::read_state_from_db(const xvbindex_t * for_index)
        {
            if(NULL == for_index)
                return NULL;

            return read_state_from_db(*for_index,for_index->get_height(),for_index->get_block_hash());
        }
        xvbstate_t*     xvblkstatestore_t::read_state_from_db(const xvaccount_t & target_account, const xvblock_t * for_block)
        {
            if(NULL == for_block)
                return NULL;

            return read_state_from_db(target_account,for_block->get_height(),for_block->get_block_hash());
        }
        xvbstate_t*     xvblkstatestore_t::read_state_from_db(const xvaccount_t & target_account, uint64_t block_height, const std::string & block_hash)
        {
            XMETRICS_GAUGE(metrics::store_state_read, 1);
            const std::string state_db_key = xvdbkey_t::create_prunable_state_key(target_account,block_height,block_hash);
            const std::string state_db_bin = xvchain_t::instance().get_xdbstore()->get_value(state_db_key);
            if(state_db_bin.empty())
            {
                xdbg("xvblkstatestore_t::read_state_from_db,fail to read from db for account=%s,height=%ld,hash=%s",target_account.get_account().c_str(), block_height, base::xstring_utl::to_hex(block_hash).c_str());
                return NULL;
            }

            xvbstate_t* state_ptr = xvblock_t::create_state_object(state_db_bin);
            if(NULL == state_ptr)//remove the error data for invalid data
            {
                xvchain_t::instance().get_xdbstore()->delete_value(state_db_key);
                xerror("xvblkstatestore_t::read_state_from_db,invalid data at db for account=%s,height=%ld,hash=%s",target_account.get_account().c_str(), block_height, base::xstring_utl::to_hex(block_hash).c_str());
                return NULL;
            }
            if(state_ptr->get_address() != target_account.get_address())
            {
                xerror("xvblkstatestore_t::read_state_from_db,bad state(%s) vs ask(account:%s) ",state_ptr->dump().c_str(),target_account.get_address().c_str());
                state_ptr->release_ref();
                return NULL;
            }
            xdbg_info("xvblkstatestore_t::read_state_from_db succ.state=%s",state_ptr->dump().c_str());
            return state_ptr;
        }

        bool   xvblkstatestore_t::delete_state_of_db(const xvbindex_t & target_index)
        {
            return delete_state_of_db(target_index,target_index.get_height(),target_index.get_block_hash());            
        }
        bool   xvblkstatestore_t::delete_state_of_db(const xvblock_t & target_block)
        {
            xvaccount_t target_account(target_block.get_account());
            return delete_state_of_db(target_account,target_block.get_height(),target_block.get_block_hash());
        }
        bool   xvblkstatestore_t::delete_state_of_db(const xvaccount_t & target_account,uint64_t block_height,const std::string & block_hash)
        {
            XMETRICS_GAUGE(metrics::store_state_delete, 1);
            const std::string state_db_key = xvdbkey_t::create_prunable_state_key(target_account,block_height,block_hash);
            return xvchain_t::instance().get_xdbstore()->delete_value(state_db_key);
        }
        bool   xvblkstatestore_t::delete_states_of_db(const xvaccount_t & target_account,const uint64_t block_height)
        {
            //delete all stated'object under target height
            xvbindex_vector auto_vector( xvchain_t::instance().get_xblockstore()->load_block_index(target_account,block_height, (int)metrics::blockstore_access_from_statestore_rebuild_state));
            for(auto index : auto_vector.get_vector())
            {
                if(index != NULL)
                {
                    xdbg_info("xvblkstatestore_t::delete_states_of_db.account=%s,height=%ld", target_account.get_account().c_str(), block_height);
                    delete_state_of_db(target_account,index->get_height(),index->get_block_hash());
                }
            }
            return true;
        }

        xobject_ptr_t<xvbstate_t> xvblkstatestore_t::rebuild_bstate(const xvaccount_t & target_account, const xobject_ptr_t<xvbstate_t> & base_state, const std::map<uint64_t, xobject_ptr_t<xvblock_t>> & latest_blocks) {
            xobject_ptr_t<xvbstate_t> current_state = base_state;
            for (auto & v : latest_blocks) {
                auto & _block = v.second;
                xassert(_block->get_height() == current_state->get_block_height() + 1);
                current_state = make_object_ptr<xvbstate_t>(*_block.get(), *current_state.get());
                if (_block->get_block_class() == enum_xvblock_class_light) {
                    if (false == xvchain_t::instance().get_xblockstore()->load_block_output(target_account, _block.get())) {
                        xerror("xvblkstatestore_t::rebuild_bstate,fail-load block output for block(%s)",_block->dump().c_str());
                        return nullptr;
                    }
                    xassert(!_block->get_binlog_hash().empty());
                    std::string binlog = _block->get_binlog();
                    xassert(!binlog.empty());
                    if(false == current_state->apply_changes_of_binlog(binlog)) {
                        xerror("xvblkstatestore_t::rebuild_bstate,invalid binlog and abort it for block(%s)",_block->dump().c_str());
                        return nullptr;
                    }
                } else if (_block->get_block_class() == enum_xvblock_class_full) {
                    std::string binlog;
                    auto canvas = current_state->rebase_change_to_snapshot();
                    canvas->encode(binlog);
                    xassert(!binlog.empty());
                    if (!_block->get_fullstate_hash().empty()) {
                        std::string binlog_hash = base::xcontext_t::instance().hash(binlog, _block->get_cert()->get_crypto_hash_type());
                        if (binlog_hash != _block->get_fullstate_hash()) {
                            xerror("xvblkstatestore_t::rebuild_bstate,unmatch binlog hash and abort it for block(%s)",_block->dump().c_str());
                            return nullptr;
                        }
                    }
                }
            }
            return current_state;
        }

        bool xvblkstatestore_t::load_latest_blocks_and_state(xvblock_t * target_block, xobject_ptr_t<xvbstate_t> & base_bstate, std::map<uint64_t, xobject_ptr_t<xvblock_t>> & latest_blocks) {
            bool table = target_block->get_block_level() == enum_xvblock_level_table ? true: false;
            if (table) {
                XMETRICS_TIME_RECORD("state_load_blk_state_table_cost");
            } else {
                XMETRICS_TIME_RECORD("state_load_blk_state_unit_cost");
            }
            xvaccount_t target_account(target_block->get_account());
            xobject_ptr_t<xvblock_t> cur_block;
            target_block->add_ref();
            cur_block.attach(target_block);
            latest_blocks[cur_block->get_height()] = cur_block;  // push target block to latest blocks
            uint32_t max_count = target_block->get_block_level() == enum_xvblock_level_table ? 4 : 0xFFFF; // TODO(jimmy) always read latest tablestate but may read old unitstate
            uint32_t count = 0;
            bool res = false;
            bool cache = false;
            
            while (count++ < max_count) {
                // load prev block state from cache
                base_bstate = get_lru_cache(cur_block->get_block_level(), cur_block->get_last_block_hash());
                if (base_bstate != nullptr) {
                    xdbg("xvblkstatestore_t::load_latest_blocks_and_state succ-get prev state from cache.block=%s", target_block->dump().c_str());
                    res = true;
                    cache = true;
                    break;
                }

                // load prev block state from db
                xvbstate_t* raw_state = read_state_from_db(target_account, cur_block->get_height() - 1, cur_block->get_last_block_hash());
                if (raw_state != nullptr)
                {
                    base_bstate.attach(raw_state);
                    xdbg("xvblkstatestore_t::load_latest_blocks_and_state succ-get prev state form db.height=%ld,block=%s",cur_block->get_height() - 1, target_block->dump().c_str());
                    res = true;
                    break;
                    // return true;
                }

                // load prev block
                auto prev_block = xvchain_t::instance().get_xblockstore()->load_block_object(target_account, cur_block->get_height() - 1, cur_block->get_last_block_hash(), false,(int)metrics::blockstore_access_from_statestore_load_lastest_state);
                if (nullptr == prev_block) {
                    xwarn("xvblkstatestore_t::load_latest_blocks_and_state fail-load block. account=%s,height=%ld,target_block=%s",
                        target_account.get_account().c_str(), cur_block->get_height() - 1, target_block->dump().c_str());
                    res = false;
                    break;
                }

                // try make prev block state
                base_bstate = make_state_from_current_block(target_account, prev_block.get());
                if (base_bstate != nullptr) {
                    res = true;
                    break;
                }

                // push prev block to latest blocks
                cur_block = prev_block;
                latest_blocks[cur_block->get_height()] = cur_block;
            }
            if (res) {
                XMETRICS_GAUGE(metrics::state_load_blk_state_suc, count);
                if (cache) {
                    XMETRICS_GAUGE(metrics::state_load_blk_state_cache_suc, count);
                }
                if (table) {
                    XMETRICS_GAUGE(metrics::state_load_blk_state_table_suc, count);
                    if (cache) {
                        XMETRICS_GAUGE(metrics::state_load_blk_state_table_cache_suc, count);
                    }
                } 
                else {
                    XMETRICS_GAUGE(metrics::state_load_blk_state_unit_suc, count);
                    if (cache) {
                        XMETRICS_GAUGE(metrics::state_load_blk_state_unit_cache_suc, count);
                    }
                }
                
            } else {
                XMETRICS_GAUGE(metrics::state_load_blk_state_fail, count);
                if (table) {
                    XMETRICS_GAUGE(metrics::state_load_blk_state_table_fail, count);
                } else {
                    XMETRICS_GAUGE(metrics::state_load_blk_state_unit_fail, count);
                }
                xwarn("xvblkstatestore_t::load_latest_blocks_and_state fail-exceed max count.target_block=%s", target_block->dump().c_str());
            }            
            return res;
        }

        xobject_ptr_t<xvbstate_t> xvblkstatestore_t::make_state_from_current_block(const xvaccount_t & target_account, xvblock_t * current_block) {
            xobject_ptr_t<xvbstate_t> current_state = nullptr;
            if (false == xvchain_t::instance().get_xblockstore()->load_block_output(target_account, current_block)) {
                xerror("xvblkstatestore_t::make_state_from_current_block,fail-load block output for block(%s)",current_block->dump().c_str());
                return nullptr;
            }
            // try make state form block self
            if (current_block->get_height() == 0
                || (current_block->get_block_class() == enum_xvblock_class_full && !current_block->get_full_state().empty()) ) {
                current_state = make_object_ptr<xvbstate_t>(*current_block);
                if (current_block->get_block_class() != enum_xvblock_class_nil) {
                    std::string binlog = current_block->get_block_class() == enum_xvblock_class_light ? current_block->get_binlog() : current_block->get_full_state();
                    xassert(!binlog.empty());
                    if(false == current_state->apply_changes_of_binlog(binlog)) {
                        xerror("xvblkstatestore_t::make_state_from_current_block,invalid binlog and abort it for block(%s)",current_block->dump().c_str());
                        return nullptr;
                    }
                }
                xdbg("xvblkstatestore_t::make_state_from_current_block succ-get state form full block.block=%s",current_block->dump().c_str());
                return current_state;
            }
            return nullptr;
        }

        // TODO(jimmy) how clear all old state
        void xvblkstatestore_t::clear_persisted_state(const xvaccount_t & target_account, xvblock_t * target_block) {
            // clear old normal state
            if(target_block->get_height() >= enum_max_bstate_newest_count)
            {
                uint64_t delete_height = target_block->get_height() - enum_max_bstate_newest_count;
                bool is_delete_full_table = target_block->get_block_level() == base::enum_xvblock_level_table && target_block->get_last_full_block_height() == delete_height;
                if (!is_delete_full_table)
                {
                    // should not delete latest full-table state
                    delete_states_of_db(target_account, delete_height);
                }
            }

            // clear old full-table state
            if (target_block->get_block_level() == base::enum_xvblock_level_table && target_block->get_block_class() == enum_xvblock_class_full)
            {
                uint64_t interval_clear_fulltable = (target_block->get_height() - target_block->get_last_full_block_height()) * enum_max_bstate_fulltable_count;
                if (target_block->get_height() >= interval_clear_fulltable)
                {
                    uint64_t delete_height = target_block->get_height() - interval_clear_fulltable;
                    delete_states_of_db(target_account, delete_height);
                }
            }
        }

        void xvblkstatestore_t::set_latest_executed_info(const xvaccount_t & target_account, uint64_t height,const std::string & blockhash)
        {
            // TODO(jimmy) no need set executed block hash xvchain_t::instance().get_xblockstore()->set_latest_executed_info(target_account, height, blockhash);
            xauto_ptr<xvaccountobj_t> account_obj(xvchain_t::instance().get_account(target_account));
            account_obj->set_latest_executed_block(height,blockhash);
        }
        uint64_t xvblkstatestore_t::get_latest_executed_block_height(const xvaccount_t & target_account)
        {
            // base::xvchain_t::instance().get_xblockstore()->get_latest_executed_block_height(target_account);
            xauto_ptr<xvaccountobj_t> account_obj(xvchain_t::instance().get_account(target_account));
            return account_obj->get_latest_executed_block_height();
        }

        bool xvblkstatestore_t::recover_highest_execute_height(const xvaccount_t & target_account, uint64_t old_execute_height)
        {            
            uint64_t max_check_height = old_execute_height + 1024;  // XTODO 1024 may be too large
            for (uint64_t i = old_execute_height; i < max_check_height; i++)
            {
                base::xauto_ptr<base::xvbindex_t> _bindex = xvchain_t::instance().get_xblockstore()->load_block_index(target_account, i, enum_xvblock_flag_committed);
                if (_bindex == nullptr) {
                    xerror("xvblkstatestore_t::recover_highest_execute_height fail-load bindex.account=%s,old_execute_height=%ld,i=%ld",
                        target_account.get_account().c_str(), old_execute_height, i);
                    return false;
                }
                base::xauto_ptr<base::xvbstate_t> _bstate = read_state_from_db(_bindex.get());
                if (_bstate != nullptr) {
                    xinfo("xvblkstatestore_t::recover_highest_execute_height succ-load bstate.account=%s,old_execute_height=%ld,i=%ld",
                        target_account.get_account().c_str(), old_execute_height, i);
                    set_latest_executed_info(target_account, i, _bindex->get_block_hash());
                    return true;
                }
                xwarn("xvblkstatestore_t::recover_highest_execute_height fail-load bstate.account=%s,old_execute_height=%ld,i=%ld",
                    target_account.get_account().c_str(), old_execute_height, i);            
            }
            xerror("xvblkstatestore_t::recover_highest_execute_height fail-recover.account=%s,old_execute_height=%ld,max_check_height=%ld",
                target_account.get_account().c_str(), old_execute_height, max_check_height);
            return false;
        }

        bool xvblkstatestore_t::try_update_execute_height(const xvaccount_t & target_account)
        {            
            // TODO(jimmy) get executed height and commit height together
            uint64_t old_execute_height = get_latest_executed_block_height(target_account);
            uint64_t _highest_commit_block_height = base::xvchain_t::instance().get_xblockstore()->get_latest_committed_block_height(target_account);

            if (old_execute_height >= _highest_commit_block_height) {
                return true;
            }

            uint64_t _begin_height = old_execute_height == 0 ? 0 : old_execute_height + 1;
            uint64_t max_count = 32;
            uint64_t new_execute_height = old_execute_height;
            std::string new_execute_hash;

            do
            {
                if (_begin_height > _highest_commit_block_height)
                {
                    xdbg("xvblkstatestore_t::try_update_execute_height finish. account=%s,_begin_height=%ld,_highest_commit_block_height=%ld,old_execute_height=%ld,new_execute_height=%ld", 
                        target_account.get_account().c_str(), _begin_height, _highest_commit_block_height, old_execute_height, new_execute_height);                    
                    break;
                }
                auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(target_account, _begin_height, enum_xvblock_flag_committed, false, (int)metrics::blockstore_access_from_statestore_get_commit_state);
                if (_block == nullptr) {
                    xwarn("xvblkstatestore_t::try_update_execute_height fail-load committed block. account=%s,height=%ld,execute_height=%ld,commit_height=%ld", 
                        target_account.get_account().c_str(), _begin_height, old_execute_height, _highest_commit_block_height);
                    break;
                }

                xauto_ptr<xvbstate_t> bstate = get_block_state_internal(target_account, _block.get(), (int)metrics::blockstore_access_from_statestore_get_commit_state);
                if (bstate == nullptr) {
                    if (false == recover_highest_execute_height(target_account, _begin_height - 1)) {
                        xerror("xvblkstatestore_t::try_update_execute_height fail-recover execute height. account=%s,height=%ld,execute_height=%ld,commit_height=%ld", 
                            target_account.get_account().c_str(), _begin_height, old_execute_height, _highest_commit_block_height);
                    }
                    break;
                }
                new_execute_height = _block->get_height();
                new_execute_hash = _block->get_block_hash();
                _begin_height++;
            }
            while(max_count-- > 0);

            if (new_execute_height > old_execute_height) {
                set_latest_executed_info(target_account, new_execute_height, new_execute_hash);
                xinfo("xvblkstatestore_t::try_update_execute_height succ-update. account=%s,height=%ld,old_execute_height=%ld,new_execute_height=%ld,commit_height=%ld", 
                    target_account.get_account().c_str(), _begin_height, old_execute_height, new_execute_height, _highest_commit_block_height);
                return true;
            }
            xdbg("xvblkstatestore_t::try_update_execute_height finish2. account=%s,_begin_height=%ld,_highest_commit_block_height=%ld,old_execute_height=%ld,new_execute_height=%ld", 
                    target_account.get_account().c_str(), _begin_height, _highest_commit_block_height, old_execute_height, new_execute_height);                      
            return false;
        }

        bool xvblkstatestore_t::execute_block(xvblock_t * target_block, const int etag)
        {            
            if (target_block->get_block_level() != enum_xvblock_level_table) {
                xassert(false);
                return false;
            }
            xdbg("xvblkstatestore_t::execute_block enter,block=%s", target_block->dump().c_str());
            xvaccount_t target_account(target_block->get_account());

            // the full state block is synced from other nodes, should execute immediately
            if ( (target_block->get_block_class() == base::enum_xvblock_class_full && target_block->is_full_state_block()) 
                || (target_block->get_height() == 0))
            {
                // double check if committed block same with target block, only execute committed full-table
                auto bindex = base::xvchain_t::instance().get_xblockstore()->load_block_index(target_account, target_block->get_height(), base::enum_xvblock_flag_committed, etag);
                if (bindex != nullptr && bindex->get_block_hash() == target_block->get_block_hash())
                {
                    xauto_ptr<xvbstate_t> bstate = get_block_state_internal(target_account, target_block, etag);
                    if (bstate == nullptr)
                    {
                        // shuold never happen
                        xerror("xvblkstatestore_t::execute_block fail-execute full-table,block=%s", target_block->dump().c_str());
                        return false;
                    }
                    else
                    {
                        xinfo("xvblkstatestore_t::execute_block succ-execute full-table,block=%s", target_block->dump().c_str());
                        set_latest_executed_info(target_account, target_block->get_height(), target_block->get_block_hash());
                    }
                }
                else
                {
                    xerror("xvblkstatestore_t::execute_block fail-full-table is not committed,block=%s", target_block->dump().c_str());
                }
            }

            // XTODO always try update execute height when stored new table block
            try_update_execute_height(target_account);
            return true;
        }
#if 0
        bool xvblkstatestore_t::execute_block(xvblock_t * target_block, const int etag)
        {
            xauto_ptr<xvbstate_t> bstate = get_block_state(target_block, etag);
            if (bstate == nullptr)
            {
                xerror("xvblkstatestore_t::execute_block fail-get block state,block=%s", target_block->dump().c_str());
                return false;
            }
            return true;
        }
#endif
        // implement by load blocks and apply these blocks
        xauto_ptr<xvbstate_t>  xvblkstatestore_t::execute_target_block(const xvaccount_t & target_account, xvblock_t * target_block)
        {
            xobject_ptr_t<xvbstate_t> target_bstate = nullptr;
            xdbg("xvblkstatestore_t::execute_target_block enter. block=%s", target_block->dump().c_str());

            // 1.try to make state for target block directly, may it is a full block or genesis block
            target_bstate = make_state_from_current_block(target_account, target_block);
            if (target_bstate != nullptr) {
                return target_bstate;
            }

            // 2.try to rebuild target block state form prev state
            std::map<uint64_t, xobject_ptr_t<xvblock_t>> latest_blocks;
            xobject_ptr_t<xvbstate_t> base_bstate = nullptr;
            if (false == load_latest_blocks_and_state(target_block, base_bstate, latest_blocks)) {
                xwarn("xvblkstatestore_t::execute_target_block fail-load_latest_blocks_and_state.block=%s",target_block->dump().c_str());
                return nullptr;
            }

            if (target_block->get_block_level() == base::enum_xvblock_level_table) {
                XMETRICS_GAUGE(metrics::statestore_get_unit_state_with_unit_count, latest_blocks.size());
            } else if (target_block->get_block_level() == base::enum_xvblock_level_unit) {
                XMETRICS_GAUGE(metrics::statestore_get_table_state_with_table_count, latest_blocks.size());
            }

            target_bstate = rebuild_bstate(target_account, base_bstate, latest_blocks);
            if (target_bstate == nullptr) {
                xwarn("xvblkstatestore_t::execute_target_block fail-rebuild_bstate.block=%s",target_block->dump().c_str());
                return nullptr;
            }
            xassert(target_bstate->get_block_height() == target_block->get_height());
            xassert(target_bstate->get_block_viewid() == target_block->get_viewid());

            xdbg("xvblkstatestore_t::execute_target_block succ.latest_blocks size=%zu,block=%s", latest_blocks.size(), target_block->dump().c_str());
            return target_bstate;
        }

        // implement by load blocks and apply these blocks        
        xauto_ptr<xvbstate_t>  xvblkstatestore_t::get_block_state_internal(const xvaccount_t & target_account, xvblock_t * target_block, const int etag)
        {
            xdbg("xvblkstatestore_t::get_block_state enter.block=%s", target_block->dump().c_str());
            xobject_ptr_t<xvbstate_t> target_bstate = nullptr;

            // try load from cache
            target_bstate = get_lru_cache(target_block->get_block_level(), target_block->get_block_hash());
            if (target_bstate != nullptr) {
                xdbg("xvblkstatestore_t::get_block_state succ-get from cache.block=%s", target_block->dump().c_str());
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)etag, 1);
                return target_bstate;
            }

            // try load from db
            XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)etag, 0);
            xvbstate_t* raw_state = read_state_from_db(target_account, target_block->get_height(), target_block->get_block_hash());
            if (raw_state != nullptr) {
                target_bstate.attach(raw_state);
                set_lru_cache(target_block->get_block_level(), target_block->get_block_hash(), target_bstate);
                xdbg("xvblkstatestore_t::get_block_state succ-get state form db.block=%s",target_block->dump().c_str());
                return target_bstate;
            }

            // try execute target block state
            target_bstate = execute_target_block(target_account, target_block);
            if (target_bstate != nullptr) {
                if (false == write_state_to_db(target_account, *target_bstate.get(),target_block->get_block_hash()))
                {
                    xerror("xvblkstatestore_t::execute_target_block fail-write block state to db,block=%s", target_block->dump().c_str());
                    return nullptr;
                }
                set_lru_cache(target_block->get_block_level(), target_block->get_block_hash(), target_bstate);
                clear_persisted_state(target_account, target_block);
                xdbg("xvblkstatestore_t::get_block_state succ-get state by execute.block=%s",target_block->dump().c_str());
                return target_bstate;
            }
            return nullptr;
        }

        xobject_ptr_t<xvbstate_t> xvblkstatestore_t::get_lru_cache(base::enum_xvblock_level blocklevel, const std::string & hash) {
            xobject_ptr_t<xvbstate_t> state = nullptr;
            if (blocklevel == enum_xvblock_level_table) {
                m_table_state_cache.get(hash, state);
                XMETRICS_GAUGE(metrics::statestore_get_table_state_from_cache, state != nullptr ? 1 : 0);
            } else if (blocklevel == enum_xvblock_level_unit) {
                m_unit_state_cache.get(hash, state);
                XMETRICS_GAUGE(metrics::statestore_get_unit_state_from_cache, state != nullptr ? 1 : 0);
            }

            return state;
        }

        void xvblkstatestore_t::set_lru_cache(base::enum_xvblock_level blocklevel, const std::string & hash, const xobject_ptr_t<xvbstate_t> & state) {
            if (blocklevel == enum_xvblock_level_table) {
                m_table_state_cache.put(hash, state);
            } else if (blocklevel == enum_xvblock_level_unit) {
                m_unit_state_cache.put(hash, state);
            }
        }

        xauto_ptr<xvbstate_t>  xvblkstatestore_t::get_block_state(xvblock_t * target_block, const int etag)
        {
            if (target_block == nullptr) {
                xassert(false);
                return nullptr;
            }
            xvaccount_t target_account(target_block->get_account());
            auto _bstate = get_block_state_internal(target_account, target_block, etag);
            if(target_block->get_block_level() == enum_xvblock_level_unit) {
                XMETRICS_GAUGE(metrics::statestore_get_unit_state_succ, _bstate != nullptr ? 1 : 0);
            } else if (target_block->get_block_level() == enum_xvblock_level_table) {
                XMETRICS_GAUGE(metrics::statestore_get_table_state_succ, _bstate != nullptr ? 1 : 0);
            }
            if (_bstate != nullptr) {
                return _bstate;
            }
            xwarn("xvblkstatestore_t::get_block_state fail-get block state,block=%s", target_block->dump().c_str());

            // XTODO always try update execute height when stored new table block
            if (target_block->get_block_level() == enum_xvblock_level_table) {
                try_update_execute_height(target_account);
            }
            return nullptr;
        }

        xauto_ptr<xvbstate_t> xvblkstatestore_t::get_block_state(const xvaccount_t & target_account,const uint64_t block_height,const std::string& block_hash, const int etag)
        {
            //step#1: check cached states object per 'account and block'hash
            {
                //XTODO
            }
            base::xauto_ptr<base::xvblock_t> current_block( xvchain_t::instance().get_xblockstore()->load_block_object(target_account,block_height,block_hash,false, (int)metrics::blockstore_access_from_statestore_get_block_state));
            if(!current_block)
            {
                xerror("xvblkstatestore_t::get_block_state,fail to load raw block(%s->height(%" PRIu64 ")->hash(%s)) from blockstore",target_account.get_address().c_str(),block_height,block_hash.c_str());
                return nullptr;
            }
            return get_block_state(current_block(), etag);
        }
        xauto_ptr<xvbstate_t> xvblkstatestore_t::get_latest_connectted_block_state(const xvaccount_t & account, const int etag)
        {
            auto _block = base::xvchain_t::instance().get_xblockstore()->get_latest_connected_block(account, (int)metrics::blockstore_access_from_statestore_get_connect_state);
            if (_block == nullptr) {
                xerror("xvblkstatestore_t::get_latest_connectted_block_state fail-load latest connectted block. account=%s", account.get_account().c_str());
                return nullptr;
            }

            if (_block->is_genesis_block() && _block->get_block_class() == base::enum_xvblock_class_nil) {
                xwarn("xvblkstatestore_t::get_latest_connectted_block_state  fail-invalid state for empty genesis block. account=%s", account.get_account().c_str());
                return nullptr;
            }
            return get_block_state(_block.get(), etag);
        }

        xauto_ptr<xvbstate_t> xvblkstatestore_t::get_committed_block_state(const xvaccount_t & account,const uint64_t block_height, const int etag)
        {
            auto _block = base::xvchain_t::instance().get_xblockstore()->load_block_object(account, block_height, enum_xvblock_flag_committed, false, (int)metrics::blockstore_access_from_statestore_get_commit_state);
            if (_block == nullptr) {
                xwarn("xvblkstatestore_t::get_committed_block_state fail-load committed block. account=%s,height=%ld", account.get_account().c_str(), block_height);
                return nullptr;
            }
            return get_block_state(_block.get(), etag);
        }

        bool xvblkstatestore_t::get_full_block_offsnapshot(xvblock_t * current_block, const int etag)
        {
            if (current_block->is_full_state_block()) {
                return true;
            }

            xobject_ptr_t<xvbstate_t> target_bstate = nullptr;
            do {
                target_bstate = get_lru_cache(current_block->get_block_level(), current_block->get_block_hash());
                if (target_bstate != nullptr) {
                    XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)etag, 1);
                    break;
                }
                XMETRICS_GAUGE((top::metrics::E_SIMPLE_METRICS_TAG)etag, 0);
                xvbstate_t* raw_state = read_state_from_db(base::xvaccount_t(current_block->get_account()), current_block->get_height(), current_block->get_block_hash());
                if (raw_state != nullptr) {
                    target_bstate.attach(raw_state);
                    break;
                }
                xwarn("xvblkstatestore_t::get_full_block_offsnapshot fail-get state for block.block=%s",current_block->dump().c_str());
                return false;
            } while(0);

            std::string property_snapshot;
            auto canvas = target_bstate->rebase_change_to_snapshot();
            canvas->encode(property_snapshot);
            current_block->set_offblock_snapshot(property_snapshot);
            return true;
        }

        //----------------------------------------xvstatestore_t-------------------------------------//
        xvstatestore_t::xvstatestore_t()
        :xobject_t(enum_xobject_type_vstatestore)
        {

        }

        xvstatestore_t::~xvstatestore_t()
        {
        }

        void*   xvstatestore_t::query_interface(const int32_t type)
        {
            if(type == enum_xobject_type_vstatestore)
                return this;

            return xobject_t::query_interface(type);
        }

        //note: when target_account = block_to_hold_state.get_account() -> return get_block_state
        //othewise return get_account_state
        xauto_ptr<xvexestate_t> xvstatestore_t::load_state(const xvaccount_t & target_account,xvblock_t * block_to_hold_state)
        {
            xassert(block_to_hold_state != NULL);
            if(NULL == block_to_hold_state)
                return nullptr;

            //unit block-based state-managment
            if(target_account.get_address() == block_to_hold_state->get_account())
            {
                xauto_ptr<xvbstate_t> block_state(get_block_state(block_to_hold_state, metrics::statestore_access_from_vledger_load_state));
                if(block_state)
                    block_state->add_ref(); //alloc reference for xauto_ptr<xvexestate_t>
                return  block_state();
            }

            //enter traditional state-management branch
            xerror("XTODO,xvstatestore_t::load_state() try go unsupported branch");
            return nullptr;
        }



    };//end of namespace of base
};//end of namespace of top
