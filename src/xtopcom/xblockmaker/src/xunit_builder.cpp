// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xvledger/xreceiptid.h"
#include "xblockmaker/xunit_builder.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xdata/xemptyblock.h"
#include "xdata/xfullunit.h"
#include "xdata/xblocktool.h"
#include "xstore/xaccount_context.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvstatestore.h"
// #include "xcontract_runtime/xaccount_vm.h"
#include "xdata/xblockbuild.h"

NS_BEG2(top, blockmaker)

xlightunit_builder_t::xlightunit_builder_t() {

}

void xlightunit_builder_t::alloc_tx_receiptid(const std::vector<xcons_transaction_ptr_t> & input_txs, const base::xreceiptid_state_ptr_t & receiptid_state) {
    for (auto & tx : input_txs) {
        data::xblocktool_t::alloc_transaction_receiptid(tx, receiptid_state);
    }
}

xblock_ptr_t        xlightunit_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::shared_ptr<xlightunit_builder_para_t> lightunit_build_para = std::dynamic_pointer_cast<xlightunit_builder_para_t>(build_para);
    xassert(lightunit_build_para != nullptr);

    // TODO(jimmy) create proposal block and proposal state, execute txs with proposal state
    base::xauto_ptr<base::xvblock_t> _temp_proposal_block = data::xblocktool_t::create_next_emptyblock(prev_block.get());
    xassert(_temp_proposal_block != nullptr);
    xassert(prev_bstate->get_block_height() == _temp_proposal_block->get_height() - 1);
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_proposal_block.get(), *prev_bstate.get());
    xaccount_ptr_t proposal_state = std::make_shared<xunit_bstate_t>(proposal_bstate.get());

    const std::vector<xcons_transaction_ptr_t> & input_txs = lightunit_build_para->get_origin_txs();
    txexecutor::xbatch_txs_result_t exec_result;
    int exec_ret = txexecutor::xtransaction_executor::exec_batch_txs(_temp_proposal_block.get(), prev_bstate, cs_para, input_txs, build_para->get_store(), exec_result);
    xinfo("xlightunit_builder_t::build_block %s,account=%s,height=%ld,exec_ret=%d,succtxs_count=%zu,failtxs_count=%zu,unconfirm_count=%d,binlog_size=%zu,binlog=%ld,state_size=%zu",
        cs_para.dump().c_str(), prev_block->get_account().c_str(), prev_block->get_height() + 1,
        exec_ret, exec_result.m_exec_succ_txs.size(), exec_result.m_exec_fail_txs.size(),
        exec_result.m_unconfirm_tx_num, exec_result.m_property_binlog.size(), base::xhash64_t::digest(exec_result.m_property_binlog), exec_result.m_full_state.size());
    // some send txs may execute fail but some recv/confirm txs may execute successfully
    if (!exec_result.m_exec_fail_txs.empty()) {
        lightunit_build_para->set_fail_txs(exec_result.m_exec_fail_txs);
    }
    if (exec_ret != xsuccess) {
        build_para->set_error_code(xblockmaker_error_tx_execute);
        return nullptr;
    }

    lightunit_build_para->set_tgas_balance_change(exec_result.m_tgas_balance_change);

    xlightunit_block_para_t lightunit_para;
    // set lightunit para by tx result
    lightunit_para.set_input_txs(exec_result.m_exec_succ_txs);
    lightunit_para.set_account_unconfirm_sendtx_num(exec_result.m_unconfirm_tx_num);
    lightunit_para.set_fullstate_bin(exec_result.m_full_state);
    lightunit_para.set_binlog(exec_result.m_property_binlog);

    base::xreceiptid_state_ptr_t receiptid_state = lightunit_build_para->get_receiptid_state();
    alloc_tx_receiptid(exec_result.m_exec_succ_txs, receiptid_state);

    xlightunit_build_t bbuild(prev_block.get(), lightunit_para, cs_para);
    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_lightunit(lightunit_para, prev_block.get(), cs_para);
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    return proposal_unit;
}

std::string     xfullunit_builder_t::make_binlog(const xblock_ptr_t & prev_block,
                                                const xobject_ptr_t<base::xvbstate_t> & prev_bstate) {
    base::xauto_ptr<base::xvblock_t> _temp_block = data::xblocktool_t::create_next_emptyblock(prev_block.get());
    xobject_ptr_t<base::xvbstate_t> proposal_bstate = make_object_ptr<base::xvbstate_t>(*_temp_block.get(), *prev_bstate.get());

    std::string property_snapshot;
    auto canvas = proposal_bstate->rebase_change_to_snapshot();
    canvas->encode(property_snapshot);
    xassert(!property_snapshot.empty());
#if 0 //test
xobject_ptr_t<base::xvbstate_t> test_bstate = make_object_ptr<base::xvbstate_t>(*_temp_block.get());
bool ret_apply = test_bstate->apply_changes_of_binlog(property_snapshot);
xassert(ret_apply == true);
#endif
    return property_snapshot;
}


xblock_ptr_t        xfullunit_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    xfullunit_block_para_t para;
    para.m_property_snapshot = make_binlog(prev_block, prev_bstate);
    para.m_first_unit_height = prev_bstate->get_last_fullblock_height();
    para.m_first_unit_hash = prev_bstate->get_last_fullblock_hash();
    xinfo("xfullunit_builder_t::build_block %s,account=%s,height=%ld,binlog_size=%zu,binlog=%ld",
        cs_para.dump().c_str(), prev_block->get_account().c_str(), prev_block->get_height() + 1,
        para.m_property_snapshot.size(), base::xhash64_t::digest(para.m_property_snapshot));

    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_fullunit(para, prev_block.get(), cs_para);
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    return proposal_unit;
}

xblock_ptr_t        xemptyunit_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xobject_ptr_t<base::xvbstate_t> & prev_bstate,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    base::xvblock_t* _proposal_block = data::xblocktool_t::create_next_emptyblock(prev_block.get(), cs_para);
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    return proposal_unit;
}
#if 0
xblock_ptr_t xtop_lightunit_builder2::build_block(xblock_ptr_t const & prev_block,
                                                  xaccount_ptr_t const & prev_state,
                                                  data::xblock_consensus_para_t const & cs_para,
                                                  xblock_builder_para_ptr_t & build_para) {
    auto * statestore = base::xvchain_t::instance().get_xstatestore();
    assert(statestore != nullptr);
    if (!statestore->get_block_state(prev_block.get())) {
        return nullptr;
    }

    auto * state_ptr = prev_block->get_state();
    assert(state_ptr != nullptr);
    state_ptr->add_ref();
    xobject_ptr_t<base::xvbstate_t> blockstate;
    blockstate.attach(state_ptr);

    std::shared_ptr<xlightunit_builder_para_t> lightunit_build_para = std::dynamic_pointer_cast<xlightunit_builder_para_t>(build_para);
    xassert(lightunit_build_para != nullptr);

    auto const & input_txs = lightunit_build_para->get_origin_txs();

    contract_runtime::xaccount_vm_t account_vm;
    auto result = account_vm.execute(input_txs, blockstate);

    if (result.status.ec) {
        for (auto i = 0u; i < result.transaction_results.size(); ++i) {
            auto const & r = result.transaction_results[i];
            if (r.status.ec) {
                lightunit_build_para->set_fail_tx(input_txs[i]);
                break;
            }
        }

        build_para->set_error_code(xblockmaker_error_tx_execute);
        return nullptr;
    }

    xlightunit_block_para_t lightunit_para;
    // set lightunit para by tx result
    lightunit_para.set_input_txs(input_txs);
    // lightunit_para.set_transaction_result(exec_result.succ_txs_result);
    // lightunit_para.set_account_unconfirm_sendtx_num(unconfirm_num);

    base::xreceiptid_state_ptr_t receiptid_state = lightunit_build_para->get_receiptid_state();
    alloc_tx_receiptid(input_txs, receiptid_state);
    alloc_tx_receiptid(lightunit_para.get_contract_create_txs(), receiptid_state);

    base::xvblock_t * _proposal_block = data::xlightunit_block_t::create_next_lightunit(lightunit_para, prev_block.get());
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t *)_proposal_block);
    proposal_unit->set_consensus_para(cs_para);
    return proposal_unit;
}

void xtop_lightunit_builder2::alloc_tx_receiptid(const std::vector<xcons_transaction_ptr_t> & input_txs, const base::xreceiptid_state_ptr_t & receiptid_state) {
    for (auto & tx : input_txs) {
        if (tx->is_self_tx()) {
            continue;
        } else if (tx->is_send_tx()) {
            base::xvaccount_t _vaccount(tx->get_transaction()->get_target_addr());
            base::xtable_shortid_t target_sid = _vaccount.get_short_table_id();

            base::xreceiptid_pair_t receiptid_pair;
            receiptid_state->find_pair_modified(target_sid, receiptid_pair);

            uint64_t current_receipt_id = receiptid_pair.get_sendid_max() + 1;
            receiptid_pair.inc_sendid_max();
            tx->set_current_receipt_id(target_sid, current_receipt_id);
            receiptid_state->add_pair_modified(target_sid, receiptid_pair);  // save to modified pairs
            xdbg("xlightunit_builder_t::alloc_tx_receiptid alloc send_tx receipt id. tx=%s", tx->dump(true).c_str());
        } else if (tx->is_recv_tx()) {
            base::xvaccount_t _vaccount(tx->get_transaction()->get_source_addr());
            base::xtable_shortid_t source_sid = _vaccount.get_short_table_id();
            // copy receipt id from last phase to current phase
            uint64_t receipt_id = tx->get_last_action_receipt_id();
            tx->set_current_receipt_id(source_sid, receipt_id);
            xdbg("xlightunit_builder_t::alloc_tx_receiptid alloc recv_tx receipt id. tx=%s", tx->dump(true).c_str());
        } else if (tx->is_confirm_tx()) {
            base::xvaccount_t _vaccount(tx->get_transaction()->get_target_addr());
            base::xtable_shortid_t target_sid = _vaccount.get_short_table_id();
            // copy receipt id from last phase to current phase
            uint64_t receipt_id = tx->get_last_action_receipt_id();
            tx->set_current_receipt_id(target_sid, receipt_id);
            xdbg("xlightunit_builder_t::alloc_tx_receiptid alloc confirm_tx receipt id. tx=%s", tx->dump(true).c_str());
        }
    }
}
#endif

NS_END2
