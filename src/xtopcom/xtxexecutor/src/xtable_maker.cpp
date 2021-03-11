// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xtxexecutor/xtable_maker.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xdata/xblocktool.h"

NS_BEG2(top, txexecutor)

xtable_maker_t::xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
: xblock_maker_t(account, resources, m_keep_latest_blocks_max) {

}

int32_t xtable_maker_t::update_full_state(const xblock_ptr_t & latest_committed_block) {
    uint64_t last_full_height = latest_committed_block->get_block_class() == base::enum_xvblock_class_full ?
        latest_committed_block->get_height() : latest_committed_block->get_last_full_block_height();

    if (m_last_full_block != nullptr && m_last_full_block->get_height() == last_full_height) {
        return xsuccess;
    }

    if (last_full_height != 0) {
        base::xauto_ptr<base::xvblock_t> latest_full_block = get_blockstore()->load_block_object(*this, last_full_height);
        if (latest_full_block == nullptr) {
            xerror("xtable_maker_t::update_full_state fail-latest full block not match. account=%s,full_height=%ld",
                get_account().c_str(), last_full_height);
            return enum_xtxexecutor_error_leader_tableblock_behind;
        }

        data::xblock_t* full_block_ptr = dynamic_cast<data::xblock_t*>(latest_full_block.get());
        const xdataunit_ptr_t & full_offstate = full_block_ptr->get_full_offstate();
        if (full_offstate == nullptr) {
            xerror("xtable_maker_t::update_full_state fail-load offstate from block. account=%s,full_height=%ld",
                get_account().c_str(), last_full_height);
            return enum_xtxexecutor_error_leader_tableblock_behind;
        }
        full_offstate->add_ref();
        m_last_full_table_mbt.attach((data::xtable_mbt_t*)full_offstate.get());

        latest_full_block->add_ref();
        m_last_full_block.attach((data::xblock_t*)latest_full_block.get());
    } else {
        m_last_full_table_mbt = make_object_ptr<xtable_mbt_t>();
    }

    m_commit_table_mbt_binlog = get_latest_committed_state()->get_table_mbt_binlog();
    return xsuccess;
}

bool xtable_maker_t::unpack_table_and_update_units(const xblock_ptr_t & block) {
    xdbg("xtable_maker_t::unpack_table_and_update_units block=%s,class=%d", block->dump().c_str(), block->get_block_class());
    if (block->get_block_class() == base::enum_xvblock_class_light) {
        const std::vector<xblock_ptr_t> & units = block->get_tableblock_units(true);
        xassert(!units.empty());
        for (auto & unit : units) {
            xunit_maker_ptr_t unitmaker = create_unit_maker(unit->get_account());
            xassert(unitmaker->check_latest_state() == xsuccess);
            unitmaker->set_latest_block(unit);
        }
    }
    return true;
}
bool xtable_maker_t::update_latest_blocks(const xblock_ptr_t & latest_block) {
    // load new latest blocks
    std::map<uint64_t, xblock_ptr_t> latest_blocks;
    if (false == load_latest_blocks(latest_block, latest_blocks)) {
        xerror("xtable_maker_t::update_latest_blocks fail-load_latest_blocks.account=%s,latest_block=%s",
            get_account().c_str(), latest_block->dump().c_str());
        return false;
    }
    xdbg("xtable_maker_t::update_latest_blocks succ-load_latest_blocks.account=%s,latest_block=%s,latest_blocks size=%d",
        get_account().c_str(), latest_block->dump().c_str(), latest_blocks.size());
    xassert(!latest_blocks.empty());

    // set new latest blocks and delete old block
    for (auto & v : latest_blocks) {
        xblock_ptr_t & _new_block = v.second;
        xblock_ptr_t _delete_block = set_latest_block(_new_block);
        if (_delete_block->get_block_hash() != _new_block->get_block_hash()) {
            clear_table_units(_delete_block);
        }
    }
    for (auto & v : latest_blocks) {
        xblock_ptr_t & _new_block = v.second;
        unpack_table_and_update_units(_new_block);
    }

    xdbg("xtable_maker_t::update_latest_blocks latest_block=%s", latest_block->dump().c_str());
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

int32_t xtable_maker_t::set_table_latest_state(base::xvblock_t* _latest_committed_block) {
    xblock_ptr_t latest_committed_block = xblock_t::raw_vblock_to_object_ptr(_latest_committed_block);
    if (!update_latest_state(latest_committed_block)) {
        return enum_xtxexecutor_error_unit_state_behind;
    }

    int32_t ret = update_full_state(latest_committed_block);
    if (ret) {
        return ret;
    }
    return xsuccess;
}

int32_t xtable_maker_t::default_check_latest_state() {
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*this);
    return check_latest_state(latest_blocks.get_latest_cert_block());
}

int32_t xtable_maker_t::check_latest_state(base::xvblock_t* latest_cert_block) {
    xassert(latest_cert_block != nullptr);
    if (is_latest_state_unchanged(latest_cert_block)) {
        xdbg("xtable_maker_t::check_latest_state unchanged.latest_cert_block=%s", latest_cert_block->dump().c_str());
        return xsuccess;
    }
    // TODO(jimmy) check again
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*this);
    if (!is_latest_blocks_valid(latest_blocks)) {
        xwarn("xtable_maker_t::check_latest_state fail-is_latest_blocks_valid.latest_cert_block=%s",
            latest_cert_block->dump().c_str());
        return enum_xtxexecutor_error_unit_state_behind;
    }
    if (latest_blocks.get_latest_cert_block()->get_height() != latest_cert_block->get_height()) {
        xwarn("xtable_maker_t::check_latest_state fail-latest cert height not match.latest_cert_block=%s",
            latest_cert_block->dump().c_str());
        return enum_xtxexecutor_error_unit_state_behind;
    }

    // use this latest cert block
    xblock_ptr_t latest_block = xblock_t::raw_vblock_to_object_ptr(latest_cert_block);
    if (!update_latest_blocks(latest_block)) {
        xwarn("xtable_maker_t::check_latest_state fail-update_latest_blocks.latest_cert_block=%s",
            latest_block->dump().c_str());
        return enum_xtxexecutor_error_unit_state_behind;
    }

    int32_t ret = set_table_latest_state(latest_blocks.get_latest_committed_block());
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::check_latest_state fail-set_table_latest_state.latest_cert_block=%s",
            latest_block->dump().c_str());
        return ret;
    }

    if (!update_unit_makers()) {
        xwarn("xtable_maker_t::check_latest_state fail-update_unit_makers.latest_cert_block=%s",
            latest_block->dump().c_str());
        return enum_xtxexecutor_error_unit_state_behind;
    }

#if 1  // TODO(jimmy)
    for (auto & v : m_unit_makers) {
        xassert(v.second->get_latest_blocks().size() > 0);
    }
#endif

    xdbg_info("xtable_maker_t::check_latest_state latest_cert_block=%s,unitmaker_size=%d",
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

xunit_maker_ptr_t xtable_maker_t::create_unit_maker(const std::string & account) {
    auto iter = m_unit_makers.find(account);
    if (iter == m_unit_makers.end()) {
        xunit_maker_ptr_t unitmaker = make_object_ptr<xunit_maker_t>(account, get_resources());
        m_unit_makers[account] = unitmaker;
        return unitmaker;
    }
    return iter->second;
}

bool xtable_maker_t::push_tx(const std::string & account, const std::vector<xcons_transaction_ptr_t> & txs) {
    xunit_maker_ptr_t unitmaker = get_unit_maker(account);
    if (unitmaker == nullptr) {
        unitmaker = make_object_ptr<xunit_maker_t>(account, get_resources());
        int32_t ret = unitmaker->check_latest_state();
        if (xsuccess != ret) {
            xerror("xtable_maker_t::get_unit_maker account=%s,fail-check latest state, error_code=%s",
                account.c_str(), chainbase::xmodule_error_to_str(ret).c_str());
            return false;
        }
        m_unit_makers[account] = unitmaker;
    }
    return unitmaker->push_tx(txs);
}

bool xtable_maker_t::update_unit_makers() {
    for (auto iter = m_unit_makers.begin(); iter != m_unit_makers.end();) {
        auto & unitmaker = iter->second;
        int32_t ret = unitmaker->check_latest_state();
        if (ret != xsuccess) {
            xerror("xtable_maker_t::update_unit_makers unit maker clear fail-check_latest_state. account=%s,height=%ld,error_code=%s",
                unitmaker->get_account().c_str(), unitmaker->get_proposal_prev_block()->get_height(), chainbase::xmodule_error_to_str(ret).c_str());
            iter++;
            continue;
        }

        if (!unitmaker->can_make_next_block()) {
            xdbg("xtable_maker_t::update_unit_makers unit maker clear. cannot make next block. account=%s,height=%ld",
                unitmaker->get_account().c_str(), unitmaker->get_proposal_prev_block()->get_height());
            iter = m_unit_makers.erase(iter);
            continue;
        }
        xdbg("xtable_maker_t::update_unit_makers account=%s,height=%ld,class=%d",
            unitmaker->get_account().c_str(), unitmaker->get_proposal_prev_block()->get_height(), unitmaker->get_proposal_prev_block()->get_block_class());
        iter++;
    }
    return true;
}

xblock_ptr_t xtable_maker_t::make_next_block(const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & result) {
    xblock_ptr_t proposal_block = nullptr;
    int32_t error_code = xsuccess;
    // firstly try to make full block
    if (can_make_next_full_block()) {
        proposal_block = make_full_table(cs_para, error_code);
    } else if (can_make_next_light_block()) {
        proposal_block = make_light_table(cs_para, result, error_code);
    } else if (can_make_next_empty_block()) {
        // proposal_block = make_empty_table(error_code);
        xassert(0);  // TODO(jimmy)
    } else {
        error_code = enum_xtxexecutor_error_account_cannot_make_unit;
    }

    result.m_block = proposal_block;
    result.m_make_block_error_code = error_code;

    if (proposal_block != nullptr) {
        proposal_block->get_cert()->set_justify_cert_hash(get_lock_output_root_hash());
        proposal_block->set_consensus_para(cs_para);
        xinfo("xtable_maker_t::make_next_block success, proposal_block=%s,class=%d", proposal_block->dump().c_str(), proposal_block->get_block_class());
    } else {
        xwarn("xtable_maker_t::make_next_block fail, latest_cert_block=%s,error=%s",
            get_proposal_prev_block()->dump().c_str(), chainbase::xmodule_error_to_str(error_code).c_str());
    }
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::make_next_block(const data::xblock_consensus_para_t & cs_para, int32_t & error_code) {
    xtablemaker_result_t result;
    xblock_ptr_t block = make_next_block(cs_para, result);
    error_code = result.m_make_block_error_code;
    return block;
}

bool xtable_maker_t::can_make_next_block() const {
    if (can_make_next_light_block() || can_make_next_empty_block() || can_make_next_full_block()) {
        return true;
    }
    return false;
}

bool xtable_maker_t::can_make_next_empty_block() const {
    // TODO(jimmy) return data::xblocktool_t::can_make_next_empty_block(latest_blocks, m_empty_block_max_num);
    return false;
}
bool xtable_maker_t::can_make_next_full_block() const {
    uint64_t last_full_block_height = get_proposal_prev_block()->get_last_full_block_height();
    if ( (get_proposal_prev_block()->get_block_class() != base::enum_xvblock_class_full)
        && (get_proposal_prev_block()->get_height() - last_full_block_height > m_full_table_interval_num) ) {
        return true;
    }
    return false;
}
bool xtable_maker_t::can_make_next_light_block() const {
    return !m_unit_makers.empty();
}

xblock_ptr_t xtable_maker_t::make_light_table(const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result, int32_t & error_code) {
    int32_t ret = xsuccess;
    xtable_block_para_t lighttable_para;
    for (auto & v : m_unit_makers) {
        auto & unitmaker = v.second;
        xunitmaker_result_t unit_result;
        xassert(unitmaker->get_latest_blocks().size() > 0);
        xblock_ptr_t unit = unitmaker->make_next_block(cs_para, unit_result);
        table_result.m_unit_results.push_back(unit_result);
        if (unit == nullptr) {
            xwarn("xtable_maker_t::make_light_table fail-make unit. account=%s,viewid=%ld,unit_account=%s,error_code=%s",
                get_account().c_str(), cs_para.get_viewid(), unitmaker->get_account().c_str(), chainbase::xmodule_error_to_str(error_code).c_str());
            continue;
        }
        lighttable_para.add_unit(unit.get());
        // TODO(jimmy)
        if (lighttable_para.get_account_units().size() > 10) {
            xassert(false);
        }
//         tableblock_batch_tx_num_residue -= unit_proposal_input.get_input_txs().size();
//         if (tableblock_batch_tx_num_residue <= 0) {
//             break;
//         }
    }
    if (lighttable_para.get_account_units().empty()) {
        xwarn("xtable_maker_t::make_light_table fail-make table. account=%s,viewid=%ld",
            get_account().c_str(), cs_para.get_viewid());
        return nullptr;
    }

    lighttable_para.set_extra_data(cs_para.get_extra_data());
    return make_light_table(get_proposal_prev_block().get(), lighttable_para, cs_para);
}

xblock_ptr_t xtable_maker_t::make_full_table(const xblock_consensus_para_t & cs_para, int32_t & error_code) {
    xassert(m_last_full_table_mbt != nullptr);
    xassert(m_commit_table_mbt_binlog != nullptr);

    std::vector<xblock_ptr_t> uncommit_blocks = get_uncommit_blocks();
    xfulltable_block_para_t fulltable_para(m_last_full_table_mbt, m_commit_table_mbt_binlog, uncommit_blocks);
    return make_full_table(get_proposal_prev_block().get(), fulltable_para, cs_para);
}

xblock_ptr_t xtable_maker_t::make_empty_table(base::xvblock_t* prev_block, const xblock_consensus_para_t & cs_para) {
    base::xvblock_t* proposal_block = data::xblocktool_t::create_next_emptyblock(prev_block);
    xblock_ptr_t block;
    block.attach((data::xblock_t*)proposal_block);
    return block;
}

xblock_ptr_t xtable_maker_t::make_light_table(base::xvblock_t* prev_block, const xtable_block_para_t & table_para, const xblock_consensus_para_t & cs_para) {
    base::xvblock_t* proposal_block = data::xtable_block_t::create_next_tableblock(table_para, prev_block);
    xblock_ptr_t block;
    block.attach((data::xblock_t*)proposal_block);
    return block;
}

xblock_ptr_t xtable_maker_t::make_full_table(base::xvblock_t* prev_block, const xfulltable_block_para_t & table_para, const xblock_consensus_para_t & cs_para) {
    base::xvblock_t* proposal_block = data::xfull_tableblock_t::create_next_block(table_para, prev_block);
    xblock_ptr_t block;
    block.attach((data::xblock_t*)proposal_block);
    return block;
}

xblock_ptr_t xtable_maker_t::make_proposal(xtablemaker_para_t & table_para,
                                           const data::xblock_consensus_para_t & cs_para,
                                           xtablemaker_result_t & tablemaker_result) {
    std::lock_guard<std::mutex> l(m_lock);

    // check table maker state
    const xblock_ptr_t & latest_cert_block = table_para.m_latest_cert_block;
    int32_t ret = check_latest_state(latest_cert_block.get());
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::make_proposal fail-check_latest_state.account=%s,viewid=%ld,height=%ld",
            get_account().c_str(), cs_para.get_viewid(), latest_cert_block->get_height());
        return nullptr;
    }

    // check unit maker state and push txs
    for (auto & v : table_para.m_proposal_input.get_unit_inputs()) {
        const std::string & unit_account = v.get_account();
        const std::vector<xcons_transaction_ptr_t> & txs = v.get_input_txs();
        xunit_maker_ptr_t unitmaker = create_unit_maker(unit_account);
        int check_ret = unitmaker->check_latest_state();
        if (check_ret != xsuccess) {
            // TODO(jimmy) invoke unit sync
            xerror("xtable_maker_t::make_proposal unit account fail-check_latest_state.account=%s,viewid=%ld,height=%ld,unit_account=%s",
                get_account().c_str(), cs_para.get_viewid(), latest_cert_block->get_height(), unit_account.c_str());
            continue;
        }
        unitmaker->push_tx(txs);
    }

    xblock_ptr_t proposal_block = make_next_block(cs_para, tablemaker_result);
    if (proposal_block == nullptr) {
        xwarn("xtable_maker_t::make_proposal fail-make_next_block.account=%s,viewid=%ld,height=%ld,error_code=%s",
            get_account().c_str(), cs_para.get_viewid(), latest_cert_block->get_height(),
            chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
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
            XMETRICS_PACKET_INFO("consensus_tableblock", "proposal_create", proposal_block->dump(), "unit", unit->dump());
            const auto & txs = unitmaker_result.m_success_txs;
            for (auto & tx : txs) {
                tx_count++;
                XMETRICS_PACKET_INFO("consensus_tableblock", "proposal_create", proposal_block->dump(), "transaction", tx->dump());
            }
            table_para.m_proposal_input.add_unit_input(unit_proposal_input);
        }
    }
    XMETRICS_PACKET_INFO("consensus_tableblock", "proposal_create", proposal_block->dump(),
        "unit_count", std::to_string(unit_count), "tx_count", std::to_string(tx_count));

    return proposal_block;
}

int32_t xtable_maker_t::verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para) {
    std::lock_guard<std::mutex> l(m_lock);

    xassert(table_para.m_latest_cert_block != nullptr);
    // check table maker state
    const xblock_ptr_t & latest_cert_block = table_para.m_latest_cert_block;
    int32_t ret = check_latest_state(latest_cert_block.get());
    if (ret != xsuccess) {
        xwarn("xtable_maker_t::verify_proposal fail-check_latest_state.proposal=%s",
            proposal_block->dump().c_str());
        return ret;
    }

    // check unit maker state and push txs
    for (auto & v : table_para.m_proposal_input.get_unit_inputs()) {
        const std::string & unit_account = v.get_account();
        const std::vector<xcons_transaction_ptr_t> & txs = v.get_input_txs();
        xunit_maker_ptr_t unitmaker = create_unit_maker(unit_account);
        int check_ret = unitmaker->check_latest_state();
        if (check_ret != xsuccess) {
            // TODO(jimmy) invoke unit sync
            xerror("xtable_maker_t::verify_proposal fail-unit check_latest_state.proposal=%s,unit_account=%s",
                proposal_block->dump().c_str(), unit_account.c_str());
            return check_ret;
        }

        if (unitmaker->get_proposal_prev_block()->get_height() != v.get_last_block_height()
            || unitmaker->get_proposal_prev_block()->get_block_hash() != v.get_last_block_hash()) {
            xwarn("xtable_maker_t::verify_proposal fail-unit not match prev block.proposal=%s,unit_account=%s",
                proposal_block->dump().c_str(), unit_account.c_str());
            return enum_xtxexecutor_error_proposal_unit_not_match_last_unit;
        }
        unitmaker->push_tx(txs);
    }

    if (false == verify_proposal_class(proposal_block)) {
        xerror("xproposal_maker_t::verify_proposal fail-proposal should be class full. proposal=%s",
            proposal_block->dump().c_str());
        return enum_xtxexecutor_error_backup_verify_fail_block_class;
    }

    xtablemaker_result_t tablemaker_result;
    xblock_ptr_t local_block = make_next_block(cs_para, tablemaker_result);
    if (local_block == nullptr) {
        xwarn("xtable_maker_t::verify_proposal fail-make_next_block.proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
        return tablemaker_result.m_make_block_error_code;
    }
    local_block->get_cert()->set_nonce(proposal_block->get_cert()->get_nonce());

    // check local and proposal is match
    if (false == verify_proposal_with_local(proposal_block, local_block.get())) {
        xerror("xtable_maker_t::verify_proposal fail-make local block. proposal=%s", proposal_block->dump().c_str());
        return enum_xtxexecutor_error_backup_verify_fail_block_not_match_local;
    }

    return xsuccess;
}

bool xtable_maker_t::verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const {
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
    if (can_make_next_full_block()) {
        if (proposal_block->get_block_class() != base::enum_xvblock_class_full) {
            return false;
        }
    } else if (proposal_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    return false;
}



NS_END2
