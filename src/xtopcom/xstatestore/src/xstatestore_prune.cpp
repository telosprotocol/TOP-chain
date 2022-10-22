// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xstatestore/xstatestore_prune.h"

#include "xbasic/xmemory.hpp"
#include "xdata/xblockbuild.h"
#include "xdata/xtable_bstate.h"
#include "xmbus/xevent_behind.h"
#include "xmetrics/xmetrics.h"
#include "xstatestore/xerror.h"
#include "xsync/xsync_on_demand.h"
#include "xvledger/xvledger.h"

#include <string>

NS_BEG2(top, statestore)

void xaccounts_prune_info_t::insert_from_tableblock(base::xvblock_t * table_block) {
    base::xaccount_indexs_t account_indexs;
    get_account_indexs(table_block, account_indexs);

    for (auto & index_pair : account_indexs.get_account_indexs()) {
        auto & account = index_pair.first;
        auto prune_upper_height = index_pair.second.get_latest_unit_height();
        xdbg("xaccounts_prune_info_t::insert_from_tableblock account:%s height:%llu", account.c_str(), prune_upper_height);
        // height 0 need not prune.
        if (prune_upper_height > 1) {
            m_prune_info[account] = prune_upper_height;
        }
    }
}

const std::map<std::string, uint64_t> & xaccounts_prune_info_t::get_prune_info() const {
    return m_prune_info;
}

void xaccounts_prune_info_t::get_account_indexs(base::xvblock_t * table_block, base::xaccount_indexs_t & account_indexs) const {
    if (table_block->get_block_class() != base::enum_xvblock_class_light) {
        return;
    }
    if (!base::xvchain_t::instance().get_xblockstore()->load_block_output(base::xvaccount_t(table_block->get_account()), table_block)) {
        xerror("xaccounts_prune_info_t::insert_from_tableblock fail-load output for block(%s)", table_block->dump().c_str());
        return;
    }

    auto account_indexs_str = table_block->get_account_indexs();
    if (!account_indexs_str.empty()) {
        account_indexs.serialize_from_string(account_indexs_str);
    }
}

void xtablestate_and_offdata_prune_info_t::insert_from_tableblock(base::xvblock_t * table_block) {
    if (table_block->get_block_class() != base::enum_xvblock_class_full) {
        const std::string delete_key = base::xvdbkey_t::create_prunable_state_key(table_block->get_account(), table_block->get_height(), table_block->get_block_hash());
        m_tablestate_keys.push_back(delete_key);
    }

    if (table_block->get_block_class() != base::enum_xvblock_class_nil) {
        const std::string delete_key = base::xvdbkey_t::create_prunable_block_output_offdata_key(table_block->get_account(), table_block->get_height(), table_block->get_viewid());
        m_offdata_keys.push_back(delete_key);
    }
}

const std::vector<std::string> & xtablestate_and_offdata_prune_info_t::get_tablestate_prune_keys() const {
    return m_tablestate_keys;
}

const std::vector<std::string> & xtablestate_and_offdata_prune_info_t::get_offdata_prune_keys() const {
    return m_offdata_keys;
}

xstatestore_prune_t::xstatestore_prune_t(common::xaccount_address_t const & table_addr, std::shared_ptr<xstatestore_resources_t> para) : m_table_addr(table_addr), m_para(para) {
    init();
}

bool xstatestore_prune_t::need_prune(uint64_t exec_height) {
    uint64_t prune_table_state_diff = XGET_CONFIG(prune_table_state_diff);

    std::lock_guard<std::mutex> l(m_prune_lock);
    if (exec_height < m_pruned_height + prune_table_state_diff) {
        return false;
    }
    xdbg("xstatestore_prune_t::need_prune table:%s will prune.pruned height:%llu,exec height:%llu", m_table_addr.to_string().c_str(), m_pruned_height, exec_height);
    return true;
}

bool xstatestore_prune_t::get_prune_section(uint64_t exec_height, uint64_t & from_height, uint64_t & to_height) {
    uint64_t keep_table_states_max_num = XGET_CONFIG(keep_table_states_max_num);
    uint64_t prune_table_state_diff = XGET_CONFIG(prune_table_state_diff);
    std::lock_guard<std::mutex> l(m_prune_lock);
    if (exec_height < m_pruned_height + prune_table_state_diff) {
        return false;
    }
    from_height = m_pruned_height + 1;
    to_height = exec_height - keep_table_states_max_num;
    return true;
}

void xstatestore_prune_t::set_pruned_height(uint64_t pruned_height) {
    std::lock_guard<std::mutex> l(m_prune_lock);
    if (pruned_height > m_pruned_height) {
        m_statestore_base.set_lowest_executed_block_height(m_table_addr, pruned_height);
        m_pruned_height = pruned_height;
    }
}

void xstatestore_prune_t::prune_imp(uint64_t exec_height) {
    uint64_t from_height;
    uint64_t to_height;
    auto ret = get_prune_section(exec_height, from_height, to_height);
    if (!ret) {
        return;
    }

    xdbg("xstatestore_prune_t::prune_imp in table:%", m_table_addr.to_string().c_str());
    bool is_storage_node = base::xvchain_t::instance().is_storage_node();
    bool is_consensus_node = base::xvchain_t::instance().has_other_node();
    uint64_t pruned_height;
    if (is_storage_node && !is_consensus_node) {
        pruned_height = prune_exec_storage(from_height, to_height);
    } else if (is_storage_node && is_consensus_node) {
        pruned_height = prune_exec_storage_and_cons(from_height, to_height);
    } else if (!is_storage_node) {
        // include consensus nodes and edge nodes.
        pruned_height = prune_exec_cons(from_height, to_height);
    } else {
        xwarn("xstatestore_prune_t::prune_imp not storage node nor cons node. can not prune.table:%s", m_table_addr.to_string().c_str());
        return;
    }

    xdbg("xstatestore_prune_t::prune_imp table:%s prune finish from %llu to %llu isstorage:%d iscons:%d.pruned_height:%llu",
         m_table_addr.to_string().c_str(),
         from_height,
         to_height,
         is_storage_node,
         is_consensus_node,
         pruned_height);

    set_pruned_height(pruned_height);
}

void xstatestore_prune_t::on_table_block_executed(uint64_t exec_height) {
    if (m_para->get_prune_dispatcher() == nullptr) {
        return;
    }

    if (!need_prune(exec_height)) {
        return;
    }

    auto self = shared_from_this();
    auto handler = [this, self, exec_height](base::xcall_t & call, const int32_t cur_thread_id, const uint64_t timenow_ms) -> bool {
        this->prune_imp(exec_height);
        return true;
    };

    base::xcall_t asyn_call(handler);
    m_para->get_prune_dispatcher()->dispatch(asyn_call);
}

void xstatestore_prune_t::unitstate_prune_batch(const xaccounts_prune_info_t & accounts_prune_info) {
    for (auto & prune_info : accounts_prune_info.get_prune_info()) {
        common::xaccount_address_t account_addr(prune_info.first);
        auto & upper_height = prune_info.second;
        // delete range for unit state.
        uint64_t account_pruned_height = m_statestore_base.get_lowest_executed_block_height(account_addr);
        if (upper_height <= account_pruned_height + 1) {
            xwarn("xstatestore_prune_t::unitstate_prune_batch height error.account:%s pruned h:%llu upper h:%llu",
                  account_addr.to_string().c_str(),
                  account_pruned_height,
                  upper_height);
            continue;
        }

        const std::string begin_delete_key = base::xvdbkey_t::create_prunable_unit_state_height_key(account_addr.vaccount(), account_pruned_height + 1);
        const std::string end_delete_key = base::xvdbkey_t::create_prunable_unit_state_height_key(account_addr.vaccount(), upper_height);
        //["begin_key", "end_key")
        if (base::xvchain_t::instance().get_xdbstore()->delete_range(begin_delete_key, end_delete_key)) {
            xinfo("xstatestore_prune_t::unitstate_prune_batch unitstate prune succ %s from %llu to %llu",
                  account_addr.to_string().c_str(),
                  account_pruned_height + 1,
                  upper_height - 1);
            m_statestore_base.set_lowest_executed_block_height(account_addr, upper_height - 1);
        } else {
            xerror("xstatestore_prune_t::unitstate_prune_batch unitstate prune failed %s from %llu to %llu",
                   account_addr.to_string().c_str(),
                   account_pruned_height + 1,
                   upper_height - 1);
        }
    }
    XMETRICS_GAUGE(metrics::state_delete_unit_state, accounts_prune_info.get_prune_info().size());
}

void xstatestore_prune_t::init() {
    m_pruned_height = m_statestore_base.get_lowest_executed_block_height(m_table_addr);
    xinfo("xstatestore_prune_t::init table:%s init prune height:%llu", m_table_addr.to_string().c_str(), m_pruned_height);
}

uint64_t xstatestore_prune_t::prune_exec_storage(uint64_t from_height, uint64_t to_height) {
    xtablestate_and_offdata_prune_info_t prune_info;
    uint64_t height = from_height;
    for (; height <= to_height; height++) {
        // prune include fork blocks.
        auto blocks = base::xvchain_t::instance().get_xblockstore()->load_block_object(get_account().vaccount(), height, false);
        if (blocks.get_vector().empty()) {
            xerror("xstatestore_prune_t::prune_exec_storage load block fail.table:%s height:%llu", m_table_addr.to_string().c_str(), height);
            break;
        }
        for (auto block : blocks.get_vector()) {
            prune_info.insert_from_tableblock(block);
        }
    }

    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_tablestate_prune_keys());
    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_offdata_prune_keys());
    XMETRICS_GAUGE(metrics::state_delete_table_data, prune_info.get_tablestate_prune_keys().size() + prune_info.get_offdata_prune_keys().size());
    xinfo("xstatestore_prune_t::prune_exec_storage prune tablestate and offdata for table %s from %llu to %llu", m_table_addr.to_string().c_str(), from_height, height - 1);
    // tablestate_prune_batch(from_height, to_height);
    return height - 1;
}

uint64_t xstatestore_prune_t::prune_exec_storage_and_cons(uint64_t from_height, uint64_t to_height) {
    xaccounts_prune_info_t accounts_prune_info;
    xtablestate_and_offdata_prune_info_t prune_info;
    uint64_t height = from_height;
    for (; height <= to_height; height++) {
        // prune include fork blocks.
        auto blocks = base::xvchain_t::instance().get_xblockstore()->load_block_object(get_account().vaccount(), height, false);
        if (blocks.get_vector().empty()) {
            xerror("xstatestore_prune_t::prune_exec_storage load block fail.table:%s height:%llu", m_table_addr.to_string().c_str(), height);
            break;
        }
        for (auto block : blocks.get_vector()) {
            prune_info.insert_from_tableblock(block);
            if (block->check_block_flag(base::enum_xvblock_flag_committed)) {
                accounts_prune_info.insert_from_tableblock(block);
            }
        }
    }
    unitstate_prune_batch(accounts_prune_info);
    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_tablestate_prune_keys());
    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_offdata_prune_keys());
    XMETRICS_GAUGE(metrics::state_delete_table_data, prune_info.get_tablestate_prune_keys().size() + prune_info.get_offdata_prune_keys().size());
    xinfo("xstatestore_prune_t::prune_exec_storage_and_cons prune tablestate,offdata,unitstate for table %s from %llu to %llu",
          m_table_addr.to_string().c_str(),
          from_height,
          height - 1);
    // tablestate_prune_batch(from_height, to_height);
    return height - 1;
}

uint64_t xstatestore_prune_t::prune_exec_cons(uint64_t from_height, uint64_t to_height) {
    uint64_t lowest_keep_height = to_height + 1;
    xobject_ptr_t<base::xvblock_t> lowest_keep_block =
        base::xvchain_t::instance().get_xblockstore()->load_block_object(get_account().vaccount(), lowest_keep_height, base::enum_xvblock_flag_committed, false);
    if (lowest_keep_block == nullptr) {
        xinfo("xstatestore_prune_t::prune_exec_cons table:%s load lowest block fail.height:%llu", get_account().to_string().c_str(), lowest_keep_height);
        return from_height - 1;
    }

    auto lowest_keep_root = m_statestore_base.get_state_root_from_block(lowest_keep_block.get());
    if (lowest_keep_root == xhash256_t{}) {
        // mpt root not pack into table block. raise pruned height directly.
        return to_height;
    }

    std::error_code ec;
    auto lowest_keep_mpt = state_mpt::xtop_state_mpt::create(get_account(), lowest_keep_root, base::xvchain_t::instance().get_xdbstore(), ec);
    if (lowest_keep_mpt == nullptr || ec) {
        xinfo("xstatestore_prune_t::prune_exec_cons create mpt fail.block:%s,root:%s", lowest_keep_block->dump().c_str(), lowest_keep_root.as_hex_str().c_str());
        return from_height - 1;
    }

    xdbg("xstatestore_prune_t::prune_exec_cons table:%s lowest keep height:%llu,root:%s",
         get_account().to_string().c_str(),
         lowest_keep_height,
         lowest_keep_root.as_hex_str().c_str());

    xaccounts_prune_info_t accounts_prune_info;
    uint32_t delete_mpt_num = 0;
    for (uint64_t height = from_height; height <= to_height; height++) {
        // prune include fork blocks.
        auto blocks = base::xvchain_t::instance().get_xblockstore()->load_block_object(get_account().vaccount(), height, false);
        if (blocks.get_vector().empty()) {
            xdbg("xstatestore_prune_t::prune_exec_cons load block fail.table:%s height:%llu", m_table_addr.to_string().c_str(), height);
            continue;
        }
        for (auto block : blocks.get_vector()) {
            auto root = m_statestore_base.get_state_root_from_block(block);
            if (root == xhash256_t{}) {
                continue;
            }

            std::error_code ec1;
            xdbg("xstatestore_prune_t::prune_exec_cons prune mpt before.table:%s,height:%llu,root:%s", m_table_addr.to_string().c_str(), height, root.as_hex_str().c_str());
            // mpt prune.
            lowest_keep_mpt->prune(root, ec1);
            if (ec1) {
                xwarn("xstatestore_prune_t::prune_exec_cons prune mpt fail.table:%s,height:%llu,root:%s", m_table_addr.to_string().c_str(), height, root.as_hex_str().c_str());
            } else {
                delete_mpt_num++;
            }
            xdbg("xstatestore_prune_t::prune_exec_cons prune mpt after.table:%s,height:%llu,root:%s", m_table_addr.to_string().c_str(), height, root.as_hex_str().c_str());
            if (block->check_block_flag(base::enum_xvblock_flag_committed)) {
                accounts_prune_info.insert_from_tableblock(block);
            }
        }
    }

    if (delete_mpt_num > 0) {
        ec.clear();
        lowest_keep_mpt->commit_pruned(ec);
        if (ec) {
            xwarn("xstatestore_prune_t::prune_exec_cons mpt commit prune fail table %s from %llu to %llu", m_table_addr.to_string().c_str(), from_height, to_height);
        }
        XMETRICS_GAUGE(metrics::state_delete_mpt, delete_mpt_num);
    }

    unitstate_prune_batch(accounts_prune_info);

    xinfo("xstatestore_prune_t::prune_exec_cons prune mpt and unitstate for table %s from %llu to %llu", m_table_addr.to_string().c_str(), from_height, to_height);
    return to_height;
}

NS_END2
