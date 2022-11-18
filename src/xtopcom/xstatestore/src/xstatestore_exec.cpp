// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbase/xutl.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xblockbuild.h"
#include "xdata/xblocktool.h"
#include "xmbus/xevent_behind.h"
#include "xstatestore/xstatestore_exec.h"
#include "xstatestore/xerror.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, statestore)

std::mutex xstatestore_executor_t::m_global_execute_lock;

xstatestore_executor_t::xstatestore_executor_t(common::xaccount_address_t const& table_addr, xexecute_listener_face_t * execute_listener)
: m_table_addr{table_addr},m_state_accessor{table_addr},m_execute_listener(execute_listener) {

}

void xstatestore_executor_t::init() {
    uint64_t old_executed_height = m_statestore_base.get_latest_executed_block_height(m_table_addr);
    recover_execute_height(old_executed_height);
}

void xstatestore_executor_t::recover_execute_height(uint64_t old_executed_height) {    
    // XTODO recover execute_height because the state of execute height may be pruned
    for (uint64_t i = old_executed_height; i < old_executed_height + 512; i++) {
        xobject_ptr_t<base::xvblock_t> _block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), i, base::enum_xvblock_flag_committed, false);
        if (nullptr == _block) {
            xwarn("xstatestore_executor_t::xstatestore_executor_t fail-load block. table=%s,execute_height=%ld,height=%ld,",
                  m_table_addr.to_string().c_str(),
                  old_executed_height,
                  i);
            if (i == 0) {
                base::xauto_ptr<base::xvblock_t> _genesis_block = data::xblocktool_t::create_genesis_empty_table(m_table_addr.to_string());
                _block = _genesis_block;
            } else {
                continue;
            }
        }

        std::error_code ec;
        xtablestate_ext_ptr_t tablestate_ext;
        if (i == 0) {
            // the state of height#0 may not writted to db
            tablestate_ext = make_state_from_current_table(_block.get(), ec);
        } else {
            tablestate_ext = m_state_accessor.read_table_bstate_from_db(m_table_addr, _block.get());
        }
        if (nullptr == tablestate_ext) {
            xwarn("xstatestore_executor_t::xstatestore_executor_t fail-load tablestate. table=%s,execute_height=%ld,height=%ld",
                  m_table_addr.to_string().c_str(),
                  old_executed_height,
                  i);
            continue;
        }

        update_latest_executed_info(_block.get());
        m_state_accessor.set_latest_connectted_tablestate(tablestate_ext);
        m_state_accessor.write_table_bstate_to_cache(m_table_addr, _block->get_block_hash(), tablestate_ext);
        xinfo("xstatestore_executor_t::xstatestore_executor_t succ table=%s,execute_height=%ld,%ld",
              m_table_addr.to_string().c_str(),
              old_executed_height,
              get_latest_executed_block_height());
        return;
    }    

    // XTODO should not happen
    xerror("xstatestore_executor_t::xstatestore_executor_t fail-recover execute height. %s,height=%ld", m_table_addr.to_string().c_str(), old_executed_height);
}

void xstatestore_executor_t::on_table_block_committed(base::xvblock_t* block) const {
    if (false == block->check_block_flag(base::enum_xvblock_flag_committed)) {
        xassert(false);
        return;
    }

    std::lock_guard<std::mutex> l(m_execute_lock);
    std::error_code ec;
    uint64_t old_execute_height = get_latest_executed_block_height();
    if (block->get_height() <= old_execute_height) {
        xdbg("xstatestore_executor_t::on_table_block_committed finish-already done.execute_height old=%ld,block=%s", old_execute_height, block->dump().c_str());
        return;
    }

    xinfo("xstatestore_executor_t::on_table_block_committed enter.execute_height=%ld,block=%s", old_execute_height,block->dump().c_str());
    if (block->get_height() == old_execute_height + 1) {
        uint32_t limit = 1;
        execute_block_recursive(block, limit, ec);
        if (ec) {
            xwarn("xstatestore_executor_t::on_table_block_committed fail-execute match block.execute_height=%ld,block=%s", old_execute_height,block->dump().c_str());
        }
        return;
    }

    uint64_t new_execute_height = update_execute_from_execute_height(old_execute_height);
    if (block->get_height() > new_execute_height) {
        // TODO(jimmy) still can't execute, for compatibility old version block, the full state block is synced from other nodes, try to jump execute immediately
        if ( block->get_block_class() == base::enum_xvblock_class_full 
            && block->is_full_state_block()
            && m_statestore_base.get_state_root_from_block(block).empty() ) {
            xinfo("xstatestore_executor_t::on_table_block_committed full state block.execute_height old=%ld,block=%s,is_fullstate=%d", old_execute_height, block->dump().c_str(),block->is_full_state_block());
            uint32_t limit = 1;
            execute_block_recursive(block, limit, ec);
            if (ec) {
                xwarn("xstatestore_executor_t::on_table_block_committed fail-execute full state block.execute_height=%ld,block=%s", old_execute_height,block->dump().c_str());
            }
        }
    }
}

void xstatestore_executor_t::update_latest_executed_info(base::xvblock_t* block) const {
    // update execute height
    if (block->check_block_flag(base::enum_xvblock_flag_committed)) {
        set_latest_executed_info(block->get_height(), block->get_block_hash());
    } else {
        if (block->get_height() > 2) {
            set_latest_executed_info(block->get_height()-2, std::string());  // TODO(jimmy) execute hash not need
        }
    }
}

xtablestate_ext_ptr_t xstatestore_executor_t::execute_and_get_tablestate_ext_unlock(base::xvblock_t* block, std::error_code & ec) const {
    uint64_t execute_height = get_latest_executed_block_height();
    xtablestate_ext_ptr_t tablestate_ext = nullptr;

    // 1. read state if block height less than execute height
    if (block->get_height() <= execute_height) {
        tablestate_ext = m_state_accessor.read_table_bstate(m_table_addr, block);
        if (nullptr == tablestate_ext) {
            ec = error::xerrc_t::statestore_load_tablestate_err;
            xwarn("xstatestore_executor_t::execute_and_get_tablestate_ext_unlock fail-read state.execute_height=%ld,block=%s",execute_height,block->dump().c_str());
            return nullptr;
        }
        xdbg("xstatestore_executor_t::execute_and_get_tablestate_ext_unlock succ-read state.execute_height=%ld,block=%s",execute_height,block->dump().c_str());
        return tablestate_ext;
    }
    
    // 2. try push block execute if long distance from execute height
    if (block->get_height() > execute_height + 2) {        
        uint64_t new_execute_height = update_execute_from_execute_height(execute_height);
        if (block->get_height() > new_execute_height + 2) {
            ec = error::xerrc_t::statestore_cannot_execute_for_long_distance_err;
            xwarn("xstatestore_executor_t::execute_and_get_tablestate_ext_unlock fail-can't execute for long distance.execute_height=%ld,block=%s", execute_height,block->dump().c_str());
            return nullptr;
        }
    }

    tablestate_ext = m_state_accessor.read_table_bstate(m_table_addr, block);
    if (nullptr == tablestate_ext) {
        uint32_t limit = 2;
        tablestate_ext = execute_block_recursive(block, limit, ec);
    }

    if (nullptr == tablestate_ext) {
        xwarn("xstatestore_executor_t::execute_and_get_tablestate_ext_unlock fail-execute recursive.execute_height=%ld,block=%s",execute_height,block->dump().c_str());
    } else {
        xdbg("xstatestore_executor_t::execute_and_get_tablestate_ext_unlock succ-execute recursive.execute_height=%ld,block=%s",execute_height,block->dump().c_str());
    }
    return tablestate_ext;
}

xtablestate_ext_ptr_t xstatestore_executor_t::execute_and_get_tablestate_ext(base::xvblock_t* block, std::error_code & ec) const {
    std::lock_guard<std::mutex> l(m_execute_lock);
    return execute_and_get_tablestate_ext_unlock(block, ec);
}

// XTODO should always get successfully
xtablestate_ext_ptr_t xstatestore_executor_t::get_latest_executed_tablestate_ext() const {
    std::lock_guard<std::mutex> l(m_execute_lock);

    uint64_t execute_height = get_latest_executed_block_height();
    uint64_t new_execute_height = update_execute_from_execute_height(execute_height);

    auto cache_tablestate = m_state_accessor.get_latest_connectted_table_state();
    if (cache_tablestate->get_table_state()->height() >= new_execute_height) {
        return cache_tablestate;
    }

    xobject_ptr_t<base::xvblock_t> latest_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), new_execute_height, base::enum_xvblock_flag_committed, false);
    if (nullptr != latest_block) {
        std::error_code ec;
        xtablestate_ext_ptr_t tablestate_ext = execute_and_get_tablestate_ext_unlock(latest_block.get(), ec);
        if (nullptr != tablestate_ext) {
            m_state_accessor.set_latest_connectted_tablestate(tablestate_ext);
            return tablestate_ext;
        }
    }
    xerror("xstatestore_executor_t::get_latest_executed_tablestate_ext fail.table=%s,height=%ld,%ld,block_exist=%d",
           m_table_addr.to_string().c_str(),
           execute_height,
           new_execute_height,
           nullptr != latest_block);
    return cache_tablestate;
}

xtablestate_ext_ptr_t xstatestore_executor_t::do_commit_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const {
    std::lock_guard<std::mutex> l(m_execute_lock);
    return write_table_all_states(current_block, tablestate_store, ec);
}

void xstatestore_executor_t::execute_and_get_accountindex(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const {
    xtablestate_ext_ptr_t tablestate_ext = execute_and_get_tablestate_ext(block, ec);
    if (nullptr != tablestate_ext) {
        tablestate_ext->get_accountindex(unit_addr.to_string(), account_index, ec);
    }
}

void xstatestore_executor_t::execute_and_get_tablestate(base::xvblock_t* block, data::xtablestate_ptr_t &tablestate, std::error_code & ec) const {
    xtablestate_ext_ptr_t tablestate_ext = execute_and_get_tablestate_ext(block, ec);
    if (nullptr != tablestate_ext) {
        tablestate = tablestate_ext->get_table_state();
    }
}

xtablestate_ext_ptr_t xstatestore_executor_t::execute_block_recursive(base::xvblock_t* block, uint32_t & limit, std::error_code & ec) const {
    xassert(!ec);
    xdbg("xstatestore_executor_t::execute_block_recursive enter.block=%s,limit=%d",block->dump().c_str(),limit);
    if (limit == 0) {
        XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 0);
        ec = error::xerrc_t::statestore_try_limit_arrive_err;
        xwarn("xstatestore_executor_t::execute_block_recursive fail-limit to zero.block=%s", block->dump().c_str());
        return nullptr;
    }
    limit--;

    // 1.try make state from current table
    xtablestate_ext_ptr_t tablestate = make_state_from_current_table(block, ec);
    if (ec) {
        XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 0);
        xwarn("xstatestore_executor_t::execute_block_recursive fail-make_state_from_current_table.limit=%d,block=%s", limit,block->dump().c_str());
        return nullptr;
    }
    if (nullptr != tablestate) {
        XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 1);
        xdbg("xstatestore_executor_t::execute_block_recursive succ by make from current.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return tablestate;
    }

    // 2.try load prev-state from cache or db      should load block first for get state-root
    xassert(block->get_height() > 0);
    xtablestate_ext_ptr_t prev_tablestate = m_state_accessor.read_table_bstate_from_cache(m_table_addr, block->get_height() - 1, block->get_last_block_hash());
    if (nullptr == prev_tablestate) {
        xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
        XMETRICS_GAUGE(metrics::statestore_load_table_block_succ, nullptr != prev_block ? 1 : 0);
        if (nullptr == prev_block) {
            XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 0);
            ec = error::xerrc_t::statestore_load_tableblock_err;
            xwarn("xstatestore_executor_t::execute_block_recursive fail-load prev block.cur_block=%s", block->dump().c_str());
            return nullptr;     
        }
        prev_tablestate = m_state_accessor.read_table_bstate_from_db(m_table_addr, prev_block.get());
        if (nullptr == prev_tablestate) {
            xwarn("xstatestore_executor_t::execute_block_recursive fail-read prev state from db.limit=%d,cur_block=%s", limit,block->dump().c_str());

            prev_tablestate = execute_block_recursive(prev_block.get(), limit, ec);
            if (nullptr == prev_tablestate) {
                XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 0);
                xwarn("xstatestore_executor_t::execute_block_recursive fail-execute prev block.limit=%d,cur_block=%s", limit,block->dump().c_str());
                return nullptr;
            }
        }
    }

    tablestate = make_state_from_prev_state_and_table(block, prev_tablestate, ec);
    if (ec) {
        XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 0);
        xwarn("xstatestore_executor_t::execute_block_recursive fail.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return nullptr;
    }

    XMETRICS_GAUGE(metrics::statestore_execute_block_recursive_succ, 1);
    xassert(nullptr != tablestate);
    xdbg("xstatestore_executor_t::execute_block_recursive succ by recursive from prev.limit=%d,cur_block=%s", limit,block->dump().c_str());
    return tablestate;
}

uint64_t xstatestore_executor_t::update_execute_from_execute_height(uint64_t old_execute_height) const {    
    uint64_t _highest_commit_block_height = m_statestore_base.get_blockstore()->get_latest_committed_block_height(m_table_addr.vaccount());

    if (old_execute_height >= _highest_commit_block_height) {
        return old_execute_height;
    }
    
    uint64_t max_count = execute_update_limit;
    uint64_t max_height = (old_execute_height + max_count) > _highest_commit_block_height ? _highest_commit_block_height : (old_execute_height + max_count);

    std::error_code ec;
    uint64_t new_execute_height = old_execute_height;

    for (uint64_t height=old_execute_height+1; height <= max_height; height++) {
        xobject_ptr_t<base::xvblock_t> cur_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), height, base::enum_xvblock_flag_committed, false);
        XMETRICS_GAUGE(metrics::statestore_load_table_block_succ, nullptr != cur_block ? 1 : 0);
        if (nullptr == cur_block) {            
            xwarn("xstatestore_executor_t::update_execute_from_execute_height fail-load committed block.account=%s,height=%ld", m_table_addr.to_string().c_str(), height);
            break;
        }

        uint32_t limit = 2;
        std::error_code ec;
        xtablestate_ext_ptr_t tablestate = execute_block_recursive(cur_block.get(), limit, ec);
        if (nullptr == tablestate) {
            xwarn("xstatestore_executor_t::update_execute_from_execute_height fail-execute block.account=%s,height=%ld", m_table_addr.to_string().c_str(), height);
            break;            
        }

        new_execute_height = height;
    }
    if (new_execute_height > old_execute_height) {
        xdbg("xstatestore_executor_t::update_execute_from_execute_height updated.account=%s,old_height=%ld,new_height=%ld",
             m_table_addr.to_string().c_str(),
             old_execute_height,
             new_execute_height);
    }
    return new_execute_height;
}

xtablestate_ext_ptr_t xstatestore_executor_t::make_state_from_current_table(base::xvblock_t* current_block, std::error_code & ec) const {
    xobject_ptr_t<base::xvbstate_t> current_state = nullptr;
    // try make state form block self
    if ( (current_block->get_height() != 0) && (current_block->get_block_class() != base::enum_xvblock_class_full) ) {
        // it is normal case
        return nullptr;
    }

    if (current_block->get_block_class() == base::enum_xvblock_class_full) {
        if (false == m_statestore_base.get_blockstore()->load_block_output(m_table_addr.vaccount(), current_block)) {
            ec = error::xerrc_t::statestore_db_read_abnormal_err;
            xerror("xstatestore_executor_t::make_state_from_current_table,fail-load block output for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
        if (current_block->get_full_state().empty()) {
            // it is normal case
            // ec = error::xerrc_t::statestore_load_unitblock_err;
            // xwarn("xstatestore_executor_t::make_state_from_current_table,fail-block has no full-state.block=%s", current_block->dump().c_str());
            return nullptr;
        }
    }

    current_state = make_object_ptr<base::xvbstate_t>(*current_block);
    if (current_block->get_block_class() != base::enum_xvblock_class_nil) {
        std::string binlog = current_block->get_block_class() == base::enum_xvblock_class_light ? current_block->get_binlog() : current_block->get_full_state();
        xassert(!binlog.empty());
        if(false == current_state->apply_changes_of_binlog(binlog)) {
            ec = error::xerrc_t::statestore_binlog_apply_err;
            xerror("xstatestore_executor_t::make_state_from_current_table fail-invalid binlog and abort it for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
    }
    
    data::xtablestate_ptr_t table_bstate = std::make_shared<data::xtable_bstate_t>(current_state.get());
    
    xhash256_t block_state_root = m_statestore_base.get_state_root_from_block(current_block);
    std::shared_ptr<state_mpt::xstate_mpt_t> cur_mpt = state_mpt::xstate_mpt_t::create(common::xaccount_address_t{current_block->get_account()}, block_state_root, m_statestore_base.get_dbstore(), ec);
    if (ec) {
        xwarn("xstatestore_executor_t::make_state_from_current_table fail-create mpt.block=%s", current_block->dump().c_str());
        return nullptr;
    }

    // write all table "state" to db
    std::vector<std::pair<data::xunitstate_ptr_t, std::string>> unitstate_units;
    xtablestate_store_ptr_t tablestate_store = std::make_shared<xtablestate_store_t>(table_bstate, cur_mpt, block_state_root, unitstate_units);
    xtablestate_ext_ptr_t tablestate = write_table_all_states(current_block, tablestate_store, ec);
    if (ec) {
        xerror("xstatestore_executor_t::make_state_from_current_table fail-write_table_all_states.block:%s", current_block->dump().c_str());
        return nullptr;            
    }

    xdbg("xstatestore_executor_t::make_state_from_current_table succ,block=%s",current_block->dump().c_str());
    // xtablestate_ext_ptr_t tablestate = std::make_shared<xtablestate_ext_t>(table_bstate, mpt);
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_executor_t::write_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const {
    if ((current_block->get_account() != tablestate_store->get_table_state()->account_address().to_string()) 
        || (current_block->get_height() != tablestate_store->get_table_state()->height())
        || current_block->get_viewid() != tablestate_store->get_table_state()->get_block_viewid() ) {
        ec = error::xerrc_t::statestore_block_invalid_err;
        xerror("xstatestore_executor_t::write_table_all_states fail-invalid block and state.block=%s,bstate=%s",current_block->dump().c_str(), tablestate_store->get_table_state()->get_bstate()->dump().c_str());
        return nullptr;
    }

    auto block_state_root = m_statestore_base.get_state_root_from_block(current_block);
    if (block_state_root != tablestate_store->get_state_root()) {
        ec = error::xerrc_t::statestore_block_invalid_err;
        xerror("xstatestore_executor_t::write_table_all_states fail-invalid state root.block=%s,state_root=%s:%s",current_block->dump().c_str(), block_state_root.as_hex_str().c_str(), tablestate_store->get_state_root().as_hex_str().c_str());
        return nullptr;        
    }

#ifdef DEBUG
    auto table_full_state_hash = current_block->get_fullstate_hash();
    if (!table_full_state_hash.empty()) { // XTODO empty block has no fullstate hash
        std::string bstate_snapshot_bin;
        tablestate_store->get_table_state()->get_bstate()->take_snapshot(bstate_snapshot_bin);
        auto table_bstate_hash = current_block->get_cert()->hash(bstate_snapshot_bin);
        if (table_full_state_hash != table_bstate_hash) {
            ec = error::xerrc_t::statestore_block_invalid_err;
            xerror("xstatestore_executor_t::write_table_all_states fail-invalid bstate hahs.block=%s,bstate_hash=%s:%s",current_block->dump().c_str(), top::to_hex(table_full_state_hash).c_str(), top::to_hex(table_bstate_hash).c_str());
            return nullptr;
        }
    }
#endif
   
    // write all table "state" to db
    for (auto & v : tablestate_store->get_unitstates()) {
        m_state_accessor.write_unitstate_to_db(v.first, v.second, ec);
        if (ec) {
            xerror("xstatestore_executor_t::write_table_all_states fail-write unitstate,block:%s", current_block->dump().c_str());
            return nullptr;
        }
        m_state_accessor.write_unitstate_to_cache(v.first, v.second);
        xdbg("xstatestore_executor_t::write_table_all_states unitstate=%s.block=%s", v.first->get_bstate()->dump().c_str(), current_block->dump().c_str());
    }

    m_state_accessor.write_table_bstate_to_db(m_table_addr, current_block->get_block_hash(), tablestate_store->get_table_state(), ec);
    if (ec) {
        xerror("xstatestore_executor_t::write_table_all_states fail-write tablestate,block:%s", current_block->dump().c_str());
        return nullptr;
    }
    xdbg("xstatestore_executor_t::write_table_all_states tablestate=%s.block=%s", tablestate_store->get_table_state()->get_bstate()->dump().c_str(), current_block->dump().c_str());
    
    if (current_block->get_block_class() != base::enum_xvblock_class_nil) {
        if (tablestate_store->get_state_root() != xhash256_t()) {
            tablestate_store->get_state_mpt()->commit(ec);
            if (ec) {
                xerror("xstatestore_executor_t::write_table_all_states fail-write mpt,block:%s.ec=%s", current_block->dump().c_str(),ec.message().c_str());
                return nullptr;
            }
            xdbg("xstatestore_executor_t::write_table_all_states mpt_root=%s.block=%s", tablestate_store->get_state_root().as_hex_str().c_str(), current_block->dump().c_str());
        }
    }
    
    std::shared_ptr<state_mpt::xstate_mpt_t> cur_mpt = state_mpt::xstate_mpt_t::create(common::xaccount_address_t{current_block->get_account()}, tablestate_store->get_state_root(), m_statestore_base.get_dbstore(), ec);
    if (ec) {
        xerror("xstatestore_executor_t::write_table_all_states fail-create mpt.block:%s", current_block->dump().c_str());
        return nullptr;            
    }
    xtablestate_ext_ptr_t tablestate = std::make_shared<xtablestate_ext_t>(tablestate_store->get_table_state(), cur_mpt);
    m_state_accessor.write_table_bstate_to_cache(m_table_addr, current_block->get_block_hash(), tablestate);
    if (current_block->check_block_flag(base::enum_xvblock_flag_committed)) {
        m_state_accessor.set_latest_connectted_tablestate(tablestate);
    }

    // update execute height
    update_latest_executed_info(current_block);
    xinfo("xstatestore_executor_t::write_table_all_states succ,block:%s,execute_height=%ld,unitstates=%zu,state_root=%s", 
        current_block->dump().c_str(), get_latest_executed_block_height(),tablestate_store->get_unitstates().size(),tablestate_store->get_state_root().as_hex_str().c_str());
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_executor_t::make_state_from_prev_state_and_table(base::xvblock_t* current_block, xtablestate_ext_ptr_t const& prev_state, std::error_code & ec) const {
    class alocker
    {
    private:
        alocker();
        alocker(const alocker &);
        alocker & operator = (const alocker &);
    public:
        alocker(std::mutex & globl_locker,bool auto_lock)
            :m_ref_mutex(globl_locker)
        {
            m_auto_lock = auto_lock;
            if(m_auto_lock)
                m_ref_mutex.lock();
        }

        ~alocker()
        {
            if(m_auto_lock) {
                m_ref_mutex.unlock();
            }
        }

    private:
        std::mutex & m_ref_mutex;
        bool         m_auto_lock;
    };


    if (current_block->get_height() != prev_state->get_table_state()->height() + 1) {
        ec = error::xerrc_t::statestore_block_unmatch_prev_err;
        xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-block and state unmatch.block=%s,state=%s",current_block->dump().c_str(),prev_state->get_table_state()->get_bstate()->dump().c_str());
        return nullptr;
    }

    if (get_need_sync_state_block_height() != 0 
        && current_block->check_block_flag(base::enum_xvblock_flag_committed)
        && get_need_sync_state_block_height() == current_block->check_block_flag(base::enum_xvblock_flag_committed)) {
        ec = error::xerrc_t::statestore_need_state_sync_fail;
        xwarn("xstatestore_executor_t::make_state_from_prev_state_and_table fail-need all state sync.block=%s,height=%ld",current_block->dump().c_str(),m_need_all_state_sync_height);
        return nullptr;
    }

    // should clone a new state for execute
    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*current_block, *prev_state->get_table_state()->get_bstate());
    std::shared_ptr<state_mpt::xstate_mpt_t> current_prev_mpt = state_mpt::xstate_mpt_t::create(m_table_addr, prev_state->get_state_mpt()->get_original_root_hash(), m_statestore_base.get_dbstore(), ec);        
    xhash256_t block_state_root = m_statestore_base.get_state_root_from_block(current_block);
    base::xaccount_indexs_t account_indexs;
    bool is_first_mpt = false;
    if (current_block->get_height() > 1
        && current_prev_mpt->get_original_root_hash() == xhash256_t{}
        && block_state_root != xhash256_t{}) {
        is_first_mpt = true;
    }
    alocker global_lock(m_global_execute_lock, is_first_mpt); // XTODO first mpt will use too much memory, so add global lock

    if (current_block->get_block_class() == base::enum_xvblock_class_light) {
        if (false == m_statestore_base.get_blockstore()->load_block_output(m_table_addr.vaccount(), current_block)) {
            ec = error::xerrc_t::statestore_load_tableblock_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-load output for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
        std::string binlog = current_block->get_binlog();
        if (binlog.empty()) {
            ec = error::xerrc_t::statestore_block_invalid_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-binlog empty for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
        if(false == current_state->apply_changes_of_binlog(binlog)) {
            ec = error::xerrc_t::statestore_block_invalid_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-invalid binlog apply for block(%s)",current_block->dump().c_str());
            return nullptr;
        }

        // upgrade for first mpt build
        if (is_first_mpt) {            
            data::xtablestate_ptr_t cur_table_bstate = std::make_shared<data::xtable_bstate_t>(current_state.get());
            std::map<std::string, std::string> indexes = cur_table_bstate->map_get(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
            xinfo("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt begin.indexes_count=%zu.block=%s",indexes.size(), current_block->dump().c_str());
            for (auto & v : indexes) {
                common::xaccount_address_t account{v.first};
                auto & account_index_str = v.second;              
                current_prev_mpt->set_account_index(account, account_index_str, ec);
                if (ec) {
                    xerror("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt fail-set mpt accountindex for block(%s)",current_block->dump().c_str());
                    return nullptr;
                }

                base::xaccount_index_t accountindex;
                accountindex.serialize_from(account_index_str);
                data::xunitstate_ptr_t unitstate = nullptr;
                build_unitstate_by_accountindex(account, accountindex, unitstate, ec);
                if (nullptr == unitstate) {
                    if (current_block->check_block_flag(base::enum_xvblock_flag_committed)) {
                        set_need_sync_state_block_height(current_block->get_height());
                    } 
                    ec = error::xerrc_t::statestore_need_state_sync_fail;
                    xwarn("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt fail-make unitstate.need do state sync for table block(%s),account=%s,%s",
                          current_block->dump().c_str(),
                          account.to_string().c_str(),
                          accountindex.dump().c_str());
                    return nullptr;
                }
                m_statestore_base.get_blockstore()->clean_caches(account.vaccount()); // XTODO clean imediately for too much block cache for fist mpt build
                m_state_accessor.write_unitstate_to_db(unitstate, accountindex.get_latest_unit_hash(), ec);
                if (ec) {
                    xerror("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt fail-write unitstate for block(%s),account=%s,%s",
                           current_block->dump().c_str(),
                           account.to_string().c_str(),
                           accountindex.dump().c_str());
                    return nullptr;
                }
                xinfo("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt write unitstate.block(%s),account=%s,%s",
                      current_block->dump().c_str(),
                      account.to_string().c_str(),
                      accountindex.dump().c_str());
            }
            xinfo("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt finish.indexes_count=%zu.block=%s",indexes.size(), current_block->dump().c_str());
        }

        // set changed accountindexs
        auto account_indexs_str = current_block->get_account_indexs();
        if (!account_indexs_str.empty()) {            
            account_indexs.serialize_from_string(account_indexs_str);
            for (auto & index : account_indexs.get_account_indexs()) {
                current_prev_mpt->set_account_index(common::xaccount_address_t{index.first}, index.second, ec);
                if (ec) {
                    xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-set mpt account index.block:%s", current_block->dump().c_str());
                    return nullptr;
                }
            }

            // check if root matches.            
            auto cur_root_hash = current_prev_mpt->get_root_hash(ec);
            if (cur_root_hash != block_state_root) {
                ec = error::xerrc_t::statestore_block_root_unmatch_mpt_root_err;
                xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-root not match cur_root_hash:%s,state_root_hash:%s,block:%s",
                    cur_root_hash.as_hex_str().c_str(),
                    block_state_root.as_hex_str().c_str(),
                    current_block->dump().c_str());
                return nullptr;
            }
        }
    }

    std::vector<std::pair<data::xunitstate_ptr_t, std::string>> unitstate_units;
    if (true == base::xvchain_t::instance().has_other_node() && account_indexs.get_account_indexs().size() > 0) {
        if (false == m_statestore_base.get_blockstore()->load_block_input(m_table_addr.vaccount(), current_block)) {
            ec = error::xerrc_t::statestore_load_tableblock_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-load input for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
        if (false == m_statestore_base.get_blockstore()->load_block_output_offdata(m_table_addr.vaccount(), current_block)) {
            ec = error::xerrc_t::statestore_load_tableblock_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-load output offdata for block(%s)",current_block->dump().c_str());
            return nullptr;
        }

        std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;  
        if (false == current_block->extract_sub_blocks(sub_blocks)) {
            ec = error::xerrc_t::statestore_block_invalid_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table,fail-extract_sub_blocks for table block(%s)", current_block->dump().c_str());   
            return nullptr;
        }
        if (account_indexs.get_account_indexs().size() != sub_blocks.size()) {
            ec = error::xerrc_t::statestore_block_invalid_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_table,fail-units count unmatch for table block(%s)", current_block->dump().c_str());   
            return nullptr;
        }

        if (!sub_blocks.empty()) {            
            for (uint32_t i=0;i<sub_blocks.size();i++) {
                auto & unit = sub_blocks[i];
                uint32_t limit = 2;  // XTODO new execution should always successfully
                data::xunitstate_ptr_t unitstate = execute_unit_recursive(common::xaccount_address_t(unit->get_account()), unit.get(), limit, ec);
                if (nullptr == unitstate) {
                    xerror("xstatestore_executor_t::make_state_from_prev_state_and_table,fail-make unitstate for table block(%s),unit=%s", current_block->dump().c_str(),unit->dump().c_str());
                    return nullptr;
                }
#ifdef DEBUG
                std::string unitstate_bin = unitstate->take_snapshot();
                std::string unitstate_hash = current_block->get_cert()->hash(unitstate_bin);
                if (unitstate_hash != account_indexs.get_account_indexs()[i].second.get_latest_state_hash()) {
                    ec = error::xerrc_t::statestore_tablestate_exec_fail;
                    xerror("xstatestore_executor_t::make_state_from_prev_state_and_table,fail-unitstate unmatch hash for table block(%s),unit=%s", current_block->dump().c_str(),unit->dump().c_str());
                    return nullptr;                    
                }
#endif
                unitstate_units.push_back(std::make_pair(unitstate, unit->get_block_hash()));
            }
        }
    }

    // write all table "state" to db
    data::xtablestate_ptr_t table_bstate = std::make_shared<data::xtable_bstate_t>(current_state.get());
    xtablestate_store_ptr_t tablestate_store = std::make_shared<xtablestate_store_t>(table_bstate, current_prev_mpt, block_state_root, unitstate_units);
    xtablestate_ext_ptr_t tablestate = write_table_all_states(current_block, tablestate_store, ec);
    if (ec) {
        xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-write_table_all_states.block:%s", current_block->dump().c_str());
        return nullptr;            
    }

    xinfo("xstatestore_executor_t::make_state_from_prev_state_and_table succ,block=%s,has_other_node=%d",current_block->dump().c_str(),base::xvchain_t::instance().has_other_node());
    return tablestate;
}

void xstatestore_executor_t::set_latest_executed_info(uint64_t height,const std::string & blockhash) const {
    std::lock_guard<std::mutex> l(m_execute_height_lock);
    if (m_executed_height < height) {                
        xinfo("xstatestore_executor_t::set_latest_executed_info succ,account=%s,old=%ld,new=%ld,need_height=%ld,this=%p",
              m_table_addr.to_string().c_str(),
              m_executed_height,
              height,
              m_need_all_state_sync_height,
              this);
        m_executed_height = height;
        m_statestore_base.set_latest_executed_info(m_table_addr, height, blockhash);

        if (m_need_all_state_sync_height != 0 && m_executed_height > m_need_all_state_sync_height) {
            m_need_all_state_sync_height = 0;
        }

        if (m_execute_listener != nullptr) {
            m_execute_listener->on_executed(height);
        }
    }
}

void xstatestore_executor_t::set_need_sync_state_block_height(uint64_t height) const {
    std::lock_guard<std::mutex> l(m_execute_height_lock);
    xinfo("xstatestore_executor_t::set_need_sync_state_block_height succ,account=%s,old=%ld,new=%ld", m_table_addr.to_string().c_str(), m_need_all_state_sync_height, height);
    m_need_all_state_sync_height = height;
}

uint64_t xstatestore_executor_t::get_latest_executed_block_height() const {
    std::lock_guard<std::mutex> l(m_execute_height_lock);
    // xinfo("xstatestore_executor_t::get_latest_executed_block_height,account=%s,%ld,this=%p",m_table_addr.value().c_str(),m_executed_height,this);
    return m_executed_height;
}
uint64_t xstatestore_executor_t::get_need_sync_state_block_height() const {
    std::lock_guard<std::mutex> l(m_execute_height_lock);
    return m_need_all_state_sync_height;
}
void xstatestore_executor_t::raise_execute_height(const xstate_sync_info_t & sync_info) {
    std::lock_guard<std::mutex> l(m_execute_lock);
    // check if root and table state are already stored.
    xobject_ptr_t<base::xvblock_t> block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), sync_info.get_height(), sync_info.get_blockhash(), false);
    if (block == nullptr) {
        xerror("xstatestore_executor_t::raise_execute_height fail-load block. table=%s,height=%ld,hash=%s",
               m_table_addr.to_string().c_str(),
               sync_info.get_height(),
               base::xstring_utl::to_hex(sync_info.get_blockhash()).c_str());
        return;
    }

    auto tablestate = m_state_accessor.read_table_bstate_from_db(m_table_addr, block.get());
    if (nullptr == tablestate) {
        xerror("xstatestore_executor_t::raise_execute_height fail-read state. table=%s,height=%ld,hash=%s",
               m_table_addr.to_string().c_str(),
               sync_info.get_height(),
               base::xstring_utl::to_hex(sync_info.get_blockhash()).c_str());
        return;
    }

    if (!sync_info.get_root_hash().empty()) {
        std::error_code ec;
        std::shared_ptr<state_mpt::xstate_mpt_t> cur_mpt = state_mpt::xstate_mpt_t::create(m_table_addr, sync_info.get_root_hash(), m_statestore_base.get_dbstore(), ec);
        if (ec) {
            xerror("xstatestore_executor_t::raise_execute_height fail-create mpt.table=%s,height=%ld,hash=%s,root=%s",
                   m_table_addr.to_string().c_str(),
                   sync_info.get_height(),
                   base::xstring_utl::to_hex(sync_info.get_blockhash()).c_str(),
                   sync_info.get_root_hash().as_hex_str().c_str());
            return;
        }
    }

    xinfo("xstatestore_executor_t::raise_execute_height succ.table=%s,height=%ld,hash=%s,root:%s",
          m_table_addr.to_string().c_str(),
          sync_info.get_height(),
          base::xstring_utl::to_hex(sync_info.get_blockhash()).c_str(),
          sync_info.get_root_hash().as_hex_str().c_str());
    set_latest_executed_info(sync_info.get_height(), sync_info.get_blockhash());
}


// =============unit executor=================
void  xstatestore_executor_t::build_unitstate_by_unit(common::xaccount_address_t const& unit_addr, base::xvblock_t* unit, data::xunitstate_ptr_t &unitstate, std::error_code & ec) const {
    // firstly, try get from cache and db
    unitstate = m_state_accessor.read_unit_bstate(unit_addr, unit->get_height(), unit->get_block_hash());
    if (nullptr != unitstate) {
        xdbg("xstatestore_executor_t::build_unitstate_by_unit get from cache.block=%s", unit->dump().c_str());
        return;
    }

    // secondly, try make state from current block
    uint32_t limit = execute_unit_limit_demand;  // XTODO
    unitstate = execute_unit_recursive(unit_addr, unit, limit, ec);
    if (nullptr != unitstate) {
        xdbg("xstatestore_executor_t::build_unitstate_by_unit get from current block.block=%s", unit->dump().c_str());
        return;
    }
    xwarn("xstatestore_executor_t::build_unitstate_by_unit fail.ec=%s,block=%s",ec.message().c_str(), unit->dump().c_str());
}

void xstatestore_executor_t::build_unitstate_by_accountindex(common::xaccount_address_t const& unit_addr, base::xaccount_index_t const& account_index, data::xunitstate_ptr_t &unitstate, std::error_code & ec) const {
    // firstly, try get from cache and db
    unitstate = m_state_accessor.read_unit_bstate(unit_addr, account_index.get_latest_unit_height(), account_index.get_latest_unit_hash());
    if (nullptr != unitstate) {
        xdbg("xstatestore_executor_t::build_unitstate_by_accountindex get from cache account=%s,accountindex=%s", unit_addr.to_string().c_str(), account_index.dump().c_str());
        return;
    }
    xobject_ptr_t<base::xvblock_t> _unit = nullptr;
    if (account_index.get_latest_unit_hash().empty()) {
        _unit = m_statestore_base.get_blockstore()->load_block_object(unit_addr.vaccount(), account_index.get_latest_unit_height(), account_index.get_latest_unit_viewid(), false);
    } else {
        _unit = m_statestore_base.get_blockstore()->load_block_object(unit_addr.vaccount(), account_index.get_latest_unit_height(), account_index.get_latest_unit_hash(), false);
    }
    if (nullptr == _unit) {
        ec = error::xerrc_t::statestore_load_unitblock_err;
        xwarn("xstatestore_executor_t::build_unitstate_by_accountindex fail-load unit account=%s,accountindex=%s", unit_addr.to_string().c_str(), account_index.dump().c_str());
        return;
    }

    // secondly, try make state from current block
    uint32_t limit = execute_unit_limit_demand;  // XTODO
    unitstate = execute_unit_recursive(unit_addr, _unit.get(), limit, ec);
    if (nullptr != unitstate) {
        xdbg("xstatestore_executor_t::build_unitstate_by_accountindex succ. get from current block.account=%s,accountindex=%s",
             unit_addr.to_string().c_str(),
             account_index.dump().c_str());
        return;
    }
    xwarn("xstatestore_executor_t::build_unitstate_by_accountindex fail.ec=%s,account=%s,accountindex=%s",
          ec.message().c_str(),
          unit_addr.to_string().c_str(),
          account_index.dump().c_str());
}

data::xunitstate_ptr_t xstatestore_executor_t::make_state_from_current_unit(common::xaccount_address_t const& unit_addr, base::xvblock_t * current_block, std::error_code & ec) const {
    xobject_ptr_t<base::xvbstate_t> current_state = nullptr;
    // try make state form block self
    if ( (current_block->get_height() != 0) && (current_block->get_block_class() != base::enum_xvblock_class_full) ) {
        // it is normal case
        return nullptr;
    }

    current_state = make_object_ptr<base::xvbstate_t>(*current_block);
    if (current_block->get_block_class() != base::enum_xvblock_class_nil) {
        if (false == m_statestore_base.get_blockstore()->load_block_output(unit_addr.vaccount(), current_block)) {
            ec = error::xerrc_t::statestore_db_read_abnormal_err;
            xerror("xstatestore_executor_t::make_state_from_current_unit,fail-load block output for block(%s)",current_block->dump().c_str());
            return nullptr;
        }

        std::string binlog = current_block->get_block_class() == base::enum_xvblock_class_light ? current_block->get_binlog() : current_block->get_full_state();
        xassert(!binlog.empty());
        if(false == current_state->apply_changes_of_binlog(binlog)) {
            ec = error::xerrc_t::statestore_binlog_apply_err;
            xerror("xstatestore_executor_t::make_state_from_current_unit,invalid binlog and abort it for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
    }
    xdbg("xstatestore_executor_t::make_state_from_current_unit succ,block=%s",current_block->dump().c_str());
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(current_state.get());
    return unitstate;
}

data::xunitstate_ptr_t xstatestore_executor_t::make_state_from_prev_state_and_unit(common::xaccount_address_t const& unit_addr, base::xvblock_t * current_block, data::xunitstate_ptr_t const& prev_bstate, std::error_code & ec) const {
    if (current_block->get_height() != prev_bstate->height() + 1) {
        ec = error::xerrc_t::statestore_block_unmatch_prev_err;
        xerror("xstatestore_executor_t::make_state_from_prev_state_and_unit fail-block and state unmatch.block=%s,state=%s",current_block->dump().c_str(),prev_bstate->get_bstate()->dump().c_str());
        return nullptr;
    }

    if (current_block->get_block_class() == base::enum_xvblock_class_full) {
        ec = error::xerrc_t::statestore_block_unmatch_prev_err;
        xerror("xstatestore_executor_t::make_state_from_prev_state_and_unit fail-should not be full.block=%s,state=%s",current_block->dump().c_str(),prev_bstate->get_bstate()->dump().c_str());
        return nullptr;
    }

    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*current_block, *prev_bstate->get_bstate());
    if (current_block->get_block_class() == base::enum_xvblock_class_light) {
        if (false == m_statestore_base.get_blockstore()->load_block_output(unit_addr.vaccount(), current_block)) {
            ec = error::xerrc_t::statestore_db_read_abnormal_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_unit fail-load output for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
        std::string binlog = current_block->get_binlog();
        if (binlog.empty()) {
            ec = error::xerrc_t::statestore_db_read_abnormal_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_unit fail-binlog empty for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
        if(false == current_state->apply_changes_of_binlog(binlog)) {
            ec = error::xerrc_t::statestore_binlog_apply_err;
            xerror("xstatestore_executor_t::make_state_from_prev_state_and_unit fail-invalid binlog apply for block(%s)",current_block->dump().c_str());
            return nullptr;
        }
    }
    xdbg("xstatestore_executor_t::make_state_from_prev_state_and_unit succ,block=%s",current_block->dump().c_str());
    data::xunitstate_ptr_t unitstate = std::make_shared<data::xunit_bstate_t>(current_state.get());
    return unitstate;
}

data::xunitstate_ptr_t xstatestore_executor_t::execute_unit_recursive(common::xaccount_address_t const& unit_addr, base::xvblock_t* block, uint32_t & limit, std::error_code & ec) const {
    xassert(!ec);
    xdbg("xstatestore_executor_t::execute_unit_recursive enter.block=%s,limit=%d",block->dump().c_str(),limit);
    if (limit == 0) {
        XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 0);
        ec = error::xerrc_t::statestore_try_limit_arrive_err;
        xwarn("xstatestore_executor_t::execute_unit_recursive fail-limit to zero.block=%s", block->dump().c_str());
        return nullptr;
    }
    limit--;

    // 1.try make state from current unit
    data::xunitstate_ptr_t unitstate = make_state_from_current_unit(unit_addr, block, ec);
    if (ec) {
        XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 0);
        xwarn("xstatestore_executor_t::execute_unit_recursive fail-make_state_from_current_unit.limit=%d,block=%s", limit,block->dump().c_str());
        return nullptr;
    }
    if (nullptr != unitstate) {
        XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 1);
        xdbg("xstatestore_executor_t::execute_unit_recursive succ by make from current.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return unitstate;
    }

    // 2.try load prev-state from cache or db
    data::xunitstate_ptr_t prev_unitstate = nullptr;
    if (block->get_height() > 1) {
        prev_unitstate = m_state_accessor.read_unit_bstate(unit_addr, block->get_height() - 1, block->get_last_block_hash());
        if (nullptr == prev_unitstate) {
            xwarn("xstatestore_executor_t::execute_unit_recursive fail-read prev state from db.limit=%d,cur_block=%s", limit,block->dump().c_str());
        }
    }
    if (nullptr == prev_unitstate) {
        // try load prev-state by recursive
        xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(unit_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
        if (nullptr == prev_block) {
            XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 0);
            ec = error::xerrc_t::statestore_load_unitblock_err;
            xwarn("xstatestore_executor_t::execute_unit_recursive fail-load prev block.limit=%d,cur_block=%s", limit,block->dump().c_str());
            return nullptr;     
        }
        prev_unitstate = execute_unit_recursive(unit_addr, prev_block.get(), limit, ec);
        if (nullptr == prev_unitstate) {
            XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 0);
            xwarn("xstatestore_executor_t::execute_unit_recursive fail-execute prev block.limit=%d,cur_block=%s", limit,block->dump().c_str());
            return nullptr;
        }
    }

    unitstate = make_state_from_prev_state_and_unit(unit_addr, block, prev_unitstate, ec);
    if (ec) {
        XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 0);
        xerror("xstatestore_executor_t::execute_unit_recursive fail.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return nullptr;
    }

    XMETRICS_GAUGE(metrics::statestore_execute_unit_recursive_succ, 1);
    xassert(nullptr != unitstate);
    xdbg("xstatestore_executor_t::execute_unit_recursive succ by recursive from prev.limit=%d,cur_block=%s", limit,block->dump().c_str());
    return unitstate;
}


NS_END2
