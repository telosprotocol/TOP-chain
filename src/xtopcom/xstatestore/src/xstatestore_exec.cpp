// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xblockbuild.h"
#include "xmbus/xevent_behind.h"
#include "xstatestore/xstatestore_exec.h"
#include "xstatestore/xerror.h"
#include "xvledger/xvledger.h"

NS_BEG2(top, statestore)

xstatestore_executor_t::xstatestore_executor_t(common::xaccount_address_t const& table_addr)
: m_table_addr{table_addr},m_state_accessor{table_addr} {
     m_executed_height = m_statestore_base.get_latest_executed_block_height(table_addr);
     xdbg("xstatestore_executor_t::xstatestore_executor_t execute_height=%ld", m_executed_height);
}

void xstatestore_executor_t::on_table_block_committed(base::xvblock_t* block) const {
    if (false == block->check_block_flag(base::enum_xvblock_flag_committed)) {
        xassert(false);
        return;
    }

    std::error_code ec;
    uint64_t old_execute_height = get_latest_executed_block_height();
    if (old_execute_height >= block->get_height()) {
        xdbg("xstatestore_executor_t::on_table_block_committed finish-already done.execute_height old=%ld,block=%ld", old_execute_height, block->get_height());
        return;
    }

    xtablestate_ext_ptr_t cache_tablestate = m_state_accessor.read_table_bstate(m_table_addr, block);
    if (nullptr != cache_tablestate) {
        xdbg("xstatestore_executor_t::on_table_block_committed succ-already executed.block=%s",block->dump().c_str());
        return;
    }

    if (old_execute_height + 1 == block->get_height()) {
        xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
        if (nullptr == prev_block) {
            xwarn("xstatestore_executor_t::on_table_block_committed fail-load prev block.cur_block=%s", block->dump().c_str());
            return;
        }
        xtablestate_ext_ptr_t prev_tablestate = m_state_accessor.read_table_bstate(m_table_addr, prev_block.get());
        if (nullptr == prev_tablestate) {
            xassert(prev_block->get_height() == 0);
            prev_tablestate = make_state_from_current_table(prev_block.get(), ec);
            if (nullptr == prev_tablestate) {
                xerror("xstatestore_executor_t::on_table_block_committed fail-load prev state block.account=%s,height=%ld", m_table_addr.value().c_str(), old_execute_height);
                return;
            }
        }
        make_state_from_prev_state_and_table(block, prev_tablestate, ec);
    } else {
        update_execute_from_execute_height();
    }
    xdbg("xstatestore_executor_t::on_table_block_committed finish.execute_height old=%ld,new=%ld", old_execute_height, get_latest_executed_block_height());
}

void xstatestore_executor_t::try_execute_block_on_demand(base::xvblock_t* block, std::error_code & ec) const {
    uint64_t execute_height = get_latest_executed_block_height();
    uint32_t limit = execute_demand_limit;
    if (block->get_height() > execute_height + limit) {
        // update from execute height firstly
        update_execute_from_execute_height();
        execute_height = get_latest_executed_block_height();
        if (block->get_height() > execute_height + limit) {
            ec = error::xerrc_t::statestore_cannot_execute_for_long_distance_err;
            xwarn("xstatestore_executor_t::try_execute_block_on_demand fail-can't execute for long distance.block=%s,execute_height=%ld", 
                block->dump().c_str(), execute_height, execute_height);
            return;
        }
    }
}

void xstatestore_executor_t::execute_and_get_tablestate_ext(base::xvblock_t* block, xtablestate_ext_ptr_t & tablestate_ext, std::error_code & ec) const {
    // firstly, try get from cache
    tablestate_ext = m_state_accessor.read_table_bstate_from_cache(m_table_addr, block->get_height(), block->get_block_hash());
    if (nullptr != tablestate_ext) {
        xdbg("xstatestore_executor_t::execute_and_get_tablestate_ext succ get from cache.block=%s", block->dump().c_str());
        return;
    }
    
    // secondly, try to push block execute from execute height
    try_execute_block_on_demand(block, ec);
    if (ec) {
        xwarn("xstatestore_executor_t::execute_and_get_tablestate_ext fail-try_execute_block_on_demand.ec=%s,block=%s", ec.message().c_str(), block->dump().c_str());
        return;
    }
    tablestate_ext = m_state_accessor.read_table_bstate_from_db(m_table_addr, block);
    if (nullptr != tablestate_ext) {
        xdbg("xstatestore_executor_t::execute_and_get_tablestate_ext succ get from db.block=%s", block->dump().c_str());
        return;
    }

    uint32_t limit = execute_demand_limit;
    tablestate_ext = execute_block_recursive(block, limit, ec);
    if (ec) {
        xwarn("xstatestore_executor_t::execute_and_get_tablestate_ext fail-execute recursive.block=%s,ec=%s", block->dump().c_str(), ec.message().c_str());
        return;
    }
    xassert(nullptr != tablestate_ext);
    xdbg("xstatestore_executor_t::execute_and_get_tablestate_ext succ get from execute recursive.block=%s", block->dump().c_str());
}

void xstatestore_executor_t::execute_and_get_accountindex(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const {
    xtablestate_ext_ptr_t tablestate_ext = nullptr;
    execute_and_get_tablestate_ext(block, tablestate_ext, ec);
    if (nullptr != tablestate_ext) {
        tablestate_ext->get_accountindex(unit_addr.value(), account_index, ec);
    }
}

void xstatestore_executor_t::execute_and_get_tablestate(base::xvblock_t* block, data::xtablestate_ptr_t &tablestate, std::error_code & ec) const {
    xtablestate_ext_ptr_t tablestate_ext = nullptr;
    execute_and_get_tablestate_ext(block, tablestate_ext, ec);
    if (nullptr != tablestate_ext) {
        tablestate = tablestate_ext->get_table_state();
    }
}

xtablestate_ext_ptr_t xstatestore_executor_t::execute_block_recursive(base::xvblock_t* block, uint32_t & limit, std::error_code & ec) const {
    xassert(!ec);
    xdbg("xstatestore_executor_t::execute_block_recursive enter.block=%s,limit=%d",block->dump().c_str(),limit);
    if (limit == 0) {
        ec = error::xerrc_t::statestore_try_limit_arrive_err;
        xwarn("xstatestore_executor_t::execute_block_recursive fail-limit to zero.block=%s", block->dump().c_str());
        return nullptr;
    }
    limit--;

    // 1.try make state from current table  TODO(jimmy)
    xtablestate_ext_ptr_t tablestate = make_state_from_current_table(block, ec);
    if (ec) {
        xwarn("xstatestore_executor_t::execute_block_recursive fail-make_state_from_current_table.limit=%d,block=%s", limit,block->dump().c_str());
        return nullptr;
    }
    if (nullptr != tablestate) {
        xdbg("xstatestore_executor_t::execute_block_recursive succ by make from current.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return tablestate;
    }

    // 2.try load prev-state from cache or db      should load block first for get state-root
    xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
    if (nullptr == prev_block) {
        ec = error::xerrc_t::statestore_load_tableblock_err;
        xwarn("xstatestore_executor_t::execute_block_recursive fail-load prev block.cur_block=%s", block->dump().c_str());
        return nullptr;     
    }

    xtablestate_ext_ptr_t prev_tablestate = m_state_accessor.read_table_bstate(m_table_addr, prev_block.get());
    if (nullptr == prev_tablestate) {
        xwarn("xstatestore_executor_t::execute_block_recursive fail-read prev state from db.limit=%d,cur_block=%s", limit,block->dump().c_str());

        prev_tablestate = execute_block_recursive(prev_block.get(), limit, ec);
        if (nullptr == prev_tablestate) {
            xwarn("xstatestore_executor_t::execute_block_recursive fail-execute prev block.limit=%d,cur_block=%s", limit,block->dump().c_str());
            return nullptr;
        }
    }

    tablestate = make_state_from_prev_state_and_table(block, prev_tablestate, ec);
    if (ec) {
        xerror("xstatestore_executor_t::execute_block_recursive fail.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return nullptr;
    }

    xassert(nullptr != tablestate);
    xdbg("xstatestore_executor_t::execute_block_recursive succ by recursive from prev.limit=%d,cur_block=%s", limit,block->dump().c_str());
    return tablestate;
}

void xstatestore_executor_t::update_execute_from_execute_height() const {
    uint64_t max_count = execute_update_limit;

    uint64_t old_execute_height = get_latest_executed_block_height();
    uint64_t _highest_commit_block_height = m_statestore_base.get_blockstore()->get_latest_committed_block_height(m_table_addr.vaccount());

    if (old_execute_height >= _highest_commit_block_height) {
        return;
    }
    uint64_t max_height = (old_execute_height + max_count) > _highest_commit_block_height ? _highest_commit_block_height : (old_execute_height + max_count);

    std::error_code ec;
    uint64_t new_execute_height = old_execute_height;

    xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), old_execute_height, base::enum_xvblock_flag_committed, false);
    if (nullptr == prev_block) {
        xwarn("xstatestore_executor_t::update_execute_from_execute_height fail-load old execute block.account=%s,height=%ld", m_table_addr.value().c_str(), old_execute_height);
        return;     
    }

    xtablestate_ext_ptr_t prev_tablestate = m_state_accessor.read_table_bstate(m_table_addr, prev_block.get());
    if (nullptr == prev_tablestate) {
        xassert(prev_block->get_height() == 0);
        prev_tablestate = make_state_from_current_table(prev_block.get(), ec);
        if (nullptr == prev_tablestate) {
            xerror("xstatestore_executor_t::update_execute_from_execute_height fail-load prev state block.account=%s,height=%ld", m_table_addr.value().c_str(), old_execute_height);
            return;
        }
    }

    for (uint64_t height=old_execute_height+1; height <= max_height; height++) {
        xobject_ptr_t<base::xvblock_t> cur_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), height, base::enum_xvblock_flag_committed, false);
        if (nullptr == cur_block) {
            xwarn("xstatestore_executor_t::update_execute_from_execute_height fail-load committed block.account=%s,height=%ld", m_table_addr.value().c_str(), height);
            break;
        }
        xtablestate_ext_ptr_t tablestate = make_state_from_prev_state_and_table(cur_block.get(), prev_tablestate, ec);
        if (ec) {
            xerror("xstatestore_executor_t::execute_block_recursive fail.cur_block=%s", cur_block->dump().c_str());
            break;
        }

        prev_tablestate = tablestate;
        new_execute_height = height;
    }
    if (new_execute_height > old_execute_height) {
        xinfo("xstatestore_executor_t::update_execute_from_execute_height updated.account=%s,old_height=%ld,new_height=%ld",m_table_addr.value().c_str(),old_execute_height,new_execute_height);
    }
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
            ec = error::xerrc_t::statestore_load_unitblock_err;
            xwarn("xstatestore_executor_t::make_state_from_current_table,fail-block has no full-state.block=%s", current_block->dump().c_str());
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
    
    xhash256_t stateroot = m_statestore_base.get_state_root_from_block(current_block);
    std::shared_ptr<state_mpt::xtop_state_mpt> mpt = state_mpt::xtop_state_mpt::create(common::xaccount_address_t{current_block->get_account()}, stateroot, m_statestore_base.get_dbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
    if (ec) {
        xwarn("xstatestore_executor_t::make_state_from_current_table fail-create mpt.block=%s", current_block->dump().c_str());
        return nullptr;
    }

    xdbg("xstatestore_executor_t::make_state_from_current_table succ,block=%s",current_block->dump().c_str());
    xtablestate_ext_ptr_t tablestate = std::make_shared<xtablestate_ext_t>(table_bstate, mpt);
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_executor_t::write_table_all_states(base::xvblock_t* current_block, xtablestate_store_ptr_t const& tablestate_store, std::error_code & ec) const {
    std::lock_guard<std::mutex> l(m_table_state_write_lock);
    xtablestate_ext_ptr_t cache_tablestate = m_state_accessor.read_table_bstate(m_table_addr, current_block);
    if (nullptr != cache_tablestate) {
        xinfo("xstatestore_executor_t::write_table_all_states succ-already writed.block=%s",current_block->dump().c_str());
        return cache_tablestate;
    }
    
    // write all table "state" to db
    for (auto & v : tablestate_store->get_unitstates()) {
        m_state_accessor.write_unitstate_to_db(v.first, v.second, ec);
        if (ec) {
            xerror("xstatestore_executor_t::write_table_all_states fail-write unitstate,block:%s", current_block->dump().c_str());
            return nullptr;
        }
        m_state_accessor.write_unitstate_to_cache(v.first, v.second);
    }

    m_state_accessor.write_table_bstate_to_db(m_table_addr, current_block->get_block_hash(), tablestate_store->get_table_state(), ec);
    if (ec) {
        xerror("xstatestore_executor_t::write_table_all_states fail-write tablestate,block:%s", current_block->dump().c_str());
        return nullptr;
    }
    
    if (current_block->get_block_class() != base::enum_xvblock_class_nil) {
        if (tablestate_store->get_state_root() != xhash256_t()) {
            tablestate_store->get_state_mpt()->commit(ec);
            if (ec) {
                xerror("xstatestore_executor_t::write_table_all_states fail-write mpt,block:%s.ec=%s", current_block->dump().c_str(),ec.message().c_str());
                return nullptr;     
            }
        }
    }
    
    std::shared_ptr<state_mpt::xtop_state_mpt> cur_mpt = state_mpt::xtop_state_mpt::create(common::xaccount_address_t{current_block->get_account()}, tablestate_store->get_state_root(), m_statestore_base.get_dbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
    if (ec) {
        xerror("xstatestore_executor_t::write_table_all_states fail-create mpt.block:%s", current_block->dump().c_str());
        return nullptr;            
    }
    xtablestate_ext_ptr_t tablestate = std::make_shared<xtablestate_ext_t>(tablestate_store->get_table_state(), cur_mpt);
    m_state_accessor.write_table_bstate_to_cache(m_table_addr, current_block->get_block_hash(), tablestate);

    // update execute height
    if (current_block->check_block_flag(base::enum_xvblock_flag_committed)) {
        set_latest_executed_info(current_block->get_height(), current_block->get_block_hash());
    } else {
        if (current_block->get_height() > 2) {
            set_latest_executed_info(current_block->get_height()-2, std::string());  // TODO(jimmy) execute hash not need
        }
    }
    xinfo("xstatestore_executor_t::write_table_all_states succ,block:%s,execute_height=%ld,unitstates=%zu,state_root=%s", 
        current_block->dump().c_str(), get_latest_executed_block_height(),tablestate_store->get_unitstates().size(),tablestate_store->get_state_root().as_hex_str().c_str());
    return tablestate;
}

xtablestate_ext_ptr_t xstatestore_executor_t::make_state_from_prev_state_and_table(base::xvblock_t* current_block, xtablestate_ext_ptr_t const& prev_state, std::error_code & ec) const {
    if (current_block->get_height() != prev_state->get_table_state()->height() + 1) {
        ec = error::xerrc_t::statestore_block_unmatch_prev_err;
        xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-block and state unmatch.block=%s,state=%s",current_block->dump().c_str(),prev_state->get_table_state()->get_bstate()->dump().c_str());
        return nullptr;
    }

    xobject_ptr_t<base::xvbstate_t> current_state = make_object_ptr<base::xvbstate_t>(*current_block, *prev_state->get_table_state()->get_bstate());
    xhash256_t block_state_root = m_statestore_base.get_state_root_from_block(current_block);
    base::xaccount_indexs_t account_indexs;

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
        if (prev_state->get_state_mpt()->get_original_root_hash() == xhash256_t{}) {
            auto cur_state_root = m_statestore_base.get_state_root_from_block(current_block);
            if (cur_state_root != xhash256_t{}) {
                data::xtablestate_ptr_t cur_table_bstate = std::make_shared<data::xtable_bstate_t>(current_state.get());
                std::map<std::string, std::string> indexes = cur_table_bstate->map_get(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
                for (auto & v : indexes) {
                    auto & account = v.first;
                    auto & accoutn_index = v.second;
                    prev_state->get_state_mpt()->set_account_index(common::xaccount_address_t{account}, accoutn_index, ec);
                    if (ec) {
                        xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-set mpt accountindex for block(%s)",current_block->dump().c_str());
                        return nullptr;
                    }
                }
                xinfo("xstatestore_executor_t::make_state_from_prev_state_and_table upgrade first mpt.indexes_count=%zu.block=%s",indexes.size(), current_block->dump().c_str());
            }
        }

        // set changed accountindexs
        auto account_indexs_str = current_block->get_account_indexs();
        if (!account_indexs_str.empty()) {            
            account_indexs.serialize_from_string(account_indexs_str);
            for (auto & index : account_indexs.get_account_indexs()) {
                prev_state->get_state_mpt()->set_account_index(common::xaccount_address_t{index.first}, index.second, ec);
                if (ec) {
                    xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-set mpt account index.block:%s", current_block->dump().c_str());
                    return nullptr;
                }
            }

            // check if root matches.            
            auto cur_root_hash = prev_state->get_state_mpt()->get_root_hash(ec);
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
    if (m_statestore_base.need_store_unitstate() && account_indexs.get_account_indexs().size() > 0) {
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
                uint32_t limit = 2;  // XTODO
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
    xtablestate_store_ptr_t tablestate_store = std::make_shared<xtablestate_store_t>(table_bstate, prev_state->get_state_mpt(), block_state_root, unitstate_units);
    xtablestate_ext_ptr_t tablestate = write_table_all_states(current_block, tablestate_store, ec);
    if (ec) {
        xerror("xstatestore_executor_t::make_state_from_prev_state_and_table fail-write_table_all_states.block:%s", current_block->dump().c_str());
        return nullptr;            
    }

    xinfo("xstatestore_executor_t::make_state_from_prev_state_and_table succ,block=%s",current_block->dump().c_str());
    return tablestate;
}

void xstatestore_executor_t::set_latest_executed_info(uint64_t height,const std::string & blockhash) const {
    std::lock_guard<std::mutex> l(m_execute_height_lock);
    m_executed_height = height;
    m_statestore_base.set_latest_executed_info(m_table_addr, height, blockhash);
}

uint64_t xstatestore_executor_t::get_latest_executed_block_height() const {
    std::lock_guard<std::mutex> l(m_execute_height_lock);
    return m_executed_height;
}
void xstatestore_executor_t::raise_execute_height(const xstate_sync_info_t & sync_info) {
    // check if root and table state are already stored.
    std::error_code ec;
    auto mpt =
        state_mpt::xtop_state_mpt::create(m_table_addr, sync_info.get_root_hash(), m_statestore_base.get_dbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
    if (mpt == nullptr || ec) {
        xerror("xstatestore_executor_t::raise_execute_height sync result is succ but mpt create fail.table:%s,h:%llu,root:%s",
               m_table_addr.value().c_str(),
               sync_info.get_height(),
               sync_info.get_root_hash().as_hex_str().c_str());
        return;
    }

    xobject_ptr_t<base::xvblock_t> block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), sync_info.get_height(), sync_info.get_blockhash(), false);
    auto state = m_statestore_base.get_blkstate_store()->get_block_state(block.get());
    if (state == nullptr) {
        xwarn("xstatestore_executor_t::raise_execute_height fail-get_block_state. block=%s", block->dump().c_str());
        return;
    }

    xinfo("xstatestore_executor_t::raise_execute_height succ.table:%s,h:%llu,root:%s",
          m_table_addr.value().c_str(),
          sync_info.get_height(),
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
    uint32_t limit = 100;  // XTODO
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
        xdbg("xstatestore_executor_t::build_unitstate_by_accountindex get from cache account=%s,accountindex=%s", unit_addr.value().c_str(), account_index.dump().c_str());
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
        xwarn("xstatestore_executor_t::build_unitstate_by_accountindex fail-load unit account=%s,accountindex=%s", unit_addr.value().c_str(), account_index.dump().c_str());
        return;
    }
    // secondly, try make state from current block
    uint32_t limit = 100;  // XTODO
    unitstate = execute_unit_recursive(unit_addr, _unit.get(), limit, ec);
    if (nullptr != unitstate) {
        xdbg("xstatestore_executor_t::build_unitstate_by_accountindex succ. get from current block.account=%s,accountindex=%s", unit_addr.value().c_str(), account_index.dump().c_str());
        return;
    }
    xwarn("xstatestore_executor_t::build_unitstate_by_accountindex fail.ec=%s,account=%s,accountindex=%s", ec.message().c_str(), unit_addr.value().c_str(), account_index.dump().c_str());
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
        ec = error::xerrc_t::statestore_try_limit_arrive_err;
        xwarn("xstatestore_executor_t::execute_unit_recursive fail-limit to zero.block=%s", block->dump().c_str());
        return nullptr;
    }
    limit--;

    // 1.try make state from current unit
    data::xunitstate_ptr_t unitstate = make_state_from_current_unit(unit_addr, block, ec);
    if (ec) {
        xwarn("xstatestore_executor_t::execute_unit_recursive fail-make_state_from_current_unit.limit=%d,block=%s", limit,block->dump().c_str());
        return nullptr;
    }
    if (nullptr != unitstate) {
        xdbg("xstatestore_executor_t::execute_unit_recursive succ by make from current.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return unitstate;
    }

    // 2.try load prev-state from cache or db
    data::xunitstate_ptr_t prev_unitstate = m_state_accessor.read_unit_bstate(unit_addr, block->get_height() - 1, block->get_last_block_hash());
    if (nullptr == prev_unitstate) {
        xwarn("xstatestore_executor_t::execute_unit_recursive fail-read prev state from db.limit=%d,cur_block=%s", limit,block->dump().c_str());
        
        // try load prev-state by recursive
        xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(unit_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
        if (nullptr == prev_block) {
            ec = error::xerrc_t::statestore_load_unitblock_err;
            xwarn("xstatestore_executor_t::execute_unit_recursive fail-load prev block.limit=%d,cur_block=%s", limit,block->dump().c_str());
            return nullptr;     
        }
        prev_unitstate = execute_unit_recursive(unit_addr, prev_block.get(), limit, ec);
        if (nullptr == prev_unitstate) {
            xwarn("xstatestore_executor_t::execute_unit_recursive fail-execute prev block.limit=%d,cur_block=%s", limit,block->dump().c_str());
            return nullptr;
        }
    }

    unitstate = make_state_from_prev_state_and_unit(unit_addr, block, prev_unitstate, ec);
    if (ec) {
        xerror("xstatestore_executor_t::execute_unit_recursive fail.limit=%d,cur_block=%s", limit,block->dump().c_str());
        return nullptr;
    }

    xassert(nullptr != unitstate);
    xdbg("xstatestore_executor_t::execute_unit_recursive succ by recursive from prev.limit=%d,cur_block=%s", limit,block->dump().c_str());
    return unitstate;
}


NS_END2
