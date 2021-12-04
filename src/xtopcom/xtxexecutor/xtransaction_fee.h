// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once
#include <unistd.h>
#include "xstore/xaccount_context.h"
#include "xdata/xtransaction.h"
#include "xdata/xcons_transaction.h"
#include "xconfig/xconfig_register.h"
#include "xvm/xvm_trace.h"
#include "xdata/xnative_contract_address.h"

NS_BEG2(top, txexecutor)

using top::store::xaccount_context_t;


class xtransaction_fee_t{
public:
    xtransaction_fee_t(xaccount_context_t* account_ctx, const xcons_transaction_ptr_t & trans)
    : m_account_ctx(account_ctx), m_trans(trans), m_used_deposit(0), m_service_fee(0) {
    }

    int32_t update_tgas_disk_sender(const uint64_t amount, bool is_contract);
    int32_t update_tgas_sender(uint64_t& used_deposit, bool is_contract);
    int32_t update_tgas_sender();
    int32_t update_disk(bool is_contract);
    bool    need_use_tgas_disk(const std::string &source_addr, const std::string &target_addr, const std::string &func_name);

    // update tgas, disk of contract & caller after execution
    int32_t update_tgas_disk_after_sc_exec(xvm::xtransaction_trace_ptr trace);
    int32_t update_tgas_disk_contract_recv(uint64_t& used_deposit, uint64_t& frozen_tgas, uint64_t deal_used_tgas);
    int32_t update_tgas_contract_recv(uint64_t& used_deposit, uint64_t& frozen_tgas, uint64_t deal_used_tgas);

    uint32_t get_tgas_usage(bool is_contract) {
        #ifdef ENABLE_SCALE
            uint16_t amplify = 5;
        #else
            uint16_t amplify = 1;
        #endif
        if(is_contract){
            amplify = 1;
        }
        uint32_t multiple = (m_trans->is_self_tx()) ? 1 : 3;
        return multiple * amplify * m_trans->get_transaction()->get_tx_len();
    }

    uint32_t get_disk_usage(bool is_contract) {
        uint64_t tx_size = m_trans->get_transaction()->get_tx_len();
        #ifdef ENABLE_SCALE
            uint16_t amplify = 100;
        #else
            uint16_t amplify = 1;
        #endif
#if 1
        return 0;
#else
        uint32_t multiple = 2;
        if (m_trans->get_transaction()->get_tx_type() == data::enum_xtransaction_type::xtransaction_type_transfer && m_trans->get_target_addr() == black_hole_addr) {
            multiple = 1;
        }
        return is_contract ? tx_size : multiple * amplify * tx_size;
#endif
    }

    uint64_t get_deposit_usage(){
        return m_used_deposit;
    }

    void update_service_fee();
    uint64_t get_service_fee();
    static uint64_t cal_service_fee(const std::string& source, const std::string& target);
    void update_fee_recv();
    int32_t update_fee_recv_self();
    int32_t update_fee_confirm();
    int32_t update_contract_fee_confirm(uint64_t amount);

private:
    xaccount_context_t*     m_account_ctx;
    xcons_transaction_ptr_t m_trans;
    uint64_t                m_used_deposit;
    uint64_t                m_service_fee;
};

NS_END2
