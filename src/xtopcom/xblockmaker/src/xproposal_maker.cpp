// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xblockmaker/xproposal_maker.h"
#include "xblockmaker/xblock_maker_para.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xerror/xerror.h"
#include "xstore/xtgas_singleton.h"
#include "xdata/xblocktool.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xblockbuild.h"
#include "xdata/xblockextract.h"
#include "xmbus/xevent_behind.h"
#include "xchain_fork/xutility.h"
#include "xgasfee/xgas_estimate.h"
#include "xstatestore/xstatestore_face.h"

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

data::xblock_consensus_para_ptr_t   xproposal_maker_t::leader_set_consensus_para_basic(base::xvblock_t* _cert_block, uint64_t viewid, uint64_t clock, std::error_code & ec) {
    uint32_t viewtoken = base::xtime_utl::get_fast_randomu();
    uint64_t gmtime = base::xtime_utl::gettimeofday();
    data::xblock_consensus_para_ptr_t cs_para = std::make_shared<data::xblock_consensus_para_t>(get_account(), clock, viewid, viewtoken, _cert_block->get_height() + 1, gmtime);

    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(*m_table_maker);
    if (!data::xblocktool_t::verify_latest_blocks(latest_blocks)) {
        ec = error::xenum_errc::blockmaker_load_block_fail;
        xwarn("xproposal_maker_t::leader_set_consensus_para_basic. fail-verify_latest_blocks.%s",
            cs_para->dump().c_str());
        return nullptr;
    }
    if (_cert_block->get_height() != latest_blocks.get_latest_cert_block()->get_height()
        || _cert_block->get_block_hash() != latest_blocks.get_latest_cert_block()->get_block_hash()) {
        ec = error::xenum_errc::blockmaker_cert_block_changed;
        xwarn("xproposal_maker_t::leader_set_consensus_para_basic. fail-cert block changed.%s",
            cs_para->dump().c_str());
        return nullptr;
    }
    cs_para->set_latest_blocks(latest_blocks);

    auto latest_connected_block = get_blockstore()->get_latest_connected_block(*m_table_maker);
    if (latest_connected_block == nullptr) {
        ec = error::xenum_errc::blockmaker_load_block_fail;
        xunit_error("xproposal_maker_t::leader_set_consensus_para_basic fail-get latest connect block,%s",
            cs_para->dump().c_str());
        return nullptr;
    }
    if (latest_connected_block->get_height() != latest_blocks.get_latest_committed_block()->get_height()) {
        ec = error::xenum_errc::blockmaker_connect_block_behind;
        xunit_warn("xproposal_maker_t::leader_set_consensus_para_basic fail-latest connect not match committed block,%s,connect_height=%ld",
            cs_para->dump().c_str(), latest_connected_block->get_height());
        return nullptr;
    }

    base::xauto_ptr<base::xvblock_t> drand_block = get_blockstore()->get_latest_committed_block(base::xvaccount_t(sys_drand_addr));
    if (drand_block->get_clock() == 0) {
        ec = error::xenum_errc::blockmaker_connect_block_behind;
        xwarn("xproposal_maker_t::leader_set_consensus_para_basic fail-no valid drand. %s", cs_para->dump().c_str());
        return nullptr;
    }
    cs_para->set_drand_block(drand_block.get());

    data::xtablestate_ptr_t tablestate = get_target_tablestate(latest_blocks.get_latest_cert_block());
    if (nullptr == tablestate) {
        ec = error::xenum_errc::blockmaker_drand_block_invalid;
        xwarn("xproposal_maker_t::leader_set_consensus_para_basic fail clone cert tablestate. %s", cs_para->dump().c_str());
        return nullptr;
    }

    data::xtablestate_ptr_t tablestate_commit = get_target_tablestate(latest_blocks.get_latest_committed_block());
    if (tablestate_commit == nullptr) {
        ec = error::xenum_errc::blockmaker_drand_block_invalid;
        xwarn("xproposal_maker_t::leader_set_consensus_para_basic fail clone commit tablestate. %s", cs_para->dump().c_str());
        return nullptr;
    }
    cs_para->set_table_state(tablestate, tablestate_commit);

    // TODO(jimmy) keep for help txpool clear cache
    update_txpool_table_state(latest_blocks.get_latest_committed_block(), latest_blocks.get_latest_locked_block(), latest_blocks.get_latest_cert_block(), tablestate_commit);
    return cs_para;
}

bool xproposal_maker_t::account_index_upgrade() {
    return m_table_maker->account_index_upgrade();
}

int xproposal_maker_t::backup_verify_and_set_consensus_para_basic(xblock_consensus_para_t & cs_para, base::xvblock_t *proposal_block) {
    uint64_t gmtime = proposal_block->get_second_level_gmtime();
    uint64_t now = (uint64_t)base::xtime_utl::gettimeofday();
    if ( (gmtime > (now + 60)) || (gmtime < (now - 60))) { // the gmtime of leader should in +-60s with backup node
        xwarn("xproposal_maker_t::verify_proposal fail-gmtime not match. proposal=%s,leader_gmtime=%ld,backup_gmtime=%ld",
            proposal_block->dump().c_str(), gmtime, now);
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_outofdate;
    }

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

    //find matched lock block
    xblock_ptr_t prev_lock_block;
    if (proposal_prev_block->get_height() > 0) {
        auto lock_block = get_blockstore()->load_block_object(*m_table_maker, proposal_prev_block->get_height() - 1, proposal_prev_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_proposer_verify_proposal);
        if (lock_block == nullptr) {
            xwarn("xproposal_maker_t::verify_proposal fail-find lock block. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;
        }
        prev_lock_block = xblock_t::raw_vblock_to_object_ptr(lock_block.get());
    } else {
        prev_lock_block = proposal_prev_block;
    }
    //find matched commit block
    xblock_ptr_t prev_commit_block;
    if (prev_lock_block->get_height() > 0) {
        // XTODO get latest connected block which can also load commit block, and it will invoke to update latest connect height.
        auto connect_block = get_blockstore()->get_latest_connected_block(*m_table_maker);
        if (connect_block == nullptr) {
            xerror("xproposal_maker_t::verify_proposal fail-find connected block. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;            
        }
        if (connect_block->get_height() != prev_lock_block->get_height() - 1) {
            xwarn("xproposal_maker_t::verify_proposal fail-connect not match commit block. proposal=%s,connect_height=%ld", proposal_block->dump().c_str(), connect_block->get_height());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_blocks_invalid, 1);
            XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
            return xblockmaker_error_proposal_cannot_connect_to_cert;            
        }
        prev_commit_block = xblock_t::raw_vblock_to_object_ptr(connect_block.get());
    } else {
        prev_commit_block = prev_lock_block;
    }

    cs_para.set_latest_blocks(proposal_prev_block, prev_lock_block, prev_commit_block);
    xdbg_info("xproposal_maker_t::verify_proposal. set latest_cert_block.proposal=%s, latest_cert_block=%s",
        proposal_block->dump().c_str(), proposal_prev_block->dump().c_str());

    // update txpool receiptid state
    const xblock_ptr_t & commit_block = cs_para.get_latest_committed_block();
    data::xtablestate_ptr_t commit_tablestate = get_target_tablestate(commit_block.get());
    if (commit_tablestate == nullptr) {
        xwarn("xproposal_maker_t::verify_proposal fail clone tablestate. %s,cert=%s", cs_para.dump().c_str(), proposal_prev_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_state_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_table_state_clone;
    }   

    // get tablestate related to latest cert block
    data::xtablestate_ptr_t tablestate = get_target_tablestate(proposal_prev_block.get());
    if (nullptr == tablestate) {
        xwarn("xproposal_maker_t::verify_proposal fail clone tablestate. %s,cert=%s", cs_para.dump().c_str(), proposal_prev_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_state_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_table_state_clone;
    }
    cs_para.set_table_state(tablestate, commit_tablestate);

    // TODO(jimmy) keep for help txpool clear cache
    update_txpool_table_state(commit_block.get(), prev_lock_block.get(), proposal_prev_block.get(), commit_tablestate);
    return xsuccess;
}

bool xproposal_maker_t::can_make_proposal(data::xblock_consensus_para_t & proposal_para) {
    xassert(false);
    return true;
}

data::xtablestate_ptr_t xproposal_maker_t::get_target_tablestate(base::xvblock_t * block) {
    return statestore::xstatestore_hub_t::instance()->get_table_state_by_block(block);
}

xblock_ptr_t xproposal_maker_t::make_proposal(data::xblock_consensus_para_t & proposal_para, uint32_t min_tx_num) {
    // XMETRICS_TIMER(metrics::cons_make_proposal_tick);
    XMETRICS_TIME_RECORD("cons_make_proposal_cost");
    // get tablestate related to latest cert block
    auto & latest_cert_block = proposal_para.get_latest_cert_block();
    data::xtablestate_ptr_t tablestate = proposal_para.get_cert_table_state();
    if (nullptr == tablestate) {
        xwarn("xproposal_maker_t::make_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", proposal_para.dump().c_str(), latest_cert_block->get_height());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_table_state, 1);
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        return nullptr;
    }

    data::xtablestate_ptr_t tablestate_commit = proposal_para.get_commit_table_state();
    if (tablestate_commit == nullptr) {
        xwarn("xproposal_maker_t::make_proposal fail clone tablestate. %s,commit_height=%" PRIu64 "", proposal_para.dump().c_str(), proposal_para.get_latest_committed_block()->get_height());
        XMETRICS_GAUGE(metrics::cons_fail_make_proposal_table_state, 1);
        XMETRICS_GAUGE(metrics::cons_table_leader_make_proposal_succ, 0);
        return nullptr;
    }

    xtablemaker_para_t table_para(tablestate, tablestate_commit);
    // get batch txs
    update_txpool_txs(proposal_para, table_para);
    if (table_para.get_origin_txs().size() < min_tx_num) {
        xinfo("xproposal_maker_t::make_proposal tx number too small:%d,min num:%d. %s,cert_height=%" PRIu64 "",
              table_para.get_origin_txs().size(),
              min_tx_num,
              proposal_para.dump().c_str(),
              latest_cert_block->get_height());
        return nullptr;
    }
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

    // need full cert block
    //get_blockstore()->load_block_input(*m_table_maker.get(), latest_cert_block.get());
    //get_blockstore()->load_block_output(*m_table_maker.get(), latest_cert_block.get());

    auto & receiptid_info_map = table_para.get_receiptid_info_map();
    for (auto & receiptid_info : receiptid_info_map) {
        auto & receiptid_state_prove = receiptid_info.second.m_property_prove_ptr;
        table_para.push_receiptid_state_prove(receiptid_state_prove);
    }

    auto & proposal_input = table_para.get_proposal();
    std::string proposal_input_str;
    proposal_input->serialize_to_string(proposal_input_str);
    proposal_block->set_proposal(proposal_input_str);
    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);

    // add metrics of tx counts / table counts ratio
    XMETRICS_GAUGE(metrics::cons_table_leader_make_unit_count, table_result.m_succ_unit_num);
    XMETRICS_GAUGE(metrics::cons_table_leader_make_tx_count, proposal_input->get_input_txs().size());    

    std::string block_object_bin;
    proposal_block->serialize_to_string(block_object_bin);
    xinfo("xproposal_maker_t::make_proposal succ.block=%s,size=%zu,%zu,%zu,%zu,%zu,input={size=%zu,txs=%zu,accounts=%zu}", 
        proposal_block->dump().c_str(),
        block_object_bin.size()+proposal_block->get_input()->get_resources_data().size()+proposal_block->get_output()->get_resources_data().size()+proposal_block->get_output_offdata().size(),
        block_object_bin.size(),proposal_block->get_input()->get_resources_data().size(), proposal_block->get_output()->get_resources_data().size(),
        proposal_block->get_output_offdata().size(),
        proposal_input_str.size(), proposal_input->get_input_txs().size(), proposal_input->get_other_accounts().size());
    return proposal_block;
}

int xproposal_maker_t::verify_proposal(xblock_consensus_para_t & cs_para, base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    // XMETRICS_TIMER(metrics::cons_verify_proposal_tick);
    XMETRICS_TIME_RECORD("cons_verify_proposal_cost");
    xdbg("xproposal_maker_t::verify_proposal enter. proposal=%s", proposal_block->dump().c_str());
    
    int ret = backup_verify_and_set_consensus_para_basic(cs_para, proposal_block);
    if (ret) {
        return ret;
    }

    xtablemaker_para_t table_para(cs_para.get_cert_table_state(), cs_para.get_commit_table_state());

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
    if (false == backup_set_consensus_para(cs_para.get_latest_cert_block().get(), proposal_block, bind_drand_cert, cs_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-backup_set_consensus_para. proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_consensus_para_get, 1);
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return xblockmaker_error_proposal_bad_consensus_para;
    }

    int32_t verify_ret = m_table_maker->verify_proposal(proposal_block, table_para, cs_para);
    if (verify_ret != xsuccess) {
        xwarn("xproposal_maker_t::verify_proposal fail-verify_proposal. proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(verify_ret).c_str());
        XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 0);
        return verify_ret;
    }
    XMETRICS_GAUGE(metrics::cons_table_backup_verify_proposal_succ, 1);
    xdbg_info("xproposal_maker_t::verify_proposal succ. proposal=%s",
        proposal_block->dump().c_str());
    return xsuccess;
}

void xproposal_maker_t::set_certauth(base::xvcertauth_t* _ca) {
    m_resources->set_certauth(_ca);
}

bool xproposal_maker_t::verify_proposal_input(base::xvblock_t *proposal_block, xtablemaker_para_t & table_para) {
    if (proposal_block->get_block_class() != base::enum_xvblock_class_light) {
        return true;
    }
    data::xtable_block_t * tableblock = dynamic_cast<data::xtable_block_t *>(proposal_block);
    if (tableblock == nullptr) {
        xerror("xproposal_maker_t::verify_proposal_input fail-not light table. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    std::string proposal_input_str = proposal_block->get_proposal();
    xtable_proposal_input_ptr_t proposal_input = make_object_ptr<xtable_proposal_input_t>();
    int32_t ret = proposal_input->serialize_from_string(proposal_input_str);
    if (ret <= 0) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table serialize from proposal input. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    const std::vector<xcons_transaction_ptr_t> & origin_txs = proposal_input->get_input_txs();
    if (origin_txs.empty()) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table proposal input empty. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    ret = get_txpool()->verify_txs(get_account(), origin_txs);
    if (ret != xsuccess) {
        return false;
    }

    auto self_sid = table_para.get_tablestate()->get_receiptid_state()->get_self_tableid();
    std::map<base::xtable_shortid_t, xtxpool_v2::xreceiptid_state_and_prove> receiptid_info_map;
    for (auto & prove : proposal_input->get_receiptid_state_proves()) {
        if (!prove->is_valid()) {
            xerror("xproposal_maker_t::verify_proposal_input fail-prove invalid. proposal=%s", proposal_block->dump().c_str());
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_confirm_id_error, 1);
            return false;
        }
        auto peer_receiptid_state = data::xblocktool_t::get_receiptid_from_property_prove(prove);
        auto peer_sid = peer_receiptid_state->get_self_tableid();
        get_txpool()->update_peer_receipt_id_state(prove, peer_receiptid_state);

        base::xreceiptid_pair_t self_pair;
        table_para.get_tablestate()->get_receiptid_state()->find_pair(peer_sid, self_pair);
        auto sendid_max = self_pair.get_sendid_max();
        auto confirmid_max = self_pair.get_confirmid_max();
        auto confirm_rsp_id_max = self_pair.get_confirm_rsp_id_max();
        auto send_rsp_id_max = self_pair.get_send_rsp_id_max();

        base::xreceiptid_pair_t peer_pair;
        peer_receiptid_state->find_pair(self_sid, peer_pair);
        auto recvid_max = peer_pair.get_recvid_max();

        if (confirm_rsp_id_max != send_rsp_id_max || recvid_max <= confirmid_max || recvid_max > sendid_max) {
            xerror("xproposal_maker_t::verify_proposal_input fail-prove invalid recvid not bigger than confirm id. proposal=%s,self_pair:%s,peer sid:%d,recvid:%llu",
                   proposal_block->dump().c_str(),
                   self_pair.dump().c_str(),
                   peer_sid,
                   recvid_max);
            XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_confirm_id_error, 1);
            return false;
        }

        xinfo("xproposal_maker_t::verify_proposal_input proposal_block:%llu,receipt id prove:peer:%d,id:%llu:%llu", proposal_block->dump().c_str(), peer_sid, confirmid_max, recvid_max);
        receiptid_info_map[peer_sid] = xtxpool_v2::xreceiptid_state_and_prove(prove, peer_receiptid_state);
    }

    if (!receiptid_info_map.empty()) {
        for (auto & tx : origin_txs) {
            if (tx->is_confirm_tx()) {
                auto it = receiptid_info_map.find(tx->get_peer_tableid());
                if (it != receiptid_info_map.end()) {
                    xerror("xproposal_maker_t::verify_proposal_input fail-tx changed same peer table confirm id with prove. proposal=%s", proposal_block->dump().c_str());
                    XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_confirm_id_error, 1);
                    return false;
                }
            }
        }
    }

    // table_para.set_origin_txs(origin_txs);
    table_para.set_pack_resource(xtxpool_v2::xpack_resource(origin_txs, receiptid_info_map));
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

void xproposal_maker_t::update_txpool_table_state(base::xvblock_t* _commit_block, base::xvblock_t* _lock_block, base::xvblock_t* _cert_block, data::xtablestate_ptr_t const& commit_tablestate) {
    // TODO(jimmy) update txpool table state
    if (_commit_block->get_height() > 0) {
        base::xvproperty_prove_ptr_t property_prove_ptr = nullptr;
        data::xtablestate_ptr_t tablestate_ptr = nullptr;
        auto ret = statestore::xstatestore_hub_t::instance()->get_receiptid_state_and_prove(common::xaccount_address_t(m_table_maker->get_account()), _commit_block, property_prove_ptr, tablestate_ptr);
        if (!ret) {
            xwarn("xproposal_maker_t::update_txpool_txs create receipt state and prove fail.table:%s, commit height:%llu", get_account().c_str(), _commit_block->get_height());
        }
        get_txpool()->update_table_state(property_prove_ptr, commit_tablestate, _lock_block, _cert_block);
    }
}

bool xproposal_maker_t::update_txpool_txs(const xblock_consensus_para_t & proposal_para, xtablemaker_para_t & table_para) {
    // get table batch txs for execute and make block
    auto & tablestate_highqc = table_para.get_tablestate();
    // raise pack tx num thresold. notice proposal size will be enlardged.
    uint16_t all_txs_max_num = 40;  // TODO(jimmy) config paras
    uint16_t confirm_and_recv_txs_max_num = 35;
    uint16_t confirm_txs_max_num = 30;
    bool forked = chain_fork::xutility_t::is_forked(fork_points::xbft_msg_upgrade, proposal_para.get_clock());
    if (forked) {
        all_txs_max_num = 200;  // TODO(jimmy) config paras
        confirm_and_recv_txs_max_num = 160;
        confirm_txs_max_num = 120;
    }

    // TODO(jimmy)  proposal_para.get_table_account() == sys_contract_eth_table_block_addr_with_suffix
    if (proposal_para.get_table_account() == sys_contract_relay_table_block_addr) {
        all_txs_max_num = 5;
        confirm_and_recv_txs_max_num = 4;
        confirm_txs_max_num = 3;
    }

    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    auto & all_table_sids = get_txpool()->get_all_table_sids();
    std::vector<base::xtable_shortid_t> all_table_sids_vec;
    all_table_sids_vec.assign(all_table_sids.begin(), all_table_sids.end());
    peer_sids_for_confirm_id = select_peer_sids_for_confirm_id(all_table_sids_vec, proposal_para.get_proposal_height());

    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(
        proposal_para.get_table_account(), tablestate_highqc, proposal_para.get_latest_cert_block().get(), all_txs_max_num, confirm_and_recv_txs_max_num, confirm_txs_max_num, peer_sids_for_confirm_id);
    // std::vector<xcons_transaction_ptr_t> origin_txs = get_txpool()->get_ready_txs(txpool_pack_para);

    auto pack_resource = get_txpool()->get_pack_resource(txpool_pack_para);
    for (auto & tx : pack_resource.m_txs) {
        xdbg_info("xproposal_maker_t::update_txpool_txs leader-get txs. %s tx=%s",
                proposal_para.dump().c_str(), tx->dump().c_str());
    }
    // table_para.set_origin_txs(origin_txs);
    table_para.set_pack_resource(pack_resource);
    return true;
}

std::set<base::xtable_shortid_t> xproposal_maker_t::select_peer_sids_for_confirm_id(const std::vector<base::xtable_shortid_t> & all_sid_vec, uint64_t height) {
    std::set<base::xtable_shortid_t> peer_sids_for_confirm_id;
    // uint32_t batch_num = 16;
    // uint64_t height_interval = 8;
    // uint32_t part_num = (all_sid_vec.size() + batch_num - 1) / batch_num;
    // if (height % height_interval == 0) {
    //     if (part_num > 0) {
    //         uint32_t pos = (height / height_interval) % part_num;
    //         uint32_t upper_idx = std::min((pos + 1) * batch_num, (uint32_t)all_sid_vec.size());
    //         peer_sids_for_confirm_id.insert(all_sid_vec.begin() + pos * batch_num, all_sid_vec.begin() + upper_idx);
    //     }
    // }

    uint32_t batch_num = 4;
    uint64_t height_interval = 4;
    uint32_t part_num = (all_sid_vec.size() + batch_num - 1) / batch_num;
    if (height % height_interval == 0) {
        if (part_num > 0) {
            uint32_t pos = (height / height_interval) % part_num;
            uint32_t upper_idx = std::min((pos + 1) * batch_num, (uint32_t)all_sid_vec.size());
            peer_sids_for_confirm_id.insert(all_sid_vec.begin() + pos * batch_num, all_sid_vec.begin() + upper_idx);
        }
    }

    // peer_sids_for_confirm_id.insert(all_sid_vec[height%all_sid_vec.size()]);

    return peer_sids_for_confirm_id;
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

bool xproposal_maker_t::leader_xip_to_leader_address(xvip2_t _xip, common::xaccount_address_t & _addr) const {
    std::string leader_address = m_resources->get_certauth()->get_signer(_xip);
    if (leader_address.empty()) {
        return false;
    }
    // only T6 and T8 node can change to leader address for eth compatibility
    common::xaccount_address_t _coinbase;
    base::enum_vaccount_addr_type addr_type = base::xvaccount_t::get_addrtype_from_account(leader_address);
    if (addr_type != base::enum_vaccount_addr_type_secp256k1_eth_user_account && addr_type != base::enum_vaccount_addr_type_secp256k1_evm_user_account) {
        _coinbase = eth_zero_address;
    } else {
        _coinbase = common::xaccount_address_t(leader_address);
    }
    _addr = _coinbase;
    return true;
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
    xassert(cs_para.get_drand_block() != nullptr);
    std::string random_seed = calc_random_seed(latest_cert_block, cs_para.get_drand_block()->get_cert(), cs_para.get_viewtoken());
    cs_para.set_parent_height(latest_cert_block->get_height() + 1);
    cs_para.set_tableblock_consensus_para(cs_para.get_drand_block()->get_height(),
                                            random_seed,
                                            total_lock_tgas_token,
                                            property_height);

    common::xaccount_address_t leader_addr;
    if (false == leader_xip_to_leader_address(cs_para.get_leader_xip(), leader_addr)) {
        xwarn("xtable_maker_t::leader_set_consensus_para fail-get leader address. %s", cs_para.dump().c_str());
        return false;
    }
    cs_para.set_coinbase(leader_addr);
    cs_para.set_block_gaslimit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit));
    cs_para.set_block_base_price(gasfee::xgas_estimate::base_price());

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
            data::xtableheader_extra_t blockheader_extradata;
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
                                              property_height);
        xdbg_info("xproposal_maker_t::backup_set_consensus_para proposal=%s,random_seed=%s,tgas_token=%" PRIu64 " backup",
            proposal->dump().c_str(), random_seed.c_str(), total_lock_tgas_token);
    }

    common::xaccount_address_t leader_addr;
    if (false == leader_xip_to_leader_address(cs_para.get_leader_xip(), leader_addr)) {
        xwarn("xproposal_maker_t::backup_set_consensus_para fail-get leader address. %s", cs_para.dump().c_str());
        return false;
    }
    cs_para.set_coinbase(leader_addr);
    cs_para.set_block_gaslimit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit));
    cs_para.set_block_base_price(gasfee::xgas_estimate::base_price());
    return true;
}


NS_END2
