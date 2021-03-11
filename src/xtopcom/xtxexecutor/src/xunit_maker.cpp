// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xtxexecutor/xunit_maker.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xdata/xblocktool.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xstore/xaccount_context.h"

NS_BEG2(top, txexecutor)

xunit_maker_t::xunit_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
: xblock_maker_t(account, resources, m_keep_latest_blocks_max) {
    m_fullunit_contain_of_unit_num_para = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);
}

void xunit_maker_t::init_unit_blocks(const base::xblock_mptrs & latest_blocks) {
    xblock_ptr_t cert_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_cert_block());
    set_latest_block(cert_block);
    if (latest_blocks.get_latest_locked_block()->get_block_hash() != get_proposal_prev_block()->get_block_hash()) {
        xblock_ptr_t lock_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block());
        set_latest_block(lock_block);
    }
    if (latest_blocks.get_latest_committed_block()->get_block_hash() != get_proposal_prev_block()->get_block_hash()) {
        xblock_ptr_t commit_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
        set_latest_block(commit_block);
    }
}

int32_t xunit_maker_t::default_check_latest_state() {
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*this);
    return check_latest_state(latest_blocks.get_latest_cert_block());
}

int32_t    xunit_maker_t::check_latest_state(base::xvblock_t* latest_cert_block) {
    m_pending_txs.clear();

    xblock_ptr_t latest_block = xblock_t::raw_vblock_to_object_ptr(latest_cert_block);

    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*this);
    if (!is_latest_blocks_valid(latest_blocks)) {
        return enum_xtxexecutor_error_unit_state_behind;
    }

    init_unit_blocks(latest_blocks);

    xblock_ptr_t commit_block = xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block());
    if (!update_latest_state(commit_block)) {
        xwarn("xunit_maker_t::check_latest_state fail-update_latest_state, account=%s", get_account().c_str());
        return enum_xtxexecutor_error_unit_state_behind;
    }

    xdbg("xunit_maker_t::check_latest_state latest_cert_block=%s,block_size=%d,nonce=%ld",
        latest_block->dump().c_str(), get_latest_blocks().size(), get_latest_committed_state()->get_account_mstate().get_latest_send_trans_number());
    return xsuccess;
}

bool xunit_maker_t::push_tx(const std::vector<xcons_transaction_ptr_t> & txs) {
    // TODO(jimmy) do repeat filter with cert/lock unit
    m_pending_txs = txs;
    return true;
}

xblock_ptr_t xunit_maker_t::make_next_block(const data::xblock_consensus_para_t & cs_para, xunitmaker_result_t & result) {
    xblock_ptr_t proposal_unit = nullptr;
    int32_t error_code = xsuccess;

    xassert(get_latest_blocks().size() > 0);

    // firstly try to make full unit
    if (can_make_next_full_block()) {
        proposal_unit = make_full_unit(error_code);
    } else if (can_make_next_light_block()) {
        xbatch_txs_result_t exec_result;
        proposal_unit = make_light_unit(cs_para, exec_result, error_code);
        result.m_success_txs = exec_result.m_exec_succ_txs;
        result.m_fail_tx = exec_result.m_exec_fail_tx;
        result.m_fail_tx_error = exec_result.m_exec_fail_tx_ret;
    } else if (can_make_next_empty_block()) {
        proposal_unit = make_empty_unit(error_code);
    } else {
        error_code = enum_xtxexecutor_error_account_cannot_make_unit;
    }

    result.m_block = proposal_unit;
    result.m_make_block_error_code = error_code;

    if (proposal_unit != nullptr) {
        xassert(get_latest_blocks().size() > 0);
        xassert(get_lock_block() != nullptr);
        std::string justify_cert_hash = get_lock_block_sign_hash();
        proposal_unit->get_cert()->set_justify_cert_hash(justify_cert_hash);
        proposal_unit->set_consensus_para(cs_para);
        xinfo("xunit_maker_t::make_next_block success, block=%s,cert:%s,class=%d",
            proposal_unit->dump().c_str(), proposal_unit->dump_cert().c_str());
    } else {
        xwarn("xunit_maker_t::make_next_block fail, latest_cert_unit=%s,error=%s",
            get_proposal_prev_block()->dump().c_str(), chainbase::xmodule_error_to_str(error_code).c_str());
    }

    m_pending_txs.clear();
    return proposal_unit;
}

xblock_ptr_t xunit_maker_t::make_next_block(const data::xblock_consensus_para_t & cs_para, int32_t & error_code) {
    xunitmaker_result_t result;
    xblock_ptr_t block = make_next_block(cs_para, result);
    error_code = result.m_make_block_error_code;
    return block;
}

bool xunit_maker_t::can_make_next_block() const {
    if (can_make_next_light_block() || can_make_next_empty_block() || can_make_next_full_block()) {
        return true;
    }
    return false;
}

bool xunit_maker_t::can_make_next_empty_block() const {
    uint32_t num = get_latest_consecutive_empty_block_num();
    // TODO(jimmy)
    if (get_proposal_prev_block()->get_height() > 0 && num < m_consecutive_empty_unit_max) {
        return true;
    }
    return false;
}

bool xunit_maker_t::can_make_next_full_block() const {
    if (has_uncommitted_blocks()) {
        xdbg("xunit_maker_t::can_make_next_full_block account locked. account=%s,height=%ld,%ld",
            get_account().c_str(), get_proposal_prev_block()->get_height(), get_latest_committed_block()->get_height());
        return false;
    }

    base::xvblock_t* prev_block = get_proposal_prev_block().get();
    uint64_t current_height = prev_block->get_height() + 1;
    uint64_t current_fullunit_height = prev_block->get_block_class() == base::enum_xvblock_class_full ? prev_block->get_height() : prev_block->get_last_full_block_height();
    uint64_t current_lightunit_count = current_height - current_fullunit_height;
    xassert(current_lightunit_count > 0);
    if (current_lightunit_count >= m_fullunit_contain_of_unit_num_para && get_latest_committed_state()->get_unconfirm_sendtx_num() == 0) {
        xdbg("xunit_maker_t::can_make_next_full_block fullunit. %s height:%ld full_unit_height:%ld unconfirm_sendtx_num:%ld",
            get_account().c_str(), current_height, current_fullunit_height, get_latest_committed_state()->get_unconfirm_sendtx_num());
        return true;
    }
    xdbg("xunit_maker_t::can_make_next_full_block lightunit. %s height:%ld full_unit_height:%ld unconfirm_sendtx_num:%d",
        get_account().c_str(), current_height, current_fullunit_height, get_latest_committed_state()->get_unconfirm_sendtx_num());
    return false;
}

bool xunit_maker_t::can_make_next_light_block() const {
    if (has_uncommitted_blocks()) {
        xdbg("xunit_maker_t::can_make_next_light_block account locked. account=%s,height=%ld,%ld",
            get_account().c_str(), get_proposal_prev_block()->get_height(), get_latest_committed_block()->get_height());
        return false;
    }

    // make two empty unit after light unit
    if (m_pending_txs.empty()) {
        return false;
    }
    return true;
}

xblock_ptr_t xunit_maker_t::make_full_unit(int32_t & error_code) {
    xfullunit_block_para_t para;
    error_code = make_fullunit_output(get_latest_committed_state(), para);
    if (error_code != xsuccess) {
        return nullptr;
    }
    base::xvblock_t* prev_block = get_proposal_prev_block().get();
    base::xvblock_t* _proposal_block = data::xfullunit_block_t::create_next_fullunit(para, prev_block);
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    error_code = xsuccess;
    return proposal_unit;
}
xblock_ptr_t xunit_maker_t::make_empty_unit(int32_t & error_code) {
    base::xvblock_t* prev_block = get_proposal_prev_block().get();
    base::xvblock_t* _proposal_block = data::xemptyblock_t::create_next_emptyblock(prev_block);
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    error_code = xsuccess;
    return proposal_unit;
}
xblock_ptr_t xunit_maker_t::make_light_unit(const xblock_consensus_para_t & cs_para, xbatch_txs_result_t & exec_result, int32_t & error_code) {
    base::xvblock_t* prev_block = get_proposal_prev_block().get();

    // TODO(jimmy) now only use commit unit account
    std::shared_ptr<store::xaccount_context_t> _account_context = std::make_shared<store::xaccount_context_t>(get_account(), get_store());
    if (_account_context->get_blockchain()->get_chain_height() != get_latest_committed_block()->get_height()) {
        xerror("xunit_maker_t::make_light_unit account=%s,height=%ld:%ld",
            get_account().c_str(), _account_context->get_blockchain()->get_chain_height(), get_latest_committed_block()->get_height());
        error_code = enum_xtxexecutor_error_unit_height_not_equal_blockchain_height;
        return nullptr;
    }

    _account_context->set_context_para(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_timestamp(), cs_para.get_total_lock_tgas_token());

    data::xlightunit_block_para_t lightunit_para;
    xassert(!m_pending_txs.empty());
    int ret = make_lightunit_output(_account_context.get(), m_pending_txs, exec_result, lightunit_para);
    if (ret) {
        error_code = ret;
        return nullptr;
    }

    base::xvblock_t* _proposal_block = data::xlightunit_block_t::create_next_lightunit(lightunit_para, prev_block);
    xblock_ptr_t proposal_unit;
    proposal_unit.attach((data::xblock_t*)_proposal_block);
    error_code = xsuccess;
    return proposal_unit;
}

int xunit_maker_t::make_lightunit_output(store::xaccount_context_t * context,
                                        const std::vector<xcons_transaction_ptr_t> & txs,
                                        xbatch_txs_result_t & exec_result,
                                        data::xlightunit_block_para_t & lightunit_para) {
    int exec_ret = xtransaction_executor::exec_batch_txs(context, txs, exec_result);
    if (exec_ret != xsuccess) {
        xassert(exec_result.m_exec_fail_tx_ret != 0);
        xassert(exec_result.m_exec_fail_tx != nullptr);
        const auto & failtx = exec_result.m_exec_fail_tx;

        xassert(failtx->is_self_tx() || failtx->is_send_tx());
        if (exec_result.m_exec_fail_tx_ret != xunit_contract_exec_no_property_change) {
            xwarn("xunit_maker_t::make_block tx exec fail. %s error:%s",
                failtx->dump().c_str(), chainbase::xmodule_error_to_str(exec_result.m_exec_fail_tx_ret).c_str());
        }
        // TODO(jimmy)
        xassert(get_txpool() != nullptr);
        get_txpool()->pop_tx_by_hash(failtx->get_source_addr(), failtx->get_transaction()->digest(), failtx->get_tx_subtype(), exec_result.m_exec_fail_tx_ret);
        return exec_ret;
    }

    xtransaction_result_t & tx_result = exec_result.succ_txs_result;
    // set lightunit para by tx result
    lightunit_para.set_input_txs(txs);
    lightunit_para.set_transaction_result(tx_result);
    lightunit_para.set_account_unconfirm_sendtx_num(context->get_blockchain()->get_unconfirm_sendtx_num());
    return xsuccess;
}

int xunit_maker_t::make_fullunit_output(const xaccount_ptr_t & bstate, xfullunit_block_para_t & para) {
    std::map<std::string, std::string> propertys;
    const auto & property_map = bstate->get_property_hash_map();
    for (auto & v : property_map) {
        xdataobj_ptr_t db_prop = get_store()->clone_property(get_account(), v.first);
        if (db_prop == nullptr) {
            xerror("xunit_blockmaker_t::make_fullunit_output account:%s,height=%ld,property(%s) not exist.",
                  get_account().c_str(), bstate->get_last_height(), v.first.c_str());
            return enum_xtxexecutor_error_unit_property_behind;
        }

        std::string db_prop_hash = xhash_base_t::calc_dataunit_hash(db_prop.get());
        if (db_prop_hash != v.second) {
            // TODO(jimmy) might happen, because property is not stored by height
            xwarn("xunit_blockmaker_t::make_fullunit_output account:%s,height=%ld,property(%s) hash not match fullunit.",
                  get_account().c_str(), bstate->get_last_height(), v.first.c_str());
            return enum_xtxexecutor_error_unit_property_behind;
        }
        base::xstream_t _stream(base::xcontext_t::instance());
        db_prop->serialize_to(_stream);
        std::string prop_str((const char *)_stream.data(), _stream.size());
        propertys[v.first] = prop_str;
    }

    para.m_account_propertys = propertys;
    para.m_account_state = bstate->get_account_mstate();
    para.m_first_unit_height = bstate->get_last_full_unit_height();
    para.m_first_unit_hash = bstate->get_last_full_unit_hash();
    return xsuccess;
}


NS_END2
