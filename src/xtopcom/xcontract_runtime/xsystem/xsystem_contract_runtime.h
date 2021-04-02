#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_runtime/xvm/xbasic_runtime.h"
#include "xcontract_runtime/xsystem_contract_manager.h"

NS_BEG3(top, contract_runtime, system)

class xtop_system_contract_runtime : public vm::xbasic_runtime_t {
    observer_ptr<xsystem_contract_manager_t> m_system_contract_mgr;

public:
    xtop_system_contract_runtime(xtop_system_contract_runtime const &) = delete;
    xtop_system_contract_runtime & operator=(xtop_system_contract_runtime const &) = delete;
    xtop_system_contract_runtime(xtop_system_contract_runtime &&) = default;
    xtop_system_contract_runtime & operator=(xtop_system_contract_runtime &&) = default;
    ~xtop_system_contract_runtime() override = default;

    explicit xtop_system_contract_runtime(observer_ptr<xsystem_contract_manager_t> sys_contract_mgr);

    xtransaction_execution_result_t execute_transaction(observer_ptr<contract_common::xcontract_execution_context_t> execution_context) override;
};
using xsystem_contract_runtime_t = xtop_system_contract_runtime;

NS_END3
