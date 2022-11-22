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
    } else {
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
    uint64_t gas_limit = XGET_ONCHAIN_GOVERNANCE_PARAMETER(block_gas_limit);
    txexecutor::xvm_para_t vmpara(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_total_lock_tgas_token(), gas_limit, cs_para.get_table_proposal_height(), cs_para.get_coinbase());
    txexecutor::xbatchtx_executor_t executor(statectx_ptr, vmpara);

    // execute all txs
    executor.execute(input_txs, execute_output);

    for (auto & txout : execute_output.pack_outputs) {
        xinfo("xtable_maker_t::execute_txs packtx is_leader=%d,%s,tx=%s,txout=%s,action=%s",
            is_leader, cs_para.dump().c_str(), txout.m_tx->dump().c_str(), txout.dump().c_str(),txout.m_tx->dump_execute_state().c_str());
    }

    for (auto & txout : execute_output.drop_outputs) {
        xinfo("xtable_maker_t::make_light_table_v2 droptx is_leader=%d,%s,tx=%s,txout=%s,action=%s",
            is_leader, cs_para.dump().c_str(), txout.m_tx->dump().c_str(), txout.dump().c_str(),txout.m_tx->dump_execute_state().c_str());
    }

    for (auto & txout : execute_output.nopack_outputs) {
        xinfo("xtable_maker_t::make_light_table_v2 nopacktx is_leader=%d,%s,tx=%s,txout=%s,action=%s",
            is_leader, cs_para.dump().c_str(), txout.m_tx->dump().c_str(), txout.dump().c_str(),txout.m_tx->dump_execute_state().c_str());
    }
}

std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> xtable_maker_t::make_units(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, txexecutor::xexecute_output_t const& execute_output, std::error_code & ec) {
    // std::vector<xblock_ptr_t> batch_units;
    std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> batch_unit_and_index;

    // TODO(jimmy) unit will pack tx hash and type
    xunitbuildber_txkeys_mgr_t  txkeys_mgr;
    for (auto & txout : execute_output.pack_outputs) {
        txkeys_mgr.add_pack_tx(txout.m_tx);
        for (auto & v : txout.m_vm_output.m_contract_create_txs) {
            txkeys_mgr.add_pack_tx(v);
        }
    }

    // create units
    std::vector<statectx::xunitstate_ctx_ptr_t> unitctxs = statectx_ptr->get_modified_unit_ctx();
    if (unitctxs.empty()) {
        // TODO(jimmy) the confirm tx may not modify state.
        xwarn("xtable_maker_t::make_units fail-no unitstates changed.is_leader=%d,%s,txs_size=%zu", is_leader, cs_para.dump().c_str(), execute_output.pack_outputs.size());
    }

    for (auto & unitctx : unitctxs) {
        base::xvtxkey_vec_t txkeys = txkeys_mgr.get_account_txkeys(unitctx->get_unitstate()->account_address().to_string());
        if (txkeys.get_txkeys().empty()) {
            // will support state change without txkeys for evm tx
            xinfo("xtable_maker_t::make_units xkeys empty.is_leader=%d,%s,addr=%s",
                  is_leader,
                  cs_para.dump().c_str(),
                  unitctx->get_unitstate()->account_address().to_string().c_str());
        }
        xunitbuilder_para_t unit_para(txkeys);
        data::xblock_ptr_t unitblock = xunitbuilder_t::make_block(unitctx->get_prev_block(), unitctx->get_unitstate(), unit_para, cs_para);
        if (nullptr == unitblock) {
            ec = blockmaker::error::xerrc_t::blockmaker_make_unit_fail;
            // should not fail
            xerror("xtable_maker_t::make_units fail-make unit.is_leader=%d,%s,prev=%s", is_leader, cs_para.dump().c_str(), unitctx->get_prev_block()->dump().c_str());
            return {};
        }

        data::xaccount_index_t aindex; // TODO(jimmy) delete future after forked
        batch_unit_and_index.push_back(std::make_pair(unitblock, aindex));
        xinfo("xtable_maker_t::make_units succ-make unit.is_leader=%d,%s,unit=%s,txkeys=%zu,size=%zu,%zu",
            is_leader, cs_para.dump().c_str(), unitblock->dump().c_str(),txkeys.get_txkeys().size(),unitblock->get_input()->get_resources_data().size(),unitblock->get_output()->get_resources_data().size());
        xtablebuilder_t::update_account_index_property(statectx_ptr->get_table_state(), unitblock, unitctx->get_unitstate());
    }

    return batch_unit_and_index;
}

std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> xtable_maker_t::make_units_v2(bool is_leader, const data::xblock_consensus_para_t & cs_para, statectx::xstatectx_ptr_t const& statectx_ptr, txexecutor::xexecute_output_t const& execute_output, std::error_code & ec) {
    // std::vector<xblock_ptr_t> batch_units;
    std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> batch_unit_and_index;

    // create units
    std::vector<statectx::xunitstate_ctx_ptr_t> unitctxs = statectx_ptr->get_modified_unit_ctx();
    if (unitctxs.empty()) {
        // XTODO the confirm tx may not modify state.
        xinfo("xtable_maker_t::make_units_v2 no unitstates changed.is_leader=%d,%s,txs_size=%zu", is_leader, cs_para.dump().c_str(), execute_output.pack_outputs.size());
    }

    for (auto & unitctx : unitctxs) {
        xunitbuilder_para_t unit_para;  // TODO(jimmy)   put last block hash to unitstate from accountindex
        data::xblock_ptr_t unitblock = xunitbuilder_t::make_block_v2(unitctx->get_unitstate(), unit_para, cs_para);
        if (nullptr == unitblock || unitblock->get_block_hash().empty()) {
            ec = blockmaker::error::xerrc_t::blockmaker_make_unit_fail;
            // should not fail
            xerror("xtable_maker_t::make_units_v2 fail-make unit.is_leader=%d,%s,prev=%s", is_leader, cs_para.dump().c_str(), unitctx->get_prev_block()->dump().c_str());
            return {};
        }

        unitctx->set_unit_hash(unitblock->get_block_hash());

        data::xaccount_index_t aindex = data::xaccount_index_t(unitblock->get_height(),
                                                               unitblock->get_block_hash(),
                                                               unitblock->get_fullstate_hash(),
                                                               unitctx->get_unitstate()->account_send_trans_number());
        batch_unit_and_index.push_back(std::make_pair(unitblock, aindex));
        xinfo("xtable_maker_t::make_units_v2 succ-make unit.is_leader=%d,%s,unit=%s,size=%zu,%zu",
            is_leader, cs_para.dump().c_str(), unitblock->dump().c_str(),unitblock->get_input()->get_resources_data().size(),unitblock->get_output()->get_resources_data().size());
    }

    return batch_unit_and_index;
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
    auto const new_version = chain_fork::xutility_t::is_forked(fork_points::v1_7_0_block_fork_point, cs_para.get_clock());

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

    if (new_version) {
        batch_unit_and_index = make_units_v2(is_leader, cs_para, statectx_ptr, execute_output, ec);
    } else {
        batch_unit_and_index = make_units(is_leader, cs_para, statectx_ptr, execute_output, ec);
    }
    if (ec) {
        table_result.m_make_block_error_code = xblockmaker_error_no_need_make_table;
        xwarn("xtable_maker_t::make_light_table_v2 fail-make units.is_leader=%d,%s,txs_size=%zu", is_leader, cs_para.dump().c_str(), input_txs.size());
        return nullptr;
    }

    // TODO(jimmy) update confirm ids in table state
    update_receiptid_state(table_para, statectx_ptr);

    evm_common::xh256_t state_root;
    std::shared_ptr<state_mpt::xstate_mpt_t> table_mpt = nullptr;

    if (new_version) {
        evm_common::xh256_t last_state_root;
        auto ret = data::xblockextract_t::get_state_root(cs_para.get_latest_cert_block().get(), last_state_root);
        if (!ret) {
            return nullptr;
        }
        table_mpt = create_new_mpt(xhash256_t{last_state_root}, cs_para, statectx_ptr, batch_unit_and_index);
        if (table_mpt == nullptr) {
            return nullptr;
        }
        auto root_hash = table_mpt->get_root_hash(ec);
        if (ec) {
            xwarn("xtable_maker_t::make_light_table_v2 get mpt root hash fail.");
            return nullptr;
        }
        xdbg("xtable_maker_t::make_light_table_v2 create mpt succ is_leader=%d,%s,root hash:%s", is_leader, cs_para.dump().c_str(), root_hash.as_hex_str().c_str());
        state_root = evm_common::xh256_t(root_hash.to_bytes());
    }

    cs_para.set_ethheader(xeth_header_builder::build(cs_para, state_root, execute_output.pack_outputs));

    xtablebuilder_t::make_table_block_para(batch_unit_and_index, statectx_ptr->get_table_state(), execute_output, lighttable_para);
    rerource_plugin_make_resource(is_leader, cs_para, lighttable_para, ec);
    if (ec) {
        xerror("xtable_maker_t::make_light_table_v2 fail-make_resource is_leader=%d,%s,ec=%s",
            is_leader, cs_para.dump().c_str(), ec.message().c_str());
        return nullptr;
    }

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();

    data::xblock_ptr_t tableblock = xtablebuilder_t::make_light_block(cs_para.get_latest_cert_block(),
                                                                        cs_para,
                                                                        lighttable_para);
    if (nullptr != tableblock) {
        xinfo("xtable_maker_t::make_light_table_v2-succ is_leader=%d,%s,binlog=%zu,snapshot=%zu,batch_units=%zu,property_hashs=%zu,tgas_change=%ld,offdata=%zu,txnum=%u",
            is_leader, tableblock->dump().c_str(), lighttable_para.get_property_binlog().size(), lighttable_para.get_fullstate_bin().size(),
            lighttable_para.get_batch_unit_and_index().size(), lighttable_para.get_property_hashs().size(),lighttable_para.get_tgas_balance_change(),tableblock->get_output_offdata().size(),
            lighttable_para.get_txs().size());
    }

    if (new_version) {
        tableblock->set_excontainer(std::make_shared<xtable_mpt_container>(statectx_ptr));
    }

    return tableblock;
}

xblock_ptr_t xtable_maker_t::leader_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    XMETRICS_TIMER(metrics::cons_make_lighttable_tick);
    return make_light_table_v2(true, table_para, cs_para, table_result);
}

xblock_ptr_t xtable_maker_t::backup_make_light_table(const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para, xtablemaker_result_t & table_result) {
    XMETRICS_TIMER(metrics::cons_verify_lighttable_tick);
    return make_light_table_v2(false, table_para, cs_para, table_result);
}

xblock_ptr_t xtable_maker_t::make_full_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code) {
    XMETRICS_TIMER(metrics::cons_make_fulltable_tick);
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

    evm_common::xh256_t last_state_root;
    auto ret = data::xblockextract_t::get_state_root(cs_para.get_latest_cert_block().get(), last_state_root);
    if (!ret) {
        return nullptr;
    }

    cs_para.set_ethheader(xeth_header_builder::build(cs_para, last_state_root));

    xblock_builder_para_ptr_t build_para = std::make_shared<xfulltable_builder_para_t>(tablestate, blocks_from_last_full, get_resources());
    xblock_ptr_t proposal_block = m_fulltable_builder->build_block(cert_block, table_para.get_tablestate()->get_bstate(), cs_para, build_para);
    if (proposal_block == nullptr) {
        xwarn("xtable_maker_t::make_full_table fail-build block.%s", cs_para.dump().c_str());
        return nullptr;
    }
    xinfo("xtable_maker_t::make_full_table succ.block=%s", proposal_block->dump().c_str());
    return proposal_block;
}

xblock_ptr_t xtable_maker_t::make_empty_table(const xtablemaker_para_t & table_para, const xblock_consensus_para_t & cs_para, int32_t & error_code) {
    // TODO(jimmy)
    XMETRICS_TIME_RECORD("cons_tableblock_verfiy_proposal_imp_make_empty_table");

    // reset justify cert hash para
    const xblock_ptr_t & cert_block = cs_para.get_latest_cert_block();
    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();
    data::xtablestate_ptr_t tablestate = table_para.get_tablestate();
    xassert(nullptr != tablestate);

    evm_common::xh256_t last_state_root;
    auto ret = data::xblockextract_t::get_state_root(cs_para.get_latest_cert_block().get(), last_state_root);
    if (!ret) {
        return nullptr;
    }
    cs_para.set_ethheader(xeth_header_builder::build(cs_para, last_state_root));

    xblock_ptr_t proposal_block = m_emptytable_builder->build_block(cert_block, table_para.get_tablestate()->get_bstate(), cs_para, m_default_builder_para);
    return proposal_block;
}

bool    xtable_maker_t::load_table_blocks_from_last_full(const xblock_ptr_t & prev_block, std::vector<xblock_ptr_t> & blocks) {
    std::vector<xblock_ptr_t> _form_highest_blocks;
    xblock_ptr_t current_block = prev_block;
    xassert(current_block->get_height() > 0);
    _form_highest_blocks.push_back(current_block);

    while (current_block->get_block_class() != base::enum_xvblock_class_full && current_block->get_height() > 1) {
        // only mini-block is enough
        base::xauto_ptr<base::xvblock_t> _block = get_blockstore()->load_block_object(*this, current_block->get_height() - 1, current_block->get_last_block_hash(), false, metrics::blockstore_access_from_blk_mk_table);
        if (_block == nullptr) {
            xerror("xfulltable_builder_t::load_table_blocks_from_last_full fail-load block.account=%s,height=%ld", get_account().c_str(), current_block->get_height() - 1);
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
    XMETRICS_TIMER(metrics::cons_tablemaker_make_proposal_tick);
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
        proposal_block = make_full_table(table_para, cs_para, tablemaker_result.m_make_block_error_code);
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
                proposal_block = make_empty_table(table_para, cs_para, tablemaker_result.m_make_block_error_code);
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

int32_t xtable_maker_t::verify_proposal(base::xvblock_t* proposal_block, const xtablemaker_para_t & table_para, const data::xblock_consensus_para_t & cs_para) {
    XMETRICS_TIMER(metrics::cons_tablemaker_verify_proposal_tick);
    std::lock_guard<std::mutex> l(m_lock);

    // check table maker state
    const xblock_ptr_t & latest_cert_block = cs_para.get_latest_cert_block();  // should update to this new table cert block
    xassert(table_para.get_tablestate() != nullptr);

    const auto & highest_block = latest_cert_block;
    if (proposal_block->get_last_block_hash() != highest_block->get_block_hash()
        || proposal_block->get_height() != highest_block->get_height() + 1) {
        xwarn("xtable_maker_t::verify_proposal fail-proposal unmatch last hash.proposal=%s, last_height=%" PRIu64 "",
            proposal_block->dump().c_str(), highest_block->get_height());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }

    const xblock_ptr_t & lock_block = cs_para.get_latest_locked_block();
    if (lock_block == nullptr) {
        xerror("xtable_maker_t::verify_proposal fail-get lock block.proposal=%s, last_height=%" PRIu64 "",
            proposal_block->dump().c_str(), highest_block->get_height());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }
    if (proposal_block->get_cert()->get_justify_cert_hash() != lock_block->get_input_root_hash()) {
        xerror("xtable_maker_t::verify_proposal fail-proposal unmatch justify hash.proposal=%s, last_height=%" PRIu64 ",justify=%s,%s",
            proposal_block->dump().c_str(), highest_block->get_height(),
            base::xstring_utl::to_hex(proposal_block->get_cert()->get_justify_cert_hash()).c_str(),
            base::xstring_utl::to_hex(lock_block->get_input_root_hash()).c_str());
        return xblockmaker_error_proposal_table_not_match_prev_block;
    }

    xblock_ptr_t local_block = nullptr;
    xtablemaker_result_t table_result;
    if (can_make_next_full_block(cs_para)) {
        local_block = make_full_table(table_para, cs_para, table_result.m_make_block_error_code);
    } else if (proposal_block->get_block_class() == base::enum_xvblock_class_nil) {
        if (can_make_next_empty_block(cs_para)) {
            local_block = make_empty_table(table_para, cs_para, table_result.m_make_block_error_code);
        }
    } else {
        local_block = backup_make_light_table(table_para, cs_para, table_result);
    }
    if (local_block == nullptr) {
        xwarn("xtable_maker_t::verify_proposal fail-make table. proposal=%s,error_code=%s",
            proposal_block->dump().c_str(), chainbase::xmodule_error_to_str(table_result.m_make_block_error_code).c_str());
        return xblockmaker_error_proposal_backup_make_block_fail;
    }

    local_block->get_cert()->set_nonce(proposal_block->get_cert()->get_nonce());  // TODO(jimmy)
    // check local and proposal is match
    if (false == verify_proposal_with_local(proposal_block, local_block.get())) {
        xwarn("xtable_maker_t::verify_proposal fail-verify_proposal_with_local. proposal=%s",
            proposal_block->dump().c_str());
        XMETRICS_GAUGE(metrics::cons_fail_verify_proposal_table_with_local, 1);
        return xblockmaker_error_proposal_not_match_local;
    }

    xinfo("xtable_maker_t::verify_proposal succ.proposal=%s",
        proposal_block->dump().c_str());
    return xsuccess;
}

bool xtable_maker_t::verify_proposal_with_local(base::xvblock_t *proposal_block, base::xvblock_t *local_block) const {
    const std::vector<base::xventity_t*> & _proposal_table_inentitys = proposal_block->get_input()->get_entitys();
    const std::vector<base::xventity_t*> & _local_table_inentitys = local_block->get_input()->get_entitys();
    if (_proposal_table_inentitys.size() != _local_table_inentitys.size()) {
        xwarn("xtable_maker_t::verify_proposal_with_local fail-entity size not same. %s %s",
            proposal_block->dump().c_str(),
            local_block->dump().c_str());
        return false;
    }
    size_t entity_count = _proposal_table_inentitys.size();
    for (size_t i = 0; i < entity_count; i++) {
        base::xvinentity_t* _proposal_inentity = dynamic_cast<base::xvinentity_t*>(_proposal_table_inentitys[i]);
        base::xvinentity_t* _local_inentity = dynamic_cast<base::xvinentity_t*>(_local_table_inentitys[i]);
        if (_proposal_inentity == nullptr || _local_inentity == nullptr) {
            xerror("xtable_maker_t::verify_proposal_with_local fail-get inentity. %s", proposal_block->dump().c_str());  // should never happen
            return false;
        }
        if (i == 0) {  // entity#0 used for table self
            if (_proposal_inentity->get_extend_data() != _local_inentity->get_extend_data()) {
                xerror("xtable_maker_t::verify_proposal_with_local fail-get inentity. %s", proposal_block->dump().c_str());  // should never happen
                return false;
            }
            continue;
        }

        base::xtable_inentity_extend_t _proposal_extend;
        _proposal_extend.serialize_from_string(_proposal_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _proposal_unit_header = _proposal_extend.get_unit_header();
        base::xtable_inentity_extend_t _local_extend;
        _local_extend.serialize_from_string(_local_inentity->get_extend_data());
        const xobject_ptr_t<base::xvheader_t> & _local_unit_header = _local_extend.get_unit_header();
        if (_proposal_unit_header->get_account() != _local_unit_header->get_account()
            || _proposal_unit_header->get_height() != _local_unit_header->get_height()
            || _proposal_unit_header->get_block_class() != _local_unit_header->get_block_class()) {
            xerror("xtable_maker_t::verify_proposal_with_local fail-unit entity not match. %s,leader=%s,local=%s",
                proposal_block->dump().c_str(),
                data::xblock_t::dump_header(_proposal_unit_header.get()).c_str(), data::xblock_t::dump_header(_local_unit_header.get()).c_str());
            return false;
        }
        if (_proposal_inentity->get_extend_data() != _local_inentity->get_extend_data()
         || _proposal_inentity->get_actions().size() != _local_inentity->get_actions().size()) {
            xwarn("xtable_maker_t::verify_proposal_with_local fail-extend data not match. %s,leader=%s,local=%s,action_size=%zu,%zu",
                proposal_block->dump().c_str(),
                data::xblock_t::dump_header(_proposal_unit_header.get()).c_str(), data::xblock_t::dump_header(_local_unit_header.get()).c_str(),
                _proposal_inentity->get_actions().size(), _local_inentity->get_actions().size());
            return false;
        }
    }

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
            xerror("xtable_maker_t::verify_proposal_with_local fail-state root not match.%s proposal:%s local:%s",proposal_block->dump().c_str(),local_state_root.as_hex_str().c_str(),proposal_state_root.as_hex_str().c_str());
            return false;
        }

        xwarn("xtable_maker_t::verify_proposal_with_local fail-header hash not match. %s proposal:%s local:%s",
            proposal_block->dump().c_str(),
            ((data::xblock_t*)proposal_block)->dump_header().c_str(),
            ((data::xblock_t*)local_block)->dump_header().c_str());
        return false;
    }
    if(!local_block->get_cert()->is_equal(*proposal_block->get_cert())){
        xerror("xtable_maker_t::verify_proposal_with_local fail-cert hash not match. proposal:%s local:%s",
            ((data::xblock_t*)proposal_block)->dump_cert().c_str(),
            ((data::xblock_t*)local_block)->dump_cert().c_str());
        return false;
    }
    bool bret = proposal_block->set_output_resources(local_block->get_output()->get_resources_data());
    if (!bret) {
        xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block output fail");
        return false;
    }
    bret = proposal_block->set_input_resources(local_block->get_input()->get_resources_data());
    if (!bret) {
        xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block input fail");
        return false;
    }
    bret = proposal_block->set_output_offdata(local_block->get_output_offdata());
    if (!bret) {
        xerror("xtable_maker_t::verify_proposal_with_local fail-set proposal block output offdata fail.%s",proposal_block->dump().c_str());
        return false;
    }

    proposal_block->set_excontainer(local_block->get_excontainer());
    return true;
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

bool xtable_maker_t::account_index_upgrade() {
    std::lock_guard<std::mutex> l(m_index_upgrade_lock);
    if (m_account_index_upgrade_finished) {
        return true;
    }

    auto latest_committed_block = get_blockstore()->get_latest_committed_block(*this);

    if (m_account_index_upgrade.get_fork_height() < latest_committed_block->get_height()) {
        auto latest_cert_block = get_blockstore()->get_latest_cert_block(*this);

        auto state_root_commit = data::xblockextract_t::get_state_root_from_block(latest_committed_block.get());
        if (!state_root_commit.empty()) {
            auto finish_tag = "upgrade_account_index_upgrade_" + latest_cert_block->get_account();
            XMETRICS_COUNTER_INCREMENT( finish_tag , 1);
            xinfo("[account_index_upgrade] xtable_maker_t::account_index_upgrade commit block already have mpt root, upgrade finished.cert block:%s", latest_cert_block->dump().c_str());
            m_account_index_upgrade.clear();
            m_account_index_upgrade_finished = true; // TODO(jimmy) commit block will not rollback
            return true;
        }

        auto state_root_cert = data::xblockextract_t::get_state_root_from_block(latest_cert_block.get());
        if (!state_root_cert.empty()) {
            xinfo("[account_index_upgrade] xtable_maker_t::account_index_upgrade cert block already have mpt root, upgrade finished.cert block:%s", latest_cert_block->dump().c_str());
            // m_account_index_upgrade.clear();
            // m_account_index_upgrade_finished = true;
            return true;
        }

        data::xtablestate_ptr_t commit_table_state = statestore::xstatestore_hub_t::instance()->get_table_state_by_block(latest_committed_block.get());
        if (commit_table_state == nullptr) {
            xwarn("[account_index_upgrade] xtable_maker_t::account_index_upgrade fail clone commit tablestate. block:%s", latest_committed_block->dump().c_str());
            return false;
        }

        std::map<std::string, std::string> indexes = commit_table_state->map_get(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
        std::vector<lack_account_info_t> lack_accounts;
        m_account_index_upgrade.init(latest_committed_block->get_account(), indexes.size(), commit_table_state->height());
        xinfo("[account_index_upgrade] begin.commit=%s,height=%ld,count=%zu", latest_committed_block->dump().c_str(), commit_table_state->height(),indexes.size());
        for (auto & index : indexes) {
            base::xaccount_index_t _account_index;
            _account_index.old_serialize_from(index.second);
            m_account_index_upgrade.add_old_index(index.first, _account_index);
        }
    }

    uint32_t convert_num_once = 50;
    return m_account_index_upgrade.upgrade(xaccount_index_upgrade_tool_t::convert_to_new_account_index, convert_num_once);
}

bool xtable_maker_t::get_new_account_indexes(const data::xblock_consensus_para_t & cs_para, std::map<std::string, base::xaccount_index_t> & new_indexes) {
    std::map<std::string, base::xaccount_index_t> new_indexes_tmp;
    bool ret = true;
    {
        std::lock_guard<std::mutex> l(m_index_upgrade_lock);
        ret = m_account_index_upgrade.get_new_indexes(new_indexes_tmp, cs_para.get_latest_committed_block()->get_height());
        if (!ret) {
            return false;
        }
    }

    ret = xaccount_index_upgrade_tool_t::update_new_indexes_by_block(new_indexes_tmp, cs_para.get_latest_locked_block());
    if (!ret) {
        return false;
    }

    ret = xaccount_index_upgrade_tool_t::update_new_indexes_by_block(new_indexes_tmp, cs_para.get_latest_cert_block());
    if (!ret) {
        return false;
    }

    new_indexes.swap(new_indexes_tmp);
    xinfo("[account_index_upgrade] xtable_maker_t::get_new_account_indexes account indexes upgrade ok. cs_para=%s,old account num:%u", cs_para.dump().c_str(), new_indexes.size());
    return true;
}

std::shared_ptr<state_mpt::xstate_mpt_t> xtable_maker_t::create_new_mpt(const xhash256_t & last_mpt_root,
                                                                          const data::xblock_consensus_para_t & cs_para,
                                                                          const statectx::xstatectx_ptr_t & table_state_ctx,
                                                                          const std::vector<std::pair<xblock_ptr_t, base::xaccount_index_t>> & batch_unit_and_index) {
    std::error_code ec;
    auto mpt = table_state_ctx->get_prev_tablestate_ext()->get_state_mpt();
    xassert(nullptr != mpt);

    // todo:delete in v1.8
    if (last_mpt_root.empty()) {  // TODO(jimmy)  delete in v1.8
        std::map<std::string, base::xaccount_index_t> new_indexes;
        auto ret = get_new_account_indexes(cs_para, new_indexes);
        if (!ret) {
            xdbg("xtable_maker_t::create_new_mpt no account indexes.");
            return nullptr;
        }

        for (auto & index : new_indexes) {
            auto & addr = index.first;
            auto & account_index = index.second;
            std::string _new_account_index_str;
            account_index.serialize_to(_new_account_index_str);
            // all old indexes set to property, easy to construct mpt by table block(e.g. archive nodes)
            table_state_ctx->get_table_state()->map_set(data::XPROPERTY_TABLE_ACCOUNT_INDEX, addr, _new_account_index_str);
            mpt->set_account_index(common::xaccount_address_t{addr}, _new_account_index_str, ec);
            if (ec) {
                xerror("xtable_maker_t::create_new_mpt set account index from table property to mpt fail.");
                return nullptr;
            }
        }
    } else {
        // remove account index after root hash already exist.
        std::map<std::string, std::string> indexes = table_state_ctx->get_table_state()->map_get(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
        if (!indexes.empty()) {
            xinfo("xtable_maker_t::create_new_mpt begin to clear accountindex.cs_para=%s,tablestate=%s,index_count=%zu",
                  cs_para.dump().c_str(), table_state_ctx->get_table_state()->get_bstate()->dump().c_str(), indexes.size());
            table_state_ctx->get_table_state()->map_clear(data::XPROPERTY_TABLE_ACCOUNT_INDEX);
        }
    }

    for (auto & unit_and_index : batch_unit_and_index) {
        auto & unit = unit_and_index.first;
        auto & index = unit_and_index.second;
        // todo(nathan): set unit state.
        mpt->set_account_index(common::xaccount_address_t{unit->get_account()}, index, ec);
        if (ec) {
            xerror("xtable_maker_t::create_new_mpt set account index to mpt fail.");
            return nullptr;
        }
    }

    return mpt;
}

const std::string xeth_header_builder::build(const xblock_consensus_para_t & cs_para, const evm_common::xh256_t & state_root, const std::vector<txexecutor::xatomictx_output_t> & pack_txs_outputs) {
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

void xaccount_index_upgrade_t::init(const std::string & table_addr, uint32_t accounts_num, uint64_t committed_height) {
    if (m_fork_height >= committed_height) {
        return;
    }
    m_table_address = table_addr;
    m_new_indexes.clear();
    m_lack_accounts.clear();
    m_lack_accounts_pos = 0;
    m_accounts_num = accounts_num;
    m_fork_height = committed_height;
}

void xaccount_index_upgrade_t::add_old_index(const std::string & addr, const base::xaccount_index_t & account_index) {
    m_lack_accounts.push_back(lack_account_info_t(addr, account_index));
}

bool xaccount_index_upgrade_t::upgrade(account_index_converter convert_func, uint32_t max_convert_num) {
    uint32_t max_total_num = max_convert_num; // XTODO
    uint32_t total_num = 0;
    uint32_t num = 0;

    if (!m_lack_accounts.empty()) {
        if (m_lack_accounts_pos >= m_lack_accounts.size()) {
            m_lack_accounts_pos = 0;
        }
        xinfo("[account_index_upgrade] upgrade continue.table=%s,total_count=%d,lack_count=%zu,pos=%d",
        m_table_address.c_str(), m_accounts_num, m_lack_accounts.size(),m_lack_accounts_pos);

        auto iter = m_lack_accounts.begin() + m_lack_accounts_pos;
        while (num < max_convert_num && iter != m_lack_accounts.end()) {
            auto & lack_account = *iter;
            base::xvaccount_t unit_vaccount(lack_account.get_addr());
            base::xaccount_index_t _new_account_index;
            auto ret = convert_func(unit_vaccount, lack_account.get_account_index(), _new_account_index);
            if (ret) {
                m_new_indexes[unit_vaccount.get_account()] = _new_account_index;
                iter = m_lack_accounts.erase(iter);
            } else {
                iter++;
                num++;
                m_lack_accounts_pos++;
            }

            // TODO(jimmy) add total num control for memory usage
            total_num++;
            if (total_num >= max_total_num) {
                break;
            }
        }

        if (m_new_indexes.size() == m_accounts_num) {
            xinfo("[account_index_upgrade] upgrade ready.table=%s,height=%ld,total=%d,lack=%zu,new_indexes=%zu,pos=%d",
            m_table_address.c_str(), m_fork_height, m_accounts_num, m_lack_accounts.size(),m_new_indexes.size(), m_lack_accounts_pos);
        }
    }

    if (m_new_indexes.size() == m_accounts_num) {
        xassert(m_lack_accounts.empty());
        return true;
    }
    xassert(m_new_indexes.size() + m_lack_accounts.size() == m_accounts_num);
    return false;
}

bool xaccount_index_upgrade_t::get_new_indexes(std::map<std::string, base::xaccount_index_t> & new_indexes, uint64_t committed_height) {
    if (committed_height != m_fork_height || m_new_indexes.size() != m_accounts_num) {
        return false;
    }
    new_indexes = m_new_indexes;
    return true;
}

uint64_t xaccount_index_upgrade_t::get_fork_height() const {
    return m_fork_height;
}

void xaccount_index_upgrade_t::clear() {
    m_new_indexes.clear();
    m_lack_accounts.clear();
    m_lack_accounts_pos = 0;
    m_accounts_num = 0;
    m_fork_height = 0;
}

bool xaccount_index_upgrade_tool_t::write_unitstate_with_new_dbkey(const base::xvaccount_t & account, data::xunitstate_ptr_t const& unitstate, std::string const& block_hash) {
    std::string state_db_key = base::xvdbkey_t::create_prunable_unit_state_key(account, unitstate->height(), block_hash);
    std::string state_db_bin;
    if(unitstate->get_bstate()->serialize_to_string(state_db_bin) < 0) {
        xerror("xtable_maker_t::convert_to_new_account_index fail-upgrade for serialize unitstate bstate=%s", unitstate->get_bstate()->dump().c_str());
        return false;
    }
    if (false == base::xvchain_t::instance().get_xdbstore()->set_value(state_db_key, state_db_bin)) {
        xerror("xtable_maker_t::convert_to_new_account_index fail-upgrade for write unitstate bstate=%s", unitstate->get_bstate()->dump().c_str());
        return false;
    }
    return true;
}

bool xaccount_index_upgrade_tool_t::convert_to_new_account_index(const base::xvaccount_t & account, const base::xaccount_index_t & old_account_index, base::xaccount_index_t & new_account_index) {
    uint64_t latest_connect_height = get_xblockstore()->get_latest_connected_block_height(account);

    bool ret = data::xblocktool_t::check_lacking_unit_and_try_sync(account, old_account_index, latest_connect_height, get_xblockstore(), "table_maker");
    if (!ret) {
        xwarn("xtable_maker_t::convert_to_new_account_index fail-lack of units.account:%s,old index:%s,connect height:%llu", account.get_account().c_str(), old_account_index.dump().c_str(), latest_connect_height);
        return false;
    }

    uint64_t height = old_account_index.get_latest_unit_height();
    auto unit = get_xblockstore()->load_block_object(account, height, old_account_index.get_latest_unit_viewid(), false);
    if (nullptr == unit) {
        xerror("xtable_maker_t::convert_to_new_account_index fail-upgrade for load unit account=%s,index=%s", account.get_account().c_str(), old_account_index.dump().c_str());
        return false;
    }

    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(unit.get());
    if (nullptr == unitstate) {
        xerror("xtable_maker_t::convert_to_new_account_index fail-upgrade for load unitstate account=%s,index=%s", account.get_account().c_str(), old_account_index.dump().c_str());
        return false;
    }

    // should write unitstate with new db key
    if (false == write_unitstate_with_new_dbkey(account, unitstate, unit->get_block_hash())) {
        xerror("xtable_maker_t::convert_to_new_account_index fail-upgrade for write db unitstate account=%s,index=%s", account.get_account().c_str(), old_account_index.dump().c_str());
        return false;
    }

    auto nonce = unitstate->account_send_trans_number();
    std::string unitstate_bin;
    unitstate->get_bstate()->take_snapshot(unitstate_bin);
    std::string statehash = unit->get_cert()->hash(unitstate_bin);
    new_account_index = base::xaccount_index_t(unit->get_height(), unit->get_block_hash(), statehash, nonce);
    xinfo("xtable_maker_t::convert_to_new_account_index succ.account:%s,old:%s,new:%s", account.get_account().c_str(), old_account_index.dump().c_str(), new_account_index.dump().c_str());
    return true;
}

bool xaccount_index_upgrade_tool_t::update_new_indexes_by_block(std::map<std::string, base::xaccount_index_t> & new_indexes, const xblock_ptr_t & block) {
    if ((block->get_block_class() != base::enum_xvblock_class_light) || (block->get_height() == 0)) {
        return true;
    }

    base::xvaccount_t vaccount(block->get_account());
    if (false == get_xblockstore()->load_block_input(vaccount, block.get())) {
        xerror("xtable_maker_t::update_new_indexes_by_block,fail-load tableblock input.block=%s", block->dump().c_str());
        return false;
    }
    if (false == get_xblockstore()->load_block_output(vaccount, block.get())) {
        xerror("xvblockstore_impl::update_new_indexes_by_block,fail-load tableblock output.block=%s", block->dump().c_str());
        return false;
    }
    std::vector<xobject_ptr_t<base::xvblock_t>> sub_blocks;
    if (!block->extract_sub_blocks(sub_blocks)) {
        xerror("xvblockstore_impl::update_new_indexes_by_block,fail-extract sub block.block=%s", block->dump().c_str());
        return false;
    }

    for (auto & unit : sub_blocks) {
        data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_state_by_unit_block(unit.get());
        if (nullptr == unitstate) {
            xerror("xtable_maker_t::update_new_indexes_by_block fail-upgrade for load unitstate unit=%s", unit->dump().c_str());
            return false;
        }

        auto & addr = unit->get_account();
        auto nonce = unitstate->account_send_trans_number();
        std::string unithash = unit->get_block_hash();
        std::string statehash = unit->get_fullstate_hash();
        if (statehash.empty()) {
            auto iter = new_indexes.find(addr);
            if (iter == new_indexes.end()) {
                xerror("xtable_maker_t::update_new_indexes_by_block fail-find fail from new_indexes unit=%s", unit->dump().c_str());
                return false;
            }

            statehash = iter->second.get_latest_state_hash();
        }

        base::xaccount_index_t _new_account_index(unit->get_height(), unithash, statehash, nonce);
        // should write unitstate with new db key
        if (false == write_unitstate_with_new_dbkey(unitstate->account_address().vaccount(), unitstate, unit->get_block_hash())) {
            xerror("xtable_maker_t::update_new_indexes_by_block fail-upgrade for write db unitstate account=%s,index=%s",
                   unitstate->account_address().to_string().c_str(),
                   _new_account_index.dump().c_str());
            return false;
        }

        xinfo("xtable_maker_t::update_new_indexes_by_block succ.account:%s,new:%s", unitstate->account_address().to_string().c_str(), _new_account_index.dump().c_str());
        new_indexes[addr] = _new_account_index;
    }
    return true;
}

base::xvblockstore_t * xaccount_index_upgrade_tool_t::get_xblockstore() {
    return base::xvchain_t::instance().get_xblockstore();
}

base::xvblkstatestore_t* xaccount_index_upgrade_tool_t::get_blkstate_store() {
    return base::xvchain_t::instance().get_xstatestore()->get_blkstate_store();
}

NS_END2
