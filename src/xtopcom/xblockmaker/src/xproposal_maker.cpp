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
    // get batch txs
    xtablemaker_para_t table_para;
    update_txpool_txs(proposal_para, table_para);

    bool can_make_next_block = m_table_maker->can_make_next_block(table_para, proposal_para);
    if (!can_make_next_block) {
        xdbg("xproposal_maker_t::make_proposal no need make next block.%s",
            proposal_para.dump().c_str());
        return nullptr;
    }

    auto & latest_cert_block = proposal_para.get_latest_cert_block();

    // get tablestate related to latest cert block
    table_para.m_tablestate = m_indexstore->clone_tablestate(latest_cert_block);
    if (nullptr == table_para.m_tablestate) {
        xwarn("xproposal_maker_t::make_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", proposal_para.dump().c_str(), latest_cert_block->get_height());
        return nullptr;
    }

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

    std::string proposal_input_str = table_para.m_proposal_input.to_string();
    proposal_block->get_input()->set_proposal(proposal_input_str);
    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);
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

    xtablemaker_para_t table_para;
    if (false == verify_proposal_input(proposal_block, cs_para.get_latest_committed_block(), table_para)) {
        xwarn("xproposal_maker_t::verify_proposal fail-proposal input invalid. proposal=%s",
            proposal_block->dump().c_str());
        return xblockmaker_error_proposal_bad_input;
    }

    // get tablestate related to latest cert block
    table_para.m_tablestate = m_indexstore->clone_tablestate(proposal_prev_block);
    if (nullptr == table_para.m_tablestate) {
        xwarn("xproposal_maker_t::verify_proposal fail clone tablestate. %s,cert_height=%" PRIu64 "", cs_para.dump().c_str(), proposal_prev_block->get_height());
        return xblockmaker_error_proposal_table_state_clone;
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

    int32_t serialize_ret = table_para.m_proposal_input.from_string(proposal_block->get_input()->get_proposal());
    if (serialize_ret <= 0) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table serialize from proposal input. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }
    const std::vector<xunit_proposal_input_t> & unit_proposal_inputs = table_para.m_proposal_input.get_unit_inputs();
    if (unit_proposal_inputs.size() == 0) {
        xerror("xproposal_maker_t::verify_proposal_input fail-table no proposal input. proposal=%s",
            proposal_block->dump().c_str());
        return false;
    }
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
    // TODO(jimmy) use base::xvaccount_t for performance
    auto ready_accounts = get_ready_txs(proposal_para);
    // xtxpool_v2::ready_accounts_t ready_accounts = get_txpool()->get_ready_accounts(get_account(), m_max_account_num);
    for (auto & ready_account : ready_accounts) {
        xassert(ready_account->get_txs().size() != 0);
        for (auto & tx : ready_account->get_txs()) {
            xinfo("xproposal_maker_t::update_txpool_txs leader-get txs. %s unit_account=%s,txs_size=%d,tx=%s",
                  proposal_para.dump().c_str(),
                  ready_account->get_addr().c_str(),
                  ready_account->get_txs().size(),
                  tx->dump().c_str());
        }
        table_para.set_unitmaker_txs(ready_account->get_addr(), ready_account->get_txs());
    }
    return table_para.m_proposal_input.get_unit_inputs().size() != 0;
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

void xproposal_maker_t::get_locked_accounts(const xblock_ptr_t & block, std::set<std::string> & locked_account_set) const {
    const auto & units = block->get_tableblock_units(false);
    for (auto unit : units) {
        if (unit->get_block_class() == base::enum_xvblock_class_light) {// TODO(jimmy) || unit->get_block_class() == base::enum_xvblock_class_full
            locked_account_set.insert(unit->get_account());
        }
    }
}

xtxpool_v2::ready_accounts_t xproposal_maker_t::get_ready_txs(const xblock_consensus_para_t & proposal_para) const {
    std::vector<xtxpool_v2::tx_info_t> locked_tx_vec;
    // locked txs come from two latest tableblock
    get_locked_txs(proposal_para.get_latest_cert_block(), locked_tx_vec);
    get_locked_txs(proposal_para.get_latest_locked_block(), locked_tx_vec);
    get_txpool()->update_locked_txs(get_account(), locked_tx_vec);

    // get tablestate related to latest cert block
    auto tablestate_commit = m_indexstore->clone_tablestate(proposal_para.get_latest_committed_block());
    if (nullptr == tablestate_commit) {
        xwarn("xproposal_maker_t::update_txpool_txs fail clone tablestate. %s", proposal_para.dump().c_str());
        return {};
    }

    get_txpool()->update_receiptid_state(proposal_para.get_table_account(), tablestate_commit->get_receiptid_state());

    auto tablestate_highqc = m_indexstore->clone_tablestate(proposal_para.get_latest_cert_block());
    if (nullptr == tablestate_highqc) {
        xwarn("xproposal_maker_t::update_txpool_txs fail clone tablestate. %s", proposal_para.dump().c_str());
        return {};
    }

    xtxpool_v2::xtxs_pack_para_t txpool_pack_para(proposal_para.get_table_account(), tablestate_highqc->get_receiptid_state(), 16, 32, 32);

    std::vector<xcons_transaction_ptr_t> ready_txs = get_txpool()->get_ready_txs(txpool_pack_para);

    std::set<std::string> locked_account_set;
    uint32_t filter_table_count = 0;
    // locked accounts come from two latest light tableblock
    if (filter_table_count < 2 && proposal_para.get_latest_cert_block()->get_block_class() == base::enum_xvblock_class_light) {
        get_locked_accounts(proposal_para.get_latest_cert_block(), locked_account_set);
        filter_table_count++;
    }
    if (filter_table_count < 2 && proposal_para.get_latest_locked_block()->get_block_class() == base::enum_xvblock_class_light) {
        get_locked_accounts(proposal_para.get_latest_locked_block(), locked_account_set);
        filter_table_count++;
    }
    if (filter_table_count < 2 && proposal_para.get_latest_committed_block()->get_block_class() == base::enum_xvblock_class_light) {
        get_locked_accounts(proposal_para.get_latest_committed_block(), locked_account_set);
        filter_table_count++;
    }
    return table_rules_filter(locked_account_set, tablestate_highqc->get_receiptid_state(), ready_txs);
}

bool xproposal_maker_t::is_match_account_fullunit_limit(const base::xvaccount_t & _account) const {
    base::xauto_ptr<base::xvblock_t> latest_cert_block = m_resources->get_blockstore()->get_latest_cert_block(_account);
    if (latest_cert_block == nullptr) {
        xassert(false);
        return false;
    }
    uint64_t current_height = latest_cert_block->get_height() + 1;
    uint64_t current_fullunit_height = latest_cert_block->get_block_class() == base::enum_xvblock_class_full ? latest_cert_block->get_height() : latest_cert_block->get_last_full_block_height();
    uint64_t current_lightunit_count = current_height - current_fullunit_height;
    uint64_t max_limit_lightunit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);  // TODO(jimmy)
    if (current_lightunit_count > max_limit_lightunit_count) {
        return true;
    }
    return false;
}

xtxpool_v2::ready_accounts_t xproposal_maker_t::table_rules_filter(const std::set<std::string> & locked_account_set,
                                                                   const base::xreceiptid_state_ptr_t & receiptid_state_highqc,
                                                                   const std::vector<xcons_transaction_ptr_t> & ready_txs) const {
    std::map<std::string, std::shared_ptr<xtxpool_v2::xready_account_t>> ready_account_map;

    enum_transaction_subtype last_tx_subtype = enum_transaction_subtype::enum_transaction_subtype_invalid;
    base::xtable_shortid_t last_peer_table_shortid;
    uint64_t last_receipt_id;

    for (auto & tx : ready_txs) {
        auto & account_addr = tx->get_account_addr();
        base::xvaccount_t _current_vaccount(account_addr);

        // remove locked accounts' txs
        auto it_locked_account_set = locked_account_set.find(account_addr);
        if (it_locked_account_set != locked_account_set.end()) {
            xdbg("xproposal_maker_t::table_rules_filter tx filtered for locked account. tx=%s", tx->dump(true).c_str());
            continue;
        }

        if (is_match_account_fullunit_limit(_current_vaccount)) {
            // send and self tx is filtered when matching fullunit limit
            if (tx->is_self_tx() || tx->is_send_tx()) {
                xdbg("xproposal_maker_t::table_rules_filter tx filtered for fullunit limit. tx=%s", tx->dump(true).c_str());
                continue;
            }
        }

        enum_transaction_subtype cur_tx_subtype;
        base::xtable_shortid_t cur_peer_table_sid;
        uint64_t cur_receipt_id;

        // make sure receipt id is continuous
        if (tx->is_recv_tx() || tx->is_confirm_tx()) {
            cur_tx_subtype = tx->get_tx_subtype();
            auto & target_account_addr = (tx->is_recv_tx()) ? tx->get_source_addr() : tx->get_target_addr();
            base::xvaccount_t _target_vaccount(target_account_addr);
            cur_peer_table_sid = _target_vaccount.get_short_table_id();
            cur_receipt_id = tx->get_last_action_receipt_id();
            if (cur_tx_subtype != last_tx_subtype || last_peer_table_shortid != cur_peer_table_sid) {
                base::xreceiptid_pair_t receiptid_pair;
                receiptid_state_highqc->find_pair(cur_peer_table_sid, receiptid_pair);
                last_receipt_id = tx->is_recv_tx() ? receiptid_pair.get_recvid_max() : receiptid_pair.get_confirmid_max();
            }
            if (cur_receipt_id != last_receipt_id + 1) {
                xdbg("xproposal_maker_t::table_rules_filter tx filtered for receiptid not contious. last_receipt_id=%ld, tx=%s", last_receipt_id, tx->dump().c_str());
                continue;
            }
        }

        // push to account level tx set, there is a filter rule for account tx set, push can be fail.
        bool ret = true;
        auto it_ready_account = ready_account_map.find(account_addr);
        if (it_ready_account == ready_account_map.end()) {
            auto ready_account = std::make_shared<xtxpool_v2::xready_account_t>(account_addr);
            ret = ready_account->put_tx(tx);
            ready_account_map[account_addr] = ready_account;
        } else {
            auto & ready_account = it_ready_account->second;
            ret = ready_account->put_tx(tx);
        }
        if (!ret) {
            xdbg("xproposal_maker_t::table_rules_filter tx filtered for account put tx fail. tx=%s", tx->dump().c_str());
        } else {
            // record success tx info for receiptid contious check
            if (tx->is_recv_tx() || tx->is_confirm_tx()) {
                last_tx_subtype = cur_tx_subtype;
                last_peer_table_shortid = cur_peer_table_sid;
                last_receipt_id = cur_receipt_id;
            }
        }
    }

    xtxpool_v2::ready_accounts_t ready_accounts;
    for (auto & ready_account_pair : ready_account_map) {
        ready_accounts.push_back(ready_account_pair.second);
    }
    return ready_accounts;
}

NS_END2
