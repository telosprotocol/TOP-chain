#include "xcontract_runtime/xsystem/xsystem_contract_runtime.h"

#include "xcontract_runtime/xerror/xerror.h"

#include <cassert>

NS_BEG3(top, contract_runtime, system)

xtop_system_contract_runtime::xtop_system_contract_runtime(observer_ptr<xsystem_contract_manager_t> sys_contract_mgr) : m_system_contract_mgr{std::move(sys_contract_mgr)} {
}

xtransaction_execution_result_t xtop_system_contract_runtime::execute_transaction(observer_ptr<contract_common::xcontract_execution_context_t> execution_context) {
    // assert(tx->get_tx_type() == data::enum_xtransaction_type::xtransaction_type_run_contract2);
    xtransaction_execution_result_t result;
    auto sys_contract = m_system_contract_mgr->system_contract(execution_context->contract_address());
    if (sys_contract == nullptr) {
        result.status.ec = error::xerrc_t::contract_not_found;
        return result;
    }

    result = sys_contract->execute(execution_context);
    return result;
}

NS_END3
