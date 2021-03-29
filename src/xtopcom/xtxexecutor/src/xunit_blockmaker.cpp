// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxexecutor/xunit_blockmaker.h"
#include "xtxexecutor/xtransaction_executor.h"
#include "xtxexecutor/xunit_service_error.h"
#include "xtxexecutor/xblock_maker_para.h"
#include "xverifier/xverifier_utl.h"
#include "xstore/xaccount_context.h"
#include "xdata/xblocktool.h"
#include "xconfig/xpredefined_configurations.h"
#include "xconfig/xconfig_register.h"
#include "xmbus/xevent_behind.h"

NS_BEG2(top, txexecutor)

using xdataobj_ptr_t = xobject_ptr_t<base::xdataobj_t>;

base::xvblock_t* xunit_blockmaker_t::create_lightunit(xaccount_context_t* context,
                                                      base::xvblock_t* prev_block,
                                                      const xblock_consensus_para_t & cs_para,
                                                      const std::vector<xcons_transaction_ptr_t> & input_txs,
                                                      int & error_code,
                                                      std::vector<xcons_transaction_ptr_t> & proposal_txs) {
    data::xlightunit_block_para_t lightunit_para;
    xassert(!input_txs.empty());
    int ret = make_lightunit_output(context, input_txs, lightunit_para);
    if (ret) {
        error_code = ret;
        return nullptr;
    }
    error_code = xsuccess;
    proposal_txs = lightunit_para.get_input_txs();
    xassert(proposal_txs.size() == input_txs.size());
    return data::xlightunit_block_t::create_next_lightunit(lightunit_para, prev_block);
}

base::xvblock_t* xunit_blockmaker_t::create_fullunit(xaccount_context_t* context,
                                                      base::xvblock_t* prev_block,
                                                      const xblock_consensus_para_t & cs_para,
                                                      int & error_code) {
    xfullunit_block_para_t para;
    int ret = make_fullunit_output(context, para);
    if (ret != xsuccess) {
        error_code = ret;
        return nullptr;
    }
    error_code = xsuccess;
    return data::xfullunit_block_t::create_next_fullunit(para, prev_block);
}

base::xvblock_t* xunit_blockmaker_t::create_unit(xaccount_context_t* context,
                                                      base::xvblock_t* prev_block,
                                                      const xblock_consensus_para_t & cs_para,
                                                      const std::vector<xcons_transaction_ptr_t> & input_txs,
                                                      const std::string & justify_cert_hash,
                                                      int & error_code,
                                                      std::vector<xcons_transaction_ptr_t> & proposal_txs) {
    base::xvblock_t* unit = nullptr;
    if (can_make_full_unit(context, prev_block)) {
        unit = create_fullunit(context, prev_block, cs_para, error_code);
    } else {
        xassert(!input_txs.empty());
        unit = create_lightunit(context, prev_block, cs_para, input_txs, error_code, proposal_txs);
    }
    if (error_code != xsuccess) {
        xassert(unit == nullptr);
        return nullptr;
    }

    data::xblock_t* block = (data::xblock_t*)unit;
    block->get_cert()->set_justify_cert_hash(justify_cert_hash);
    block->set_consensus_para(cs_para);
    return unit;
}


int xunit_blockmaker_t::make_block(const std::string &account,
                                   const xblock_consensus_para_t & cs_para,
                                   uint64_t committed_height,
                                   xtxpool::xtxpool_table_face_t* txpool_table,
                                   base::xvblock_t* & out_unit,
                                   xunit_proposal_input_t & proposal_input) {
    auto latest_unit_block = m_blockstore->get_latest_committed_block(account);
    xassert(latest_unit_block != nullptr);

    int ret = leader_check_state(account, latest_unit_block.get());
    if (ret) {
        return ret;
    }

    std::shared_ptr<xaccount_context_t> _account_context = std::make_shared<xaccount_context_t>(account, m_store.get());
    _account_context->set_context_para(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_timestamp(), cs_para.get_total_lock_tgas_token());
    if (_account_context->get_blockchain()->get_chain_height() != latest_unit_block->get_height()) {
        return enum_xtxexecutor_error_unit_height_not_equal_blockchain_height;
    }

    txpool_table->update_committed_unit_block(dynamic_cast<xblock_t*>(latest_unit_block.get()),
                                              _account_context->get_blockchain()->account_send_trans_number(),
                                              _account_context->get_blockchain()->account_send_trans_hash());
    auto txs = txpool_table->get_account_txs(account,
                                         committed_height,
                                         latest_unit_block->get_height(),
                                         _account_context->get_blockchain()->account_send_trans_number(),
                                         _account_context->get_blockchain()->account_send_trans_hash(),
                                         cs_para.get_timestamp());
    if (txs.empty()) {
        return enum_xtxexecutor_error_account_have_no_tx;
    }

    std::string justify_cert_hash;
    if (!get_lock_block_sign_hash(latest_unit_block.get(), justify_cert_hash)) {
        return enum_xtxexecutor_error_unit_state_behind;
    }

    std::vector<xcons_transaction_ptr_t> proposal_txs;
    base::xvblock_t* unit = create_unit(_account_context.get(), latest_unit_block.get(), cs_para, txs, justify_cert_hash, ret, proposal_txs);
    if (ret != xsuccess) {
        return ret;
    }

    data::xblock_t* block = (data::xblock_t*)unit;
    out_unit = unit;
    proposal_input.set_basic_info(block->get_account(), block->get_height() - 1, block->get_last_block_hash());
    proposal_input.set_input_txs(proposal_txs);
    xkinfo("xunit_blockmaker_t::make_block.unit=%s,cert=%s",
        block->dump().c_str(),
        block->dump_cert().c_str());

    return xsuccess;
}

int xunit_blockmaker_t::verify_block(const xunit_proposal_input_t & proposal_input,
                                    const xblock_consensus_para_t & cs_para,
                                    base::xvblock_t* & proposal_unit,
                                    xtxpool::xtxpool_table_face_t* txpool_table,
                                    uint64_t committed_height) {
    const std::string & account = proposal_input.get_account();
    base::xauto_ptr<base::xvblock_t> latest_unit_block = m_blockstore->get_latest_committed_block(account);

    int ret = backup_check_state(proposal_input, latest_unit_block.get());
    if (ret) {
        return ret;
    }

    std::shared_ptr<xaccount_context_t> _account_context = std::make_shared<xaccount_context_t>(account, m_store.get());
    _account_context->set_context_para(cs_para.get_clock(), cs_para.get_random_seed(), cs_para.get_timestamp(), cs_para.get_total_lock_tgas_token());

    if (!proposal_input.get_input_txs().empty()) {
        txpool_table->update_committed_unit_block(dynamic_cast<xblock_t*>(latest_unit_block.get()),
                                                _account_context->get_blockchain()->account_send_trans_number(),
                                                _account_context->get_blockchain()->account_send_trans_hash());
        int ret = m_txpool->verify_txs(account, committed_height, latest_unit_block->get_height(), proposal_input.get_input_txs(),
                                    _account_context->get_blockchain()->account_send_trans_number(), _account_context->get_blockchain()->account_send_trans_hash());
        if (ret) {
            xwarn("xunit_blockmaker_t::verify_block input txs fail. account=%s",
                account.c_str());
            return ret;
        }
    }

    std::string justify_cert_hash;
    if (!get_lock_block_sign_hash(latest_unit_block.get(), justify_cert_hash)) {
        return enum_xtxexecutor_error_unit_state_behind;
    }

    std::vector<xcons_transaction_ptr_t> proposal_txs;
    base::xvblock_t* unit = create_unit(_account_context.get(), latest_unit_block.get(), cs_para, proposal_input.get_input_txs(), justify_cert_hash, ret, proposal_txs);
    if (ret != xsuccess) {
        return ret;
    }
    xassert(proposal_txs.size() == proposal_input.get_input_txs().size());

    ret = verify_proposal_block_class(_account_context.get(), unit->get_header(), latest_unit_block.get());
    if (ret) {
        unit->release_ref();
        return ret;
    }

    data::xblock_t* block = (data::xblock_t*)unit;
    xkinfo("xunit_blockmaker_t::verify_block.unit=%s,cert=%s",
        block->dump().c_str(),
        block->dump_cert().c_str());

    proposal_unit = unit;
    return xsuccess;
}

bool xunit_blockmaker_t::get_lock_block_sign_hash(base::xvblock_t* highqc_block, std::string & hash) {
    const std::string & account = highqc_block->get_account();
    if (highqc_block->get_height() < 2) {
        hash = highqc_block->get_cert()->get_hash_to_sign();
    } else {
        base::xauto_ptr<base::xvblock_t> lock_block = m_blockstore->load_block_object(account, highqc_block->get_height() - 1);
        if (lock_block == nullptr) {
            xerror("xunit_blockmaker_t::get_lock_block_sign_hash fail to load block, account=%s,height=%ld", account.c_str(), highqc_block->get_height() - 1);
            return false;
        }
        hash = lock_block->get_cert()->get_hash_to_sign();
    }
    return true;
}

int xunit_blockmaker_t::verify_proposal_block_class(xaccount_context_t * context, base::xvheader_t* unitheader, base::xvblock_t * latest_unit) {
    bool can_fullunit = can_make_full_unit(context, latest_unit);
    if (can_fullunit && unitheader->get_block_class() != base::enum_xvblock_class_full) {
        xerror("xunit_blockmaker_t::verify_proposal_block_class should be fullunit. latest_unit:%s height:%ld",
            unitheader->get_account().c_str(), latest_unit->get_height());
        return enum_xtxexecutor_error_proposal_unit_should_be_fullunit;
    }
    if (!can_fullunit && unitheader->get_block_class() != base::enum_xvblock_class_light) {
        xerror("xunit_blockmaker_t::verify_proposal_block_class should be lightunit unit:%s height:%ld",
            unitheader->get_account().c_str(), latest_unit->get_height());
        return enum_xtxexecutor_error_proposal_unit_should_be_lightunit;
    }
    return xsuccess;
}

bool xunit_blockmaker_t::can_make_full_unit(xaccount_context_t * context, base::xvblock_t* prev_block) {
    uint32_t full_unit_count = XGET_ONCHAIN_GOVERNANCE_PARAMETER(fullunit_contain_of_unit_num);

    auto account_obj = context->get_blockchain();

    uint64_t current_height = prev_block->get_height() + 1;
    uint64_t current_fullunit_height = prev_block->get_block_class() == base::enum_xvblock_class_full ? prev_block->get_height() : prev_block->get_last_full_block_height();
    uint64_t current_lightunit_count = current_height - current_fullunit_height;
    xassert(current_lightunit_count > 0);
    if (current_lightunit_count >= full_unit_count && account_obj->get_unconfirm_sendtx_num() == 0) {
        xdbg("xunit_blockmaker_t::can_make_full_unit fullunit. %s height:%ld full_unit_height:%ld unconfirm_sendtx_num:%ld",
            account_obj->get_account().c_str(), current_height, current_fullunit_height, account_obj->get_unconfirm_sendtx_num());
        return true;
    }
    xdbg("xunit_blockmaker_t::can_make_full_unit lightunit. %s height:%ld full_unit_height:%ld unconfirm_sendtx_num:%ld",
        account_obj->get_account().c_str(), current_height, current_fullunit_height, account_obj->get_unconfirm_sendtx_num());
    return false;
}

int xunit_blockmaker_t::make_lightunit_output(xaccount_context_t * context,
                                                const std::vector<xcons_transaction_ptr_t> & txs,
                                                data::xlightunit_block_para_t & lightunit_para) {
    xbatch_txs_result_t exec_result;
    int exec_ret = xtransaction_executor::exec_batch_txs(context, txs, exec_result);
    if (exec_ret != xsuccess) {
        xassert(exec_result.m_exec_fail_tx_ret != 0);
        xassert(exec_result.m_exec_fail_tx != nullptr);
        const auto & failtx = exec_result.m_exec_fail_tx;

        xassert(failtx->is_self_tx() || failtx->is_send_tx());
        if (exec_result.m_exec_fail_tx_ret != xunit_contract_exec_no_property_change) {
            xwarn("xunit_blockmaker_t::make_block tx exec fail. %s error:%s",
                failtx->dump().c_str(), chainbase::xmodule_error_to_str(exec_result.m_exec_fail_tx_ret).c_str());
        }
        m_txpool->pop_tx_by_hash(failtx->get_source_addr(), failtx->get_transaction()->digest(), failtx->get_tx_subtype(), exec_result.m_exec_fail_tx_ret);
        // one tx fail will not make unit with other txs
        return exec_ret;
    }

    xtransaction_result_t & tx_result = exec_result.succ_txs_result;
    // set lightunit para by tx result
    lightunit_para.set_input_txs(txs);
    lightunit_para.set_transaction_result(tx_result);
    lightunit_para.set_account_unconfirm_sendtx_num(context->get_blockchain()->get_unconfirm_sendtx_num());
    return xsuccess;
}

int xunit_blockmaker_t::make_fullunit_output(xaccount_context_t * context, xfullunit_block_para_t & para) {
    std::map<std::string, std::string> propertys;
    const auto & property_map = context->get_blockchain()->get_property_hash_map();
    for (auto & v : property_map) {
        xdataobj_ptr_t db_prop = m_store->clone_property(context->get_address(), v.first);
        if (db_prop == nullptr) {
            xerror("xunit_blockmaker_t::make_fullunit_output account:%s,height=%ld,property(%s) not exist.",
                  context->get_address().c_str(), context->get_blockchain()->get_chain_height(), v.first.c_str());
            return enum_xtxexecutor_error_unit_property_behind;
        }

        std::string db_prop_hash = xhash_base_t::calc_dataunit_hash(db_prop.get());
        if (db_prop_hash != v.second) {
            // TODO(jimmy) might happen, because property is not stored by height
            xwarn("xunit_blockmaker_t::make_fullunit_output account:%s,height=%ld,property(%s) hash not match fullunit.",
                  context->get_address().c_str(), context->get_blockchain()->get_chain_height(), v.first.c_str());
            return enum_xtxexecutor_error_unit_property_behind;
        }
        base::xstream_t _stream(base::xcontext_t::instance());
        db_prop->serialize_to(_stream);
        std::string prop_str((const char *)_stream.data(), _stream.size());
        propertys[v.first] = prop_str;
    }

    para.m_account_propertys = propertys;
    para.m_account_state = context->get_blockchain()->get_account_mstate();
    para.m_first_unit_height = context->get_blockchain()->get_last_full_unit_height();
    para.m_first_unit_hash = context->get_blockchain()->get_last_full_unit_hash();
    return xsuccess;
}

int xunit_blockmaker_t::backup_check_state(const xunit_proposal_input_t & proposal_input, base::xvblock_t* latest_commit_block) {
    const std::string & account = proposal_input.get_account();

    // the latest block must connected and executed
    if (!xblocktool_t::is_connect_and_executed_block(latest_commit_block)) {
        xwarn("xunit_blockmaker_t::backup_check_state,backup unit behind. latest_commit_block=%s",
            latest_commit_block->dump().c_str());
        invoke_sync(account, "unit backup sync");
        return enum_xtxexecutor_error_unit_state_behind;
    }

    uint64_t leader_height = proposal_input.get_last_block_height();
    if (leader_height > latest_commit_block->get_height()) {
        xwarn("xunit_blockmaker_t::backup_check_state height less than leader, account:%s leader:%ld, backup:%ld",
              account.c_str(), leader_height, latest_commit_block->get_height());
        invoke_sync(account, "unit backup sync");
        return enum_xtxexecutor_error_unit_height_less_than_leader;
    } else if (leader_height < latest_commit_block->get_height()) {
        xwarn("xunit_blockmaker_t::backup_check_state height larger than leader, account:%s leader:%ld, backup:%ld",
              account.c_str(), leader_height, latest_commit_block->get_height());
        return enum_xtxexecutor_error_unit_height_larger_than_leader;
    } else {
        if (proposal_input.get_last_block_hash() != latest_commit_block->get_block_hash()) {
            xerror("xunit_blockmaker_t::backup_check_state not match prev unit hash, latest_commit_block:%s leader_last_hash:%s",
                latest_commit_block->dump().c_str(), base::xstring_utl::to_hex(proposal_input.get_last_block_hash()).c_str());
            return enum_xtxexecutor_error_unit_leader_hash_not_equal_backup;
        }
    }

    //  TODO(jimmy) cold account should delay xcold_account_unknown_sync_time_configuration_t second for consensus
    // uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    // if (now < account_state.m_update_timestamp) {
    //     xwarn("xunit_blockmaker_t::backup_check_state account usable timestamp not arrive, %s unit_height:%ld usable_timestamp:%ld now:%ld",
    //           account.c_str(),
    //           account_state.m_unit_height,
    //           account_state.m_update_timestamp,
    //           now);
    //     return enum_xtxexecutor_error_unit_usable_timestamp_not_arrive;
    // }

    return xsuccess;
}

int xunit_blockmaker_t::leader_check_state(const std::string & account, base::xvblock_t* latest_commit_block) {
    // unit has no lock and highqc block
    if (!xblocktool_t::is_connect_and_executed_block(latest_commit_block)) {
        xwarn("xunit_blockmaker_t::leader_check_state,leader unit behind. latest_commit_block=%s",
            latest_commit_block->dump().c_str());
        invoke_sync(account, "unit leader sync");
        return enum_xtxexecutor_error_unit_state_behind;
    }

    // TODO(jimmy)  cold account check should delay xcold_account_unknown_sync_time_configuration_t second for consensus
    // uint64_t now = xverifier::xtx_utl::get_gmttime_s();
    // if (now < account_state.m_update_timestamp) {
    //     xwarn("xunit_blockmaker_t::leader_check_state account usable timestamp not arrive, %s unit_height:%ld usable_timestamp:%ld now:%ld",
    //           account.c_str(),
    //           account_state.m_unit_height,
    //           account_state.m_update_timestamp,
    //           now);
    //     return enum_xtxexecutor_error_unit_usable_timestamp_not_arrive;
    // }
    return xsuccess;
}

void xunit_blockmaker_t::invoke_sync(const std::string & account, const std::string & reason) {
    // TODO(jimmy) unit invoke sync
    // mbus::xevent_ptr_t block_event = std::make_shared<mbus::xevent_behind_origin_t>(account, mbus::enum_behind_type_common, reason);
    // m_store->get_mbus()->push_event(block_event);
}

NS_END2
