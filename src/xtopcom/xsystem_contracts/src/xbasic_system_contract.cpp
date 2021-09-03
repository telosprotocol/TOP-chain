#include "xsystem_contracts/xbasic_system_contract.h"

NS_BEG2(top, system_contracts)

xtop_basic_system_contract::xtop_basic_system_contract(observer_ptr<contract_common::xcontract_execution_context_t> const & exec_context)
  : contract_common::xbasic_contract_t(exec_context) {
}

NS_END2
