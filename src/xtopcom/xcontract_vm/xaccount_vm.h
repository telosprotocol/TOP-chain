// Copyright (c) 2018-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_runtime/xuser/xuser_action_runtime.h"
#include "xcontract_vm/xaccount_vm_execution_result.h"
#include "xcontract_vm/xvm_executor_face.h"
#include "xdata/xcons_transaction.h"
#include "xsystem_contract_runtime/xsystem_action_runtime.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"

#if defined(__clang__)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wpedantic"
#elif defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wpedantic"
#elif defined(_MSC_VER)
#    pragma warning(push, 0)
#endif

#include "xvledger/xvstate.h"

#if defined(__clang__)
#    pragma clang diagnostic pop
#elif defined(__GNUC__)
#    pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#    pragma warning(pop)
#endif

NS_BEG2(top, contract_vm)

class xtop_account_vm : public xvm_executor_face_t {
private:
    std::unique_ptr<contract_runtime::user::xuser_action_runtime_t> user_action_runtime_{top::make_unique<contract_runtime::user::xuser_action_runtime_t>()};
    std::unique_ptr<contract_runtime::system::xsystem_action_runtime_t> sys_action_runtime_;

public:
    // xtop_account_vm() = default;
    xtop_account_vm(xtop_account_vm const &) = delete;
    xtop_account_vm & operator=(xtop_account_vm const &) = delete;
    xtop_account_vm(xtop_account_vm &&) = default;
    xtop_account_vm & operator=(xtop_account_vm &&) = default;
    ~xtop_account_vm() override = default;

    explicit xtop_account_vm(observer_ptr<contract_runtime::system::xsystem_contract_manager_t> const & system_contract_manager);

    xaccount_vm_execution_result_t execute(std::vector<data::xcons_transaction_ptr_t> const & txs, xobject_ptr_t<base::xvbstate_t> block_state) override;
};
using xaccount_vm_t = xtop_account_vm;

NS_END2
