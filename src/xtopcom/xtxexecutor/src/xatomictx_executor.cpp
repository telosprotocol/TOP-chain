// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxexecutor/xatomictx_executor.h"

#include "xdata/xblocktool.h"
#include "xdata/xtransaction.h"
#include "xdata/xethreceipt.h"
#include "xevm/xevm.h"
#include "xgasfee/xerror/xerror.h"
#include "xgasfee/xgasfee.h"
#include "xgasfee/xgasfee_consortium.h"
#include "xgasfee/xgas_estimate.h"
#include "xtxexecutor/xtvm.h"
#include "xtxexecutor/xtvm_v2.h"

#include <string>
#include <vector>

NS_BEG2(top, txexecutor)

std::string xatomictx_output_t::dump() const {
    char local_param_buf[256];
    xprintf(local_param_buf,sizeof(local_param_buf),"{is_pack=%d,snapshot_height=%zu,is_state_dirty=%d,vm_error=%d,tgas_change=%ld,vmcode=%d,subtxs=%zu",
        m_is_pack, m_snapshot_size,m_is_state_dirty,m_result,
        m_vm_output.m_tgas_balance_change,m_vm_output.m_ec.value(),m_vm_output.m_contract_create_txs.size());
    return std::string(local_param_buf);
}

xatomictx_executor_t::xatomictx_executor_t(const statectx::xstatectx_face_ptr_t & statectx, const xvm_para_t & para)
: m_statectx(statectx), m_para(para) {

}

bool xatomictx_executor_t::update_nonce_and_hash(const data::xunitstate_ptr_t & unitstate, const xcons_transaction_ptr_t & tx) {
    if (tx->is_send_or_self_tx()) {
        uint64_t tx_nonce = tx->get_tx_nonce();
        uint256_t tx_hash = tx->get_tx_hash_256();
        uint64_t account_nonce = unitstate->get_latest_send_trans_number();
        if (account_nonce + 1 != tx_nonce) {
            xerror("xatomictx_executor_t::update_nonce_and_hash fail-set nonce.tx=%s,account_nonce=%ld,nonce=%ld", tx->dump().c_str(), account_nonce, tx_nonce);
            return false;
        }
        unitstate->set_tx_info_latest_sendtx_num(tx_nonce);
        std::string transaction_hash_str = std::string(reinterpret_cast<char*>(tx_hash.data()), tx_hash.size());
        unitstate->set_tx_info_latest_sendtx_hash(transaction_hash_str); // XTODO(jimmy)?
        xdbg("xatomictx_executor_t::update_nonce_and_hash set nonce.tx=%s,nonce=%ld", tx->dump().c_str(), tx_nonce);
    }
    return true;
}

bool xatomictx_executor_t::update_gasfee(const xvm_gasfee_detail_t detail, const data::xunitstate_ptr_t & unitstate, const xcons_transaction_ptr_t & tx) {
    if (detail.m_state_unlock_balance > 0) {
        auto lock_balance = unitstate->lock_balance();
        xassert(lock_balance >= detail.m_state_unlock_balance);
        unitstate->token_withdraw(data::XPROPERTY_BALANCE_LOCK, base::vtoken_t(detail.m_state_unlock_balance));
        unitstate->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(detail.m_state_unlock_balance));
    }
    if (detail.m_state_burn_balance > 0) {
        auto balance = unitstate->balance();
        auto token = std::min(balance, detail.m_state_burn_balance);
        unitstate->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(token));
        unitstate->token_deposit(data::XPROPERTY_BALANCE_BURN, base::vtoken_t(token));
    }
    if (detail.m_state_burn_eth_balance > 0) {
        auto balance = unitstate->tep_token_balance(common::xtoken_id_t::eth);
        auto token = std::min(balance, detail.m_state_burn_eth_balance);
        unitstate->tep_token_withdraw(common::xtoken_id_t::eth, token);
    }
    if (detail.m_state_lock_balance > 0) {
        auto balance = unitstate->balance();
        xassert(balance >= detail.m_state_lock_balance);
        unitstate->token_withdraw(data::XPROPERTY_BALANCE_AVAILABLE, base::vtoken_t(detail.m_state_lock_balance));
        unitstate->token_deposit(data::XPROPERTY_BALANCE_LOCK, base::vtoken_t(detail.m_state_lock_balance));
    }
    if (detail.m_state_used_tgas > 0) {
        unitstate->string_set(data::XPROPERTY_USED_TGAS_KEY, std::to_string(detail.m_state_used_tgas));
    }
    if (detail.m_state_last_time > 0) {
        unitstate->string_set(data::XPROPERTY_LAST_TX_HOUR_KEY, std::to_string(detail.m_state_last_time));
    }
    if (detail.m_tx_used_tgas > 0) {
        tx->set_current_used_tgas(detail.m_tx_used_tgas);
    }
    if (detail.m_tx_used_deposit > 0) {
        tx->set_current_used_deposit(detail.m_tx_used_deposit);
    }
    return true;
}

bool xatomictx_executor_t::set_tx_account_state(const data::xunitstate_ptr_t & unitstate, const xcons_transaction_ptr_t & tx) {
    // update account create time propertys
    if (unitstate->get_block_height() < 2) {
        unitstate->set_account_create_time(m_para.get_clock());
    }
    return update_nonce_and_hash(unitstate, tx);

    // TODO(jimmy) recvtx num has no value but will cause to state change
    // if (tx->is_recv_tx()) {
    //     uint64_t recv_num = unitstate->account_recv_trans_number();
    //     unitstate->set_tx_info_recvtx_num(recv_num + 1);
    // }
    // return true;
}

bool xatomictx_executor_t::set_tx_table_state(const data::xtablestate_ptr_t & tablestate, const xcons_transaction_ptr_t & tx) {
    if ((tx->is_self_tx() || tx->get_inner_table_flag())) {
        xdbg("xatomictx_executor_t::set_tx_table_state not need.tx=%s", tx->dump().c_str());
        return true;
    }

    base::xtable_shortid_t peer_tableid = tx->get_peer_tableid();
    base::xreceiptid_pair_t receiptid_pair;
    tablestate->find_receiptid_pair(peer_tableid, receiptid_pair);

    if (data::xblocktool_t::alloc_transaction_receiptid(tx, receiptid_pair)) {
        tablestate->set_receiptid_pair(peer_tableid, receiptid_pair);  // save to modified pairs
        xinfo("xatomictx_executor_t::set_tx_table_state succ.tx=%s,pair=%s", tx->dump().c_str(), receiptid_pair.dump().c_str());
    } else {
        xerror("xatomictx_executor_t::set_tx_table_state fail.tx=%s,pair=%s", tx->dump().c_str(), receiptid_pair.dump().c_str());
    }
    return true;
}

bool xatomictx_executor_t::update_tx_related_state(const data::xunitstate_ptr_t & tx_unitstate, const xcons_transaction_ptr_t & tx, const xvm_output_t & vmoutput) {
    bool ret = false;
    for (auto & subtx : vmoutput.m_contract_create_txs) {
        ret = set_tx_account_state(tx_unitstate, subtx);
        if (!ret) {
            return ret;
        }
    }

    ret = set_tx_table_state(m_statectx->get_table_state(), tx);
    if (!ret) {
        return ret;
    }
    for (auto & subtx : vmoutput.m_contract_create_txs) {
        ret = set_tx_table_state(m_statectx->get_table_state(), subtx);
        if (!ret) {
            return ret;
        }
    }
    xdbg("xatomictx_executor_t::update_tx_related_state succ.tx=%s,subtxs=%zu",tx->dump().c_str(), vmoutput.m_contract_create_txs.size());
    return true;
}


bool xatomictx_executor_t::check_account_order(const xcons_transaction_ptr_t & tx) {
    // check account nonce order
    if (tx->is_send_or_self_tx()) {
        std::string address = tx->get_source_addr();
        base::xvaccount_t vaddr(address);
        data::xunitstate_ptr_t unitstate = m_statectx->load_unit_state(vaddr);
        if (nullptr == unitstate) {
            xwarn("xatomictx_executor_t::check_account_order fail-load unit state.tx=%s", tx->dump().c_str());
            return false;
        }
        uint64_t accountnonce = unitstate->get_latest_send_trans_number();
        if (tx->get_tx_nonce() != accountnonce+1) {
            xwarn("xatomictx_executor_t::check_account_order fail-nonce unmatch.tx=%s,account_nonce=%ld", tx->dump().c_str(), accountnonce);
            return false;
        }
    }
    return true;
}

bool xatomictx_executor_t::check_receiptid_order(const xcons_transaction_ptr_t & tx) {
    // check account nonce order
    if (tx->is_recv_tx() || tx->is_confirm_tx()) {
        uint64_t tx_receipt_id = tx->get_last_action_receipt_id();
        base::xtable_shortid_t tx_tableid = tx->get_peer_tableid();
        base::xreceiptid_pair_t receiptid_pair;
        m_statectx->get_table_state()->find_receiptid_pair(tx_tableid, receiptid_pair);

        if (tx->is_recv_tx()) {
            uint64_t last_receipt_id = receiptid_pair.get_recvid_max();
            if (tx_receipt_id != last_receipt_id + 1) {
                xwarn("xtxexecutor_top_vm_t::check_table_order fail-receiptid unmatch.tx=%s,tx_id=%ld,cur_id=%ld", tx->dump().c_str(), tx_receipt_id, last_receipt_id);
                return false;
            }
        }

        if (tx->is_confirm_tx()) {
            uint64_t tx_rsp_id = tx->get_last_action_rsp_id();
            uint64_t last_rsp_id = receiptid_pair.get_confirm_rsp_id_max();
            if (tx_rsp_id != last_rsp_id + 1) {
                xwarn("xtxexecutor_top_vm_t::check_table_order fail-rsp id unmatch.tx=%s,tx_id=%ld,cur_id=%ld", tx->dump().c_str(), tx_rsp_id, last_rsp_id);
                return false;
            }

            uint64_t max_sendrspid = receiptid_pair.get_send_rsp_id_max();
            if (tx_rsp_id > max_sendrspid) {
                xwarn("xtxexecutor_top_vm_t::check_table_order fail-rsp id larger than send rspid.tx=%s,tx_id=%ld,cur_id=%ld", tx->dump().c_str(), tx_rsp_id, max_sendrspid);
                return false;
            }

            uint64_t sendid = receiptid_pair.get_sendid_max();
            uint64_t tx_confirmid = tx->get_last_action_receipt_id();
            if (tx_confirmid > sendid) {
                xwarn("xtxexecutor_top_vm_t::check_table_order fail-txconfirmid larger than sendid.tx=%s,tx_id=%ld,cur_id=%ld", tx->dump().c_str(), tx_confirmid, sendid);
                return false;
            }
            uint64_t confirmid = receiptid_pair.get_confirmid_max();
            if (tx_confirmid <= confirmid) {
                xerror("xtxexecutor_top_vm_t::check_table_order fail-txconfirmid smaller than confirmid.tx=%s,tx_id=%ld,cur_id=%ld", tx->dump().c_str(), tx_confirmid, confirmid);
                return false;
            }
        }

    }
    return true;
}

static void set_evm_receipt_info(const xcons_transaction_ptr_t & tx, const xvm_output_t & vmoutput, uint64_t gas_used) {
    if (tx->get_tx_version() != data::xtransaction_version_3) {
        return;
    }

    data::xeth_store_receipt_t evm_tx_receipt;

    data::enum_ethreceipt_status status =
    (vmoutput.m_tx_result.status == evm_common::xevm_transaction_status_t::Success) ? data::ethreceipt_status_successful : data::ethreceipt_status_failed;

    evm_tx_receipt.set_tx_status(status);
    evm_tx_receipt.set_cumulative_gas_used(gas_used + vmoutput.m_tx_result.used_gas);
    evm_tx_receipt.set_gas_used(vmoutput.m_tx_result.used_gas);
    evm_tx_receipt.set_gas_price(gasfee::xgas_estimate::flexible_price(tx, (evm_common::u256)vmoutput.m_tx_result.used_gas));
    if (vmoutput.m_tx_result.status == evm_common::xevm_transaction_status_t::Success) {
        if (!vmoutput.m_tx_result.logs.empty()) {
            evm_tx_receipt.set_logs(vmoutput.m_tx_result.logs);
        }
        if (tx->get_tx_type() == data::xtransaction_type_deploy_evm_contract) {
            evm_tx_receipt.set_contract_address(common::xtop_eth_address::build_from(vmoutput.m_tx_result.extra_msg));
        }
    }
    tx->set_evm_tx_receipt(evm_tx_receipt);
}

enum_execute_result_type xatomictx_executor_t::vm_execute(const xcons_transaction_ptr_t & tx, xatomictx_output_t & output) {
    // XTODO(TOP-tx or ETH-tx)
    xvm_input_t vminput(m_statectx, m_para, tx);
    xvm_output_t vmoutput;
    enum_execute_result_type ret = enum_exec_error_invalid;

    // update tx flag before execute
    tx->set_not_need_confirm();
    tx->set_inner_table_flag();

    data::xunitstate_ptr_t unitstate = m_statectx->load_unit_state(tx->get_account_addr());
    std::shared_ptr<gasfee::xgasfee_interface> gasfee;
    if (XGET_CONFIG(enable_gas_fee)) {
        gasfee = std::make_shared<gasfee::xtop_gasfee>(unitstate, tx, m_para.get_clock(), m_para.get_lock_tgas_token());
    } else {
        gasfee = std::make_shared<gasfee::xconsortium_gasfee>(unitstate, tx, m_para.get_clock(), m_para.get_lock_tgas_token());
    }
    // gasfee::xgasfee_t gasfee{unitstate, tx, m_para.get_clock(), m_para.get_lock_tgas_token()};
    std::error_code ec;

    do {
        if (false == tx->is_evm_tx()) {
            if (tx->get_tx_version() == data::xtransaction_version_3) {
                gasfee->preprocess(ec);
                if (ec) {
                    vmoutput.m_ec = ec;
                    ret = enum_exec_error_estimate_gas;
                    break;
                }
                xtvm_v2_t tvm;
                ret = tvm.execute(vminput, vmoutput);
                gasfee->postprocess(vmoutput.m_tx_result.used_gas, ec);
                if (ret == enum_exec_success && ec) {
                    vmoutput.m_ec = ec;
                    if (ec == make_error_code(gasfee::error::xenum_errc::tx_out_of_gas)) {
                        ret = enum_exec_error_out_of_gas;
                        vmoutput.m_tx_result.status = evm_common::xevm_transaction_status_t::OutOfGas;
                        vmoutput.m_tx_result.used_gas = static_cast<uint64_t>(gasfee->tx_eth_gas_limit());
                        xwarn(
                            "xatomictx_executor_t::vm_execute outof gas, ret: %d, evm_status: %d, used_gas: %lu", ret, vmoutput.m_tx_result.status, vmoutput.m_tx_result.used_gas);
                    } else if (ec == make_error_code(gasfee::error::xenum_errc::account_balance_not_enough)) {
                        ret = enum_exec_error_estimate_gas;
                        vmoutput.m_tx_result.status = evm_common::xevm_transaction_status_t::Revert;
                        vmoutput.m_tx_result.used_gas = static_cast<uint64_t>(gasfee->tx_eth_gas_limit());
                        xwarn("xatomictx_executor_t::vm_execute balance not enough");
                    } else {
                        xassert(false);
                    }
                    // no break here, to do after works
                } else if (ret == enum_exec_success) {
                    vmoutput.m_tx_result.status = evm_common::xevm_transaction_status_t::Success;
                } else {
                    vmoutput.m_tx_result.status = evm_common::xevm_transaction_status_t::OtherExecuteError;
                    vmoutput.m_tx_result.used_gas = static_cast<uint64_t>(gasfee->tx_eth_gas_limit());
                    xwarn("xatomictx_executor_t::vm_execute outof gas, ret: %d, evm_status: %d, used_gas: %lu", ret, vmoutput.m_tx_result.status, vmoutput.m_tx_result.used_gas);
                }
            } else {
                xtvm_t tvm;
                ret = tvm.execute(vminput, vmoutput);
            }
            if (ret == enum_exec_success) {
                set_tx_account_state(unitstate, tx);
            }
        } else {
#ifdef BUILD_EVM
            gasfee->preprocess(ec);
            if (ec) {
                vmoutput.m_ec = ec;
                ret = enum_exec_error_estimate_gas;
                break;
            }
            evm::xtop_evm evm{m_statectx};
            evm.execute(vminput,vmoutput);
            gasfee->postprocess(vmoutput.m_tx_result.used_gas, ec);
            if (vmoutput.m_tx_result.status == evm_common::xevm_transaction_status_t::Success && ec) {
                ret = enum_exec_error_evm_execute;
                vmoutput.m_ec = ec;
                if (ec == make_error_code(gasfee::error::xenum_errc::tx_out_of_gas)) {
                    ret = enum_exec_error_out_of_gas;
                    vmoutput.m_tx_result.status = evm_common::xevm_transaction_status_t::OutOfGas;
                    vmoutput.m_tx_result.used_gas = static_cast<uint64_t>(gasfee->tx_eth_gas_limit());
                    xwarn("xatomictx_executor_t::vm_execute outof gas, evm_status: %d", vmoutput.m_tx_result.status);
                } else if (ec == make_error_code(gasfee::error::xenum_errc::account_balance_not_enough)) {
                    ret = enum_exec_error_estimate_gas;
                    vmoutput.m_tx_result.status = evm_common::xevm_transaction_status_t::Revert;
                    vmoutput.m_tx_result.used_gas = static_cast<uint64_t>(gasfee->tx_eth_gas_limit());
                    xwarn("xatomictx_executor_t::vm_execute balance not enough");
                } else {
                    xassert(false);
                }
            } else if (vmoutput.m_tx_result.status == evm_common::xevm_transaction_status_t::Success) {
                ret = enum_exec_success;
            } else {
                ret = enum_exec_error_evm_execute;
                xwarn("xatomictx_executor_t::vm_execute error evm_status: %d", vmoutput.m_tx_result.status);
            }
#else
            xassert(false);
#endif
        }
    } while(0);

    if (ret != enum_exec_success) {
        xassert(ret != enum_exec_error_invalid);
        xwarn("xatomictx_executor_t::vm_execute tx error: %s, ret: %d, error_code: %d, error_msg: %s", tx->dump().c_str(), ret, vmoutput.m_ec.value(), vmoutput.m_ec.message().c_str());
    }
    vmoutput.m_gasfee_detail = gasfee->gasfee_detail();
    output.m_vm_output = vmoutput;
    return ret;
}

enum_execute_result_type xatomictx_executor_t::vm_execute_before_process(const xcons_transaction_ptr_t & tx) {
    if (false == check_account_order(tx)) {
        return enum_exec_error_nonce_mismatch;
    }
    if (false == check_receiptid_order(tx)) {
        return enum_exec_error_receiptid_mismatch;
    }
    
    if (m_statectx->is_state_dirty()) {
        xerror("xatomictx_executor_t::execute fail-state dirty.tx=%s", tx->dump().c_str());
        return enum_exec_error_state_dirty;
    }
    return enum_exec_success;
}

void xatomictx_executor_t::vm_execute_after_process(const data::xunitstate_ptr_t & tx_unitstate,
                                                    const xcons_transaction_ptr_t & tx,
                                                    enum_execute_result_type vm_result,
                                                    xatomictx_output_t & output,
                                                    uint64_t gas_used) {
    // do state rollback and check state dirty
    bool is_state_dirty = false;
    if (enum_exec_success != vm_result) {
        m_statectx->do_rollback();
        update_gasfee(output.m_vm_output.m_gasfee_detail, tx_unitstate, tx);
        is_state_dirty = m_statectx->is_state_dirty();
        tx->set_current_exec_status(data::enum_xunit_tx_exec_status_fail);
        set_tx_account_state(tx_unitstate, tx);
    } else {
        update_gasfee(output.m_vm_output.m_gasfee_detail, tx_unitstate, tx);
        is_state_dirty = m_statectx->is_state_dirty();
        tx->set_current_exec_status(data::enum_xunit_tx_exec_status_success);
    }

    // estimate_gas_error should always not be packed
    if (vm_result == enum_exec_error_estimate_gas) {
        m_statectx->do_rollback();
        output.m_is_state_dirty = false;
        output.m_is_pack = false;
        output.m_snapshot_size = 0;
        return;
    }

    // check if pack tx
    bool is_pack_tx = false;
    if (vm_result == enum_exec_error_out_of_gas) {  // out_of_gas should always be packed
        xdbg("xatomictx_executor_t::vm_execute_after_process vm_result out_of_gas, set is_pack_tx true");
        is_pack_tx = true;
    }
    if (is_state_dirty) { // state dirty tx should always be packed
        xdbg("xatomictx_executor_t::vm_execute_after_process state dirty, set is_pack_tx true");
        is_pack_tx = true;
    } else {
        if (tx->is_recv_or_confirm_tx()) {  // recv or confirm tx should always be packed
            xdbg("xatomictx_executor_t::vm_execute_after_process state clear but recv or confirm, set is_pack_tx true");
            is_pack_tx = true;
        }
    }

    if (is_pack_tx) {  // tx packed should update tx related state
        set_evm_receipt_info(tx, output.m_vm_output, gas_used);
        bool tx_related_update = update_tx_related_state(tx_unitstate, tx, output.m_vm_output);
        if (false == tx_related_update) {
            xassert(false);
            is_pack_tx = false;
            xwarn("xatomictx_executor_t::vm_execute_after_process update_tx_related_state failed");
        }
    }

    size_t _snapshot_size = 0;
    if (is_pack_tx) {
        _snapshot_size = m_statectx->do_snapshot();
        xassert(!m_statectx->is_state_dirty());
    } else {
        m_statectx->do_rollback();
        is_state_dirty = false;
    }

    output.m_is_state_dirty = is_state_dirty;
    output.m_is_pack = is_pack_tx;
    output.m_snapshot_size = _snapshot_size;
}

enum_execute_result_type xatomictx_executor_t::execute(const xcons_transaction_ptr_t & tx, xatomictx_output_t & output, uint64_t gas_used) {
    tx->clear_modified_state(); // TODO(jimmy)

    std::string address = tx->get_account_addr();
    base::xvaccount_t vaddr(address);
    data::xunitstate_ptr_t tx_unitstate = m_statectx->load_unit_state(vaddr);
    if (nullptr == tx_unitstate) {
        xwarn("xatomictx_executor_t::execute fail-load unit state.tx=%s", tx->dump().c_str());
        return enum_exec_error_load_state;
    }

    enum_execute_result_type result = vm_execute_before_process(tx);
    if (enum_exec_success != result) {
        return result;
    }

    result = vm_execute(tx, output);
    vm_execute_after_process(tx_unitstate, tx, result, output, gas_used);
    return result;
}

NS_END2
