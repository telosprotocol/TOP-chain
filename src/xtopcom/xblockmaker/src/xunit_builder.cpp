// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xblockmaker/xunit_builder.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xstore/xaccount_context.h"
#include "xdata/xfullunit.h"
#include "xdata/xemptyblock.h"

NS_BEG2(top, blockmaker)

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

        return nullptr;
    }

    xlightunit_block_para_t lightunit_para;
    // set lightunit para by tx result
    lightunit_para.set_input_txs(input_txs);
    lightunit_para.set_transaction_result(exec_result.succ_txs_result);
    xassert(unconfirm_num == _account_context->get_blockchain()->get_unconfirm_sendtx_num());
    lightunit_para.set_account_unconfirm_sendtx_num(unconfirm_num);

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


NS_END2
