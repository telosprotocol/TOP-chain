// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xdata/xtable_bstate.h"
#include "xdata/xblockbuild.h"
#include "xstatestore/xstatestore_exec.h"
#include "xstatestore/xerror.h"

NS_BEG2(top, statestore)



xstatestore_executor_t::xstatestore_executor_t(common::xaccount_address_t const& table_addr)
: m_table_addr{table_addr} {
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
    if (old_execute_height + 1 == block->get_height()) {
        xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
        if (nullptr == prev_block) {
            xwarn("xstatestore_executor_t::on_table_block_committed fail-load prev block.cur_block=%s", block->dump().c_str());
            return;
        }
        execute_block_with_prev(block, prev_block, ec);
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

void xstatestore_executor_t::get_accountindex_from_block(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const {
    auto state_root = m_statestore_base.get_state_root_from_block(block);
    if (state_root != xhash256_t{}) {
        auto mpt = state_mpt::xtop_state_mpt::create(m_table_addr.to_string(), state_root, m_statestore_base.get_dbstore(), state_mpt::xstate_mpt_cache_t::instance(), ec);
        if (ec) {
            xwarn("xstatestore_executor_t::execute_and_get_accountindex fail-create mpt.state_root:%s.block:%s", state_root.as_hex_str().c_str(), block->dump().c_str());
            return;
        }

        account_index = mpt->get_account_index(unit_addr.value(), ec);
        if (ec) {
            xerror("xstatestore_executor_t::execute_and_get_accountindex fail-get_account_index from mpt.root hash:%s.block:%s", state_root.as_hex_str().c_str(), block->dump().c_str());
            return;
        }
        xdbg("xstatestore_executor_t::execute_and_get_accountindex succ by mpt. block:%s.account:%s index:%s", block->dump().c_str(), unit_addr.value().c_str(), account_index.dump().c_str());
        return;
    }

    data::xtablestate_ptr_t tablestate = m_statestore_base.get_table_state_by_block(block);
    if (nullptr == tablestate) {
        ec = error::xerrc_t::statestore_tablestate_exec_fail;
        xwarn("xstatestore_executor_t::execute_and_get_accountindex fail-get tablestate.block=%s",block->dump().c_str());
        return;
    }

    tablestate->get_account_index(unit_addr.value(), account_index);
    xdbg("xstatestore_executor_t::execute_and_get_accountindex succ by tablestate. block:%s.account:%s index:%s", block->dump().c_str(), unit_addr.value().c_str(), account_index.dump().c_str());
    return;
}

void xstatestore_executor_t::execute_and_get_tablestate_ext(base::xvblock_t* block, xtablestate_ext_ptr_t & tablestate_ext, std::error_code & ec) const {
    try_execute_block_on_demand(block, ec);
    if (ec) {
        return;
    }
    m_statestore_base.get_tablestate_ext_from_block(block, tablestate_ext, ec);
    if (ec) {
        // must clear ec and try execute on demand
        uint32_t limit = execute_demand_limit;
        ec.clear();
        execute_block_recursive(block, limit, ec);
        if (ec) {
            xwarn("xstatestore_executor_t::execute_and_get_tablestate_ext fail-execute recursive.block=%s,ec=%s", block->dump().c_str(), ec.message().c_str());
            return;
        }
        // try again
        m_statestore_base.get_tablestate_ext_from_block(block, tablestate_ext, ec);
        if (ec) {
            xerror("xstatestore_executor_t::execute_and_get_tablestate_ext fail-try again.block=%s,ec=%s", block->dump().c_str(), ec.message().c_str());
            return;
        }
    }
}

void xstatestore_executor_t::execute_and_get_accountindex(base::xvblock_t* block, common::xaccount_address_t const& unit_addr, base::xaccount_index_t & account_index, std::error_code & ec) const {
    try_execute_block_on_demand(block, ec);
    if (ec) {
        return;
    }

    get_accountindex_from_block(block, unit_addr, account_index, ec);
    if (ec) {
        // must clear ec and try execute on demand
        uint32_t limit = execute_demand_limit;
        ec.clear();
        execute_block_recursive(block, limit, ec);
        if (ec) {
            xwarn("xstatestore_executor_t::execute_and_get_accountindex fail-execute recursive.block=%s,unit_addr=%s,ec=%s", block->dump().c_str(), unit_addr.value().c_str(), ec.message().c_str());
            return;
        }
        // try again
        get_accountindex_from_block(block, unit_addr, account_index, ec);
        if (ec) {
            xerror("xstatestore_executor_t::execute_and_get_accountindex fail-get accountindex.block=%s,unit_addr=%s,ec=%s", block->dump().c_str(), unit_addr.value().c_str(), ec.message().c_str());
            return;
        }
    }
}

void xstatestore_executor_t::execute_and_get_tablestate(base::xvblock_t* block, data::xtablestate_ptr_t &tablestate, std::error_code & ec) const {
    try_execute_block_on_demand(block, ec);
    if (ec) {
        return;
    }
    tablestate = m_statestore_base.get_table_state_by_block(block);
    if (nullptr == tablestate) {
        // must clear ec and try execute on demand
        uint32_t limit = execute_demand_limit;
        ec.clear();
        execute_block_recursive(block, limit, ec);
        if (ec) {
            xwarn("xstatestore_executor_t::execute_and_get_tablestate fail-execute recursive.block=%s,ec=%s", block->dump().c_str(), ec.message().c_str());
            return;
        }
        // try again
        tablestate = m_statestore_base.get_table_state_by_block(block);
        if (nullptr == tablestate) {
            ec = error::xerrc_t::statestore_tablestate_exec_fail;
            xerror("xstatestore_executor_t::execute_and_get_tablestate fail-get tablestate.block=%s,ec=%s", block->dump().c_str(), ec.message().c_str());
        }
    }
}

void xstatestore_executor_t::execute_block_recursive(base::xvblock_t* block, uint32_t & limit, std::error_code & ec) const {
    xassert(!ec);
    xdbg("xstatestore_executor_t::execute_block_recursive enter.block=%s,limit=%d",block->dump().c_str(),limit);
    if (limit == 0) {
        ec = error::xerrc_t::statestore_try_limit_arrive_err;
        xwarn("xstatestore_executor_t::execute_block_recursive fail-limit to zero.block=%s", block->dump().c_str());
        return;
    }
    limit--;
    if (block->get_height() == 0) {
        ec = error::xerrc_t::statestore_try_limit_arrive_err;
        xerror("xstatestore_executor_t::execute_block_recursive fail-to genesis.block=%s, limit=%d", block->dump().c_str(), limit);
        return;
    }

    xobject_ptr_t<base::xvblock_t> prev_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), block->get_height()-1, block->get_last_block_hash(), false);
    if (nullptr == prev_block) {
        ec = error::xerrc_t::statestore_load_tableblock_err;
        xwarn("xstatestore_executor_t::execute_block_recursive fail-load prev block.cur_block=%s", block->dump().c_str());
        return;     
    }

    execute_block_with_prev(block, prev_block, ec);
    if (ec) {
        xdbg("xstatestore_executor_t::execute_block_recursive fail-try firstly.cur_block=%s", block->dump().c_str());
        // ec must clear and try execute current block again
        ec.clear();
        execute_block_recursive(prev_block.get(), limit, ec);
        if (ec) {
            xwarn("xstatestore_executor_t::execute_block_recursive fail-try prev and finish.cur_block=%s.ec=%s", block->dump().c_str(),ec.message().c_str());
            return;
        }
        execute_block_with_prev(block, prev_block, ec);
        if (ec) {
            xerror("xstatestore_executor_t::execute_block_recursive fail-try again.cur_block=%s", block->dump().c_str());
            return;
        }
        xdbg("xstatestore_executor_t::execute_block_recursive succ-try again.cur_block=%s", block->dump().c_str());
        return;
    }
    xdbg("xstatestore_executor_t::execute_block_recursive succ-try firstly.cur_block=%s", block->dump().c_str());
    return;
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

    for (uint64_t height=old_execute_height+1; height <= max_height; height++) {
        xobject_ptr_t<base::xvblock_t> cur_block = m_statestore_base.get_blockstore()->load_block_object(m_table_addr.vaccount(), height, base::enum_xvblock_flag_committed, false);
        if (nullptr == cur_block) {
            xwarn("xstatestore_executor_t::update_execute_from_execute_height fail-load committed block.account=%s,height=%ld", m_table_addr.value().c_str(), height);
            break;
        }
        execute_block_with_prev(cur_block.get(), prev_block, ec);
        if (ec) {
            xerror("xstatestore_executor_t::update_execute_from_execute_height fail-execute block.account=%s,height=%ld", m_table_addr.value().c_str(), height);
            break;
        }
        new_execute_height = height;
        prev_block = cur_block;
    }
    if (new_execute_height > old_execute_height) {
        xinfo("xstatestore_executor_t::update_execute_from_execute_height updated.account=%s,old_height=%ld,new_height=%ld",m_table_addr.value().c_str(),old_execute_height,new_execute_height);
    }
}

void xstatestore_executor_t::execute_block_with_prev(base::xvblock_t* block, xobject_ptr_t<base::xvblock_t> const& prev_block, std::error_code & ec) const {
    xassert(!ec);
    if ( (block->get_height() != prev_block->get_height()+1) || block->get_last_block_hash() != prev_block->get_block_hash()) {
        ec = error::xerrc_t::statestore_load_tableblock_output_err;
        xerror("xstatestore_executor_t::execute_block_with_prev fail-invalid block and prev, block=%s,prev=%s", block->dump().c_str(), prev_block->dump().c_str());
        return;
    }

    // execute current block for tablestate
    auto state = m_statestore_base.get_blkstate_store()->get_block_state(block);
    if (state == nullptr) {
        ec = error::xerrc_t::statestore_tablestate_exec_fail;
        xwarn("xstatestore_executor_t::execute_block_with_prev fail-get_block_state. block=%s", block->dump().c_str());
        return;
    }

    // load prev state
    std::shared_ptr<state_mpt::xtop_state_mpt> pre_mpt;
    m_statestore_base.get_mpt_from_block(prev_block.get(), pre_mpt, ec);
    if (ec) {
        xwarn("xstatestore_executor_t::execute_block_with_prev fail-load prev state. block=%s", block->dump().c_str());
        return;
    }

    auto block_state_root = m_statestore_base.get_state_root_from_block(block);

    // if pre root is empty and this root is not empty, table is forked from this block.
    if (block_state_root != xhash256_t() && pre_mpt->get_original_root_hash() == xhash256_t()) {
        data::xtablestate_ptr_t tablestate = std::make_shared<data::xtable_bstate_t>(state.get());
        std::map<std::string, std::string> indexes = tablestate->map_get(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
        for (auto & index_pair : indexes) {
            auto & account = index_pair.first;
            auto & account_index = index_pair.second;
            pre_mpt->set_account_index(account, account_index, ec);
        }
    }

    // execute current block for accountindex mpt state
    if (false == m_statestore_base.get_blockstore()->load_block_output(m_table_addr.vaccount(), block, metrics::blockstore_access_from_statestore_get_block_state)) {
        ec = error::xerrc_t::statestore_load_tableblock_output_err;
        xerror("xstatestore_executor_t::execute_block_with_prev fail-load block output, block=%s", block->dump().c_str());
        return;
    }

    auto account_indexs_str = block->get_account_indexs();
    if (!account_indexs_str.empty()) {
        base::xaccount_indexs_t account_indexs;
        account_indexs.serialize_from_string(account_indexs_str);
        for (auto & index : account_indexs.get_account_indexs()) {
            pre_mpt->set_account_index(index.first, index.second, ec);
            if (ec) {
                xerror("xstatestore_executor_t::execute_block_with_prev fail-set mpt account index.block:%s", block->dump().c_str());
                return;
            }
        }

        // check if root matches.
        auto cur_root_hash = pre_mpt->get_root_hash(ec);
        if (cur_root_hash != block_state_root) {
            ec = error::xerrc_t::statestore_block_root_unmatch_mpt_root_err;
            xerror("xstatestore_executor_t::execute_block_with_prev fail-root not match cur_root_hash:%s,state_root_hash:%s,block:%s",
                cur_root_hash.as_hex_str().c_str(),
                block_state_root.as_hex_str().c_str(),
                block->dump().c_str());
            return;
        }

        pre_mpt->commit(ec);
        if (ec) {
            xerror("xstatestore_executor_t::execute_block_with_prev fail-mpt commit.block:%s", block->dump().c_str());
            return;
        }
        xdbg("xstatestore_executor_t::execute_block_with_prev mpt committed. block:%s,root:%s,indexs_count=%zu", block->dump().c_str(), cur_root_hash.as_hex_str().c_str(), account_indexs.get_account_indexs().size());
    }

    // update execute height
    if (block->check_block_flag(base::enum_xvblock_flag_committed)) {
        set_latest_executed_info(block->get_height(), block->get_block_hash());
    }
    xdbg("xstatestore_executor_t::execute_block_with_prev succ. execute_height=%ld. block:%s", get_latest_executed_block_height(), block->dump().c_str());
    return;
}

void xstatestore_executor_t::set_latest_executed_info(uint64_t height,const std::string & blockhash) const {
    std::lock_guard<std::mutex> l(m_execute_lock);
    if (m_executed_height < height) {
        m_executed_height = height;
        m_statestore_base.set_latest_executed_info(m_table_addr, height, blockhash);
    }
}

uint64_t xstatestore_executor_t::get_latest_executed_block_height() const {
    std::lock_guard<std::mutex> l(m_execute_lock);
    return m_executed_height;
}

NS_END2
