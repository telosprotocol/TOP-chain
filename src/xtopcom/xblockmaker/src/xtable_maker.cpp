// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xtable_maker.h"
#include "xblockmaker/xtable_builder.h"
#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xchain_fork/xchain_upgrade_center.h"

NS_BEG2(top, blockmaker)

// unconfirm rate limit parameters
#define table_pair_unconfirm_tx_num_max (64)
#define table_total_unconfirm_tx_num_max  (256)

xtable_maker_t::xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
: xblock_maker_t(account, resources, m_keep_latest_blocks_max) {
    xdbg("xtable_maker_t::xtable_maker_t create,this=%p,account=%s", this, account.c_str());
    m_fulltable_builder = std::make_shared<xfulltable_builder_t>();
    m_lighttable_builder = std::make_shared<xlighttable_builder_t>();
    m_emptytable_builder = std::make_shared<xemptytable_builder_t>();
    m_default_builder_para = std::make_shared<xblock_builder_para_face_t>(resources);
    m_full_table_interval_num = XGET_CONFIG(fulltable_interval_block_num);
}

xtable_maker_t::~xtable_maker_t() {
    xdbg("xtable_maker_t::xtable_maker_t destroy,this=%p", this);
}

int32_t xtable_maker_t::default_check_latest_state() {
    auto _latest_cert_block = get_blockstore()->get_latest_cert_block(*this);
    xblock_ptr_t cert_block = xblock_t::raw_vblock_to_object_ptr(_latest_cert_block.get());
    return check_latest_state(cert_block);
}

void xtable_maker_t::refresh_cache_unit_makers() {
    XMETRICS_TIMER(metrics::cons_tablemaker_refresh_cache);
    // clear old unit makers, only cache latest makers
    clear_old_unit_makers();
    // clear all pending txs
    clear_all_pending_txs();
}

int32_t xtable_maker_t::check_latest_state(const xblock_ptr_t & latest_block) {
    XMETRICS_TIMER(metrics::cons_tablemaker_check_state_tick);
    if ( m_check_state_success && latest_block->get_block_hash() == get_highest_height_block()->get_block_hash()) {
        // already latest state
        return xsuccess;
    }

    m_check_state_success = false;
    // cache latest block
    if (!load_and_cache_enough_blocks(latest_block)) {
        xwarn("xunit_maker_t::check_latest_state fail-load_and_cache_enough_blocks.account=%s", get_account().c_str());
        return xblockmaker_error_latest_table_blocks_invalid;
    }

    if (false == check_latest_blocks(latest_block)) {
        xerror("xtable_maker_t::check_latest_state fail-check_latest_blocks.latest_block=%s",
            latest_block->dump().c_str());
        return xblockmaker_error_latest_table_blocks_invalid;
    }

    xdbg_info("xtable_maker_t::check_latest_state finish.latest_cert_block=%s,unitmaker_size=%d",
        latest_block->dump().c_str(), m_unit_makers.size());
    return xsuccess;
}

xunit_maker_ptr_t xtable_maker_t::create_unit_maker(const std::string & account) {
    auto iter = m_unit_makers.find(account);
    if (iter == m_unit_makers.end()) {
        xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account, get_resources());
        xassert(get_zone_index() == unitmaker->get_zone_index());
        xassert(get_ledger_subaddr() == unitmaker->get_ledger_subaddr());
        m_unit_makers[account] = unitmaker;
        xdbg("xtable_maker_t::create_unit_maker unit_maker_changed add. account=%s",
            unitmaker->get_account().c_str());
        return unitmaker;
    }
    return iter->second;
}

void xtable_maker_t::clear_old_unit_makers() {
    uint64_t highest_table_height = get_highest_height_block()->get_height();
    for (auto iter = m_unit_makers.begin(); iter != m_unit_makers.end();) {
        auto & unitmaker = iter->second;
        if (unitmaker->get_latest_blocks().empty()) {
            xdbg("xtable_maker_t::clear_old_unit_makers unit_maker_changed delete empty account. account=%s,highest_table_height=%" PRIu64 "",
                unitmaker->get_account().c_str(), highest_table_height);
            iter = m_unit_makers.erase(iter);
            continue;
        }

        // TODO(jimmy) cache clear rules update
        uint64_t highest_unit_parent_height = unitmaker->get_highest_height_block()->get_parent_block_height();
        if (highest_unit_parent_height + m_cache_unit_maker_parent_height < highest_table_height) {
            xdbg("xtable_maker_t::clear_old_unit_makers unit_maker_changed delete old account. account=%s,height=%" PRIu64 ",highest_unit_parent_height=%" PRIu64 ",highest_table_height=%" PRIu64 "",
                unitmaker->get_account().c_str(), unitmaker->get_highest_height_block()->get_height(), highest_unit_parent_height, highest_table_height);
            iter = m_unit_makers.erase(iter);
            continue;
        }
        iter++;
    }
    xdbg("xtable_maker_t::clear_old_unit_makers account=%s,total_size=%zu", get_account().c_str(), m_unit_makers.size());
}

void xtable_maker_t::clear_all_pending_txs() {
    for (auto & v : m_unit_makers) {
        auto & unitmaker = v.second;
        unitmaker->clear_tx();
    }
}

bool xtable_maker_t::can_make_next_full_block() const {
    uint64_t proposal_height = get_highest_height_block()->get_height() + 1;
    if ((proposal_height & (m_full_table_interval_num - 1)) == 0) {
        return true;
    }
    return false;
}
bool xtable_maker_t::can_make_next_light_block(xtablemaker_para_t & table_para) const {
    // TODO(jimmy)
    return !table_para.get_origin_txs().empty();
}

void xtable_maker_t::set_packtx_metrics(const xcons_transaction_ptr_t & tx, bool bsucc) const {
    #ifdef ENABLE_METRICS
    XMETRICS_GAUGE(metrics::cons_packtx_succ, bsucc ? 1 : 0);
    if (tx->is_self_tx() || tx->is_send_tx()) {
        XMETRICS_GAUGE(metrics::cons_packtx_sendtx_succ, bsucc ? 1 : 0);
    } else if (tx->is_recv_tx()) {
        XMETRICS_GAUGE(metrics::cons_packtx_recvtx_succ, bsucc ? 1 : 0);
    } else if (tx->is_confirm_tx()) {
        XMETRICS_GAUGE(metrics::cons_packtx_confirmtx_succ, bsucc ? 1 : 0);
    }
    #endif
}

bool xtable_maker_t::create_lightunit_makers(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, std::map<std::string, xunit_maker_ptr_t> & unitmakers) {
    const data::xtablestate_ptr_t & tablestate = table_para.get_tablestate();
    const std::vector<xcons_transaction_ptr_t> & input_table_txs = table_para.get_origin_txs();

    base::enum_transaction_subtype last_tx_subtype = base::enum_transaction_subtype_invalid;
    base::xtable_shortid_t last_tableid = 0xFFFF;
    uint64_t last_receipt_id = 0;
    std::map<std::string, bool> account_check_state_ret;

    for (auto & tx : input_table_txs) {
        std::string unit_account = tx->get_account_addr();
        bool need_check_state = true;
        auto iter = account_check_state_ret.find(unit_account);
        if (iter != account_check_state_ret.end()) {
            if (iter->second == true) {
                need_check_state = false;
            } else {
                continue;
            }
        }

        // 1.check unit maker state
        xunit_maker_ptr_t unitmaker = create_unit_maker(unit_account);
        if (need_check_state) {
            base::xaccount_index_t accountindex;
            tablestate->get_account_index(unit_account, accountindex);
            base::xaccount_index_t commit_accountindex;
            table_para.get_commit_tablestate()->get_account_index(unit_account, commit_accountindex);
            int32_t ret = unitmaker->check_latest_state(cs_para, accountindex, commit_accountindex);
            account_check_state_ret[unit_account] = (ret != xsuccess) ? false : true;
            if (ret != xsuccess) {
                XMETRICS_GAUGE(metrics::cons_packtx_fail_unit_check_state, 1);
                set_packtx_metrics(tx, false);
                xwarn("xtable_maker_t::create_lightunit_makers fail-tx filtered for unit check_latest_state,%s,account=%s,error_code=%s,tx=%s",
                    cs_para.dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(ret).c_str(), tx->dump(true).c_str());
                continue;
            }
        }

        // 2.try to make empty or full unit  TODO(jimmy) fullunit can also pack tx future
        if (false == tx->is_confirm_tx() && unitmaker->must_make_next_full_block()) {
            XMETRICS_GAUGE(metrics::cons_packtx_fail_fullunit_limit, 1);
            set_packtx_metrics(tx, false);
            unitmakers[unit_account] = unitmaker;
            xwarn("xtable_maker_t::create_lightunit_makers fail-tx filtered for fullunit but make fullunit,%s,account=%s,tx=%s",
                cs_para.dump().c_str(), unit_account.c_str(), tx->dump(true).c_str());
            auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
            bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
            if (!is_forked)
                continue;
        }

        // 3.then check if tx is invalid
        base::enum_transaction_subtype cur_tx_subtype = base::enum_transaction_subtype_invalid;
        base::xtable_shortid_t cur_tableid = 0xFFFF;
        uint64_t cur_receipt_id = 0;
        if (tx->is_recv_tx() || tx->is_confirm_tx()) {
            cur_tx_subtype = tx->get_tx_subtype();
            cur_tableid = tx->get_peer_tableid();

            cur_receipt_id = tx->get_last_action_receipt_id();
            if (last_tx_subtype != cur_tx_subtype || cur_tableid != last_tableid) {
                base::xreceiptid_pair_t receiptid_pair;
                tablestate->find_receiptid_pair(cur_tableid, receiptid_pair);
                last_receipt_id = tx->is_recv_tx() ? receiptid_pair.get_recvid_max() : receiptid_pair.get_confirmid_max();
            }
            if (cur_receipt_id != last_receipt_id + 1) {
                XMETRICS_GAUGE(metrics::cons_packtx_fail_receiptid_contious, 1);
                set_packtx_metrics(tx, false);
                xwarn("xtable_maker_t::create_lightunit_makers fail-tx filtered for receiptid not contious. %s last_receipt_id=%ld, tx=%s",
                    cs_para.dump().c_str(), last_receipt_id, tx->dump(true).c_str());
                continue;
            }
        }

        if (tx->is_send_tx()) {
            if (tablestate->get_unconfirm_tx_num() > table_total_unconfirm_tx_num_max) {
                XMETRICS_GAUGE(metrics::cons_packtx_fail_total_unconfirm_limit, 1);
                set_packtx_metrics(tx, false);
                xwarn("xtable_maker_t::create_lightunit_makers fail-tx filtered for table total unconfirm tx num too much. %s unconfirm=%d, tx=%s",
                    cs_para.dump().c_str(), tablestate->get_unconfirm_tx_num(), tx->dump(true).c_str());
                continue;
            }

            auto peer_table_sid = tx->get_peer_tableid();
            base::xreceiptid_pair_t receiptid_pair;
            tablestate->find_receiptid_pair(peer_table_sid, receiptid_pair);
            if (receiptid_pair.get_unconfirm_num() >= table_pair_unconfirm_tx_num_max) {
                XMETRICS_GAUGE(metrics::cons_packtx_fail_table_unconfirm_limit, 1);
                set_packtx_metrics(tx, false);
                xwarn("xtable_maker_t::create_lightunit_makers fail-tx filtered for table unconfirm tx num too much. %s unconfirm_num=%u, tx=%s",
                    cs_para.dump().c_str(), receiptid_pair.get_unconfirm_num(), tx->dump(true).c_str());
                continue;
            }
        }

        if (false == unitmaker->push_tx(cs_para, tx)) {
            auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
            bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
            xdbg("xtable_maker_t::create_lightunit_makers remove empty unit:%d", is_forked);
            if (is_forked && unitmaker->can_make_next_full_block()) {
                XMETRICS_GAUGE(metrics::cons_packtx_fail_fullunit_limit, 1);
                set_packtx_metrics(tx, false);
                unitmakers[unit_account] = unitmaker;
                xwarn("xtable_maker_t::create_lightunit_makers fail-tx filtered for fullunit but make fullunit,%s,account=%s,tx=%s",
                    cs_para.dump().c_str(), unit_account.c_str(), tx->dump(true).c_str());
                continue;
            } else {
                set_packtx_metrics(tx, false);
                continue;
            }
        }
        set_packtx_metrics(tx, true);

        // finally, record success tx info for receiptid contious check
        if (tx->is_recv_tx() || tx->is_confirm_tx()) {
            last_tx_subtype = cur_tx_subtype;
            last_tableid = cur_tableid;
            last_receipt_id = cur_receipt_id;
        }

        table_para.push_tx_to_proposal(tx);  // save tx to proposal
        unitmakers[unit_account] = unitmaker;
    }
    return true;
}

bool xtable_maker_t::create_non_lightunit_makers(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, std::map<std::string, xunit_maker_ptr_t> & unitmakers) {
    const data::xtablestate_ptr_t & tablestate = table_para.get_tablestate();
    std::set<std::string> empty_unit_accounts;
    uint32_t max_lighttable_count = 2;  // TODO(jimmy) all empty and full unit account come from tow latest light-table
    uint32_t filter_table_count = 0;
    if (filter_table_count < max_lighttable_count && cs_para.get_latest_cert_block()->get_block_class() == base::enum_xvblock_class_light) {
        get_unit_accounts(cs_para.get_latest_cert_block(), empty_unit_accounts);
        filter_table_count++;
    }
    if (filter_table_count < max_lighttable_count && cs_para.get_latest_locked_block()->get_block_class() == base::enum_xvblock_class_light) {
        get_unit_accounts(cs_para.get_latest_locked_block(), empty_unit_accounts);
        filter_table_count++;
    }
    if (filter_table_count < max_lighttable_count && cs_para.get_latest_committed_block()->get_block_class() == base::enum_xvblock_class_light) {
        get_unit_accounts(cs_para.get_latest_committed_block(), empty_unit_accounts);
        filter_table_count++;
    }

    auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
    xdbg("xtable_maker_t::create_non_lightunit_makers remove empty unit:%d", is_forked);

    for (auto & unit_account : empty_unit_accounts) {
        xunit_maker_ptr_t unitmaker = create_unit_maker(unit_account);

        base::xaccount_index_t accountindex;
        tablestate->get_account_index(unit_account, accountindex);
        base::xaccount_index_t commit_accountindex;
        table_para.get_commit_tablestate()->get_account_index(unit_account, commit_accountindex);
        int32_t ret = unitmaker->check_latest_state(cs_para, accountindex, commit_accountindex);
        if (ret != xsuccess) {
            // empty unit maker must create success
            xwarn("xtable_maker_t::create_non_lightunit_makers fail-check_latest_state,%s,account=%s,error_code=%s",
                cs_para.dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return false;
        }

        bool can_make = false;
        if (is_forked) {
            can_make = unitmaker->can_make_next_block_v2();
        } else {
            can_make = unitmaker->can_make_next_block();
        }
        if (can_make) {
            unitmakers[unit_account] = unitmaker;
        }
    }
    return true;
}

bool xtable_maker_t::create_other_makers(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, std::map<std::string, xunit_maker_ptr_t> & unitmakers) {
    const data::xtablestate_ptr_t & tablestate = table_para.get_tablestate();
    const std::vector<std::string> & other_accounts = table_para.get_other_accounts();

    auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
    xdbg("xtable_maker_t::create_other_makers remove empty unit:%d", is_forked);

    for (auto & unit_account : other_accounts) {
        xunit_maker_ptr_t unitmaker = create_unit_maker(unit_account);

        base::xaccount_index_t accountindex;
        tablestate->get_account_index(unit_account, accountindex);
        base::xaccount_index_t commit_accountindex;
        table_para.get_commit_tablestate()->get_account_index(unit_account, commit_accountindex);
        int32_t ret = unitmaker->check_latest_state(cs_para, accountindex, commit_accountindex);
        if (ret != xsuccess) {
            // other unit maker must create success
            xwarn("xtable_maker_t::create_other_makers fail-check_latest_state,%s,account=%s,error_code=%s",
                cs_para.dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return false;
        }
        bool can_make = false;
        if (is_forked) {
            can_make = unitmaker->can_make_next_block_v2();
        } else {
            can_make = unitmaker->can_make_next_block();
        }
        if (can_make) {
            unitmakers[unit_account] = unitmaker;
        } else {
            // other unit maker must create success
            xwarn("xtable_maker_t::create_non_lightunit_makers fail-can not make next block,%s,account=%s,error_code=%s",
                cs_para.dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return false;
        }
    }
    return true;
}

void xtable_maker_t::get_unit_accounts(const xblock_ptr_t & block, std::set<std::string> & accounts) const {
    auto unit_headers = block->get_sub_block_headers();
    for (auto & _unit_header : unit_headers) {
        accounts.insert(_unit_header->get_account());
    }
}

xblock_ptr_t xtable_maker_t::make_light_table(bool is_leader, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    // refresh all cache unit makers
    refresh_cache_unit_makers();

    // try to make non-empty-unit for left unitmakers
    std::map<std::string, xunit_maker_ptr_t> unitmakers;
    // find lightunit makers
    if (false == create_lightunit_makers(table_para, cs_para, unitmakers)) {
        xwarn("xtable_maker_t::make_light_table fail-create_lightunit_makers.proposal=%s",
            cs_para.dump().c_str());
        table_result.m_make_block_error_code = xblockmaker_error_tx_check;
        return nullptr;
    }
    const auto & fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
    xdbg("xtable_maker_t::make_light_table remove empty unit:%d", is_forked);
    if (!is_forked) {
        // find all empty and fullunit makers
        if (false == create_non_lightunit_makers(table_para, cs_para, unitmakers)) {
            xwarn("xtable_maker_t::make_light_table fail-create_non_lightunit_makers.proposal=%s",
                cs_para.dump().c_str());
            table_result.m_make_block_error_code = xblockmaker_error_tx_check;
            return nullptr;
        }
    }

    // find other makers by leader assigned
    if (false == create_other_makers(table_para, cs_para, unitmakers)) {
        xwarn("xtable_maker_t::make_light_table fail-create_other_makers.proposal=%s",
            cs_para.dump().c_str());
        table_result.m_make_block_error_code = xblockmaker_error_tx_check;
        return nullptr;
    }

    // backup should check if origin txs is changed
    if (!is_leader && table_para.get_proposal()->get_input_txs().size() != table_para.get_origin_txs().size()) {
        xwarn("xtable_maker_t::make_light_table fail-verify_proposal txs not match.proposal=%s,origin_txs_size=%zu,actual_txs_size=%zu",
            cs_para.dump().c_str(), table_para.get_origin_txs().size(), table_para.get_proposal()->get_input_txs().size());
        table_result.m_make_block_error_code = xblockmaker_error_tx_check;
        return nullptr;
    }

    auto & receiptid_state = table_para.get_tablestate()->get_receiptid_state();
    receiptid_state->clear_pair_modified();

    int64_t tgas_balance_change = 0;
    std::vector<xblock_ptr_t> batch_units;
    std::vector<xlightunit_tx_info_ptr_t> txs_info;
    uint32_t unchange_tx_only_unit_num = 0;
    // try to make unit for unitmakers
    for (auto & v : unitmakers) {
        xunit_maker_ptr_t & unitmaker = v.second;
        xunitmaker_result_t unit_result;
        xunitmaker_para_t unit_para(table_para.get_tablestate(), is_leader);
        xblock_ptr_t proposal_unit = unitmaker->make_proposal(unit_para, cs_para, unit_result);
        table_result.add_unit_result(unit_result);
        tgas_balance_change += unit_result.m_tgas_balance_change;
        xdbg("total_tgas_balance_change=%lld, change=%lld", tgas_balance_change, unit_result.m_tgas_balance_change);
        if (false == table_para.delete_fail_tx_from_proposal(unit_result.m_fail_txs) ) {
            xerror("xtable_maker_t::make_light_table fail-delete_fail_tx_from_proposal.is_leader=%d,%s,account=%s",
                is_leader, cs_para.dump().c_str(), unitmaker->get_account().c_str());
            return nullptr;
        }
        if (proposal_unit == nullptr) {
            xwarn("xtable_maker_t::make_light_table fail-make unit.is_leader=%d,%s,account=%s", is_leader, cs_para.dump().c_str(), unitmaker->get_account().c_str());
            // if the txs to be packed won't change unit state, then no need to create unit, but table is still needed 
            if (unit_result.m_make_block_error_code == xblockmaker_error_no_need_make_unit) {
                unchange_tx_only_unit_num++;
                for (auto & tx : unit_result.m_unchange_txs) {
                    txs_info.push_back(build_tx_info(tx));
                }
            }
            continue;
        }

        batch_units.push_back(proposal_unit);

        if (proposal_unit->get_block_class() == base::enum_xvblock_class_full || proposal_unit->get_block_class() == base::enum_xvblock_class_nil) {
            table_para.get_proposal()->set_other_account(proposal_unit->get_account());
        }

        for (auto & tx : unit_result.m_pack_txs) {
            txs_info.push_back(build_tx_info(tx));
        }
        for (auto & tx : unit_result.m_unchange_txs) {
            txs_info.push_back(build_tx_info(tx));
        }
    }

    // backup should check if origin txs is changed
    if (!is_leader && table_para.get_proposal()->get_other_accounts().size() != table_para.get_other_accounts().size()) {
        xwarn("xtable_maker_t::make_light_table fail-verify_proposal other accounts.proposal=%s,leader_size=%zu,actual_size=%zu",
            cs_para.dump().c_str(), table_para.get_other_accounts().size(), table_para.get_proposal()->get_other_accounts().size());
        table_result.m_make_block_error_code = xblockmaker_error_tx_check;
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_unit_count, 1);
        return nullptr;
    }

    if (unitmakers.empty() || batch_units.empty()) {
        if (unchange_tx_only_unit_num == 0) {
            table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
            xdbg("xtable_maker_t::make_light_table fail-make table.is_leader=%d,%s,unitmaker_size=%zu",
                is_leader, cs_para.dump().c_str(), unitmakers.size());
            return nullptr;
        }
    }

    base::xreceiptid_check_t receiptid_check;
    xblock_t::txs_to_receiptids(txs_info, receiptid_check);
    if (false == receiptid_check.check_contious(table_para.get_tablestate()->get_receiptid_state())) {
        table_result.m_make_block_error_code = xblockmaker_error_tx_check;
        xerror("xtable_maker_t::make_light_table fail check receiptid contious.is_leader=%d,%s", is_leader, cs_para.dump().c_str());
        return nullptr;
    }

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    xblock_ptr_t lock_block = get_prev_block_from_cache(cert_block);
    if (lock_block == nullptr) {
        xerror("xtable_maker_t::make_light_table fail-get block block.is_leader=%d,%s", is_leader, cs_para.dump().c_str());
        return nullptr;
    }
    cs_para.set_justify_cert_hash(lock_block->get_input_root_hash());
    cs_para.set_parent_height(0);
    xblock_builder_para_ptr_t build_para = std::make_shared<xlighttable_builder_para_t>(batch_units, get_resources(), txs_info);
    build_para->set_tgas_balance_change(tgas_balance_change);
    xblock_ptr_t proposal_block = m_lighttable_builder->build_block(get_highest_height_block(), table_para.get_tablestate()->get_bstate(), cs_para, build_para);
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::leader_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    XMETRICS_TIMER(metrics::cons_make_lighttable_tick);
    return make_light_table(true, table_para, cs_para, table_result);
}

xblock_ptr_t xtable_maker_t::backup_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    XMETRICS_TIMER(metrics::cons_verify_lighttable_tick);
    return make_light_table(false, table_para, cs_para, table_result);
}

xblock_ptr_t xtable_maker_t::make_full_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code) {
    XMETRICS_TIMER(metrics::cons_make_fulltable_tick);
    std::vector<xblock_ptr_t> blocks_from_last_full;
    if (false == load_table_blocks_from_last_full(get_highest_height_block(), blocks_from_last_full)) {
        xerror("xtable_maker_t::make_full_table fail-load blocks. %s", cs_para.dump().c_str());
        return nullptr;
    }

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    xblock_ptr_t lock_block = get_prev_block_from_cache(cert_block);
    if (lock_block == nullptr) {
        xerror("xtable_maker_t::make_full_table fail-get block block.%s", cs_para.dump().c_str());
        return nullptr;
    }
    cs_para.set_justify_cert_hash(lock_block->get_input_root_hash());
    cs_para.set_parent_height(0);
    data::xtablestate_ptr_t tablestate = table_para.get_tablestate();
    xassert(nullptr != tablestate);

    xblock_builder_para_ptr_t build_para = std::make_shared<xfulltable_builder_para_t>(tablestate, blocks_from_last_full, get_resources());
    xblock_ptr_t proposal_block = m_fulltable_builder->build_block(cert_block, table_para.get_tablestate()->get_bstate(), cs_para, build_para);
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::make_empty_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code) {
    // TODO(jimmy)
    XMETRICS_TIME_RECORD("cons_tableblock_verfiy_proposal_imp_make_empty_table");

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    xblock_ptr_t lock_block = get_prev_block_from_cache(cert_block);
    if (lock_block == nullptr) {
        xerror("xtable_maker_t::make_empty_table fail-get block block.%s", cs_para.dump().c_str());
        return nullptr;
    }
    cs_para.set_justify_cert_hash(lock_block->get_input_root_hash());
    cs_para.set_parent_height(0);
    data::xtablestate_ptr_t tablestate = table_para.get_tablestate();
    xassert(nullptr != tablestate);

    xblock_ptr_t proposal_block = m_emptytable_builder->build_block(cert_block, table_para.get_tablestate()->get_bstate(), cs_para, m_default_builder_para);
    return proposal_block;
}

bool    xtable_maker_t::load_table_blocks_from_last_full(const xblock_ptr_t & prev_block, std::vector<xblock_ptr_t> & blocks) {
    std::vector<xblock_ptr_t> _form_highest_blocks;
    xblock_ptr_t current_block = prev_block;
    xassert(current_block->get_height() > 0);
    _form_highest_blocks.push_back(current_block);

    while (current_block->get_block_class() != base::enum_xvblock_class_full && current_block->get_height() > 1) {
        // only mini-block is enough
        base::xauto_ptr<base::xvblock_t> _block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_table);
        if (_block == nullptr) {
            xerror("xfulltable_builder_t::load_table_blocks_from_last_full fail-load block.account=%s,height=%ld", get_account().c_str(), current_block->get_height() - 1);
            return false;
        }
        current_block = xblock_t::raw_vblock_to_object_ptr(_block.get());
        blocks.push_back(current_block);
    }

    // TOOD(jimmy) use map
    for (auto iter = _form_highest_blocks.rbegin(); iter != _form_highest_blocks.rend(); iter++) {
        blocks.push_back(*iter);
    }
    return true;;
}

xblock_ptr_t xtable_maker_t::make_proposal(xtablemaker_para_t & table_para,
                                           const data::xblock_consensus_para_t & cs_para,
                                           xtablemaker_result_t & tablemaker_result) {
    XMETRICS_TIMER(metrics::cons_tablemaker_make_proposal_tick);
    std::lock_guard<std::mutex> l(m_lock);
    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();
    xassert(table_para.get_tablestate() != nullptr);

    int32_t ret = check_latest_state(latest_cert_block);
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::make_proposal fail-check_latest_state. %s", cs_para.dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_table_check_latest_state, 1);
        return nullptr;
    }

    auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
    bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
    xdbg("xtable_maker_t::make_proposal remove empty unit:%d", is_forked);
    bool can_make_empty_table_block = false;
    if (is_forked) {
        xinfo("xtable_maker_t::make_proposal forked,check if can make emtpy table block");
        can_make_empty_table_block = can_make_next_empty_block();
        if (table_para.get_origin_txs().empty() && !can_make_empty_table_block) {
            xinfo("xtable_maker_t::make_proposal no txs and no need make empty block. %s,cert_height=%" PRIu64 "", cs_para.dump().c_str(), latest_cert_block->get_height());
            return nullptr;
        }        
    }

    xblock_ptr_t proposal_block = nullptr;
    if (can_make_next_full_block()) {
        proposal_block = make_full_table(table_para, cs_para, tablemaker_result.m_make_block_error_code);
    } else {
        proposal_block = leader_make_light_table(table_para, cs_para, tablemaker_result);
    }

    if (proposal_block == nullptr) {
        if (tablemaker_result.m_make_block_error_code != xblockmaker_error_no_need_make_table) {
            xwarn("xtable_maker_t::make_proposal fail-make table. %s,error_code=%s",
                cs_para.dump().c_str(), chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
            return nullptr;
        } else {
            if (can_make_empty_table_block) {
                proposal_block = make_empty_table(table_para, cs_para, tablemaker_result.m_make_block_error_code);
                if (proposal_block == nullptr) {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }
    }

    xinfo("xtable_maker_t::make_proposal succ.%s,proposal=%s,tx_count=%zu,other_accounts_count=%zu,remove empty unit forked:%d",
        cs_para.dump().c_str(), proposal_block->dump().c_str(), table_para.get_proposal()->get_input_txs().size(), table_para.get_proposal()->get_other_accounts().size(), is_forked);
    return proposal_block;
}

int32_t xtable_maker_t::verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para) {
    XMETRICS_TIMER(metrics::cons_tablemaker_verify_proposal_tick);
    std::lock_guard<std::mutex> l(m_lock);

    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();  // should update to this new table cert block
    xassert(table_para.get_tablestate() != nullptr);

    int32_t ret = check_latest_state(latest_cert_block);
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::verify_proposal fail-check_latest_state.proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_check_latest_state, 1);
        return ret;
    }

    const auto & highest_block = get_highest_height_block();
    if (proposal_block->get_last_block_hash() != highest_block->get_block_hash()
        || proposal_block->get_height() != highest_block->get_height() + 1) {
        xwarn("xtable_maker_t::verify_proposal fail-proposal unmatch last hash.proposal=%s, last_height=%" PRIu64 "",
            proposal_block->dump().c_str(), highest_block->get_height());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }

    auto lock_block = get_prev_block_from_cache(highest_block);
    if (lock_block == nullptr) {
        xerror("xtable_maker_t::verify_proposal fail-get lock block.proposal=%s, last_height=%" PRIu64 "",
            proposal_block->dump().c_str(), highest_block->get_height());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }
    if (proposal_block->get_cert()->get_justify_cert_hash() != lock_block->get_input_root_hash()) {
        xerror("xtable_maker_t::verify_proposal fail-proposal unmatch justify hash.proposal=%s, last_height=%" PRIu64 ",justify=%s,%s",
            proposal_block->dump().c_str(), highest_block->get_height(),
            base::xstring_utl::to_hex(proposal_block->get_cert()->get_justify_cert_hash()).c_str(),
            base::xstring_utl::to_hex(lock_block->get_input_root_hash()).c_str());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }

    xblock_ptr_t local_block = nullptr;
    xtablemaker_result_t table_result;
    if (can_make_next_full_block()) {
        local_block = make_full_table(table_para, cs_para, table_result.m_make_block_error_code);
    } else if (proposal_block->get_block_class() == base::enum_xvblock_class_nil) {
        auto fork_config = top::chain_fork::xtop_chain_fork_config_center::chain_fork_config();
        bool is_forked = chain_fork::xtop_chain_fork_config_center::is_forked(fork_config.block_fork_point, cs_para.get_clock());
        xdbg("xtable_maker_t::verify_proposal remove empty unit:%d", is_forked);
        if (is_forked) {
            local_block = make_empty_table(table_para, cs_para, table_result.m_make_block_error_code);
        } else {
            return xblockmaker_error_proposal_bad_consensus_para;
        }
    } else {
        local_block = backup_make_light_table(table_para, cs_para, table_result);
    }
    if (local_block == nullptr) {
        xwarn("xtable_maker_t::verify_proposal fail-make table. proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(table_result.m_make_block_error_code).c_str());
        return table_result.m_make_block_error_code;
    }

    local_block->get_cert()->set_nonce(proposal_block->get_cert()->get_nonce());  // TODO(jimmy)
    // check local and proposal is match
    if (false == verify_proposal_with_local(proposal_block, local_block.get())) {
        xwarn("xtable_maker_t::verify_proposal fail-verify_proposal_with_local. proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_with_local, 1);
        return xblockmaker_error_proposal_not_match_local;
    }

    xinfo("xtable_maker_t::verify_proposal succ.proposal=%s",
        proposal_block->dump().c_str());
    return xsuccess;
}

bool xtable_maker_t::verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const {
    const std::vector<base::xventity_t*> & _proposal_table_inentitys = proposal_block->get_input()->get_entitys();
    const std::vector<base::xventity_t*> & _local_table_inentitys = local_block->get_input()->get_entitys();
    if (_proposal_table_inentitys.size() != _local_table_inentitys.size()) {
        xwarn("xtable_maker_t::verify_proposal_with_local fail-entity size not same. %s %s",
            proposal_block->dump().c_str(),
            local_block->dump().c_str());
        return false;
    }
    size_t entity_count = _proposal_table_inentitys.size();
    for (size_t i = 0; i < entity_count; i++) {
        base::xvinentity_t* _proposal_inentity = dynamic_cast<base::xvinentity_t*>(_proposal_table_inentitys[i]);
        base::xvinentity_t* _local_inentity = dynamic_cast<base::xvinentity_t*>(_local_table_inentitys[i]);
        if (_proposal_inentity == nullptr || _local_inentity == nullptr) {
            xerror("xproposal_maker_t::verify_proposal_input fail-get inentity. %s", proposal_block->dump().c_str());  // should never happen
            return false;
        }
        if (i == 0) {  // entity#0 used for table self
            if (_proposal_inentity->get_extend_data() != _local_inentity->get_extend_data()) {
                xerror("xproposal_maker_t::verify_proposal_input fail-get inentity. %s", proposal_block->dump().c_str());  // should never happen
                return false;
            }
            continue;
        }

        base::xtable_inentity_extend_t _proposal_extend;
        _proposal_extend.serialize_from_string(_proposal_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _proposal_unit_header = _proposal_extend.get_unit_header();
        base::xtable_inentity_extend_t _local_extend;
        _local_extend.serialize_from_string(_local_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _local_unit_header = _local_extend.get_unit_header();
        if (_proposal_unit_header->get_account() != _local_unit_header->get_account()
            || _proposal_unit_header->get_height() != _local_unit_header->get_height()
            || _proposal_unit_header->get_block_class() != _local_unit_header->get_block_class()) {
            xerror("xtable_maker_t::verify_proposal_with_local fail-unit entity not match. %s,leader=%s,local=%s",
                proposal_block->dump().c_str(),
                data::xblock_t::dump_header(_proposal_unit_header.get()).c_str(), data::xblock_t::dump_header(_local_unit_header.get()).c_str());
            return false;
        }
        if (_proposal_inentity->get_extend_data() != _local_inentity->get_extend_data()
         || _proposal_inentity->get_actions().size() != _local_inentity->get_actions().size()) {
            xwarn("xtable_maker_t::verify_proposal_with_local fail-extend data not match. %s,leader=%s,local=%s,action_size=%zu,%zu",
                proposal_block->dump().c_str(),
                data::xblock_t::dump_header(_proposal_unit_header.get()).c_str(), data::xblock_t::dump_header(_local_unit_header.get()).c_str(),
                _proposal_inentity->get_actions().size(), _local_inentity->get_actions().size());
            return false;
        }
    }

    if (local_block->get_input_hash() != proposal_block->get_input_hash()) {
        xwarn("xtable_maker_t::verify_proposal_with_local fail-input hash not match. %s %s",
            proposal_block->dump().c_str(),
            local_block->dump().c_str());
        return false;
    }
    if (local_block->get_output_hash() != proposal_block->get_output_hash()) {
        xwarn("xtable_maker_t::verify_proposal_with_local fail-output hash not match. %s %s",
            proposal_block->dump().c_str(),
            local_block->dump().c_str());
        return false;
    }
    if (local_block->get_header_hash() != proposal_block->get_header_hash()) {
        xwarn("xtable_maker_t::verify_proposal_with_local fail-header hash not match. %s proposal:%s local:%s",
            proposal_block->dump().c_str(),
            ((data::xblock_t*)proposal_block)->dump_header().c_str(),
            ((data::xblock_t*)local_block)->dump_header().c_str());
        return false;
    }
    if (local_block->get_cert()->get_hash_to_sign() != proposal_block->get_cert()->get_hash_to_sign()) {
        xerror("xtable_maker_t::verify_proposal_with_local fail-cert hash not match. proposal:%s local:%s",
            ((data::xblock_t*)proposal_block)->dump_cert().c_str(),
            ((data::xblock_t*)local_block)->dump_cert().c_str());
        return false;
    }
    bool bret = proposal_block->set_output_resources(local_block->get_output()->get_resources_data());
    if (!bret) {
        xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block output fail");
        return false;
    }
    bret = proposal_block->set_input_resources(local_block->get_input()->get_resources_data());
    if (!bret) {
        xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block input fail");
        return false;
    }
    return true;
}

bool xtable_maker_t::verify_proposal_class(base::xvblock_t *proposal_block) const {
    bool can_make_full = can_make_next_full_block();
    if (can_make_full && (proposal_block->get_block_class() == base::enum_xvblock_class_full)) {
        return true;
    } else if (!can_make_full && proposal_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    return false;
}

std::string xtable_maker_t::dump() const {
    char local_param_buf[256];
    uint64_t highest_height = get_highest_height_block()->get_height();
    uint32_t unitmaker_size = m_unit_makers.size();
    uint32_t total_units_size = 0;
    for (auto & v : m_unit_makers) {
        total_units_size += v.second->get_latest_blocks().size();
    }
    xprintf(local_param_buf,sizeof(local_param_buf),"{highest=%" PRIu64 ",unitmakers=%d,total_units=%d}",
        highest_height, unitmaker_size, total_units_size);
    return std::string(local_param_buf);
}

bool xtable_maker_t::can_make_next_empty_block() const {
    const xblock_ptr_t & current_block = get_highest_height_block();
    if (current_block->get_height() == 0) {
        return false;
    }
    if (current_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    xblock_ptr_t prev_block = get_prev_block_from_cache(current_block);
    if (prev_block == nullptr) {
        return false;
    }
    if (prev_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    return false;
}

NS_END2
