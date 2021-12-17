// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xtxexecutor/xtransaction_fee.h"
#include "xdata/xgenesis_data.h"
#include "xtxexecutor/xunit_service_error.h"
#include <algorithm>

using namespace top::data;

NS_BEG2(top, txexecutor)

int32_t xtransaction_fee_t::update_tgas_disk_sender(const uint64_t amount, bool is_contract) {
    xdbg("tgas_disk deposit: %d, balance: %d, amount: %d, is_contract: %d",
          m_trans->get_transaction()->get_deposit(), m_account_ctx->get_blockchain()->balance(), amount, is_contract);

    if (m_account_ctx->get_blockchain()->balance() < amount) {
        return xconsensus_service_error_balance_not_enough;
    }

    // deposit more than balance
    if (m_trans->get_transaction()->get_deposit() > (m_account_ctx->get_blockchain()->balance() - amount)) {
        return xtransaction_too_much_deposit;
    }

    if (!is_sys_contract_address(common::xaccount_address_t{ m_trans->get_source_addr() })) {
        if (m_trans->get_transaction()->get_deposit() < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)) {
            xdbg("not_enough_deposit, source addr: %s, target addr: %s, deposit: %d",
                m_trans->get_source_addr().c_str(), m_trans->get_target_addr().c_str(), m_trans->get_transaction()->get_deposit());
            return xtransaction_not_enough_deposit;
        }
    }
    uint64_t tgas_used_deposit{0};
    auto ret = update_tgas_sender(tgas_used_deposit, is_contract);
    if (ret != 0) {
        return ret;
    }
    uint64_t frozen_tgas = std::min((m_trans->get_transaction()->get_deposit() - tgas_used_deposit) / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio),
                                     m_account_ctx->get_available_tgas() - m_account_ctx->get_blockchain()->lock_tgas());
    xdbg("tgas_disk sender frozen_tgas: %llu, available_tgas: %llu, lock_tgas: %llu",
          frozen_tgas, m_account_ctx->get_available_tgas(), m_account_ctx->get_blockchain()->lock_tgas());

    ret = update_disk(is_contract);
    m_trans->set_current_used_disk(get_disk_usage(is_contract));
    xdbg("tgas_disk m_used_disk %d, %s, %s", m_trans->get_current_used_disk(), m_trans->get_target_addr().c_str(), m_trans->get_digest_hex_str().c_str());
    if(ret != 0){
        return ret;
    }
    m_used_deposit = tgas_used_deposit;
    xdbg("tgas_disk sender, ret: %d, used_deposit: %d, amount: %d", ret, m_used_deposit, amount);

    if(is_contract && frozen_tgas > 0){
        m_trans->set_current_send_tx_lock_tgas(frozen_tgas);
        ret = m_account_ctx->uint64_add(XPROPERTY_LOCK_TGAS, frozen_tgas);
        if (ret != xsuccess) {
            return ret;
        }
        xdbg("tgas_disk tx hash: %s, frozen_tgas: %u", m_trans->get_digest_hex_str().c_str(), frozen_tgas);
    }

    if (m_trans->get_transaction()->get_deposit() > 0) {
        m_account_ctx->available_balance_to_other_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(m_trans->get_transaction()->get_deposit()));
    }
    xdbg("tgas_disk tx hash: %s, deposit: %u", m_trans->get_digest_hex_str().c_str(), m_trans->get_transaction()->get_deposit());

    return ret;
}

int32_t xtransaction_fee_t::update_tgas_sender(uint64_t& used_deposit, bool is_contract){
    uint64_t tgas_usage = get_tgas_usage(is_contract);
    auto ret = m_account_ctx->update_tgas_sender(tgas_usage, m_trans->get_transaction()->get_deposit(), used_deposit, is_contract);
    m_trans->set_current_used_tgas(tgas_usage - used_deposit / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio));
    m_trans->set_current_used_deposit(used_deposit);
    xdbg("tgas_disk m_used_tgas %d, %d, %s, %s", m_trans->get_current_used_tgas(), used_deposit, m_trans->get_target_addr().c_str(), m_trans->get_digest_hex_str().c_str());
    return ret;
}

int32_t xtransaction_fee_t::update_tgas_sender() {
    uint64_t tgas_usage = get_tgas_usage(false);
    uint64_t used_deposit = 0;
    auto ret = m_account_ctx->update_tgas_sender(tgas_usage, m_trans->get_transaction()->get_deposit(), used_deposit);
    uint64_t used_tgas = tgas_usage - used_deposit / XGET_ONCHAIN_GOVERNANCE_PARAMETER(tx_deposit_gas_exchange_ratio);
    m_trans->set_current_used_tgas(used_tgas);
    if (ret != 0) {
        m_account_ctx->incr_used_tgas(used_tgas);
    }
    return ret;
}

bool xtransaction_fee_t::need_use_tgas_disk(const std::string &source_addr, const std::string &target_addr, const std::string &func_name){
    xdbg("need_use_tgas_disk:%s, %s", target_addr.c_str(), func_name.c_str());
    return !is_sys_contract_address(common::xaccount_address_t{ source_addr });
}

int32_t xtransaction_fee_t::update_disk(bool is_contract){
    uint64_t disk_usage = get_disk_usage(is_contract);
    auto ret = m_account_ctx->update_disk(disk_usage);
    return ret;
}

int32_t xtransaction_fee_t::update_tgas_disk_after_sc_exec(xvm::xtransaction_trace_ptr trace){
    uint32_t instruction_usage = trace->m_instruction_usage + trace->m_tgas_usage;
    if(trace->m_errno == xvm::enum_xvm_error_code::ok && 0 == instruction_usage){
        // system contract consumes no resource, return 0 directly
        return 0;
    }
    uint64_t used_deposit{0};
    uint64_t frozen_tgas = m_trans->get_last_action_send_tx_lock_tgas();

    // set contract exec duration to be instruction_usage * 40 temporarily
    uint32_t time_instruction_ratio = 40;
    auto exec_duration = instruction_usage * time_instruction_ratio;
    auto cpu_gas_exchange_ratio = XGET_ONCHAIN_GOVERNANCE_PARAMETER(cpu_gas_exchange_ratio);
    auto deal_used_tgas = exec_duration / cpu_gas_exchange_ratio;
    xdbg("tgas_disk contract exec before update frozen_tgas: %d, used_deposit: %d, instruction: %d, cpu_gas_exchange_ratio: %d, deal_used_tgas: %d",
          frozen_tgas, used_deposit, instruction_usage, cpu_gas_exchange_ratio, deal_used_tgas);
    // frozen_tgas after consuming
    auto ret = update_tgas_disk_contract_recv(used_deposit, frozen_tgas, deal_used_tgas);
    xdbg("tgas_disk contract exec after update ret: %d, unused frozen_tgas: %d, used_deposit: %d", ret, frozen_tgas, used_deposit);
    if(ret != 0){
        return ret;
    }

    uint32_t send_tx_lock_tgas = m_trans->get_last_action_send_tx_lock_tgas();
    m_trans->set_current_send_tx_lock_tgas(send_tx_lock_tgas);
    m_trans->set_current_recv_tx_use_send_tx_tgas(send_tx_lock_tgas - frozen_tgas);
    xdbg("tgas_disk tx hash: %s, lock_tgas: %u, addr: %s, ret: %d, frozen_tgas: %d, used_deposit: %d",
          m_trans->get_digest_hex_str().c_str(), send_tx_lock_tgas, m_trans->get_source_addr().c_str(), ret, frozen_tgas, used_deposit);
    return ret;
}

int32_t xtransaction_fee_t::update_tgas_disk_contract_recv(uint64_t& used_deposit, uint64_t& frozen_tgas, uint64_t deal_used_tgas){
    // minimum tx deposit
    if(m_trans->get_transaction()->get_deposit() < XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit)){
        return xtransaction_not_enough_deposit;
    }
    auto ret = update_tgas_contract_recv(used_deposit, frozen_tgas, deal_used_tgas);
    if(ret != 0){
        xdbg("tgas_disk update_tgas_contract_recv(used_deposit: %d, frozen_tgas: %d), ret: %d", used_deposit, frozen_tgas, ret);
        return ret;
    }

    ret = update_disk(true);
    uint32_t used_disk = get_disk_usage(true);
    m_trans->set_current_used_disk(used_disk);
    xdbg("tgas_disk m_used_disk %d, %s, %s", used_disk, m_trans->get_target_addr().c_str(), m_trans->get_digest_hex_str().c_str());
    if(ret != 0){
        xdbg("tgas_disk update_disk(used_deposit: %d, frozen_tgas: %d), ret: %d", used_deposit, frozen_tgas, ret);
        return ret;
    }

    return ret;
}

int32_t xtransaction_fee_t::update_tgas_contract_recv(uint64_t& used_deposit, uint64_t& frozen_tgas, uint64_t deal_used_tgas){
    // for contract as receiver, do not count tx size as
    uint64_t tgas_usage = 0;
    uint64_t sender_used_deposit = m_trans->get_last_action_used_deposit();
    auto ret = m_account_ctx->update_tgas_contract_recv(sender_used_deposit, m_trans->get_transaction()->get_deposit(), used_deposit, frozen_tgas, deal_used_tgas);
    m_trans->set_current_used_tgas(m_account_ctx->m_cur_used_tgas);
    m_trans->set_current_used_deposit(sender_used_deposit + used_deposit);
    xdbg("tgas_disk m_used_tgas %d, used_deposit: %d, %s, %s",
          m_trans->get_current_used_tgas(), m_trans->get_current_used_deposit(), m_trans->get_target_addr().c_str(), m_trans->get_digest_hex_str().c_str());
    return ret;
}

uint64_t xtransaction_fee_t::get_service_fee(){
    return m_service_fee;
}

void xtransaction_fee_t::update_service_fee(){
    m_service_fee = cal_service_fee(m_trans->get_source_addr(), m_trans->get_target_addr());
}

uint64_t xtransaction_fee_t::cal_service_fee(const std::string& source, const std::string& target) {
    uint64_t beacon_tx_fee{0};
#ifndef XENABLE_MOCK_ZEC_STAKE
    if (!is_sys_contract_address(common::xaccount_address_t{ source })
     && is_beacon_contract_address(common::xaccount_address_t{ target })){
        beacon_tx_fee = XGET_ONCHAIN_GOVERNANCE_PARAMETER(beacon_tx_fee);
    }
#endif
    return beacon_tx_fee;
}

void xtransaction_fee_t::update_fee_recv() {
    m_trans->set_current_used_deposit(m_trans->get_last_action_used_deposit());
    xdbg("xtransaction_fee_t::update_fee_recv used deposit:%u", m_trans->get_last_action_used_deposit());
}

int32_t xtransaction_fee_t::update_fee_recv_self() {
    int32_t ret = xsuccess;
    // only v2 token burn tx meets the following requirments
    if ((m_trans->get_tx_type() == xtransaction_type_transfer)
     && (m_trans->get_transaction()->get_tx_version() == xtransaction_version_2)) {
        xassert(m_trans->get_target_addr() == black_hole_addr);
        return ret;
    }
    if (m_trans->get_transaction()->get_deposit() > 0) {
        ret = m_account_ctx->other_balance_to_available_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(m_trans->get_transaction()->get_deposit()));
        if (xsuccess != ret) {
            return ret;
        }
    }
    if (get_deposit_usage() > 0) {
        ret = m_account_ctx->available_balance_to_other_balance(XPROPERTY_BALANCE_BURN, base::vtoken_t(get_deposit_usage()));
        if (xsuccess != ret) {
            return ret;
        }
    }
    return ret;
}

int32_t xtransaction_fee_t::update_fee_confirm() {
    if (m_trans->get_transaction()->get_deposit() > 0) {
        m_account_ctx->other_balance_to_available_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(m_trans->get_transaction()->get_deposit()));
    }
    uint32_t last_action_used_deposit = m_trans->get_last_action_used_deposit();
    m_trans->set_current_used_deposit(last_action_used_deposit);
    int32_t ret = xsuccess;
    if (last_action_used_deposit > 0) {
        ret = m_account_ctx->available_balance_to_other_balance(XPROPERTY_BALANCE_BURN, base::vtoken_t(last_action_used_deposit));
        if (ret != xsuccess) {
            xerror("xtransaction_fee_t::update_fee_confirm fail-top_token_transfer_out last_action_used_deposit=%d", last_action_used_deposit);
            return ret;
        }
    }
    xassert(ret == xsuccess);
    return ret;
}

int32_t xtransaction_fee_t::update_contract_fee_confirm(uint64_t amount) {
    uint32_t last_action_used_deposit = m_trans->get_last_action_used_deposit();
    auto status = m_trans->get_last_action_exec_status();
    int32_t ret;
    if (m_trans->get_last_action_send_tx_lock_tgas() > 0) {
        ret = m_account_ctx->uint64_sub(XPROPERTY_LOCK_TGAS, m_trans->get_last_action_send_tx_lock_tgas());
        if (ret != xsuccess) {
            xerror("xtransaction_fee_t::update_contract_fee_confirm fail-lock tgas sub. last_action_send_tx_lock_tgas=%d", m_trans->get_last_action_send_tx_lock_tgas());
            return ret;
        }
    }

    if (m_trans->get_transaction()->get_deposit() > 0) {
        m_account_ctx->other_balance_to_available_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(m_trans->get_transaction()->get_deposit()));
    }
    xdbg("tgas_disk tx hash: %s, deposit: %u, frozen_tgas: %u, status: %d",
          m_trans->get_digest_hex_str().c_str(), m_trans->get_transaction()->get_deposit(), m_trans->get_last_action_send_tx_lock_tgas(), status);

    uint64_t target_used_tgas = m_trans->get_last_action_recv_tx_use_send_tx_tgas();
    // auto total_tgas = m_fee.get_tgas_usage(true) + target_used_tgas;
    xdbg("tgas_disk tx hash: %s, recv_tx_use_send_tx_tgas: %llu, used_deposit: %u, lock_tgas: %u",
          m_trans->get_digest_hex_str().c_str(), target_used_tgas, last_action_used_deposit, m_trans->get_last_action_send_tx_lock_tgas());
    ret = m_account_ctx->calc_resource(target_used_tgas, m_trans->get_transaction()->get_deposit(), last_action_used_deposit);
    m_trans->set_current_used_tgas(target_used_tgas);
    m_trans->set_current_used_deposit(last_action_used_deposit);

    if (amount > 0) {
        // should unlock token by recv tx execute status
        if(status == enum_xunit_tx_exec_status_fail){
            // unlock token and revert to available balance if recv tx execute fail
            ret = m_account_ctx->other_balance_to_available_balance(XPROPERTY_BALANCE_LOCK, base::vtoken_t(amount));
            if (ret) {
                xerror("xtransaction_fee_t::update_contract_fee_confirm,fail-revert back token. amount=%ld", amount);
            }
        } else {
            // unlock token only
            ret = m_account_ctx->token_withdraw(XPROPERTY_BALANCE_LOCK, base::vtoken_t(amount));
            if (ret) {
                xerror("xtransaction_fee_t::update_contract_fee_confirm,fail-withdraw lock token. amount=%ld", amount);
            }
        }
    }
    return ret;
}

NS_END2
