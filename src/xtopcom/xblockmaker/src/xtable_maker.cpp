// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <string>
#include <cinttypes>
#include "xbasic/xhex.h"
#include "xblockmaker/xblockmaker_error.h"
#include "xblockmaker/xtable_maker.h"
#include "xblockmaker/xtable_builder.h"
#include "xblockmaker/xblock_builder.h"
#include "xblockmaker/xerror/xerror.h"
#include "xblockmaker/xrelayblock_plugin.h"
#include "xblockmaker/xtable_cross_plugin.h"
#include "xdata/xblocktool.h"
#include "xdata/xblockbuild.h"
#include "xdata/xethreceipt.h"
#include "xdata/xethbuild.h"
#include "xdata/xnative_contract_address.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xchain_fork/xutility.h"
#include "xgasfee/xgas_estimate.h"
#include "xtxexecutor/xbatchtx_executor.h"
#include "xstatectx/xstatectx.h"
#include "xverifier/xtx_verifier.h"
#include "xstatestore/xstatestore_face.h"
#include "xstate_reset/xstate_reseter.h"
#include "xgenesis/xgenesis_manager.h"

NS_BEG2(top, blockmaker)

// unconfirm rate limit parameters
#define table_pair_unconfirm_tx_num_max (512)
#define table_total_unconfirm_tx_num_max  (2048)
#define RELAY_BLOCK_TXS_PACK_NUM_MAX    (32)

xtable_maker_t::xtable_maker_t(const std::string & account, const xblockmaker_resources_ptr_t & resources)
: xblock_maker_t(account, resources) {
    xdbg("xtable_maker_t::xtable_maker_t create,this=%p,account=%s", this, account.c_str());
    m_fulltable_builder = std::make_shared<xfulltable_builder_t>();
    m_emptytable_builder = std::make_shared<xemptytable_builder_t>();
    m_default_builder_para = std::make_shared<xblock_builder_para_face_t>(resources);
    m_full_table_interval_num = XGET_CONFIG(fulltable_interval_block_num);

    if (is_make_relay_chain()) {
        m_resource_plugin = std::make_shared<xrelayblock_plugin_t>();
    } else if(is_evm_table_chain()) {
        m_resource_plugin = std::make_shared<xtable_cross_plugin_t>();
    }
    else {
        m_resource_plugin = std::make_shared<xblock_resource_plugin_face_t>();
    }
}

xtable_maker_t::~xtable_maker_t() {
    xdbg("xtable_maker_t::xtable_maker_t destroy,this=%p", this);
}

bool xtable_maker_t::can_make_next_full_block(const data::xblock_consensus_para_t & cs_para) const {
    uint64_t proposal_height = cs_para.get_latest_cert_block()->get_height() + 1;
    if ((proposal_height & (m_full_table_interval_num - 1)) == 0) {
        return true;
    }
    return false;
}

void xtable_maker_t::set_packtx_metrics(const xcons_transaction_ptr_t & tx, bool bsucc) const {
    #ifdef ENABLE_METRICS
    XMETRICS_GAUGE(metrics::cons_packtx_succ, bsucc ? 1 : 0);
    if (tx->is_self_tx() || tx->is_send_tx()) {
        XMETRICS_GAUGE(metrics::cons_packtx_sendtx_succ, bsucc ? 1 : 0);
    } else if (tx->is_recv_tx()) {
        XMETRICS_GAUGE(metrics::cons_packtx_recvtx_succ, bsucc ? 1 : 0);
    } else if (tx->is_confirm_tx()) {
        XMETRICS_GAUGE(metrics::cons_packtx_confirmtx_succ, bsucc ? 1 : 0);
    }
    #endif
}

std::vector<xcons_transaction_ptr_t> xtable_maker_t::check_input_txs(bool is_leader, const data::xblock_consensus_para_t & cs_para, const std::vector<xcons_transaction_ptr_t> & input_table_txs, uint64_t now) {
    std::vector<xcons_transaction_ptr_t> input_txs;
    for (auto & tx : input_table_txs) {
        if (tx->get_transaction() == nullptr) {
            if (tx->is_confirm_tx()) {
                auto raw_tx = get_txpool()->get_raw_tx(tx->get_account_addr(), tx->get_peer_tableid(), tx->get_last_action_receipt_id());
                if (raw_tx == nullptr) {
                    XMETRICS_GAUGE(metrics::cons_packtx_fail_load_origintx, 1);
                    xwarn("xtable_maker_t::check_input_txs fail-tx filtered load origin tx.%s tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
                    continue;
                }
                if (false == tx->set_raw_tx(raw_tx.get())) {
                    xerror("xtable_maker_t::check_input_txs fail-tx filtered set origin tx.%s tx=%s", cs_para.dump().c_str(), tx->dump().c_str());
                    continue;
                }
            }
        }

        // TODO(jimmy) leader add sendtx expire check, should do this in txpool future
        if (is_leader) {
            if (tx->is_send_or_self_tx()) {
                if (xsuccess != xverifier::xtx_verifier::verify_tx_fire_expiration(tx->get_transaction(), now, false)) {
                    xtxpool_v2::tx_info_t txinfo(tx->get_source_addr(), tx->get_tx_hash_256(), tx->get_tx_subtype());
                    get_txpool()->pop_tx(txinfo);
                    xwarn("xtable_maker_t::check_input_txs fail-tx filtered expired.is_leader=%d,%s tx=%s", is_leader, cs_para.dump().c_str(), tx->dump().c_str());
                    continue;
                }
            }
        }

        // update tx flag before execute
        input_txs.push_back(tx);
    }
    return input_txs;
}

void xtable_maker_t::execute_txs(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, const std::vector<xcons_transaction_ptr_t> & input_txs, txexecutor::xexecute_output_t & execute_output, std::error_code & ec) {
    // create batch executor
    XMETRICS_TIME_RECORD("tps_execute_txs");
    uint64_t gas_limit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token(), gas_limit, cs_para.get_table_proposal_height(), cs_para.get_coinbase());
    txexecutor::xbatchtx_executor_t executor(statectx_ptr, vmpara);

    // execute all txs
    executor.execute(input_txs, execute_output);
    uint64_t total_burn_gas = 0;
    for (auto & txout : execute_output.pack_outputs) {
        xinfo("xtable_maker_t::execute_txs packtx is_leader=%d,%s,tx=%s,txout=%s,action=%s, gas_brun=%ld", 
            is_leader, cs_para.dump().c_str(), txout.m_tx->dump().c_str(), txout.dump().c_str(),txout.m_tx->dump_execute_state().c_str(),
            txout.m_vm_output.m_total_gas_burn);
        //total born gas to cs_para
        total_burn_gas +=  txout.m_vm_output.m_total_gas_burn;
    }

#if defined(XBUILD_CONSORTIUM)
    cs_para.set_total_burn_gas(total_burn_gas);
#endif 

    for (auto & txout : execute_output.drop_outputs) {
        xinfo("xtable_maker_t::make_light_table_v2 droptx is_leader=%d,%s,tx=%s,txout=%s,action=%s",
            is_leader, cs_para.dump().c_str(), txout.m_tx->dump().c_str(), txout.dump().c_str(),txout.m_tx->dump_execute_state().c_str());
    }

    for (auto & txout : execute_output.nopack_outputs) {
        xinfo("xtable_maker_t::make_light_table_v2 nopacktx is_leader=%d,%s,tx=%s,txout=%s,action=%s",
            is_leader, cs_para.dump().c_str(), txout.m_tx->dump().c_str(), txout.dump().c_str(),txout.m_tx->dump_execute_state().c_str());
    }
}

void xtable_maker_t::make_genesis_account_index(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, data::xtable_block_para_t & lighttable_para, std::error_code & ec) {
    if (false == chain_fork::xutility_t::is_forked(fork_points::v1_11_0_genesis_account_mpt_fork_point, cs_para.get_clock()) ) { // TODO(jimmy) forked
        return;
    }
    
    std::vector<statectx::xunitstate_ctx_ptr_t> unitctxs = statectx_ptr->get_modified_unit_ctx();

    auto mpt = statectx_ptr->get_prev_tablestate_ext()->get_state_mpt();

    // init genesis accounts for not in mpt
    if (!m_mpt_genesis_accounts_init) {
        for (auto & account : genesis::xgenesis_manager_t::get_all_genesis_accounts()) {
            if (!base::xvaccount_t::is_unit_address_type(account.type())) {
                continue;
            }
            // check if account same table
            if (account.table_address().to_string() != cs_para.get_table_account()) {
                continue;
            }
            // check if account exist in mpt
            base::xaccount_index_t aindex = mpt->get_account_index(account, ec);
            if (ec) {
                xerror("xtable_maker_t::make_genesis_account_index fail-check genesis.get index.is_leader=%d,%s,account=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str());
                return;
            }
            if (aindex.is_valid_mpt_index()) {
                if (cs_para.get_latest_committed_block()->get_height() > 0) {
                    base::xaccount_index_t commit_aindex;
                    statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(account, cs_para.get_latest_committed_block().get(), commit_aindex);
                    if (commit_aindex.is_valid_mpt_index()) {
                        m_mpt_genesis_accounts.clear();
                        m_mpt_genesis_accounts_init = true;
                        xinfo("xtable_maker_t::make_genesis_account_index finish genesis already commit.is_leader=%d,%s,account=%s,index=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str(),commit_aindex.dump().c_str());
                        return;
                    }
                }
                xinfo("xtable_maker_t::make_genesis_account_index in mpt but not commit.is_leader=%d,%s,account=%s,index=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str(),aindex.dump().c_str());
            }
            m_mpt_genesis_accounts.push_back(account);
            xinfo("xtable_maker_t::make_genesis_account_index add genesis.is_leader=%d,%s,account=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str());
        }
        m_mpt_genesis_accounts_init = true;
        xinfo("xtable_maker_t::make_genesis_account_index init genesis.is_leader=%d,%s,count=%zu", is_leader, cs_para.dump().c_str(), m_mpt_genesis_accounts.size());
    }
    
    for (auto & account : m_mpt_genesis_accounts) {            
        // check if account exist in mpt again
        base::xaccount_index_t aindex = mpt->get_account_index(account, ec);
        if (ec) {
            xerror("xtable_maker_t::make_genesis_account_index fail-check genesis.get index.is_leader=%d,%s,account=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str());
            return;
        }
        if (aindex.is_valid_mpt_index()) { 
            if (cs_para.get_latest_committed_block()->get_height() > 0) {
                base::xaccount_index_t commit_aindex;
                statestore::xstatestore_hub_t::instance()->get_accountindex_from_table_block(account, cs_para.get_latest_committed_block().get(), commit_aindex);
                if (commit_aindex.is_valid_mpt_index()) {
                    m_mpt_genesis_accounts.clear();
                    m_mpt_genesis_accounts_init = true;
                    xinfo("xtable_maker_t::make_genesis_account_index finish genesis.is_leader=%d,%s,account=%s,index=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str(),commit_aindex.dump().c_str());
                    return;
                }
            }
            xinfo("xtable_maker_t::make_genesis_account_index not commit.is_leader=%d,%s,account=%s,index=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str(),aindex.dump().c_str());
            return;
        }
        base::xvblock_ptr_t _genesis_unit = get_blockstore()->load_block_object(account.vaccount(), 0, base::enum_xvblock_flag_committed, false);
        if (nullptr == _genesis_unit) {
            ec = blockmaker::error::xerrc_t::blockmaker_make_unit_fail;
            xerror("xtable_maker_t::make_genesis_account_index fail-check genesis.load genesis unit.is_leader=%d,%s,account=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str());
            return;                     
        }

        bool find_in_unitctxs = false;
        for (auto & unitctx : unitctxs) {
            if (unitctx->get_unitstate()->account_address() == account) {
                find_in_unitctxs = true;
                break;
            }
        }
        if (find_in_unitctxs) {
            xinfo("xtable_maker_t::make_genesis_account_index already in statectx.is_leader=%d,%s,account=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str());
            continue;
        }

        data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(_genesis_unit.get());
        if (nullptr == unitstate) {
            ec = blockmaker::error::xerrc_t::blockmaker_make_unit_fail;
            xerror("xtable_maker_t::make_genesis_account_index fail-check genesis.load genesis unistate.is_leader=%d,%s,account=%s,index=%s", is_leader, cs_para.dump().c_str(), account.to_string().c_str(),aindex.dump().c_str());
            return;                     
        }
        std::string snapshot = unitstate->take_snapshot();   
        std::string _state_hash = base::xcontext_t::instance().hash(snapshot, enum_xhash_type_sha2_256);
        data::xaccount_index_t aindex_new = data::xaccount_index_t(0, _genesis_unit->get_block_hash(),_state_hash,0);
        xinfo("xtable_maker_t::make_genesis_account_index make genenis account index.is_leader=%d,%s,unit=%s,index=%s",
            is_leader, cs_para.dump().c_str(), _genesis_unit->dump().c_str(),aindex_new.dump().c_str());

        lighttable_para.set_accountindex(_genesis_unit->get_account(), aindex_new);
    }
}

void xtable_maker_t::make_account_unit_and_index(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, data::xtable_block_para_t & lighttable_para, std::error_code & ec) {
    XMETRICS_TIME_RECORD("tps_make_units");

    // create units
    std::vector<statectx::xunitstate_ctx_ptr_t> unitctxs = statectx_ptr->get_modified_unit_ctx();
    if (unitctxs.empty()) {
        // XTODO the confirm tx may not modify state.
        xinfo("xtable_maker_t::make_account_unit_and_index no unitstates changed.is_leader=%d,%s", is_leader, cs_para.dump().c_str());
    }

    for (auto & unitctx : unitctxs) {
        base::xvblock_ptr_t unitblock = xunitbuilder_t::make_block_v2(unitctx->get_unitstate(), cs_para);
        if (nullptr == unitblock || unitblock->get_block_hash().empty() || unitblock->get_fullstate_hash().empty()) {
            ec = blockmaker::error::xerrc_t::blockmaker_make_unit_fail;
            xerror("xtable_maker_t::make_account_unit_and_index fail-make unit.is_leader=%d,%s", is_leader, cs_para.dump().c_str());
            return;
        }

        data::xaccount_index_t aindex = data::xaccount_index_t(unitblock->get_height(),
                                                               unitblock->get_block_hash(),
                                                               unitblock->get_fullstate_hash(),
                                                               unitctx->get_accoutstate()->get_tx_nonce());
        unitctx->get_accoutstate()->update_account_index(aindex); // TODO(jimmy) update to state ctx for save unitstate

        lighttable_para.set_unit(unitblock);
        lighttable_para.set_accountindex(unitblock->get_account(), aindex);
        xinfo("xtable_maker_t::make_account_unit_and_index succ-make unit.is_leader=%d,%s,unit=%s,index=%s",
            is_leader, cs_para.dump().c_str(), unitblock->dump().c_str(),aindex.dump().c_str());
    }

    make_genesis_account_index(is_leader, cs_para, statectx_ptr, lighttable_para, ec);

    return;
}

void xtable_maker_t::update_receiptid_state(const xtablemaker_para_t & table_para, statectx::xstatectx_ptr_t const& statectx_ptr) {
    auto self_table_sid = get_short_table_id();
    auto & receiptid_info_map = table_para.get_receiptid_info_map();
    std::map<base::xtable_shortid_t, uint64_t> changed_confirm_ids;
    for (auto & receiptid_info : receiptid_info_map) {
        auto & peer_table_sid = receiptid_info.first;

        base::xreceiptid_pair_t peer_pair;
        receiptid_info.second.m_receiptid_state->find_pair(self_table_sid, peer_pair);
        auto recvid_max = peer_pair.get_recvid_max();
        changed_confirm_ids[peer_table_sid] = recvid_max;
    }

    xtablebuilder_t::update_receipt_confirmids(statectx_ptr->get_table_state(), changed_confirm_ids);
}

void xtable_maker_t::resource_plugin_make_txs(bool is_leader, statectx::xstatectx_ptr_t const& statectx_ptr, const data::xblock_consensus_para_t & cs_para, std::vector<xcons_transaction_ptr_t> & input_txs, std::error_code & ec) {
    m_resource_plugin->init(statectx_ptr, ec);
    if (ec) {
        xerror("xtable_maker_t::resource_plugin_make_txs fail-init is_leader=%d,%s,ec=%s",
            is_leader,cs_para.dump().c_str(), ec.message().c_str());
        return;
    }

    // only leader need create txs
    if (is_leader) {
        std::vector<data::xcons_transaction_ptr_t> contract_txs = m_resource_plugin->make_contract_txs(statectx_ptr, cs_para.get_timestamp(), ec);
        if (ec) {
            xerror("xtable_maker_t::make_light_table_v2 fail-make_contract_txs %s,ec=%s",
                cs_para.dump().c_str(), ec.message().c_str());
            return;
        }
        if (!contract_txs.empty()) {
            for (auto & tx : contract_txs) {
                input_txs.push_back(tx);
                xinfo("xtable_maker_t::resource_plugin_make_txs %s,tx=%s",cs_para.dump().c_str(),tx->dump().c_str());
            }
        }
    }
}

std::vector<xcons_transaction_ptr_t> xtable_maker_t::plugin_make_txs_after_execution(statectx::xstatectx_ptr_t const& statectx_ptr, 
                                                                                    const data::xblock_consensus_para_t & cs_para, 
                                                                                    std::vector<txexecutor::xatomictx_output_t> const& pack_outputs, std::error_code & ec) {
    std::vector<data::xcons_transaction_ptr_t> contract_txs = m_resource_plugin->make_contract_txs_after_execution(statectx_ptr,  cs_para.get_timestamp(),pack_outputs, ec);
    if (ec) {
        xerror("xtable_maker_t::plugin_make_txs_after_execution fail-make_contract_txs %s,ec=%s",
            cs_para.dump().c_str(), ec.message().c_str());
        return {};
    }
    if (!contract_txs.empty()) {
        for (auto & tx : contract_txs) {
            xinfo("xtable_maker_t::plugin_make_txs_after_execution %s,tx=%s",cs_para.dump().c_str(),tx->dump().c_str());
        }
    }
    return contract_txs;
}
void xtable_maker_t::rerource_plugin_make_resource(bool is_leader, const data::xblock_consensus_para_t & cs_para, data::xtable_block_para_t & lighttable_para, std::error_code & ec) {
    xblock_resource_description_t block_resource = m_resource_plugin->make_resource(cs_para, ec);
    if (ec) {
        xerror("xtable_maker_t::rerource_plugin_make_resource fail-make_resource is_leader=%d,%s,ec=%s",
            is_leader, cs_para.dump().c_str(), ec.message().c_str());
        return;
    }
    if (!block_resource.resource_key_name.empty()) {
        lighttable_para.set_resource(block_resource.is_input_resource, block_resource.resource_key_name, block_resource.resource_value);
        if (block_resource.need_signature) {
            cs_para.set_vote_extend_hash(block_resource.signature_hash);
            cs_para.set_need_relay_prove(true);
        }
        xdbg("xtable_maker_t::rerource_plugin_make_resource succ-make_resource is_leader=%d,%s,resource:%s,%zu,hash=%s",
            is_leader, cs_para.dump().c_str(), block_resource.resource_key_name.c_str(), block_resource.resource_value.size(), top::to_hex(top::to_bytes(block_resource.signature_hash)).c_str());
    }
}

xblock_ptr_t xtable_maker_t::make_light_table_v2(bool is_leader, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    uint64_t now = cs_para.get_gettimeofday_s();
    const std::vector<xcons_transaction_ptr_t> & input_table_txs = table_para.get_origin_txs();
    std::vector<xcons_transaction_ptr_t> input_txs = check_input_txs(is_leader, cs_para, input_table_txs, now);
    // backup should has same txs with leader
    if (!is_leader && input_txs.size() != input_table_txs.size()) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xwarn("xtable_maker_t::make_light_table_v2 fail tx is filtered.is_leader=%d,%s,txs_size=%zu:%zu", is_leader, cs_para.dump().c_str(), input_txs.size(), input_table_txs.size());
        return nullptr;
    }
    // must has txs except relay block
    if (!is_make_relay_chain() && input_txs.empty()) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xwarn("xtable_maker_t::make_light_table_v2 fail-no input txs");
        return nullptr;
    }

    std::error_code ec;
    data::xtable_block_para_t lighttable_para;
    txexecutor::xexecute_output_t execute_output;
    std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> batch_unit_and_index;
    // create statectx
    statectx::xstatectx_para_t statectx_para(cs_para.get_clock());
    statectx::xstatectx_ptr_t statectx_ptr = statectx::xstatectx_factory_t::create_latest_cert_statectx(cs_para.get_latest_cert_block().get(), cs_para.get_latest_committed_block().get(), statectx_para);
    if (nullptr == statectx_ptr) {
        ec = blockmaker::error::xerrc_t::blockmaker_create_statectx_fail;
        xwarn("xtable_maker_t::make_light_table_v2 fail-create statectx is_leader=%d,%s",
            is_leader, cs_para.dump().c_str());
        return nullptr;
    }

    resource_plugin_make_txs(is_leader, statectx_ptr, cs_para, input_txs, ec);
    if (ec) {
        xerror("xtable_maker_t::make_light_table_v2 fail-resource_plugin_make_txs is_leader=%d,%s,ec=%s",
            is_leader, cs_para.dump().c_str(), ec.message().c_str());
        return nullptr;
    }

    if (input_txs.empty()) {
        return nullptr;
    }

    {
        state_reset::xstate_reseter reseter{
            statectx_ptr, std::make_shared<std::vector<xcons_transaction_ptr_t>>(input_txs), cs_para.get_clock()};
        reseter.exec_reset();
    }

    xinfo("xtable_maker_t::make_light_table_v2 tps_key before execute txs is_leader=%d,%s", is_leader, cs_para.dump().c_str());

    execute_txs(is_leader, cs_para, statectx_ptr, input_txs, execute_output, ec);
    for (auto & txout : execute_output.drop_outputs) {  // drop tx from txpool
        xtxpool_v2::tx_info_t txinfo(txout.m_tx->get_source_addr(), txout.m_tx->get_tx_hash_256(), txout.m_tx->get_tx_subtype());
        get_txpool()->pop_tx(txinfo);
    }
    // set pack origin tx to proposal
    for (auto & txout : execute_output.pack_outputs) {
        table_para.push_tx_to_proposal(txout.m_tx);
    }
    if (ec || execute_output.pack_outputs.empty()) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xwarn("xtable_maker_t::make_light_table_v2 fail-no pack txs.is_leader=%d,%s,txs_size=%zu", is_leader, cs_para.dump().c_str(), input_txs.size());
        return nullptr;
    }

    // local txs no need take by leader to backups
    std::vector<xcons_transaction_ptr_t> local_txs_after = plugin_make_txs_after_execution(statectx_ptr, cs_para, execute_output.pack_outputs, ec);
    if (ec) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xwarn("xtable_maker_t::make_light_table_v2 plugin_make_txs_after_execution error code: %s.", ec.message().c_str());
        return nullptr;
    }
    if (!local_txs_after.empty()) {
        txexecutor::xexecute_output_t execute_output_after;
        execute_txs(is_leader, cs_para, statectx_ptr, local_txs_after, execute_output_after, ec);
        if (ec || execute_output_after.pack_outputs.size() != local_txs_after.size()) {
            table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
            xerror("xtable_maker_t::make_light_table_v2 plugin_make_txs_after_execution tx error:%s. pack_outputs size%ld, local_txs_after size%ld. ", ec.message().c_str(),
                                                                                                    execute_output_after.pack_outputs.size(),
                                                                                                    local_txs_after.size());
            return nullptr;
        }
        for (auto & txout : execute_output_after.pack_outputs) {
            execute_output.pack_outputs.push_back(txout);
        }
    }

    xinfo("xtable_maker_t::make_light_table_v2 tps_key execute txs finish is_leader=%d,%s", is_leader, cs_para.dump().c_str());

    make_account_unit_and_index(is_leader, cs_para, statectx_ptr, lighttable_para, ec);
    if (ec) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xwarn("xtable_maker_t::make_light_table_v2 fail-make units.is_leader=%d,%s,txs_size=%zu", is_leader, cs_para.dump().c_str(), input_txs.size());
        return nullptr;
    }

    xinfo("xtable_maker_t::make_light_table_v2 tps_key make unit and index finish is_leader=%d,%s", is_leader, cs_para.dump().c_str());

    // TODO(jimmy) update confirm ids in table state
    update_receiptid_state(table_para, statectx_ptr);

    std::shared_ptr<state_mpt::xstate_mpt_t> table_mpt = nullptr;

    table_mpt = create_new_mpt(cs_para, statectx_ptr, lighttable_para.get_accountindexs());
    if (table_mpt == nullptr) {
        xerror("xtable_maker_t::make_light_table_v2 fail-create mpt. %s", cs_para.dump().c_str());
        return nullptr;
    }
    xinfo("xtable_maker_t::make_light_table_v2 tps_key create_new_mpt finish is_leader=%d,%s", is_leader, cs_para.dump().c_str());
    evm_common::xh256_t state_root = table_mpt->get_root_hash(ec);
    if (ec) {
        xerror("xtable_maker_t::make_light_table_v2 fail-get mpt root. %s, ec=%s", cs_para.dump().c_str(), ec.message().c_str());
        return nullptr;
    }
    xinfo("xtable_maker_t::make_light_table_v2 tps_key get_root_hash finish is_leader=%d,%s", is_leader, cs_para.dump().c_str());
    xdbg("xtable_maker_t::make_light_table_v2 create mpt succ is_leader=%d,%s,root hash:%s", is_leader, cs_para.dump().c_str(), state_root.hex().c_str());

    cs_para.set_ethheader(xeth_header_builder::build(cs_para, state_root, execute_output.pack_outputs));

    xtablebuilder_t::make_table_block_para(statectx_ptr->get_table_state(), execute_output, lighttable_para);
    rerource_plugin_make_resource(is_leader, cs_para, lighttable_para, ec);
    if (ec) {
        xerror("xtable_maker_t::make_light_table_v2 fail-make_resource is_leader=%d,%s,ec=%s",
            is_leader, cs_para.dump().c_str(), ec.message().c_str());
        return nullptr;
    }
    xinfo("xtable_maker_t::make_light_table_v2 tps_key make_table_block_para finish is_leader=%d,%s", is_leader, cs_para.dump().c_str());

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();

    data::xblock_ptr_t tableblock = xtablebuilder_t::make_light_block(cs_para.get_latest_cert_block(),
                                                                        cs_para,
                                                                        lighttable_para);
    if (nullptr != tableblock) {
        xinfo("xtable_maker_t::make_light_table_v2-succ tps_key is_leader=%d,%s,binlog=%zu,snapshot=%zu,units=%zu,indexs=%zu,property_hashs=%zu,tgas_change=%ld,offdata=%zu,txnum=%u",
            is_leader, tableblock->dump().c_str(), lighttable_para.get_property_binlog().size(), lighttable_para.get_fullstate_bin().size(),
            lighttable_para.get_units().size(), lighttable_para.get_accountindexs().get_account_indexs().size(), 
            lighttable_para.get_property_hashs().size(),lighttable_para.get_tgas_balance_change(),tableblock->get_output_offdata().size(),
            lighttable_para.get_txs().size());
    }

    tableblock->set_excontainer(std::make_shared<xtable_mpt_container>(statectx_ptr));
    return tableblock;
}

xblock_ptr_t xtable_maker_t::leader_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    // XMETRICS_TIMER(metrics::cons_make_lighttable_tick);
    XMETRICS_TIME_RECORD("cons_make_lighttable_cost");
    return make_light_table_v2(true, table_para, cs_para, table_result);
}

xblock_ptr_t xtable_maker_t::backup_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    // XMETRICS_TIMER(metrics::cons_verify_lighttable_tick);
    XMETRICS_TIME_RECORD("cons_verify_lighttable_cost");
    return make_light_table_v2(false, table_para, cs_para, table_result);
}

xblock_ptr_t xtable_maker_t::make_full_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, bool is_leader, int32_t & error_code) {
    // XMETRICS_TIMER(metrics::cons_make_fulltable_tick);
    XMETRICS_TIME_RECORD("cons_make_fulltable_cost");
    std::vector<xblock_ptr_t> blocks_from_last_full;
    if (false == load_table_blocks_from_last_full(cs_para.get_latest_cert_block(), blocks_from_last_full)) {
        xerror("xtable_maker_t::make_full_table fail-load blocks. %s", cs_para.dump().c_str());
        return nullptr;
    }

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();
    data::xtablestate_ptr_t tablestate = table_para.get_tablestate();
    xassert(nullptr != tablestate);

    std::error_code ec;
    auto const & last_state_root = data::xblockextract_t::get_state_root(cs_para.get_latest_cert_block().get(), ec);
    if (ec) {
        return nullptr;
    }

    cs_para.set_ethheader(xeth_header_builder::build(cs_para, last_state_root));

    xblock_builder_para_ptr_t build_para = std::make_shared<xfulltable_builder_para_t>(tablestate, blocks_from_last_full, get_resources());
    xblock_ptr_t proposal_block = m_fulltable_builder->build_block(cert_block, table_para.get_tablestate()->get_bstate(), cs_para, build_para);
    if (proposal_block == nullptr) {
        xwarn("xtable_maker_t::make_full_table fail-build block.%s", cs_para.dump().c_str());
        return nullptr;
    }
    xinfo("xtable_maker_t::make_full_table succ.block=%s,is_leader:%d", proposal_block->dump().c_str(), is_leader);
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::make_empty_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, bool is_leader, int32_t & error_code) {
    // TODO(jimmy)
    XMETRICS_TIME_RECORD("cons_make_emptytable_cost");

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();
    data::xtablestate_ptr_t tablestate = table_para.get_tablestate();
    xassert(nullptr != tablestate);

    std::error_code ec;
    auto const & last_state_root = data::xblockextract_t::get_state_root(cs_para.get_latest_cert_block().get(), ec);
    if (ec) {
        return nullptr;
    }
    cs_para.set_ethheader(xeth_header_builder::build(cs_para, last_state_root));

    xblock_ptr_t proposal_block = m_emptytable_builder->build_block(cert_block, table_para.get_tablestate()->get_bstate(), cs_para, m_default_builder_para);
    xinfo("xtable_maker_t::make_empty_table succ.block=%s,is_leader:%d", proposal_block->dump().c_str(), is_leader);
    return proposal_block;
}

bool    xtable_maker_t::load_table_blocks_from_last_full(const xblock_ptr_t & prev_block, std::vector<xblock_ptr_t> & blocks) {
    std::vector<xblock_ptr_t> _form_highest_blocks;
    xblock_ptr_t current_block = prev_block;
    xassert(current_block->get_height() > 0);
    _form_highest_blocks.push_back(current_block);
    // load input for txactions count statistics
    if (false == get_blockstore()->load_block_input(*this, current_block.get())) {
        xerror("xfulltable_builder_t::load_table_blocks_from_last_full fail-load block body.account=%s,height=%ld", get_account().c_str(), current_block->get_height());
        return false;
    }

    while (current_block->get_block_class() != base::enum_xvblock_class_full && current_block->get_height() > 1) {
        // only mini-block is enough
        base::xauto_ptr<base::xvblock_t> _block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_table);
        if (_block == nullptr) {
            xerror("xfulltable_builder_t::load_table_blocks_from_last_full fail-load block.account=%s,height=%ld", get_account().c_str(), current_block->get_height() - 1);
            return false;
        }
        // load input for txactions count statistics
        if (false == get_blockstore()->load_block_input(*this, _block.get()) ) {
            xerror("xfulltable_builder_t::load_table_blocks_from_last_full fail-load block body.account=%s,height=%ld", get_account().c_str(), _block->get_height());
            return false;
        }
        current_block = xblock_t::raw_vblock_to_object_ptr(_block.get());
        blocks.push_back(current_block);
    }

    // TOOD(jimmy) use map
    for (auto iter = _form_highest_blocks.rbegin(); iter != _form_highest_blocks.rend(); iter++) {
        blocks.push_back(*iter);
    }
    return true;;
}

xblock_ptr_t xtable_maker_t::make_proposal(xtablemaker_para_t & table_para,
                                           const data::xblock_consensus_para_t & cs_para,
                                           xtablemaker_result_t & tablemaker_result) {
    // XMETRICS_TIMER(metrics::cons_tablemaker_make_proposal_tick);
    XMETRICS_TIME_RECORD("cons_tablemaker_make_proposal_cost");
    std::lock_guard<std::mutex> l(m_lock);
    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();
    xassert(table_para.get_tablestate() != nullptr);

    bool can_make_empty_table_block = can_make_next_empty_block(cs_para);
    if (table_para.get_origin_txs().empty() && !can_make_empty_table_block && !is_make_relay_chain()) {
        xinfo("xtable_maker_t::make_proposal no txs and no need make empty block. %s,cert_height=%" PRIu64 "", cs_para.dump().c_str(), latest_cert_block->get_height());
        return nullptr;
    }

    xblock_ptr_t proposal_block = nullptr;
    if (can_make_next_full_block(cs_para)) {
        proposal_block = make_full_table(table_para, cs_para, true, tablemaker_result.m_make_block_error_code);
    } else {
        proposal_block = leader_make_light_table(table_para, cs_para, tablemaker_result);
    }

    if (proposal_block == nullptr) {
        if (tablemaker_result.m_make_block_error_code != xblockmaker_error_no_need_make_table) {
            xwarn("xtable_maker_t::make_proposal fail-make table. %s,error_code=%s",
                cs_para.dump().c_str(), chainbase::xmodule_error_to_str(tablemaker_result.m_make_block_error_code).c_str());
            return nullptr;
        } else {
            if (can_make_empty_table_block) {
                proposal_block = make_empty_table(table_para, cs_para, true, tablemaker_result.m_make_block_error_code);
                if (proposal_block == nullptr) {
                    return nullptr;
                }
            } else {
                return nullptr;
            }
        }
    }

    xinfo("xtable_maker_t::make_proposal succ.%s,proposal=%s,tx_count=%zu,other_accounts_count=%zu",
        cs_para.dump().c_str(), proposal_block->dump().c_str(), table_para.get_proposal()->get_input_txs().size(), table_para.get_proposal()->get_other_accounts().size());
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::make_proposal_backup(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, bool empty_block) {
    // XMETRICS_TIMER(metrics::cons_tablemaker_verify_proposal_tick);
    XMETRICS_TIME_RECORD("cons_tablemaker_verify_proposal_cost");
    std::lock_guard<std::mutex> l(m_lock);

    xblock_ptr_t local_block = nullptr;
    xtablemaker_result_t table_result;
    if (can_make_next_full_block(cs_para)) {
        local_block = make_full_table(table_para, cs_para, false, table_result.m_make_block_error_code);
    } else if (empty_block) {
        if (can_make_next_empty_block(cs_para)) {
            local_block = make_empty_table(table_para, cs_para, false, table_result.m_make_block_error_code);
        }
    } else {
        local_block = backup_make_light_table(table_para, cs_para, table_result);
    }
    if (local_block == nullptr) {
        xwarn("xtable_maker_t::make_proposal_backup fail-make table. cs_para=%s,error_code=%s",
            cs_para.dump().c_str(), chainbase::xmodule_error_to_str(table_result.m_make_block_error_code).c_str());
        return nullptr;
    }
    return local_block;
}

bool xtable_maker_t::verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const {
    local_block->get_cert()->set_nonce(proposal_block->get_cert()->get_nonce());  // TODO(jimmy)
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
        auto local_state_root = data::xblockextract_t::get_state_root_from_block(local_block);
        auto proposal_state_root = data::xblockextract_t::get_state_root_from_block(local_block);
        if (local_state_root != proposal_state_root) {
            xerror("xtable_maker_t::verify_proposal_with_local fail-state root not match.%s proposal:%s local:%s",proposal_block->dump().c_str(),local_state_root.hex().c_str(),proposal_state_root.hex().c_str());
            return false;
        }

        xwarn("xtable_maker_t::verify_proposal_with_local fail-header hash not match. %s proposal:%s local:%s",
            proposal_block->dump().c_str(),
            proposal_block->get_header()->dump().c_str(),
            local_block->get_header()->dump().c_str());
        return false;
    }
    if(!local_block->get_cert()->is_equal(*proposal_block->get_cert())){
        xerror("xtable_maker_t::verify_proposal_with_local fail-cert hash not match. proposal:%s local:%s",
            proposal_block->get_cert()->dump().c_str(),
            local_block->get_cert()->dump().c_str());
        return false;
    }

    bool bret = false;
    if (proposal_block->get_block_class() != base::enum_xvblock_class_nil) {
        std::error_code ec;
        std::string vinput_bin;
        base::xvinput_t* _input_object = local_block->load_input(ec);
        std::string voutput_bin;
        base::xvoutput_t* _output_object = local_block->load_output(ec);
        bret = proposal_block->set_input_output(_input_object, _output_object);
        if (!bret) {
            xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block input fail");
            return false;
        }        

        bret = proposal_block->set_output_offdata(local_block->get_output_offdata());
        if (!bret) {
            xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block output offdata fail.%s",proposal_block->dump().c_str());
            return false;
        }        
    }

    proposal_block->set_excontainer(local_block->get_excontainer());
    return true;
}

bool xtable_maker_t::can_make_block_with_no_tx(const data::xblock_consensus_para_t & cs_para) const {
    return (can_make_next_empty_block(cs_para) || is_make_relay_chain());
}

bool xtable_maker_t::can_make_next_empty_block(const data::xblock_consensus_para_t & cs_para) const {
    const xblock_ptr_t & current_block = cs_para.get_latest_cert_block();
    if (current_block->get_height() == 0) {
        return false;
    }
    if (current_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();
    if (lock_block == nullptr) {
        return false;
    }
    if (lock_block->get_block_class() == base::enum_xvblock_class_light) {
        return true;
    }
    return false;
}

bool xtable_maker_t::is_make_relay_chain() const {
    return get_zone_index() == base::enum_chain_zone_relay_index;
}

bool xtable_maker_t::is_evm_table_chain() const {
    return get_zone_index() == base::enum_chain_zone_evm_index;
}

std::shared_ptr<state_mpt::xstate_mpt_t> xtable_maker_t::create_new_mpt(const data::xblock_consensus_para_t & cs_para,
                                                                          const statectx::xstatectx_ptr_t & table_state_ctx,
                                                                          const base::xaccount_indexs_t & accountindexs) {
    XMETRICS_TIME_RECORD("tps_create_new_mpt");
    std::error_code ec;
    auto mpt = table_state_ctx->get_prev_tablestate_ext()->get_state_mpt();
    xassert(nullptr != mpt);

    for (auto & unit_and_index : accountindexs.get_account_indexs()) {
        auto & addr = unit_and_index.first;
        auto & index = unit_and_index.second;
        // todo(nathan): set unit state.
        mpt->set_account_index(common::xaccount_address_t{addr}, index, ec);
        if (ec) {
            xerror("xtable_maker_t::create_new_mpt set account index to mpt fail.");
            return nullptr;
        }
    }

    return mpt;
}

std::string xeth_header_builder::build(const xblock_consensus_para_t & cs_para, evm_common::xh256_t const & state_root, const std::vector<txexecutor::xatomictx_output_t> & pack_txs_outputs) {
    std::error_code ec;
    uint64_t gas_used = 0;
    data::xeth_receipts_t eth_receipts;
    data::xeth_transactions_t eth_txs;
    for (auto & txout : pack_txs_outputs) {
        if (txout.m_tx->get_tx_version() != data::xtransaction_version_3) {
            continue;
        }

        data::xeth_transaction_t ethtx = txout.m_tx->get_transaction()->to_eth_tx(ec);
        if (ec) {
            xerror("xeth_header_builder::build fail-to eth tx");
            continue;
        }

        auto & evm_result = txout.m_vm_output.m_tx_result;
        gas_used += evm_result.used_gas;
        data::enum_ethreceipt_status status = (evm_result.status == evm_common::xevm_transaction_status_t::Success) ? data::ethreceipt_status_successful : data::ethreceipt_status_failed;
        data::xeth_receipt_t eth_receipt(ethtx.get_tx_version(), status, gas_used, evm_result.logs);
        eth_receipt.create_bloom();
        eth_receipts.push_back(eth_receipt);
        eth_txs.push_back(ethtx);
    }

    data::xeth_header_t eth_header;
    data::xethheader_para_t header_para;
    header_para.m_gaslimit = cs_para.get_block_gaslimit();
    header_para.m_baseprice = cs_para.get_block_base_price();
    if (!cs_para.get_coinbase().empty()) {
        header_para.m_coinbase = common::xeth_address_t::build_from(cs_para.get_coinbase());
    }
    data::xeth_build_t::build_ethheader(header_para, eth_txs, eth_receipts, state_root, eth_header);

    std::string _ethheader_str = eth_header.serialize_to_string();
    xdbg("xeth_header_builder::build ethheader txcount=%zu,headersize=%zu", eth_receipts.size(), _ethheader_str.size());
    return _ethheader_str;
}

bool xeth_header_builder::string_to_eth_header(const std::string & eth_header_str, data::xeth_header_t & eth_header) {
    std::error_code ec;
    eth_header.serialize_from_string(eth_header_str, ec);
    if (ec) {
        xerror("xeth_header_builder::string_to_eth_header decode fail");
        return false;
    }
    return true;
}

NS_END2
