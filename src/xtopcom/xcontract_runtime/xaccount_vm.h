#pragma once

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

#include "xbase/xobject_ptr.h"
#include "xbasic/xmemory.hpp"
#include "xcontract_runtime/xaccount_vm_execution_result.h"
#include "xcontract_runtime/xsystem/xsystem_contract_runtime.h"
#include "xcontract_runtime/xsystem_contract_manager.h"
#include "xcontract_runtime/xuser/xuser_contract_runtime.h"
#include "xcontract_runtime/xuser_action_runtime.h"
#include "xcontract_runtime/xvm_executor_face.h"
#include "xdata/xcons_transaction.h"

NS_BEG2(top, contract_runtime)

static auto _ = []() -> std::unique_ptr<xsystem_contract_manager_t> {
    std::unique_ptr<xsystem_contract_manager_t> system_contract_manager{top::make_unique<xsystem_contract_manager_t>()};
    system_contract_manager->deploy();
    return system_contract_manager;
};

static auto x = _();

class xtop_account_vm : public xvm_executor_face_t {
private:
    std::unique_ptr<user::xuser_contract_runtime_t> user_contract_runtime_{ top::make_unique<user::xuser_contract_runtime_t>() };
    std::unique_ptr<system::xsystem_contract_runtime_t> system_contract_runtime_{top::make_unique<system::xsystem_contract_runtime_t>(top::make_observer(x.get()))};

    std::unique_ptr<xuser_action_runtime_t> user_action_runtime_{ top::make_unique<xuser_action_runtime_t>() };

public:
    xtop_account_vm() = default;
    xtop_account_vm(xtop_account_vm const &) = delete;
    xtop_account_vm & operator=(xtop_account_vm const &) = delete;
    xtop_account_vm(xtop_account_vm &&) = default;
    xtop_account_vm & operator=(xtop_account_vm &&) = default;
    ~xtop_account_vm() override = default;

    xaccount_vm_execution_result_t execute(std::vector<data::xcons_transaction_ptr_t> const & txs, xobject_ptr_t<base::xvbstate_t> block_state) override;
    // xaccount_vm_execution_result_t execute(std::vector<data::xtop_action_t> const & actions, xobject_ptr_t<base::xvbstate_t> block_state) override;
};
using xaccount_vm_t = xtop_account_vm;

NS_END2
