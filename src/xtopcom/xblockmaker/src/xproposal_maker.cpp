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

NS_BEG2(top, blockmaker)

REG_XMODULE_LOG(chainbase::enum_xmodule_type::xmodule_type_xblockmaker, xblockmaker_error_to_string, xblockmaker_error_base+1, xblockmaker_error_max);

xproposal_maker_t::xproposal_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources) {
    m_resources = resources;
    m_indexstore = resources->get_indexstorehub()->get_index_store(account);
    m_table_maker = make_object_ptr<xtable_maker_t>(account, resources);  // TOOD(jimmy) global
    m_tableblock_batch_tx_num_residue = XGET_CONFIG(tableblock_batch_tx_max_num);  // TOOD(jimmy)
    m_max_account_num = XGET_CONFIG(tableblock_batch_unitblock_max_num);
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

xblock_ptr_t xproposal_maker_t::make_proposal(data::xblock_consensus_para_t & proposal_para) {
    // get tablestate related to latest cert block
    auto & latest_cert_block = proposal_para.get_latest_cert_block();
    xtablestate_ptr_t tablestate = m_indexstore->clone_tablestate(latest_cert_block);
    if (nullptr == tablestate) {
        xwarn("xproposal_maker_t::make_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", proposal_para.dump().c_str(), latest_cert_block->get_height());
        return nullptr;
    }

    xtablemaker_para_t table_para(tablestate);
    // get batch txs
    update_txpool_txs(proposal_para, table_para);

    if (false == leader_set_consensus_para(latest_cert_block.get(), proposal_para)) {
        xwarn("xproposal_maker_t::make_proposal fail-leader_set_consensus_para.%s",
            proposal_para.dump().c_str());
        return nullptr;
    }

    xtablemaker_result_t tablemaker_result;
    xblock_ptr_t proposal_block = m_table_maker->make_proposal(table_para, proposal_para, tablemaker_result);
    if (proposal_block == nullptr) {
        if (xblockmaker_error_no_need_make_table != tablemaker_result.m_make_block_error_code) {
            xwarn("xproposal_maker_t::make_proposal fail-make_proposal.%s error_code=%s",
                proposal_para.dump().c_str(), chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
        } else {
            xdbg("xproposal_maker_t::make_proposal no need make table.%s",
                proposal_para.dump().c_str());
        }
        return nullptr;
    }

    auto & proposal_input = table_para.get_proposal();
    std::string proposal_input_str;
    proposal_input->serialize_to_string(proposal_input_str);
    proposal_block->get_input()->set_proposal(proposal_input_str);
    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);
    xdbg("xproposal_maker_t::make_proposal succ.%s,proposal_block=%s", proposal_para.dump().c_str(), proposal_block->dump().c_str());
    return proposal_block;
}

int xproposal_maker_t::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(get_account());
    xblock_consensus_para_t cs_para(get_account(), proposal_block->get_clock(), proposal_block->get_viewid(), proposal_block->get_viewtoken(), proposal_block->get_height());
    cs_para.set_latest_blocks(latest_blocks);

    if (latest_blocks.get_latest_cert_block()->get_height() >= proposal_block->get_height()) {
        xwarn("xproposal_maker_t::verify_proposal fail-proposal behind. proposal=%s,cert=%s",
            proposal_block->dump().c_str(), latest_blocks.get_latest_cert_block()->dump().c_str());
        return xblockmaker_error_proposal_outofdate;
    }

    if (latest_blocks.get_latest_cert_block()->get_height() + 1 < proposal_block->get_height()) {
        xwarn("xproposal_maker_t::verify_proposal fail-proposal behind. proposal=%s,cert=%s",
            proposal_block->dump().c_str(), latest_blocks.get_latest_cert_block()->dump().c_str());
        return xblockmaker_error_proposal_too_future;
    }

    if (!xblocktool_t::verify_latest_blocks(latest_blocks)) {
        xwarn("xproposal_maker_t::verify_proposal. fail-verify_latest_blocks fail.proposal=%s",
            proposal_block->dump().c_str());
        return xblockmaker_error_latest_table_blocks_invalid;
    }

    // get proposal prev block
    xblock_ptr_t proposal_prev_block = verify_proposal_prev_block(proposal_block, latest_blocks.get_latest_cert_block());
    if (proposal_prev_block == nullptr) {
        xwarn("xproposal_maker_t::verify_proposal fail-find no prev block. proposal=%s", proposal_block->dump().c_str());
        return xblockmaker_error_proposal_cannot_connect_to_cert;
    }
    if (proposal_prev_block->get_block_hash() != latest_blocks.get_latest_cert_block()->get_block_hash()) {
        cs_para.update_latest_cert_block(proposal_prev_block);
        xinfo("xproposal_maker_t::verify_proposal. change latest_cert_block.proposal=%s, latest_cert_block=%s",
            proposal_block->dump().c_str(), proposal_prev_block->dump().c_str());
    }

    // get tablestate related to latest cert block
    xtablestate_ptr_t tablestate = m_indexstore->clone_tablestate(proposal_prev_block);
    if (nullptr == tablestate) {
        xwarn("xproposal_maker_t::verify_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", cs_para.dump().c_str(), proposal_prev_block->get_height());
        return xblockmaker_error_proposal_table_state_clone;
    }
    xtablemaker_para_t table_para(tablestate);
    if (false == verify_proposal_input(proposal_block, cs_para.get_latest_committed_block(), table_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-proposal input invalid. proposal=%s",
            proposal_block->dump().c_str());
        return xblockmaker_error_proposal_bad_input;
    }

    // get proposal drand block
    xblock_ptr_t drand_block = nullptr;
    if (false == verify_proposal_drand_block(proposal_block, drand_block)) {
        xwarn("xproposal_maker_t::verify_proposal fail-find drand block. proposal=%s,drand_height=%" PRIu64 "",
            proposal_block->dump().c_str(), proposal_block->get_cert()->get_drand_height());
        // TODO(jimmy) invoke_sync(account, "tableblock backup sync");  XMETRICS_COUNTER_INCREMENT("txexecutor_cons_invoke_sync", 1);
        return xblockmaker_error_proposal_bad_drand;
    }

    base::xvqcert_t* bind_drand_cert = drand_block != nullptr ? drand_block->get_cert() : nullptr;
    if (false == backup_set_consensus_para(proposal_prev_block.get(), proposal_block, bind_drand_cert, cs_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-backup_set_consensus_para. proposal=%s",
            proposal_block->dump().c_str());
        return xblockmaker_error_proposal_bad_consensus_para;
    }

    int32_t verify_ret = m_table_maker->verify_proposal(proposal_block, table_para, cs_para);
    if (verify_ret != xsuccess) {
        xwarn("xproposal_maker_t::verify_proposal fail-verify_proposal. proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(verify_ret).c_str());
        return verify_ret;
    }
    xdbg_info("xproposal_maker_t::verify_proposal succ. proposal=%s",
        proposal_block->dump().c_str());
    return xsuccess;
}

bool xproposal_maker_t::verify_proposal_input(base::xvblock_t *proposal_block, const xblock_ptr_t & committed_block, xtablemaker_para_t & table_para) {
    if (proposal_block->get_block_class() != base::enum_xvblock_class_light) {
        return true;
    }
    xtable_block_t* tableblock = dynamic_cast<xtable_block_t*>(proposal_block);
    if (tableblock == nullptr) {
        xerror("xproposal_maker_t::verify_proposal_input fail-not light table. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    uint64_t table_commit_height = committed_block->get_height();
    xassert(table_commit_height < proposal_block->get_height());

    std::string proposal_input_str = proposal_block->get_input()->get_proposal();
    xtable_proposal_input_ptr_t proposal_input = make_object_ptr<xtable_proposal_input_t>();
    int32_t ret = proposal_input->serialize_from_string(proposal_input_str);
    if (ret <= 0) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table serialize from proposal input. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }

    const std::vector<xcons_transaction_ptr_t> & origin_txs = proposal_input->get_input_txs();
    const std::vector<std::string> & other_accounts = proposal_input->get_other_accounts();
    if (origin_txs.empty() && other_accounts.empty()) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table proposal input empty. proposal=%s",
            proposal_block->dump().c_str());
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

    base::xauto_ptr<base::xvblock_t> _drand_vblock = get_blockstore()->load_block_object(base::xvaccount_t(sys_drand_addr), drand_height, 0, true);
    if (_drand_vblock == nullptr) {
        XMETRICS_PACKET_INFO("consensus_tableblock",
                            "fail_find_drand", proposal_block->dump(),
                            "drand_height", drand_height);
        return false;
    }

    drand_block = xblock_t::raw_vblock_to_object_ptr(_drand_vblock.get());
    return true;
}

xblock_ptr_t xproposal_maker_t::verify_proposal_prev_block(base::xvblock_t * proposal_block, base::xvblock_t* default_latest_cert) const {
    if (proposal_block->get_last_block_hash() == default_latest_cert->get_block_hash()) {
        xblock_ptr_t proposal_prev_block = xblock_t::raw_vblock_to_object_ptr(default_latest_cert);
        xassert(proposal_block->get_height() == proposal_prev_block->get_height() + 1);
        return proposal_prev_block;
    }

    base::xblock_vector latest_certs = get_blockstore()->load_block_object(*m_table_maker, proposal_block->get_height() - 1);
    if (false == latest_certs.get_vector().empty()) {
        for (auto & latest_cert : latest_certs.get_vector()) {
            if (latest_cert->get_block_hash() == proposal_block->get_last_block_hash()) {  // found conected prev one
                xblock_ptr_t proposal_prev_block = xblock_t::raw_vblock_to_object_ptr(latest_cert);
                return proposal_prev_block;
            }
        }
    }
    return nullptr;
}

bool xproposal_maker_t::update_txpool_txs(const xblock_consensus_para_t & proposal_para, xtablemaker_para_t & table_para) {
    // update committed receiptid state for txpool, pop output finished txs
    if (proposal_para.get_latest_committed_block()->get_height() > 0) {
        auto tablestate_commit = m_indexstore->clone_tablestate(proposal_para.get_latest_committed_block());
        if (nullptr == tablestate_commit) {
            xwarn("xproposal_maker_t::update_txpool_txs fail clone tablestate. %s,committed_block=%s",
                proposal_para.dump().c_str(), proposal_para.get_latest_committed_block()->dump().c_str());
            return false;
        }
        get_txpool()->update_receiptid_state(proposal_para.get_table_account(), tablestate_commit->get_receiptid_state());

        // update locked txs for txpool, locked txs come from two latest tableblock
        std::vector<xtxpool_v2::tx_info_t> locked_tx_vec;
        get_locked_txs(proposal_para.get_latest_cert_block(), locked_tx_vec);
        get_locked_txs(proposal_para.get_latest_locked_block(), locked_tx_vec);
        get_txpool()->update_locked_txs(get_account(), locked_tx_vec, tablestate_commit->get_receiptid_state());
    }

    // get table batch txs for execute and make block
    auto & tablestate_highqc = table_para.get_tablestate();
    uint16_t send_txs_max_num = 16;  // TODO(jimmy) config paras
    uint16_t recv_txs_max_num = 32;
    uint16_t confirm_txs_max_num = 32;
    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(proposal_para.get_table_account(), tablestate_highqc->get_receiptid_state(), send_txs_max_num, recv_txs_max_num, confirm_txs_max_num);
    std::vector<xcons_transaction_ptr_t> origin_txs = get_txpool()->get_ready_txs(txpool_pack_para);
    for (auto & tx : origin_txs) {
        xdbg("xproposal_maker_t::update_txpool_txs leader-get txs. %s tx=%s",
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
    uint64_t total_lock_tgas_token = 0;
    uint64_t property_height = 0;
    bool ret = store::xtgas_singleton::get_instance().leader_get_total_lock_tgas_token(get_blockstore(),
        cs_para.get_clock(), total_lock_tgas_token, property_height);
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
    cs_para.set_tableblock_consensus_para(cs_para.get_drand_block()->get_height(),
                                            random_seed,
                                            total_lock_tgas_token,
                                            extra_data);
    xdbg_info("xtable_blockmaker_t::set_consensus_para %s random_seed=%s,tgas_token=%" PRIu64 ",tgas_height=%" PRIu64 " leader",
        cs_para.dump().c_str(), random_seed.c_str(), total_lock_tgas_token, property_height);
    return true;
}

bool xproposal_maker_t::backup_set_consensus_para(base::xvblock_t* latest_cert_block, base::xvblock_t* proposal, base::xvqcert_t * bind_drand_cert, xblock_consensus_para_t & cs_para) {
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
            bool bret = store::xtgas_singleton::get_instance().backup_get_total_lock_tgas_token(get_blockstore(), proposal->get_cert()->get_clock(), property_height, total_lock_tgas_token);
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

void xproposal_maker_t::get_locked_txs(const xblock_ptr_t & block, std::vector<xtxpool_v2::tx_info_t> & locked_tx_vec) const {
    const auto & units = block->get_tableblock_units(false);
    for (auto unit : units) {
        if (unit->get_block_class() != base::enum_xvblock_class_light) {
            continue;
        }
        data::xlightunit_block_t * lightunit = dynamic_cast<data::xlightunit_block_t *>(unit.get());
        const std::vector<xlightunit_tx_info_ptr_t> & txs = lightunit->get_txs();

        for (auto & tx : txs) {
            xtxpool_v2::tx_info_t txinfo(lightunit->get_account(), tx->get_tx_hash_256(), tx->get_tx_subtype());
            locked_tx_vec.push_back(txinfo);
        }
    }
}

NS_END2
