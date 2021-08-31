#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xbasic_contract.h"

NS_BEG2(top, system_contracts)

class xtop_basic_system_contract : public contract_common::xbasic_contract_t {
public:
    xtop_basic_system_contract(observer_ptr<contract_common::xcontract_execution_context_t> const& exec_context):
                                contract_common::xbasic_contract_t(exec_context){}
};
using xbasic_system_contract_t = xtop_basic_system_contract;

NS_END2
