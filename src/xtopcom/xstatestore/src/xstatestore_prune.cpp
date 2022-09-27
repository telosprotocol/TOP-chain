// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatestore/xstatestore_prune.h"

#include "xbasic/xmemory.hpp"
#include "xdata/xblockbuild.h"
#include "xdata/xtable_bstate.h"
#include "xmbus/xevent_behind.h"
#include "xstatestore/xerror.h"
#include "xsync/xsync_on_demand.h"
#include "xvledger/xvledger.h"

#include <string>

NS_BEG2(top, statestore)

#define prune_height_section_size_max (50)

xstatestore_prune_t::xstatestore_prune_t(common::xaccount_address_t const & table_addr) : m_table_addr{table_addr} {
    init();
}

void xstatestore_prune_t::init() {
    m_pruned_height = m_statestore_base.get_lowest_executed_block_height(m_table_addr);
    xinfo("xstatestore_prune_t::init table:%s init prune height:%llu", m_table_addr.value().c_str(), m_pruned_height);
}

void xstatestore_prune_t::on_table_mpt_and_unitstate_executed(uint64_t table_block_height) {
    std::lock_guard<std::mutex> l(m_prune_lock);
    if (m_pruned_height == 0) {
        m_statestore_base.set_lowest_executed_block_height(m_table_addr, table_block_height);
        m_pruned_height = table_block_height;
        xinfo("statestore_prune_t::on_table_mpt_and_unitstate_executed table:%s init prune height:%llu", m_table_addr.value().c_str(), table_block_height);
    }
}

void xstatestore_prune_t::prune(uint64_t latest_executed_height) {
    uint64_t from_height;
    uint64_t to_height;
    {
        uint64_t keep_table_states_max_num = XGET_CONFIG(keep_table_states_max_num);
        std::lock_guard<std::mutex> l(m_prune_lock);
        if (latest_executed_height <= m_pruned_height + keep_table_states_max_num) {
            xdbg("xstatestore_prune_t::prune table:%s no need prune. pruned height:%llu, exec height:%llu", m_table_addr.value().c_str(), m_pruned_height, latest_executed_height);
            return;
        }
        from_height = m_pruned_height + 1;
        uint64_t max_prune_height = latest_executed_height - keep_table_states_max_num;
        to_height = (max_prune_height < m_pruned_height + prune_height_section_size_max) ? max_prune_height : (m_pruned_height + prune_height_section_size_max);
    }

    auto pruned_height = prune_by_height_section(from_height, to_height);
    xdbg("xstatestore_prune_t::prune table:%s pruned. pruned height:%llu,exec height:%llu,from:%llu,to:%llu",
         m_table_addr.value().c_str(),
         pruned_height,
         latest_executed_height,
         from_height,
         to_height);
    {
        std::lock_guard<std::mutex> l(m_prune_lock);
        m_statestore_base.set_lowest_executed_block_height(m_table_addr, pruned_height);
        m_pruned_height = pruned_height;
    }
}

uint64_t xstatestore_prune_t::prune_by_height_section(uint64_t from_height, uint64_t to_height) {
    // todo(nathan):prune unitstates of forked table blocks.
    xobject_ptr_t<base::xvblock_t> prune_block =
        base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_addr.vaccount(), from_height, base::enum_xvblock_flag_committed, false);

    for (uint64_t height = from_height + 1; height <= to_height + 1; height++) {
        if (prune_block == nullptr) {
            xdbg("xstatestore_prune_t::prune_by_height_section block load fail, skip it.table:%s height:%llu", m_table_addr.value().c_str(), height - 1);
            prune_block = base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_addr.vaccount(), height, base::enum_xvblock_flag_committed, false);
            continue;
        }

        std::shared_ptr<state_mpt::xtop_state_mpt> mpt;
        std::error_code ec;
        m_statestore_base.get_mpt_from_block(prune_block.get(), mpt, ec);
        if (ec) {
            xdbg("xstatestore_prune_t::prune_by_height_section mpt create fail,skip it.block:%s", prune_block->dump().c_str());
            continue;
        }

        xobject_ptr_t<base::xvblock_t> next_block =
            base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_addr.vaccount(), height, base::enum_xvblock_flag_committed, false);
        if (next_block == nullptr) {
            // sync H+1 table block for prune unitstate of H table block.
            mbus::xevent_behind_ptr_t ev = make_object_ptr<mbus::xevent_behind_on_demand_t>(m_table_addr.value(), height, (uint32_t)1, false, "unitstate_prune", "", false);
            base::xvchain_t::instance().get_xevmbus()->push_event(ev);
            xwarn("xstatestore_prune_t::prune_by_height_section try sync table block.table:%s,try sync h %llu", m_table_addr.value().c_str(), height);
            return height - 2;
        }

        prune_imp(mpt, next_block.get());
        prune_block = next_block;
    }

    return to_height;
}

void xstatestore_prune_t::prune_imp(const std::shared_ptr<state_mpt::xtop_state_mpt> & mpt, base::xvblock_t * next_block) {
    if (false == m_statestore_base.get_blockstore()->load_block_output(m_table_addr.vaccount(), next_block)) {
        xerror("xstatestore_prune_t::prune_imp fail-load output for block(%s)", next_block->dump().c_str());
        return;
    }

    base::xaccount_indexs_t account_indexs;
    auto account_indexs_str = next_block->get_account_indexs();
    if (!account_indexs_str.empty()) {
        account_indexs.serialize_from_string(account_indexs_str);
        for (auto & index : account_indexs.get_account_indexs()) {
            // todo(nathan): delete unitstate from db by mpt.
            std::error_code ec;
            mpt->prune_unit( common::xaccount_address_t(index.first), ec);
            xdbg("xstatestore_prune_t::prune_imp table:%s,next block:%s,prune account:%s", m_table_addr.value().c_str(), next_block->dump().c_str(), index.first.c_str());
        }
    }
}

NS_END2
