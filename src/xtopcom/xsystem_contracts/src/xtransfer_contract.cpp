#include "xsystem_contracts/xtransfer_contract.h"

NS_BEG2(top, system_contracts)

xtop_transfer_contract::xtop_transfer_contract(observer_ptr<contract_common::xcontract_execution_context_t> const& exec_context):
                                                xtop_basic_system_contract(exec_context) {
}

void xtop_transfer_contract::transfer(uint64_t const amount) {
    if (at_source_action_stage()) {
        m_balance.withdraw(amount);
    }

    if (at_confirm_action_stage()) {

    }

    if (at_target_action_stage()) {
        m_balance.deposit(amount);
    }
}

NS_END2
