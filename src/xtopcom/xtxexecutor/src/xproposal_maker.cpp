// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include "xbasic/xmemory.hpp"
#include "xtxexecutor/xproposal_maker.h"
#include "xtxexecutor/xblock_maker_para.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xstore/xtgas_singleton.h"
#include "xdata/xblocktool.h"

NS_BEG2(top, txexecutor)

xproposal_maker_t::xproposal_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources) {
    m_table_maker = make_object_ptr<xtable_maker_t>(account, resources);  // TOOD(jimmy) global
    m_txpool_table = make_observer(resources->get_txpool()->get_txpool_table(account));
    m_tableblock_batch_tx_num_residue = XGET_CONFIG(tableblock_batch_tx_max_num);  // TOOD(jimmy)
}

bool xproposal_maker_t::can_make_proposal(xblock_maker_para_t & proposal_para) {
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(get_account());

    if (proposal_para.get_viewid() <= latest_blocks.get_latest_cert_block()->get_viewid()) {
        xwarn("xproposal_maker_t::can_make_proposal fail-behind viewid. account=%s,viewid=%ld,clock=%ld,latest_viewid=%ld",
            get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock(), latest_blocks.get_latest_cert_block()->get_viewid());
        return false;
    }

    if (!xblocktool_t::verify_latest_blocks(latest_blocks)) {
        xwarn("xproposal_maker_t::can_make_proposal. fail-verify_latest_blocks fail.account=%s,viewid=%ld,clock=%ld",
            get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock());
        return false;
    }

    proposal_para.set_latest_blocks(latest_blocks);

    base::xauto_ptr<base::xvblock_t> timer_block = get_blockstore()->get_latest_cert_block(sys_contract_beacon_timer_addr);
    if (timer_block->get_clock() == proposal_para.get_clock()) {
        proposal_para.set_timer_block(timer_block.get());
    } else {
        base::xauto_ptr<base::xvblock_t> timer_block_2 = get_blockstore()->load_block_object(sys_contract_beacon_timer_addr, proposal_para.get_clock());
        if (timer_block_2 != nullptr) {
            xassert(timer_block_2->get_clock() == proposal_para.get_clock());
            proposal_para.set_timer_block(timer_block_2.get());
        } else {
            xwarn("xproposal_maker_t::can_make_proposal fail-no match timer. account=%s,viewid=%ld,clock=%ld",
                get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock());
            return false;
        }
    }

    base::xauto_ptr<base::xvblock_t> drand_block = get_blockstore()->get_latest_committed_block(sys_drand_addr);
    if (drand_block->get_clock() == 0) {
        xwarn("xproposal_maker_t::can_make_proposal fail-no valid drand. account=%s,viewid=%ld,clock=%ld",
            get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock());
        return false;
    }
    proposal_para.set_drand_block(drand_block.get());

    xdbg("xproposal_maker_t::can_make_proposal succ.account=%s,viewid=%ld,clock=%ld",
        get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock());
    return true;
}

xblock_ptr_t xproposal_maker_t::make_proposal(const xblock_maker_para_t & proposal_para) {
    auto & latest_cert_block = proposal_para.get_latest_cert_block();

    xblock_consensus_para_t cs_para;
    if (false == leader_set_consensus_para(latest_cert_block.get(), proposal_para, cs_para)) {
        xwarn("xproposal_maker_t::make_proposal fail-leader_set_consensus_para.account=%s,viewid=%ld,clock=%ld,cert=%s",
            get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock(), latest_cert_block->dump().c_str());
        return nullptr;
    }

    // get batch txs
    xtablemaker_para_t table_para(proposal_para.get_latest_cert_block(), proposal_para.get_latest_locked_block(), proposal_para.get_latest_committed_block());
    update_txpool_txs(proposal_para, cs_para.get_timestamp(), table_para);

    xtablemaker_result_t tablemaker_result;
    xblock_ptr_t proposal_block = m_table_maker->make_proposal(table_para, cs_para, tablemaker_result);
    if (proposal_block == nullptr) {
        xwarn("xproposal_maker_t::make_proposal fail-make_proposal.account=%s,viewid=%ld,clock=%ld,cert=%s,error_code=%s",
            get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock(), latest_cert_block->dump().c_str(),
            chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
        return nullptr;
    }

    std::string proposal_input_str = table_para.m_proposal_input.serialize_to_string();
    proposal_block->get_input()->set_proposal(proposal_input_str);
    bool bret = proposal_block->reset_prev_block(latest_cert_block.get());
    xassert(bret);
    xdbg_info("xproposal_maker_t::make_proposal succ-make_proposal.proposal=%s", proposal_block->dump().c_str());
    return proposal_block;
}

int xproposal_maker_t::verify_proposal(base::xvblock_t * proposal_block, base::xvqcert_t * bind_clock_cert) {
    base::xblock_mptrs latest_blocks = get_blockstore()->get_latest_blocks(get_account());
    if (latest_blocks.get_latest_cert_block()->get_height() >= proposal_block->get_height()) {
        xerror("xproposal_maker_t::verify_proposal fail-proposal behind. proposal=%s,cert=%s",
            proposal_block->dump().c_str(), latest_blocks.get_latest_cert_block()->dump().c_str());
        return enum_xtxexecutor_error_backup_tableblock_behind;
    }

    if (!xblocktool_t::verify_latest_blocks(latest_blocks)) {
        xwarn("xproposal_maker_t::verify_proposal. fail-verify_latest_blocks fail.proposal=%s",
            proposal_block->dump().c_str());
        return enum_xtxexecutor_error_backup_tableblock_behind;
    }

    // get proposal prev block
    xblock_ptr_t proposal_prev_block = verify_proposal_prev_block(proposal_block, latest_blocks.get_latest_cert_block());
    if (proposal_prev_block == nullptr) {
        xerror("xproposal_maker_t::verify_proposal fail-find no prev block. proposal=%s,", proposal_block->dump().c_str());
        return enum_xtxexecutor_error_backup_tableblock_behind;
    }

    xtablemaker_para_t table_para(proposal_prev_block,
                                  xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_locked_block()),
                                  xblock_t::raw_vblock_to_object_ptr(latest_blocks.get_latest_committed_block()));
    if (false == verify_proposal_input(proposal_block, table_para)) {
        xerror("xproposal_maker_t::verify_proposal fail-proposal input invalid. proposal=%s",
            proposal_block->dump().c_str());
        return enum_xtxexecutor_error_backup_verify_fail_block_class;
    }

    // get proposal drand block
    xblock_ptr_t drand_block = nullptr;
    if (false == verify_proposal_drand_block(proposal_block, drand_block)) {
        xwarn("xproposal_maker_t::verify_proposal fail-find drand block. proposal=%s,drand_height=%ld",
            proposal_block->dump().c_str(), proposal_block->get_cert()->get_drand_height());
        // TODO(jimmy) invoke_sync(account, "tableblock backup sync");  XMETRICS_COUNTER_INCREMENT("txexecutor_cons_invoke_sync", 1);
        return enum_xtxexecutor_error_backup_tableblock_behind;
    }

    xblock_consensus_para_t cs_para;
    base::xvqcert_t* bind_drand_cert = drand_block != nullptr ? drand_block->get_cert() : nullptr;
    if (false == backup_set_consensus_para(proposal_prev_block.get(), proposal_block, bind_drand_cert, cs_para)) {
        return enum_xtxexecutor_error_backup_verify_fail_check_consensus_para;
    }

    int32_t verify_ret = m_table_maker->verify_proposal(proposal_block, table_para, cs_para);
    if (verify_ret != xsuccess) {
        xerror("xproposal_maker_t::verify_proposal fail-verify_proposal. proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(verify_ret).c_str());
        return verify_ret;
    }
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

    uint64_t table_commit_height = table_para.m_latest_committed_block->get_height();
    xassert(table_commit_height < proposal_block->get_height());

    int32_t serialize_ret = table_para.m_proposal_input.serialize_from_string(proposal_block->get_input()->get_proposal());
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

    // update txpool
    m_txpool_table->update_committed_table_block(table_para.m_latest_committed_block.get());

    for (auto & input_unit : unit_proposal_inputs) {
        const std::string & account = input_unit.get_account();
        // TODO(jimmy) txs repeat check, valid check
        base::xauto_ptr<base::xvblock_t> commmit_unit = get_blockstore()->get_latest_committed_block(account);
        xaccount_ptr_t blockchain = get_store()->query_account(account);
        if (blockchain == nullptr) {
            blockchain = make_object_ptr<xblockchain2_t>(account);
        }
        if (blockchain->get_chain_height() != commmit_unit->get_height()) {
            xerror("xproposal_maker_t::verify_proposal_input fail-blockchain not match committed block. proposal=%s,unit_account=%s,height=%ld,%ld",
                proposal_block->dump().c_str(), account.c_str(), blockchain->get_chain_height(), commmit_unit->get_height());
            return false;
        }

        m_txpool_table->update_committed_unit_block(dynamic_cast<xblock_t*>(commmit_unit.get()),
                                                blockchain->account_send_trans_number(),
                                                blockchain->account_send_trans_hash());
        int ret = get_txpool()->verify_txs(account, table_commit_height, commmit_unit->get_height(), input_unit.get_input_txs(),
                                    blockchain->account_send_trans_number(), blockchain->account_send_trans_hash());
        if (ret) {
            xerror("xproposal_maker_t::verify_proposal_input input txs fail. proposal=%s,account=%s,unit_account=%s,nonce=%ld,txs_size=%d",
                proposal_block->dump().c_str(), account.c_str(), blockchain->account_send_trans_number(), input_unit.get_input_txs().size());
            return false;
        }
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

    base::xauto_ptr<base::xvblock_t> _drand_vblock = get_blockstore()->load_block_object(sys_drand_addr, drand_height);
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

    base::xblock_vector latest_certs = get_blockstore()->query_block(*m_table_maker, proposal_block->get_height() - 1);
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


bool xproposal_maker_t::update_txpool_txs(const xblock_maker_para_t & proposal_para, uint64_t timestamp, xtablemaker_para_t & table_para) {
    // get batch txs and set to unit maker
    uint64_t table_committed_height = proposal_para.get_latest_committed_block()->get_height();
    // TODO(jimmy) update txpool
    auto & committed_block = proposal_para.get_latest_committed_block();
    m_txpool_table->update_committed_table_block(committed_block.get());

    std::vector<std::string> unit_accounts = m_txpool_table->get_accounts();
    for (auto & account : unit_accounts) {
        base::xauto_ptr<base::xvblock_t> commmit_unit = get_blockstore()->get_latest_committed_block(account);
        xaccount_ptr_t blockchain = get_store()->query_account(account);
        if (blockchain == nullptr) {
            blockchain = make_object_ptr<xblockchain2_t>(account);
        }
        if (blockchain->get_chain_height() != commmit_unit->get_height()) {
            xerror("xproposal_maker_t::update_txpool_txs fail-blockchain not match committed block. account=%s,viewid=%ld,clock=%ld,unit_account=%s,height=%ld,%ld",
                get_account().c_str(), proposal_para.get_viewid(), proposal_para.get_clock(), account.c_str(), blockchain->get_chain_height(), commmit_unit->get_height());
            continue;
        }
        m_txpool_table->update_committed_unit_block(dynamic_cast<xblock_t*>(commmit_unit.get()),
                                                blockchain->account_send_trans_number(),
                                                blockchain->account_send_trans_hash());
        auto txs = m_txpool_table->get_account_txs(account,
                                                table_committed_height,
                                                commmit_unit->get_height(),
                                                blockchain->account_send_trans_number(),
                                                blockchain->account_send_trans_hash(),
                                                timestamp);
        if (!txs.empty()) {
            table_para.set_unitmaker_info(account, commmit_unit->get_height(), commmit_unit->get_block_hash(), txs);
            xdbg("xproposal_maker_t::update_txpool_txs succ-get txs.account=%s,unit_account=%s,nonce=%ld,txs_size=%d",
                get_account().c_str(), account.c_str(), blockchain->account_send_trans_number(), txs.size());
        } else {
            xwarn("xproposal_maker_t::update_txpool_txs fail-get txs.account=%s,unit_account=%s,nonce=%ld,txs_size=%d",
                get_account().c_str(), account.c_str(), blockchain->account_send_trans_number(), txs.size());
        }
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

bool xproposal_maker_t::leader_set_consensus_para(base::xvblock_t* latest_cert_block, const xblock_maker_para_t & proposal_para, xblock_consensus_para_t & cs_para) {
    uint32_t viewtoken = base::xtime_utl::get_fast_randomu();
    xassert(viewtoken != 0);
    cs_para.set_common_consensus_para(proposal_para.get_timer_block()->get_clock(),
                                      proposal_para.get_validator_xip(),
                                      proposal_para.get_auditor_xip(),
                                      proposal_para.get_viewid(),
                                      viewtoken,
                                      proposal_para.get_drand_block()->get_height());

    uint64_t total_lock_tgas_token = 0;
    uint64_t property_height = 0;
    bool ret = store::xtgas_singleton::get_instance().leader_get_total_lock_tgas_token(get_blockstore(),
        proposal_para.get_timer_block()->get_height(), total_lock_tgas_token, property_height);
    if (!ret) {
        xwarn("xtable_blockmaker_t::make_block fail-leader_set_consensus_para.");
        return ret;
    }
    xblockheader_extra_data_t blockheader_extradata;
    blockheader_extradata.set_tgas_total_lock_amount_property_height(property_height);
    std::string extra_data;
    blockheader_extradata.serialize_to_string(extra_data);
    uint64_t timestamp = (uint64_t)(proposal_para.get_timer_block()->get_clock() * 10) + base::TOP_BEGIN_GMTIME;
    std::string random_seed = calc_random_seed(latest_cert_block, proposal_para.get_drand_block()->get_cert(), viewtoken);
    cs_para.set_tableblock_consensus_para(timestamp,
                                            proposal_para.get_drand_block()->get_height(),
                                            random_seed,
                                            total_lock_tgas_token,
                                            extra_data);
    xdbg_info("xtable_blockmaker_t::set_consensus_para account=%s,height=%ld,viewtoken=%d,random_seed=%s,tgas_token=%ld,tgas_height=%ld leader",
        latest_cert_block->get_account().c_str(), latest_cert_block->get_height(), viewtoken, random_seed.c_str(), total_lock_tgas_token, property_height);
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
                xwarn("xtable_blockmaker_t::verify_block fail-backup_set_consensus_para.");
                return bret;
            }
        }

        std::string random_seed = calc_random_seed(latest_cert_block, bind_drand_cert, proposal->get_cert()->get_viewtoken());
        cs_para.set_tableblock_consensus_para(proposal->get_cert()->get_gmtime(),
                                              proposal->get_cert()->get_drand_height(),
                                              random_seed,
                                              total_lock_tgas_token,
                                              proposal->get_header()->get_extra_data());
        xdbg_info("xtable_blockmaker_t::set_consensus_para account=%s,height=%ld,viewtoken=%d,random_seed=%s,tgas_token=%ld backup",
            latest_cert_block->get_account().c_str(), latest_cert_block->get_height(), proposal->get_cert()->get_viewtoken(),
            random_seed.c_str(), total_lock_tgas_token);
    }
    return true;
}

NS_END2
