// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xstateless_contract_execution_context.h"
#include "xcontract_runtime/xtransaction_execution_result.h"
#include "xtxexecutor/xvm_face.h"

#include <string>
#include <vector>

NS_BEG2(top, txexecutor)

class xtvm_v2_t : public xvm_face_t {
public:
    enum_execute_result_type execute(const xvm_input_t & input, xvm_output_t & output) override;

private:
    enum {
        default_eth_tx_gas = 21000,
    };
    bool execute_impl(const xvm_input_t & input, xvm_output_t & output);
    contract_runtime::xtransaction_execution_result_t execute_tx(const statectx::xstatectx_face_ptr_t & statectx, const xcons_transaction_ptr_t & tx);
    contract_runtime::xtransaction_execution_result_t execute_one_tx(const statectx::xstatectx_face_ptr_t & statectx, const xcons_transaction_ptr_t & tx);

    void fill_transfer_context(const statectx::xstatectx_face_ptr_t & statectx,
                               const xcons_transaction_ptr_t & tx,
                               contract_common::xstateless_contract_execution_context_t & ctx,
                               std::error_code & ec);
};

NS_END2
