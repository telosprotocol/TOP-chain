// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <string>
#include <vector>

#include "xtxexecutor/xvm_face.h"

NS_BEG2(top, txexecutor)

class xatomictx_output_t {
 public:
    std::string     dump() const;

    bool                        m_is_pack{false};
    bool                        m_is_state_dirty{false};
    size_t                      m_snapshot_size{0};
    xvm_output_t                m_vm_output;
    enum_execute_result_type    m_result{enum_exec_error_invalid};
    xcons_transaction_ptr_t     m_tx;
};

class xatomictx_executor_t {
 public:
    xatomictx_executor_t(const statectx::xstatectx_face_ptr_t & statectx, const xvm_para_t & para);

 public:
    enum_execute_result_type execute(const xcons_transaction_ptr_t & tx, xatomictx_output_t & output, uint64_t gas_used);

 private:
    enum_execute_result_type vm_execute(const xcons_transaction_ptr_t & tx, xatomictx_output_t & output); 
    enum_execute_result_type vm_execute_before_process(const xcons_transaction_ptr_t & tx);
    bool    set_tx_account_state(const data::xunitstate_ptr_t & unitstate, const xcons_transaction_ptr_t & tx);
    bool    set_tx_table_state(const data::xtablestate_ptr_t & tablestate, const xcons_transaction_ptr_t & tx);
    bool    update_tx_related_state(const data::xunitstate_ptr_t & tx_unitstate, const xcons_transaction_ptr_t & tx, const xvm_output_t & vmoutput);
    void    vm_execute_after_process(const data::xunitstate_ptr_t & tx_unitstate,
                                    const xcons_transaction_ptr_t & tx,
                                    enum_execute_result_type vm_result,
                                    xatomictx_output_t & output,
                                    uint64_t gas_used);
    bool    check_account_order(const xcons_transaction_ptr_t & tx);
    bool    check_receiptid_order(const xcons_transaction_ptr_t & tx);
    bool    update_nonce_and_hash(const data::xunitstate_ptr_t & unitstate, const xcons_transaction_ptr_t & tx);
    bool    update_gasfee(const xvm_gasfee_detail_t detail, const data::xunitstate_ptr_t & unitstate, const xcons_transaction_ptr_t & tx);
 private:
    statectx::xstatectx_face_ptr_t  m_statectx{nullptr};
    xvm_para_t                      m_para;
};

NS_END2
