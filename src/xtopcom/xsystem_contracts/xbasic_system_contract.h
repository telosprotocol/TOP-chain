#pragma once

#include "xbasic/xmemory.hpp"
#include "xcontract_common/xbasic_contract.h"

NS_BEG2(top, system_contracts)

class xtop_basic_system_contract : public contract_common::xbasic_contract_t {
public:
    xtop_basic_system_contract(xtop_basic_system_contract const &) = delete;
    xtop_basic_system_contract & operator=(xtop_basic_system_contract const &) = delete;
    xtop_basic_system_contract(xtop_basic_system_contract &&) = default;
    xtop_basic_system_contract & operator=(xtop_basic_system_contract &&) = default;
    ~xtop_basic_system_contract() override = default;

protected:
    xtop_basic_system_contract() = default;
};
using xbasic_system_contract_t = xtop_basic_system_contract;

NS_END2
