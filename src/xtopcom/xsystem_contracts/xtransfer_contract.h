#pragma once

#include "xcontract_common/xcontract_execution_result.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xsystem_contracts/xbasic_system_contract.h"
#include "xcontract_common/xproperties/xproperty_token.h"
#include "xsystem_contract_runtime/xsystem_contract_runtime_helper.h"

NS_BEG2(top, system_contracts)

class xtop_transfer_contract : public xbasic_system_contract_t {
public:
    xtop_transfer_contract() = default;
    xtop_transfer_contract(xtop_transfer_contract const &) = delete;
    xtop_transfer_contract & operator=(xtop_transfer_contract const &) = delete;
    xtop_transfer_contract(xtop_transfer_contract &&) = default;
    xtop_transfer_contract & operator=(xtop_transfer_contract &&) = default;
    ~xtop_transfer_contract() override = default;

    BEGIN_CONTRACT_API()
    DECLARE_API(xtop_transfer_contract::setup);
    DECLARE_API(xtop_transfer_contract::transfer);
    DECLARE_API(xtop_transfer_contract::follow_up_delay);
    END_CONTRACT_API

private:
    void setup();
    void transfer(uint64_t const amount);
    void follow_up_delay();
};
using xtransfer_contract_t = xtop_transfer_contract;

NS_END2
