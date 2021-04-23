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
#include "xcontract_runtime/xaccount_vm.h"

NS_BEG2(top, blockmaker)

xlightunit_builder_t::xlightunit_builder_t() {

}

void xlightunit_builder_t::alloc_tx_receiptid(const std::vector<xcons_transaction_ptr_t> & input_txs, const base::xreceiptid_state_ptr_t & receiptid_state) {
    for (auto & tx : input_txs) {
        data::xblocktool_t::alloc_transaction_receiptid(tx, receiptid_state);
        xdbg("xlightunit_builder_t::alloc_tx_receiptid alloc receipt id. tx=%s", tx->dump(true).c_str());
    }
}

xblock_ptr_t        xlightunit_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xaccount_ptr_t & prev_state,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::shared_ptr<xlightunit_builder_para_t> lightunit_build_para = std::dynamic_pointer_cast<xlightunit_builder_para_t>(build_para);
    xassert(lightunit_build_para != nullptr);

    uint32_t unconfirm_num = prev_state->get_unconfirm_sendtx_num();
    std::shared_ptr<store::xaccount_context_t> _account_context = std::make_shared<store::xaccount_context_t>(prev_state.get(), build_para->get_store());
    _account_context->set_context_para(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_timestamp(), cs_para.get_total_lock_tgas_token());
    xassert(!cs_para.get_table_account().empty());
    xassert(cs_para.get_table_proposal_height() > 0);
    uint64_t table_committed_height = cs_para.get_table_proposal_height() >= 3 ? cs_para.get_table_proposal_height() - 3 : 0;
    _account_context->set_context_pare_current_table(cs_para.get_table_account(), table_committed_height);

    const std::vector<xcons_transaction_ptr_t> & input_txs = lightunit_build_para->get_origin_txs();
    txexecutor::xbatch_txs_result_t exec_result;
    int exec_ret = txexecutor::xtransaction_executor::exec_batch_txs(_account_context.get(), input_txs, exec_result);
    if (exec_ret != xsuccess) {
        xassert(exec_result.m_exec_fail_tx_ret != 0);
        xassert(exec_result.m_exec_fail_tx != nullptr);
        const auto & failtx = exec_result.m_exec_fail_tx;
        xassert(failtx->is_self_tx() || failtx->is_send_tx());
        xwarn("xlightunit_builder_t::build_block fail-tx execute. %s,account:%s,height=%" PRIu64 ",tx=%s",
            cs_para.dump().c_str(), account.c_str(), prev_height, failtx->dump().c_str());
        // tx execute fail, this tx and follower txs should be pop out for hash must be not match with the new tx witch will replace the fail tx.
        for (auto & tx : input_txs) {
            if (tx->get_transaction()->get_tx_nonce() >= failtx->get_transaction()->get_tx_nonce()) {
                lightunit_build_para->set_fail_tx(tx);
            }
        }
        build_para->set_error_code(xblockmaker_error_tx_execute);
        return nullptr;
    }

    xlightunit_block_para_t lightunit_para;
    // set lightunit para by tx result
    lightunit_para.set_input_txs(input_txs);
    lightunit_para.set_transaction_result(exec_result.succ_txs_result);
    xassert(unconfirm_num == _account_context->get_blockchain()->get_unconfirm_sendtx_num());
    lightunit_para.set_account_unconfirm_sendtx_num(unconfirm_num);

    base::xreceiptid_state_ptr_t receiptid_state = lightunit_build_para->get_receiptid_state();
    alloc_tx_receiptid(input_txs, receiptid_state);
    alloc_tx_receiptid(lightunit_para.get_contract_create_txs(), receiptid_state);

    base::xvblock_t* _proposal_block = data::xlightunit_block_t::create_next_lightunit(lightunit_para, prev_block.get());
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    proposal_unit->set_consensus_para(cs_para);
    return proposal_unit;
}

xblock_ptr_t        xfullunit_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xaccount_ptr_t & prev_state,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    const std::string & account = prev_block->get_account();
    uint64_t prev_height = prev_block->get_height();
    std::map<std::string, std::string> propertys;
    const auto & property_map = prev_state->get_property_hash_map();
    for (auto & v : property_map) {
        xdataobj_ptr_t db_prop = build_para->get_store()->clone_property(account, v.first);
        if (db_prop == nullptr) {
            build_para->set_error_code(xblockmaker_error_property_load);
            xerror("xfullunit_builder_t::build_block fail-property load,%s,account:%s,height=%" PRIu64 ",property(%s) not exist.",
                  cs_para.dump().c_str(), account.c_str(), prev_height, v.first.c_str());
            return nullptr;
        }

        std::string db_prop_hash = xhash_base_t::calc_dataunit_hash(db_prop.get());
        if (db_prop_hash != v.second) {
            build_para->set_error_code(xblockmaker_error_property_unmatch);
            // TODO(jimmy) might happen, because property is not stored by height
            xwarn("xfullunit_builder_t::build_block fail-property unmatch,%s,account:%s,height=%" PRIu64 ",property(%s) hash not match fullunit.",
                  cs_para.dump().c_str(), account.c_str(), prev_height, v.first.c_str());
            return nullptr;
        }
        base::xstream_t _stream(base::xcontext_t::instance());
        db_prop->serialize_to(_stream);
        std::string prop_str((const char *)_stream.data(), _stream.size());
        propertys[v.first] = prop_str;
    }

    xfullunit_block_para_t para;
    para.m_account_propertys = propertys;
    para.m_account_state = prev_state->get_account_mstate();
    para.m_first_unit_height = prev_state->get_last_full_unit_height();
    para.m_first_unit_hash = prev_state->get_last_full_unit_hash();

    base::xvblock_t* _proposal_block = data::xfullunit_block_t::create_next_fullunit(para, prev_block.get());
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    proposal_unit->set_consensus_para(cs_para);
    return proposal_unit;
}

xblock_ptr_t        xemptyunit_builder_t::build_block(const xblock_ptr_t & prev_block,
                                                    const xaccount_ptr_t & prev_state,
                                                    const data::xblock_consensus_para_t & cs_para,
                                                    xblock_builder_para_ptr_t & build_para) {
    base::xvblock_t* _proposal_block = data::xemptyblock_t::create_next_emptyblock(prev_block.get());
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    proposal_unit->set_consensus_para(cs_para);
    return proposal_unit;
}

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
NS_END2
