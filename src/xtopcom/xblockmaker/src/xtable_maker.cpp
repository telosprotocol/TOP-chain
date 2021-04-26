// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xtable_maker.h"
#include "xblockmaker/xtable_builder.h"
#include "xdata/xblocktool.h"
#include "xindexstore/xindexstore_face.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"

NS_BEG2(top, blockmaker)

xtable_maker_t::xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
: xblock_maker_t(account, resources, m_keep_latest_blocks_max) {
    m_fulltable_builder = std::make_shared<xfulltable_builder_t>();
    m_lighttable_builder = std::make_shared<xlighttable_builder_t>();
    m_indexstore = resources->get_indexstorehub()->get_index_store(account);
    m_full_table_interval_num = XGET_CONFIG(fulltable_interval_block_num);
}

bool xtable_maker_t::unpack_table_and_update_units(const xblock_ptr_t & block) {
    if (block->get_block_class() == base::enum_xvblock_class_light) {
        const std::vector<xblock_ptr_t> & units = block->get_tableblock_units(true);
        xassert(!units.empty());
        for (auto & unit : units) {
            xunit_maker_ptr_t unitmaker = create_unit_maker(unit->get_account());
            unitmaker->set_latest_block(unit);
        }
    }
    return true;
}
bool xtable_maker_t::update_latest_blocks(const xblock_ptr_t & latest_block) {
    // load new latest blocks
    std::map<uint64_t, xblock_ptr_t> latest_blocks;
    if (false == load_latest_blocks(latest_block, latest_blocks)) {
        xwarn("xtable_maker_t::update_latest_blocks fail-load_latest_blocks.account=%s,latest_block=%s",
            get_account().c_str(), latest_block->dump().c_str());
        return false;
    }
    xassert(!latest_blocks.empty());

    // set table new latest blocks, delete old table block
    uint32_t delete_count = 0;
    for (auto & v : latest_blocks) {
        xblock_ptr_t & _new_block = v.second;
        xblock_ptr_t _delete_block = set_latest_block(_new_block);
        if (_delete_block->get_height() == _new_block->get_height()
            && _delete_block->get_block_hash() != _new_block->get_block_hash()) {
            // clear forked table and related units
            xdbg("xtable_maker_t::update_latest_blocks rollback block %s", _delete_block->dump().c_str());
            clear_table_units(_delete_block);
            delete_count++;
        }
    }
    for (auto & v : latest_blocks) {
        xblock_ptr_t & _new_block = v.second;
        unpack_table_and_update_units(_new_block);
    }

    xdbg("xtable_maker_t::update_latest_blocks latest_block=%s add_count=%d,delete_count=%d",
        latest_block->dump().c_str(), latest_blocks.size(), delete_count);
    xassert(!get_latest_blocks().empty());
    return true;
}

void xtable_maker_t::clear_table_units(const xblock_ptr_t & block) {
    if (block->get_block_class() == base::enum_xvblock_class_light) {
        const std::vector<xblock_ptr_t> & units = block->get_tableblock_units(true);
        xassert(!units.empty());
        for (auto & unit : units) {
            xunit_maker_ptr_t unitmaker = get_unit_maker(unit->get_account());
            xassert(unitmaker != nullptr);
            if (unitmaker != nullptr) {
                unitmaker->clear_block(unit);
            }
        }
    }
}

int32_t xtable_maker_t::default_check_latest_state() {
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*this);
    if (!is_latest_blocks_valid(latest_blocks)) {
        xwarn("xtable_maker_t::default_check_latest_state fail-is_latest_blocks_valid.latest_cert_block=%s",
            latest_blocks.get_latest_cert_block()->dump().c_str());
        return xblockmaker_error_latest_table_blocks_invalid;
    }
    xblock_ptr_t cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
    return check_latest_state(cert_block);
}

int32_t xtable_maker_t::check_latest_state(const xblock_ptr_t & latest_block) {
    xassert(latest_block != nullptr);
    // use this latest cert block to update latest blocks
    if (is_latest_state_unchanged(latest_block)) {
        return xsuccess;
    }
    m_check_state_success = false;
    if (!update_latest_blocks(latest_block)) {
        xwarn("xtable_maker_t::check_latest_state fail-update_latest_blocks.latest_cert_block=%s",
            latest_block->dump().c_str());
        return xblockmaker_error_latest_table_blocks_invalid;  // TODO(jimmy) error_code
    }

    // update latest committed block and blockchain state
    xblock_ptr_t latest_committed_block = get_highest_commit_block();
    set_latest_committed_block(latest_committed_block);
    clear_old_unit_makers();

    if (false == check_latest_blocks()) {
        xerror("xtable_maker_t::check_latest_state fail-check_latest_blocks.latest_block=%s",
            latest_block->dump().c_str());
        return xblockmaker_error_latest_table_blocks_invalid;
    }

    xdbg_info("xtable_maker_t::check_latest_state finish.latest_cert_block=%s,unitmaker_size=%d",
        latest_block->dump().c_str(), m_unit_makers.size());
    return xsuccess;
}

xunit_maker_ptr_t xtable_maker_t::get_unit_maker(const std::string & account) {
    auto iter = m_unit_makers.find(account);
    if (iter == m_unit_makers.end()) {
        return nullptr;
    }
    return iter->second;
}

xunit_maker_ptr_t xtable_maker_t::pop_unit_maker(const std::string & account) {
    auto iter = m_unit_makers.find(account);
    if (iter != m_unit_makers.end()) {
        xunit_maker_ptr_t unitmaker = iter->second;
        m_unit_makers.erase(iter);
        return unitmaker;
    }
    return nullptr;
}

void xtable_maker_t::set_unit_maker(const xunit_maker_ptr_t & unitmaker) {
    auto iter = m_unit_makers.find(unitmaker->get_account());
    if (iter != m_unit_makers.end()) {
        return;
    }
    xdbg("xtable_maker_t::set_unit_maker unit_maker_changed add. account=%s",
        unitmaker->get_account().c_str());
    m_unit_makers[unitmaker->get_account()] = unitmaker;
}

xunit_maker_ptr_t xtable_maker_t::create_unit_maker(const std::string & account) {
    auto iter = m_unit_makers.find(account);
    if (iter == m_unit_makers.end()) {
        xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account, get_resources(), m_indexstore);
        m_unit_makers[account] = unitmaker;
        xdbg("xtable_maker_t::create_unit_maker unit_maker_changed add. account=%s",
            unitmaker->get_account().c_str());
        return unitmaker;
    }
    return iter->second;
}

void xtable_maker_t::clear_old_unit_makers() {
    uint64_t lowest_table_height = get_lowest_height_block()->get_height();
    for (auto iter = m_unit_makers.begin(); iter != m_unit_makers.end();) {
        auto & unitmaker = iter->second;
        if (unitmaker->get_latest_blocks().empty()) {
            xdbg("xtable_maker_t::clear_old_unit_makers unit_maker_changed delete empty account. account=%s,table_lowest_height=%" PRIu64 "",
                unitmaker->get_account().c_str(), lowest_table_height);
            iter = m_unit_makers.erase(iter);
            continue;
        }

        // TODO(jimmy) cache clear rules update
        uint64_t highest_unit_parent_height = unitmaker->get_highest_height_block()->get_parent_block_height();
        if (highest_unit_parent_height < lowest_table_height) {
            xdbg("xtable_maker_t::clear_old_unit_makers unit_maker_changed delete old account. account=%s,height=%" PRIu64 ",highest_unit_parent_height=%" PRIu64 ",table_lowest_height=%" PRIu64 "",
                unitmaker->get_account().c_str(), unitmaker->get_highest_height_block()->get_height(), highest_unit_parent_height, lowest_table_height);
            iter = m_unit_makers.erase(iter);
            continue;
        }
        iter++;
    }
}

bool xtable_maker_t::can_make_next_empty_block() const {
    // TODO(jimmy)
    for (auto & v : m_unit_makers) {
        const xunit_maker_ptr_t & unitmaker = v.second;
        if (unitmaker->can_make_next_empty_block()) {
            return true;
        }
    }
    return false;
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
    return !table_para.m_proposal_input.get_unit_inputs().empty();
}

bool xtable_maker_t::is_latest_state_unchanged(const xblock_ptr_t & latest_block) const {
    if ( m_check_state_success && latest_block->get_block_hash() == get_highest_height_block()->get_block_hash()) {
        // already latest state
        return true;
    }
    return false;
}

xblock_ptr_t xtable_maker_t::leader_make_unit(const xunit_maker_ptr_t & unitmaker, const xunit_proposal_input_t & unit_input, const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & unit_result) {
    int32_t ret = unitmaker->check_latest_state(get_latest_committed_block());
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::leader_make_unit fail-unit check_latest_state,%s,account=%s,error_code=%s",
            cs_para.dump().c_str(), unitmaker->get_account().c_str(), chainbase::xmodule_error_to_str(ret).c_str());
        return nullptr;
    }
    xblock_ptr_t proposal_unit = unitmaker->make_proposal(unit_input, cs_para, unit_result);
    return proposal_unit;
}

xblock_ptr_t xtable_maker_t::leader_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    const xtableblock_proposal_input_t & proposal_input = table_para.m_proposal_input;
    std::vector<xblock_ptr_t> batch_units;
    // try to make non-empty-unit for left unitmakers
    std::map<std::string, xunit_maker_ptr_t>  txs_unitmakers;
    const std::vector<xunit_proposal_input_t> & unit_inputs = proposal_input.get_unit_inputs();
    for (auto & v : unit_inputs) {
        const xunit_proposal_input_t & unit_input = v;
        const std::string & unit_account = unit_input.get_account();
        xunit_maker_ptr_t unitmaker = pop_unit_maker(unit_account);
        if (unitmaker != nullptr) {
            txs_unitmakers[unit_account] = unitmaker;
        } else {
            unitmaker = make_object_ptr<xunit_maker_t>(unit_account, get_resources(), m_indexstore);
        }

        xunitmaker_result_t unit_result;
        unit_result.m_tablestate = table_para.m_tablestate;
        xblock_ptr_t proposal_unit = leader_make_unit(unitmaker, unit_input, cs_para, unit_result);
        table_result.m_unit_results.push_back(unit_result);
        if (proposal_unit != nullptr) {
            batch_units.push_back(proposal_unit);
        } else {
            // recv and confirm should always make unit success
            if (unit_input.has_recv_or_confirm_tx()) {
                xwarn("xtable_maker_t::leader_make_light_table fail-make unit. %s, account=%s", cs_para.dump().c_str(), unit_account.c_str());
                return nullptr;
            }
        }
    }

    // try to make empty-unit for left unitmakers
    for (auto & v : m_unit_makers) {
        xunit_maker_ptr_t & unitmaker = v.second;
        xunitmaker_result_t unit_result;
        xunit_proposal_input_t unit_input;
        // if (!unitmaker->can_make_next_empty_block()) {
        //     continue;
        // }
        unit_result.m_tablestate = table_para.m_tablestate;
        xblock_ptr_t proposal_unit = leader_make_unit(unitmaker, unit_input, cs_para, unit_result);
        table_result.m_unit_results.push_back(unit_result);
        if (proposal_unit != nullptr) {
            batch_units.push_back(proposal_unit);
        }
    }

    // push back unitmakers
    for (auto & v : txs_unitmakers) {
        set_unit_maker(v.second);
    }

    if (batch_units.empty()) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xdbg("xtable_maker_t::leader_make_light_table fail-make table. %s",
            cs_para.dump().c_str());
        return nullptr;
    }

// TODO(jimmy) move to block rules
    base::xreceiptid_check_t receiptid_check;
    xblock_t::batch_units_to_receiptids(batch_units, receiptid_check);
    if (false == receiptid_check.check_contious(table_para.m_tablestate->get_receiptid_state())) {
        table_result.m_make_block_error_code = xblockmaker_error_receiptid_check;
        xwarn("xtablestate_t receiptid binlog=%s", table_para.m_tablestate->get_receiptid_state()->get_binlog()->dump().c_str());
        xwarn("xtablestate_t receiptid full=%s", table_para.m_tablestate->get_receiptid_state()->get_last_full_state()->dump().c_str());
        xwarn("xtablestate_t receiptid check=%s", receiptid_check.dump().c_str());
        xerror("xtablestate_t::execute_lighttable fail check receiptid contious. %s", cs_para.dump().c_str());
        return nullptr;
    }

    cs_para.set_justify_cert_hash(get_lock_output_root_hash());
    cs_para.set_parent_height(0);
    xblock_builder_para_ptr_t build_para = std::make_shared<xlighttable_builder_para_t>(batch_units, get_resources());
    xblock_ptr_t proposal_block = m_lighttable_builder->build_block(get_highest_height_block(), nullptr, cs_para, build_para);
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::backup_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    const xtableblock_proposal_input_t & proposal_input = table_para.m_proposal_input;
    const std::vector<xunit_proposal_input_t> & unit_inputs = proposal_input.get_unit_inputs();
    if (unit_inputs.empty()) {
        xassert(false);
        return nullptr;
    }

    std::vector<xblock_ptr_t> batch_units;
    for (auto & v : proposal_input.get_unit_inputs()) {
        const xunit_proposal_input_t & unit_input = v;
        const std::string & unit_account = unit_input.get_account();
        xunit_maker_ptr_t unitmaker = get_unit_maker(unit_account);
        if (unitmaker == nullptr) {
            unitmaker = make_object_ptr<xunit_maker_t>(unit_account, get_resources(), m_indexstore);
        }
        int32_t ret = unitmaker->check_latest_state(get_latest_committed_block());
        if (ret != xsuccess) {
            xwarn("xtable_maker_t::backup_make_light_table fail-unit check_latest_state,%s,account=%s,error_code=%s",
                cs_para.dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            table_result.m_make_block_error_code = xblockmaker_error_missing_state;
            return nullptr;
        }

        xunitmaker_result_t unit_result;
        unit_result.m_tablestate = table_para.m_tablestate;
        xblock_ptr_t proposal_unit = unitmaker->verify_proposal(unit_input, cs_para, unit_result);
        if (proposal_unit == nullptr) {
            xwarn("xtable_maker_t::backup_make_light_table fail-unit verify_proposal.proposal=%s,account=%s,error_code=%s",
                cs_para.dump().c_str(), unit_account.c_str(), chainbase::xmodule_error_to_str(unit_result.m_make_block_error_code).c_str());
            table_result.m_make_block_error_code = unit_result.m_make_block_error_code;
            return nullptr;
        }
        batch_units.push_back(proposal_unit);
    }

    if (batch_units.empty()) {
        table_result.m_make_block_error_code = xblockmaker_error_proposal_bad_input;
        xwarn("xtable_maker_t::backup_make_light_table fail-no unit. %s",
            cs_para.dump().c_str());
        return nullptr;
    }

    cs_para.set_justify_cert_hash(get_lock_output_root_hash());
    cs_para.set_parent_height(0);
    xblock_builder_para_ptr_t build_para = std::make_shared<xlighttable_builder_para_t>(batch_units, get_resources());
    xblock_ptr_t proposal_block = m_lighttable_builder->build_block(get_highest_height_block(), nullptr, cs_para, build_para);
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::make_full_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code) {
    // TODO(jimmy)
    std::vector<xblock_ptr_t> blocks_from_last_full;
    if (false == load_table_blocks_from_last_full(get_highest_height_block(), blocks_from_last_full)) {
        xerror("xtable_maker_t::make_full_table fail-load blocks. %s", cs_para.dump().c_str());
        return nullptr;
    }

    cs_para.set_justify_cert_hash(get_lock_output_root_hash());
    cs_para.set_parent_height(0);
    data::xtablestate_ptr_t tablestate = table_para.m_tablestate;
    xassert(nullptr != tablestate);
    xassert(get_highest_height_block()->get_height() == tablestate->get_binlog_height());

    xblock_builder_para_ptr_t build_para = std::make_shared<xfulltable_builder_para_t>(tablestate, blocks_from_last_full, get_resources());
    xblock_ptr_t proposal_block = m_fulltable_builder->build_block(get_highest_height_block(), nullptr, cs_para, build_para);
    return proposal_block;
}

bool    xtable_maker_t::load_table_blocks_from_last_full(const xblock_ptr_t & prev_block, std::vector<xblock_ptr_t> & blocks) {
    std::vector<xblock_ptr_t> _form_highest_blocks;
    xblock_ptr_t current_block = prev_block;
    xassert(current_block->get_height() > 0);
    _form_highest_blocks.push_back(current_block);

    while (current_block->get_block_class() != base::enum_xvblock_class_full && current_block->get_height() > 1) {
        base::xauto_ptr<base::xvblock_t> _block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), true);
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

bool xtable_maker_t::can_make_next_block(xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para) {
    std::lock_guard<std::mutex> l(m_lock);
    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();
    xassert(latest_cert_block != nullptr);

    int32_t ret = check_latest_state(latest_cert_block);
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::can_make_next_block fail-check_latest_state. %s", cs_para.dump().c_str());
        return false;
    }

#if 0
    if (can_make_next_light_block(table_para) || can_make_next_empty_block() || can_make_next_full_block()) {
        return true;
    }
    return false;
#else
    return true;  // TODO(jimmy) always need try
#endif
}

xblock_ptr_t xtable_maker_t::make_proposal(xtablemaker_para_t & table_para,
                                           const data::xblock_consensus_para_t & cs_para,
                                           xtablemaker_result_t & tablemaker_result) {
    std::lock_guard<std::mutex> l(m_lock);
    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();

    if (get_latest_blocks().empty()) {
        int32_t ret = check_latest_state(latest_cert_block);
        if (ret != xsuccess) {
            xwarn("xtable_maker_t::can_make_next_block fail-check_latest_state. %s", cs_para.dump().c_str());
            return nullptr;
        }
    }

    if (get_highest_height_block()->get_block_hash() != latest_cert_block->get_block_hash()) {
        xwarn("xtable_maker_t::make_proposal fail-highest block changed. %s", cs_para.dump().c_str());
        return nullptr;
    }

    xblock_ptr_t proposal_block = nullptr;
    if (can_make_next_full_block()) {
        proposal_block = make_full_table(table_para, cs_para, tablemaker_result.m_make_block_error_code);
    } else {
        proposal_block = leader_make_light_table(table_para, cs_para, tablemaker_result);
    }

    if (proposal_block == nullptr) {
        xwarn("xtable_maker_t::make_proposal fail-make table. %s,error_code=%s",
            cs_para.dump().c_str(), chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
        return nullptr;
    }

    // update proposal input, collect the successful units infomation
    table_para.m_proposal_input.clear();
    uint32_t unit_count = 0;
    uint32_t tx_count = 0;
    for (auto & unitmaker_result : tablemaker_result.m_unit_results) {
        if (unitmaker_result.m_block != nullptr) {
            unit_count++;
            xunit_proposal_input_t unit_proposal_input;
            auto & unit = unitmaker_result.m_block;
            unit_proposal_input.set_basic_info(unit->get_account(), unit->get_height() - 1, unit->get_last_block_hash());
            unit_proposal_input.set_input_txs(unitmaker_result.m_success_txs);
            table_para.m_proposal_input.add_unit_input(unit_proposal_input);
        }
    }
    xinfo("xtable_maker_t::make_proposal succ.proposal=%s,class=%d,unit_count=%d,tx_count=%d,maker=%s",
        proposal_block->dump().c_str(), proposal_block->get_block_class(), unit_count, tx_count, dump().c_str());
    return proposal_block;
}

int32_t xtable_maker_t::verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para) {
    std::lock_guard<std::mutex> l(m_lock);

    xassert(cs_para.get_latest_cert_block() != nullptr);
    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();
    xassert(proposal_block->get_last_block_hash() == latest_cert_block->get_block_hash());
    xassert(proposal_block->get_height() == latest_cert_block->get_height() + 1);

    int32_t ret = check_latest_state(latest_cert_block);
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::verify_proposal fail-check_latest_state.proposal=%s",
            proposal_block->dump().c_str());
        return ret;
    }

    const auto & highest_block = get_highest_height_block();
    if (proposal_block->get_last_block_hash() != highest_block->get_block_hash()
        || proposal_block->get_height() != highest_block->get_height() + 1) {
        xwarn("xtable_maker_t::verify_proposal fail-proposal unmatch last hash.proposal=%s, last_height=%" PRIu64 "",
            proposal_block->dump().c_str(), highest_block->get_height());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }

    if (proposal_block->get_cert()->get_justify_cert_hash() != get_lock_output_root_hash()) {
        xerror("xtable_maker_t::verify_proposal fail-proposal unmatch justify hash.proposal=%s, last_height=%" PRIu64 ",justify=%s,%s",
            proposal_block->dump().c_str(), highest_block->get_height(),
            base::xstring_utl::to_hex(proposal_block->get_cert()->get_justify_cert_hash()).c_str(),
            base::xstring_utl::to_hex(get_lock_output_root_hash()).c_str());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }

    xblock_ptr_t local_block = nullptr;
    xtablemaker_result_t table_result;
    if (can_make_next_full_block()) {
        local_block = make_full_table(table_para, cs_para, table_result.m_make_block_error_code);
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
        xwarn("xtable_maker_t::verify_proposal fail-make local block. proposal=%s,unit_counts=%d,%d,maker=%s,",
            proposal_block->dump().c_str(),
            table_para.m_proposal_input.get_unit_inputs().size(), table_result.m_unit_results.size(),
            dump().c_str());
        return xblockmaker_error_proposal_not_match_local;
    }

    xinfo("xtable_maker_t::verify_proposal succ.proposal=%s,maker=%s",
        proposal_block->dump().c_str(), dump().c_str());
    return xsuccess;
}

bool xtable_maker_t::verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const {
    // TODO(jimmy) input hash not match may happen when use different property
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
        xerror("xtable_maker_t::verify_proposal_with_local fail-header hash not match. %s proposal:%s local:%s",
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
    uint64_t lowest_height = get_lowest_height_block()->get_height();
    uint32_t unitmaker_size = m_unit_makers.size();
    uint32_t total_units_size = 0;
    for (auto & v : m_unit_makers) {
        total_units_size += v.second->get_latest_blocks().size();
    }
    xprintf(local_param_buf,sizeof(local_param_buf),"{highest=%" PRIu64 ",lowest=%" PRIu64 ",unitmakers=%d,total_units=%d}",
        highest_height, lowest_height, unitmaker_size, total_units_size);
    return std::string(local_param_buf);
}


NS_END2
