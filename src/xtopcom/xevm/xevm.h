// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xns_macro.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xblock.h"
#include "xdata/xtop_action_fwd.h"
#include "xevm/xevm_execution_result.h"
#include "xevm_contract_runtime/xevm_action_runtime.h"
#include "xstatectx/xstatectx_face.h"
#include "xtxexecutor/xvm_face.h"

NS_BEG2(top, evm)

class xtop_evm : public txexecutor::xvm_face_t {
private:
    statectx::xstatectx_face_ptr_t m_evm_statectx;
    std::unique_ptr<contract_runtime::evm::xevm_action_runtime_t> evm_action_runtime_;

public:
    // xtop_account_vm() = default;
    xtop_evm(xtop_evm const &) = delete;
    xtop_evm & operator=(xtop_evm const &) = delete;
    xtop_evm(xtop_evm &&) = default;
    xtop_evm & operator=(xtop_evm &&) = default;
    ~xtop_evm() = default;

    xtop_evm(observer_ptr<contract_runtime::evm::xevm_contract_manager_t> const evm_contract_manager, statectx::xstatectx_face_ptr_t const evm_statectx);

public:
    txexecutor::enum_execute_result_type execute(txexecutor::xvm_input_t const & input, txexecutor::xvm_output_t & output) override;

private:
    xevm_output_t execute(std::vector<data::xcons_transaction_ptr_t> const & txs);
    contract_runtime::xtransaction_execution_result_t execute_action(std::unique_ptr<data::xbasic_top_action_t const> action);
};

NS_END2