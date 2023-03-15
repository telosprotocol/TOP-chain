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
}

const std::vector<std::string> & xtablestate_and_offdata_prune_info_t::get_tablestate_prune_keys() const {
    return m_tablestate_keys;
}

xstatestore_prune_t::xstatestore_prune_t(common::xtable_address_t const & table_addr, std::shared_ptr<xstatestore_resources_t> para) : m_table_addr(table_addr), m_table_vaddr(table_addr.vaccount()), m_para(para) {
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
    uint64_t prune_table_state_max = XGET_CONFIG(prune_table_state_max);
    std::lock_guard<std::mutex> l(m_prune_lock);
    if (exec_height < m_pruned_height + prune_table_state_diff) {
        return false;
    }
    from_height = m_pruned_height + 1;
    uint64_t to_height_max = exec_height - keep_table_states_max_num;
    to_height = to_height_max < (m_pruned_height + prune_table_state_max) ? to_height_max : (m_pruned_height + prune_table_state_max);
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

    xdbg("xstatestore_prune_t::prune_imp in table:%s exec_height:%llu,from_height:%llu,to_height:%llu", m_table_addr.to_string().c_str(), exec_height, from_height, to_height);
    bool is_storage_node = base::xvchain_t::instance().is_storage_node();
    bool is_consensus_node = base::xvchain_t::instance().has_other_node();
    uint64_t pruned_height;
    if (is_storage_node && !is_consensus_node) {
        pruned_height = prune_exec_storage(from_height, to_height);
    } else if (is_storage_node && is_consensus_node) {
        pruned_height = prune_exec_storage_and_cons(from_height, to_height);
    } else if (!is_storage_node) {
        // include consensus nodes and edge nodes.
        pruned_height = prune_exec_cons(from_height, to_height, exec_height);
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
    if (accounts_prune_info.get_prune_info().empty()) {
        return;
    }

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
        XMETRICS_GAUGE(metrics::xmetrics_tag_t::prune_state_unitstate, upper_height - account_pruned_height);
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
    bool need_prune_unitstates = !base::xvchain_t::instance().need_store_units(m_table_vaddr.get_zone_index());
    xaccounts_prune_info_t accounts_prune_info;
    uint64_t height = from_height;
    for (; height <= to_height; height++) {
        // prune include fork blocks.
        auto blocks = base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_vaddr, height, false);
        if (blocks.get_vector().empty()) {
            xerror("xstatestore_prune_t::prune_exec_storage load block fail.table:%s height:%llu", m_table_addr.to_string().c_str(), height);
            break;
        }
        for (auto block : blocks.get_vector()) {
            prune_info.insert_from_tableblock(block);
            if (need_prune_unitstates && block->check_block_flag(base::enum_xvblock_flag_committed)) {
                accounts_prune_info.insert_from_tableblock(block);
            }
        }
    }

    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_tablestate_prune_keys());
    unitstate_prune_batch(accounts_prune_info);
    XMETRICS_GAUGE(metrics::state_delete_table_data, prune_info.get_tablestate_prune_keys().size());
    xinfo("xstatestore_prune_t::prune_exec_storage prune tablestate and offdata for table %s from %llu to %llu", m_table_addr.to_string().c_str(), from_height, height - 1);
    return height - 1;
}

uint64_t xstatestore_prune_t::prune_exec_storage_and_cons(uint64_t from_height, uint64_t to_height) {
    xtablestate_and_offdata_prune_info_t prune_info;
    bool need_prune_unitstates = !base::xvchain_t::instance().need_store_units(m_table_vaddr.get_zone_index());
    xaccounts_prune_info_t accounts_prune_info;
    uint64_t height = from_height;
    for (; height <= to_height; height++) {
        // prune include fork blocks.
        auto blocks = base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_vaddr, height, false);
        if (blocks.get_vector().empty()) {
            xerror("xstatestore_prune_t::prune_exec_storage load block fail.table:%s height:%llu", m_table_addr.to_string().c_str(), height);
            break;
        }
        for (auto block : blocks.get_vector()) {
            prune_info.insert_from_tableblock(block);
            if (need_prune_unitstates && block->check_block_flag(base::enum_xvblock_flag_committed)) {
                accounts_prune_info.insert_from_tableblock(block);
            }
        }
    }
    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_tablestate_prune_keys());
    unitstate_prune_batch(accounts_prune_info);
    XMETRICS_GAUGE(metrics::state_delete_table_data, prune_info.get_tablestate_prune_keys().size());
    xinfo("xstatestore_prune_t::prune_exec_storage_and_cons prune tablestate,offdata,unitstate for table %s from %llu to %llu",
          m_table_addr.to_string().c_str(),
          from_height,
          height - 1);
    return height - 1;
}

uint64_t xstatestore_prune_t::prune_exec_cons(uint64_t from_height, uint64_t to_height, uint64_t exec_height) {
    uint64_t lowest_keep_height = to_height + 1;
    xobject_ptr_t<base::xvblock_t> lowest_keep_block =
        base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_vaddr, lowest_keep_height, base::enum_xvblock_flag_committed, false);
    if (lowest_keep_block == nullptr) {
        xinfo("xstatestore_prune_t::prune_exec_cons table:%s load lowest block fail.height:%llu", get_account().to_string().c_str(), lowest_keep_height);
        return from_height - 1;
    }

    std::shared_ptr<state_mpt::xtop_state_mpt> lowest_keep_mpt = nullptr;
    auto lowest_keep_root = m_statestore_base.get_state_root_from_block(lowest_keep_block.get());
    if (!lowest_keep_root.empty()) {
        std::error_code ec;
        lowest_keep_mpt = state_mpt::xtop_state_mpt::create(get_account(), lowest_keep_root, base::xvchain_t::instance().get_xdbstore(), ec);
        if (lowest_keep_mpt == nullptr || ec) {
            xinfo("xstatestore_prune_t::prune_exec_cons create mpt fail.block:%s,root:%s", lowest_keep_block->dump().c_str(), lowest_keep_root.hex().c_str());
            XMETRICS_GAUGE(metrics::state_delete_create_mpt_fail, 1);

            xobject_ptr_t<base::xvblock_t> latest_exec_block =
                base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_vaddr, exec_height, base::enum_xvblock_flag_committed, false);
            if (latest_exec_block == nullptr) {
                xerror("xstatestore_prune_t::prune_exec_cons table:%s load latest exec block fail.height:%llu", get_account().to_string().c_str(), exec_height);
                return from_height - 1;
            }

            // use last full block's mpt to prune old mpt.
            uint64_t latest_full_height = latest_exec_block->get_last_full_block_height();
            if (latest_full_height < to_height) {
                xerror("xstatestore_prune_t::prune_exec_cons table:%s lastest full height:%llu lower than prune to height:%llu,not prune this time.", get_account().to_string().c_str(), latest_full_height, to_height);
                return from_height - 1;
            }
            auto & latest_full_hash = latest_exec_block->get_last_full_block_hash();
            xobject_ptr_t<base::xvblock_t> latest_full_block =
                base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_vaddr, exec_height, latest_full_hash, false);
            if (latest_full_block == nullptr) {
                xwarn("xstatestore_prune_t::prune_exec_cons table:%s load latest full block fail.height:%llu", get_account().to_string().c_str(), latest_full_height);
                XMETRICS_GAUGE(metrics::state_delete_by_full_table, 0);
                return from_height - 1;
            }

            auto last_full_block_root = m_statestore_base.get_state_root_from_block(latest_full_block.get());
            if (!last_full_block_root.empty()) {
                ec.clear();
                lowest_keep_mpt = state_mpt::xtop_state_mpt::create(get_account(), last_full_block_root, base::xvchain_t::instance().get_xdbstore(), ec);
                if (lowest_keep_mpt == nullptr || ec) {
                    xwarn("xstatestore_prune_t::prune_exec_cons create last full block mpt fail.block:%s,root:%s",
                          latest_full_block->dump().c_str(),
                          last_full_block_root.hex().c_str());
                    XMETRICS_GAUGE(metrics::state_delete_by_full_table, 0);
                    return from_height - 1;
                }
                XMETRICS_GAUGE(metrics::state_delete_by_full_table, 1);
            }
        }
    }

    xinfo("xstatestore_prune_t::prune_exec_cons table:%s lowest keep height:%llu,root:%s,from:%llu,to:%llu",
         get_account().to_string().c_str(),
         lowest_keep_height,
         lowest_keep_root.hex().c_str(),
         from_height,
         to_height);

    bool need_prune_unitstates = !base::xvchain_t::instance().need_store_units(m_table_vaddr.get_zone_index());
    xtablestate_and_offdata_prune_info_t prune_info;
    xaccounts_prune_info_t accounts_prune_info;
    uint32_t delete_mpt_num = 0;
    std::unordered_set<evm_common::xh256_t> pruned_hashes;
    for (uint64_t height = from_height; height <= to_height; height++) {
        // prune include fork blocks.
        auto blocks = base::xvchain_t::instance().get_xblockstore()->load_block_object(m_table_vaddr, height, false);
        if (blocks.get_vector().empty()) {
            xdbg("xstatestore_prune_t::prune_exec_cons load block fail.table:%s height:%llu", m_table_addr.to_string().c_str(), height);
            continue;
        }
        for (auto block : blocks.get_vector()) {
            prune_info.insert_from_tableblock(block);
            if (lowest_keep_mpt != nullptr) {
                auto root = m_statestore_base.get_state_root_from_block(block);
                if (root.empty()) {
                    continue;
                }

                xdbg("xstatestore_prune_t::prune_exec_cons prune mpt before.table:%s,height:%llu,root:%s", m_table_addr.to_string().c_str(), height, root.hex().c_str());
                // mpt prune.
                {
                    std::error_code ec1;
                    XMETRICS_TIME_RECORD("state_mpt_prune");
                    lowest_keep_mpt->prune(root, pruned_hashes, ec1);

                    if (ec1) {
                        xwarn("xstatestore_prune_t::prune_exec_cons prune mpt fail.table:%s,height:%llu,root:%s", m_table_addr.to_string().c_str(), height, root.hex().c_str());
                    } else {
                        delete_mpt_num++;
                    }
                }
                xdbg("xstatestore_prune_t::prune_exec_cons prune mpt after.table:%s,height:%llu,root:%s", m_table_addr.to_string().c_str(), height, root.hex().c_str());
                if (need_prune_unitstates && block->check_block_flag(base::enum_xvblock_flag_committed)) {
                    accounts_prune_info.insert_from_tableblock(block);
                }
            }
        }
    }

    if (delete_mpt_num > 0) {
        {
            std::error_code ec;
            XMETRICS_TIME_RECORD("state_mpt_commit_pruned");
            lowest_keep_mpt->commit_pruned(pruned_hashes, ec);
            if (ec) {
                xwarn("xstatestore_prune_t::prune_exec_cons mpt commit prune fail table %s from %llu to %llu", m_table_addr.to_string().c_str(), from_height, to_height);
            }
        }
        XMETRICS_GAUGE(metrics::state_delete_mpt, delete_mpt_num);
    }

    base::xvchain_t::instance().get_xdbstore()->delete_values(prune_info.get_tablestate_prune_keys());
    // prune offdata with tableblock for non strorage nodes.
    XMETRICS_GAUGE(metrics::state_delete_table_data, prune_info.get_tablestate_prune_keys().size());
    unitstate_prune_batch(accounts_prune_info);

    xinfo("xstatestore_prune_t::prune_exec_cons prune mpt tablestate and unitstate for table %s from %llu to %llu", m_table_addr.to_string().c_str(), from_height, to_height);
    return to_height;
}

NS_END2
