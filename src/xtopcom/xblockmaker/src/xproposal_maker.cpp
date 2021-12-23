// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xblockmaker/xproposal_maker.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xstore/xtgas_singleton.h"
#include "xdata/xblocktool.h"
#include "xdata/xnative_contract_address.h"
#include "xtxpool_v2/xtxpool_tool.h"
#include "xmbus/xevent_behind.h"

NS_BEG2(top, blockmaker)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xblockmaker, xblockmaker_error_to_string, xblockmaker_error_base+1, xblockmaker_error_max);

xproposal_maker_t::xproposal_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources) {
    xdbg("xproposal_maker_t::xproposal_maker_t create,this=%p,account=%s", this, account.c_str());
    m_resources = resources;
    m_table_maker = make_object_ptr<xtable_maker_t>(account, resources);  // TOOD(jimmy) global
    m_tableblock_batch_tx_num_residue = XGET_CONFIG(tableblock_batch_tx_max_num);  // TOOD(jimmy)
    m_max_account_num = XGET_CONFIG(tableblock_batch_unitblock_max_num);
}

xproposal_maker_t::~xproposal_maker_t() {
    xdbg("xproposal_maker_t::xproposal_maker_t destroy,this=%p", this);
}

bool xproposal_maker_t::can_make_proposal(data::xblock_consensus_para_t & proposal_para) {
    if (proposal_para.get_viewid() <= proposal_para.get_latest_cert_block()->get_viewid()) {
        xwarn("xproposal_maker_t::can_make_proposal fail-behind viewid. %s,latest_viewid=%" PRIu64 "",
            proposal_para.dump().c_str(), proposal_para.get_latest_cert_block()->get_viewid());
        return false;
    }

    if (!xblocktool_t::verify_latest_blocks(proposal_para.get_latest_cert_block().get(), proposal_para.get_latest_locked_block().get(), proposal_para.get_latest_committed_block().get())) {
        xwarn("xproposal_maker_t::can_make_proposal. fail-verify_latest_blocks fail.%s",
            proposal_para.dump().c_str());
        return false;
    }

    base::xauto_ptr<base::xvblock_t> drand_block = get_blockstore()->get_latest_committed_block(base::xvaccount_t(sys_drand_addr));
    if (drand_block->get_clock() == 0) {
        xwarn("xproposal_maker_t::can_make_proposal fail-no valid drand. %s", proposal_para.dump().c_str());
        return false;
    }
    proposal_para.set_drand_block(drand_block.get());
    return true;
}

xtablestate_ptr_t xproposal_maker_t::get_target_tablestate(base::xvblock_t* block) {
    base::xauto_ptr<base::xvbstate_t> bstate = m_resources->get_xblkstatestore()->get_block_state(block,metrics::statestore_access_from_blkmaker_get_target_tablestate);
    if (bstate == nullptr) {
        xwarn("xproposal_maker_t::get_target_tablestate fail-get target state.block=%s",block->dump().c_str());
        return nullptr;
    }
    xtablestate_ptr_t tablestate = std::make_shared<xtable_bstate_t>(bstate.get());
    return tablestate;
}

xblock_ptr_t xproposal_maker_t::make_proposal(data::xblock_consensus_para_t & proposal_para) {
    XMETRICS_TIMER(metrics::cons_make_proposal_tick);
    // get tablestate related to latest cert block
    auto & latest_cert_block = proposal_para.get_latest_cert_block();
    xtablestate_ptr_t tablestate = get_target_tablestate(latest_cert_block.get());
    if (nullptr == tablestate) {
        xwarn("xproposal_maker_t::make_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", proposal_para.dump().c_str(), latest_cert_block->get_height());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_table_state, 1);
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        return nullptr;
    }

    xtablestate_ptr_t tablestate_commit = get_target_tablestate(proposal_para.get_latest_committed_block().get());
    if (tablestate_commit == nullptr) {
        xwarn("xproposal_maker_t::make_proposal fail clone tablestate. %s,commit_height=%" PRIu64 "", proposal_para.dump().c_str(), proposal_para.get_latest_committed_block()->get_height());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_table_state, 1);
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        return nullptr;
    }

    xtablemaker_para_t table_para(tablestate, tablestate_commit);
    // get batch txs
    update_txpool_txs(proposal_para, table_para);
    XMETRICS_GAUGE(metrics::cons_table_leader_get_txpool_tx_count, table_para.get_origin_txs().size());

    if (false == leader_set_consensus_para(latest_cert_block.get(), proposal_para)) {
        xwarn("xproposal_maker_t::make_proposal fail-leader_set_consensus_para.%s",
            proposal_para.dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_consensus_para, 1);
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        return nullptr;
    }

    xtablemaker_result_t table_result;
    xblock_ptr_t proposal_block = m_table_maker->make_proposal(table_para, proposal_para, table_result);
    if (proposal_block == nullptr) {
        if (xblockmaker_error_no_need_make_table != table_result.m_make_block_error_code) {
            XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
            xwarn("xproposal_maker_t::make_proposal fail-make_proposal.%s error_code=%s",
                proposal_para.dump().c_str(), chainbase::xmodule_error_to_str(table_result.m_make_block_error_code).c_str());
        } else {
            XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 1);  // TODO(jimmy) no need make tableblock, improve check performance
            xinfo("xproposal_maker_t::make_proposal no need make table.%s",
                proposal_para.dump().c_str());
        }
        return nullptr;
    }

    // only invoke sync when make proposal successfully, avoiding too much call
    if (proposal_para.get_latest_committed_block()->get_height() > 0) {
        sys_contract_sync(tablestate_commit);
    }

    // need full cert block
    //get_blockstore()->load_block_input(*m_table_maker.get(), latest_cert_block.get());
    //get_blockstore()->load_block_output(*m_table_maker.get(), latest_cert_block.get());

    auto & proposal_input = table_para.get_proposal();
    std::string proposal_input_str;
    proposal_input->serialize_to_string(proposal_input_str);
    proposal_block->get_input()->set_proposal(proposal_input_str);
    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);

    // add metrics of tx counts / table counts ratio
    XMETRICS_GAUGE(metrics::cons_table_leader_make_unit_count, table_result.m_succ_unit_num);
    XMETRICS_GAUGE(metrics::cons_table_leader_make_tx_count, proposal_input->get_input_txs().size());    
    xinfo("xproposal_maker_t::make_proposal succ.proposal_block=%s,units_info={total=%d,fail=%d,succ=%d,empty=%d,light=%d,full=%d},txs_info={txpool=%d,ufm=%d,total=%d,self=%d,send=%d,recv=%d,confirm=%d},proposal_input={size=%zu,txs=%zu,accounts=%zu}", 
        proposal_block->dump().c_str(),
        table_result.m_total_unit_num, table_result.m_fail_unit_num, table_result.m_succ_unit_num, table_result.m_empty_unit_num, table_result.m_light_unit_num, table_result.m_full_unit_num,
        table_para.get_origin_txs().size(), tablestate->get_unconfirm_tx_num(), table_result.m_total_tx_num, table_result.m_self_tx_num, table_result.m_send_tx_num, table_result.m_recv_tx_num, table_result.m_confirm_tx_num,
        proposal_input_str.size(), proposal_input->get_input_txs().size(), proposal_input->get_other_accounts().size());
    return proposal_block;
}

int xproposal_maker_t::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    XMETRICS_TIMER(metrics::cons_verify_proposal_tick);
    xdbg("xproposal_maker_t::verify_proposal enter. proposal=%s", proposal_block->dump().c_str());
    xblock_consensus_para_t cs_para(get_account(), proposal_block->get_clock(), proposal_block->get_viewid(), proposal_block->get_viewtoken(), proposal_block->get_height());

    auto cert_block = get_blockstore()->get_latest_cert_block(*m_table_maker);
    if (proposal_block->get_height() < cert_block->get_height()) {
        xwarn("xproposal_maker_t::verify_proposal fail-proposal height less than cert block. proposal=%s,cert=%s",
            proposal_block->dump().c_str(), cert_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_cannot_connect_to_cert;
    }

    // TODO(jimmy) xbft callback and pass cert/lock/commit to us for performance
    // find matched cert block
    xblock_ptr_t proposal_prev_block = nullptr;
    if (proposal_block->get_last_block_hash() == cert_block->get_block_hash()
        && proposal_block->get_height() == cert_block->get_height() + 1) {
        proposal_prev_block = xblock_t::raw_vblock_to_object_ptr(cert_block.get());
    } else {
        auto _demand_cert_block = get_blockstore()->load_block_object(*m_table_maker, proposal_block->get_height() - 1, proposal_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_proposer_verify_proposal);
        if (_demand_cert_block == nullptr) {
            xwarn("xproposal_maker_t::verify_proposal fail-find cert block. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        proposal_prev_block = xblock_t::raw_vblock_to_object_ptr(_demand_cert_block.get());
    }
    cs_para.update_latest_cert_block(proposal_prev_block);  // prev table block is key info

    //find matched lock block
    if (proposal_prev_block->get_height() > 0) {
        auto lock_block = get_blockstore()->load_block_object(*m_table_maker, proposal_prev_block->get_height() - 1, proposal_prev_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_proposer_verify_proposal);
        if (lock_block == nullptr) {
            xwarn("xproposal_maker_t::verify_proposal fail-find lock block. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        xblock_ptr_t prev_lock_block = xblock_t::raw_vblock_to_object_ptr(lock_block.get());
        cs_para.update_latest_lock_block(prev_lock_block);
    } else {
        cs_para.update_latest_lock_block(proposal_prev_block);
    }
    //find matched commit block
    if (cs_para.get_latest_locked_block()->get_height() > 0) {
        // XTODO get latest connected block which can also load commit block, and it will invoke to update latest connect height.
        auto connect_block = get_blockstore()->get_latest_connected_block(*m_table_maker);
        if (connect_block == nullptr) {
            xerror("xproposal_maker_t::verify_proposal fail-find connected block. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;            
        }
        if (connect_block->get_height() != cs_para.get_latest_locked_block()->get_height() - 1) {
            xwarn("xproposal_maker_t::verify_proposal fail-connect not match commit block. proposal=%s,connect_height=%ld", proposal_block->dump().c_str(), connect_block->get_height());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;            
        }
        xblock_ptr_t prev_commit_block = xblock_t::raw_vblock_to_object_ptr(connect_block.get());
        cs_para.update_latest_commit_block(prev_commit_block);
    } else {
        cs_para.update_latest_commit_block(cs_para.get_latest_locked_block());
    }

    xdbg_info("xproposal_maker_t::verify_proposal. set latest_cert_block.proposal=%s, latest_cert_block=%s",
        proposal_block->dump().c_str(), proposal_prev_block->dump().c_str());

    // update txpool receiptid state
    const xblock_ptr_t & commit_block = cs_para.get_latest_committed_block();
    xtablestate_ptr_t commit_tablestate = get_target_tablestate(commit_block.get());
    if (commit_tablestate == nullptr) {
        xwarn("xproposal_maker_t::verify_proposal fail clone tablestate. %s,cert=%s", cs_para.dump().c_str(), proposal_prev_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_state_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_table_state_clone;
    }
    get_txpool()->update_table_state(commit_tablestate);

    // get tablestate related to latest cert block
    xtablestate_ptr_t tablestate = get_target_tablestate(proposal_prev_block.get());
    if (nullptr == tablestate) {
        xwarn("xproposal_maker_t::verify_proposal fail clone tablestate. %s,cert=%s", cs_para.dump().c_str(), proposal_prev_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_state_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_table_state_clone;
    }

    xtablemaker_para_t table_para(tablestate, commit_tablestate);
    if (false == verify_proposal_input(proposal_block, table_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-proposal input invalid. proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_bad_input;
    }

    // get proposal drand block
    xblock_ptr_t drand_block = nullptr;
    if (false == verify_proposal_drand_block(proposal_block, drand_block)) {
        xwarn("xproposal_maker_t::verify_proposal fail-find drand block. proposal=%s,drand_height=%" PRIu64 "",
            proposal_block->dump().c_str(), proposal_block->get_cert()->get_drand_height());
        // TODO(jimmy) invoke_sync(account, "tableblock backup sync");  XMETRICS_COUNTER_INCREMENT("txexecutor_cons_invoke_sync", 1);
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_drand_invalid, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_bad_drand;
    }

    base::xvqcert_t* bind_drand_cert = drand_block != nullptr ? drand_block->get_cert() : nullptr;
    if (false == backup_set_consensus_para(proposal_prev_block.get(), proposal_block, bind_drand_cert, cs_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-backup_set_consensus_para. proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_consensus_para_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_bad_consensus_para;
    }

    sys_contract_sync(commit_tablestate);

    int32_t verify_ret = m_table_maker->verify_proposal(proposal_block, table_para, cs_para);
    if (verify_ret != xsuccess) {
        xwarn("xproposal_maker_t::verify_proposal fail-verify_proposal. proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(verify_ret).c_str());
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return verify_ret;
    }
    XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 1);
    xdbg_info("xproposal_maker_t::verify_proposal succ. proposal=%s,latest_cert_block=%s",
        proposal_block->dump().c_str(), proposal_prev_block->dump().c_str());
    return xsuccess;
}

bool xproposal_maker_t::verify_proposal_input(base::xvblock_t *proposal_block, xtablemaker_para_t & table_para) {
    if (proposal_block->get_block_class() != base::enum_xvblock_class_light) {
        return true;
    }
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(proposal_block);
    if (tableblock == nullptr) {
        xerror("xproposal_maker_t::verify_proposal_input fail-not light table. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    std::string proposal_input_str = proposal_block->get_input()->get_proposal();
    xtable_proposal_input_ptr_t proposal_input = make_object_ptr<xtable_proposal_input_t>();
    int32_t ret = proposal_input->serialize_from_string(proposal_input_str);
    if (ret <= 0) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table serialize from proposal input. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    // set other accounts for tableblock
    std::vector<std::string> other_accounts;
    auto unit_headers = proposal_block->get_sub_block_headers();
    for (auto & _unit_header : unit_headers) {
        if (_unit_header->get_block_class() == base::enum_xvblock_class_nil || _unit_header->get_block_class() == base::enum_xvblock_class_full) {
            other_accounts.push_back(_unit_header->get_account());
        }        
    }

    const std::vector<xcons_transaction_ptr_t> & origin_txs = proposal_input->get_input_txs();
    if (origin_txs.empty() && other_accounts.empty()) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table proposal input empty. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    ret = get_txpool()->verify_txs(get_account(), origin_txs);
    if (ret != xsuccess) {
        return false;
    }

    table_para.set_origin_txs(origin_txs);
    table_para.set_other_accounts(other_accounts);
    return true;
}


bool xproposal_maker_t::verify_proposal_drand_block(base::xvblock_t *proposal_block, xblock_ptr_t & drand_block) const {
    if (proposal_block->get_block_class() != base::enum_xvblock_class_light) {
        drand_block = nullptr;
        return true;
    }

    uint64_t drand_height = proposal_block->get_cert()->get_drand_height();
    if (drand_height == 0) {
        xerror("xproposal_maker_t::verify_proposal_drand_block, fail-drand height zero. proposal=%s", proposal_block->dump().c_str());
        return false;
    }

    base::xauto_ptr<base::xvblock_t> _drand_vblock = get_blockstore()->load_block_object(base::xvaccount_t(sys_drand_addr), drand_height, 0, true, metrics::blockstore_access_from_blk_mk_proposer_verify_proposal_drand);
    if (_drand_vblock == nullptr) {
         xunit_info("xproposal_maker_t::verify_proposal_drand_block, consensus_tableblock  fail_find_drand=%s, drand_height=%llu.",
                proposal_block->dump().c_str(), drand_height);
        return false;
    }

    drand_block = xblock_t::raw_vblock_to_object_ptr(_drand_vblock.get());
    return true;
}

bool xproposal_maker_t::update_txpool_txs(const xblock_consensus_para_t & proposal_para, xtablemaker_para_t & table_para) {
    // std::map<std::string, uint64_t> locked_nonce_map;
    // update committed receiptid state for txpool, pop output finished txs
    if (proposal_para.get_latest_committed_block()->get_height() > 0) {
        if (nullptr == table_para.get_commit_tablestate()) {
            xwarn("xproposal_maker_t::update_txpool_txs fail clone tablestate. %s,committed_block=%s",
                proposal_para.dump().c_str(), proposal_para.get_latest_committed_block()->dump().c_str());
            return false;
        }
        get_txpool()->update_table_state(table_para.get_commit_tablestate());

        // update locked txs for txpool, locked txs come from two latest tableblock
        // get_locked_nonce_map(proposal_para.get_latest_locked_block(), locked_nonce_map);
        // get_locked_nonce_map(proposal_para.get_latest_cert_block(), locked_nonce_map);
    }

    // get table batch txs for execute and make block
    auto & tablestate_highqc = table_para.get_tablestate();
    uint16_t all_txs_max_num = 40;  // TODO(jimmy) config paras
    uint16_t confirm_and_recv_txs_max_num = 35;
    uint16_t confirm_txs_max_num = 30;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(proposal_para.get_table_account(), tablestate_highqc, /*locked_nonce_map,*/ all_txs_max_num, confirm_and_recv_txs_max_num, confirm_txs_max_num);
    std::vector<xcons_transaction_ptr_t> origin_txs = get_txpool()->get_ready_txs(txpool_pack_para);
    for (auto & tx : origin_txs) {
        xdbg_info("xproposal_maker_t::update_txpool_txs leader-get txs. %s tx=%s",
                proposal_para.dump().c_str(), tx->dump().c_str());
    }
    table_para.set_origin_txs(origin_txs);
    return true;
}

std::string xproposal_maker_t::calc_random_seed(base::xvblock_t* latest_cert_block, base::xvqcert_t* drand_cert, uint64_t viewtoken) {
    std::string random_str;
    uint64_t last_block_nonce = latest_cert_block->get_cert()->get_nonce();
    std::string drand_signature = drand_cert->get_verify_signature();
    xassert(!drand_signature.empty());
    random_str = base::xstring_utl::tostring(last_block_nonce);
    random_str += base::xstring_utl::tostring(viewtoken);
    random_str += drand_signature;
    uint64_t seed = base::xhash64_t::digest(random_str);
    return base::xstring_utl::tostring(seed);
}

bool xproposal_maker_t::leader_set_consensus_para(base::xvblock_t* latest_cert_block, xblock_consensus_para_t & cs_para) {
    uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
    cs_para.set_timeofday_s(now);

    uint64_t total_lock_tgas_token = 0;
    uint64_t property_height = 0;
    bool ret = store::xtgas_singleton::get_instance().leader_get_total_lock_tgas_token(cs_para.get_clock(), total_lock_tgas_token, property_height);
    if (!ret) {
        xwarn("xproposal_maker_t::leader_set_consensus_para fail-leader_get_total_lock_tgas_token. %s", cs_para.dump().c_str());
        return ret;
    }
    xblockheader_extra_data_t blockheader_extradata;
    blockheader_extradata.set_tgas_total_lock_amount_property_height(property_height);
    std::string extra_data;
    blockheader_extradata.serialize_to_string(extra_data);
    xassert(cs_para.get_drand_block() != nullptr);
    std::string random_seed = calc_random_seed(latest_cert_block, cs_para.get_drand_block()->get_cert(), cs_para.get_viewtoken());
    cs_para.set_parent_height(latest_cert_block->get_height() + 1);
    cs_para.set_tableblock_consensus_para(cs_para.get_drand_block()->get_height(),
                                            random_seed,
                                            total_lock_tgas_token,
                                            extra_data);
    xdbg_info("xtable_blockmaker_t::set_consensus_para %s random_seed=%s,tgas_token=%" PRIu64 ",tgas_height=%" PRIu64 " leader",
        cs_para.dump().c_str(), random_seed.c_str(), total_lock_tgas_token, property_height);
    return true;
}

bool xproposal_maker_t::backup_set_consensus_para(base::xvblock_t* latest_cert_block, base::xvblock_t* proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para) {
    uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
    cs_para.set_timeofday_s(now);

    cs_para.set_parent_height(latest_cert_block->get_height() + 1);
    cs_para.set_common_consensus_para(proposal->get_cert()->get_clock(),
                                      proposal->get_cert()->get_validator(),
                                      proposal->get_cert()->get_auditor(),
                                      proposal->get_cert()->get_viewid(),
                                      proposal->get_cert()->get_viewtoken(),
                                      proposal->get_cert()->get_drand_height());
    if (proposal->get_block_class() == base::enum_xvblock_class_light) {
        uint64_t property_height = 0;
        uint64_t total_lock_tgas_token = 0;
        xassert(!proposal->get_header()->get_extra_data().empty());
        if (!proposal->get_header()->get_extra_data().empty()) {
            const std::string & extra_data = proposal->get_header()->get_extra_data();
            xblockheader_extra_data_t blockheader_extradata;
            int32_t ret = blockheader_extradata.deserialize_from_string(extra_data);
            if (ret <= 0) {
                xerror("xtable_blockmaker_t::verify_block fail-extra data invalid");
                return false;
            }

            property_height = blockheader_extradata.get_tgas_total_lock_amount_property_height();
            bool bret = store::xtgas_singleton::get_instance().backup_get_total_lock_tgas_token(proposal->get_cert()->get_clock(), property_height, total_lock_tgas_token);
            if (!bret) {
                xwarn("xtable_blockmaker_t::verify_block fail-backup_set_consensus_para.proposal=%s", proposal->dump().c_str());
                return bret;
            }
        }

        std::string random_seed = calc_random_seed(latest_cert_block, bind_drand_cert, proposal->get_cert()->get_viewtoken());
        cs_para.set_tableblock_consensus_para(proposal->get_cert()->get_drand_height(),
                                              random_seed,
                                              total_lock_tgas_token,
                                              proposal->get_header()->get_extra_data());
        xdbg_info("xtable_blockmaker_t::set_consensus_para proposal=%s,random_seed=%s,tgas_token=%" PRIu64 " backup",
            proposal->dump().c_str(), random_seed.c_str(), total_lock_tgas_token);
    }
    return true;
}

void xproposal_maker_t::get_locked_nonce_map(const xblock_ptr_t & block, std::map<std::string, uint64_t> & locked_nonce_map) const {
    auto tx_actions = block->get_tx_actions();
    for (auto & action : tx_actions) {
        if (action.get_org_tx_hash().empty()) {  // not txaction
            continue;
        }
        base::enum_transaction_subtype _subtype = (base::enum_transaction_subtype)action.get_org_tx_action_id();

        if (_subtype == enum_transaction_subtype_self || _subtype == enum_transaction_subtype_send) {
            xlightunit_action_t txaction(action);
            xtransaction_ptr_t _rawtx = block->query_raw_transaction(txaction.get_tx_hash());
            if (_rawtx != nullptr) {
                uint64_t txnonce = _rawtx->get_tx_nonce();
                const std::string uri = txaction.get_contract_uri();
                const std::string & account_addr = _rawtx->get_source_addr();
                auto it = locked_nonce_map.find(account_addr);
                xdbg("xproposal_maker_t::get_locked_nonce_map account:%s,nonce:%u", account_addr.c_str(), txnonce);
                if (it == locked_nonce_map.end()) {
                    locked_nonce_map[account_addr] = txnonce;
                } else {
                    if (it->second < txnonce) {
                        locked_nonce_map[account_addr] = txnonce;
                    }
                }
            }
        }
    }
}

void xproposal_maker_t::sys_contract_sync(const xtablestate_ptr_t & tablestate) const {
    // always sync sharding sys contract
    if (m_table_maker->get_zone_index() != base::enum_chain_zone_consensus_index) {
        return;  // sync module already do rec and zec table sync
    }
    std::string sharding_vote_addr = base::xvaccount_t::make_account_address(sys_contract_sharding_vote_addr, m_table_maker->get_ledger_subaddr());
    std::string sharding_reward_claiming_addr = base::xvaccount_t::make_account_address(sys_contract_sharding_reward_claiming_addr, m_table_maker->get_ledger_subaddr());
    check_and_sync_account(tablestate, sharding_vote_addr);
    check_and_sync_account(tablestate, sharding_reward_claiming_addr);
}

void xproposal_maker_t::check_and_sync_account(const xtablestate_ptr_t & tablestate, const std::string & addr) const {
    base::xvaccount_t _vaddr(addr);
    base::xaccount_index_t accountindex;
    tablestate->get_account_index(addr, accountindex);
    xblocktool_t::check_lacking_unit_and_try_sync(_vaddr, accountindex, get_blockstore(), "proposal_maker");
}


NS_END2
