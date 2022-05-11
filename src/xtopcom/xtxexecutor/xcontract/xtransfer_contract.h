#pragma once

#include "xcontract_common/xbasic_stateless_contract.h"
#include "xcontract_common/xcontract_execution_context.h"
#include "xcontract_common/xcontract_execution_result.h"
#include "xtxexecutor/xcontract/xcontract_runtime_helper.h"

NS_BEG3(top, txexecutor, contract)

class xtop_transfer_contract : public contract_common::xbasic_stateless_contract_t {
public:
    xtop_transfer_contract() = default;
    xtop_transfer_contract(xtop_transfer_contract const &) = delete;
    xtop_transfer_contract & operator=(xtop_transfer_contract const &) = delete;
    xtop_transfer_contract(xtop_transfer_contract &&) = default;
    xtop_transfer_contract & operator=(xtop_transfer_contract &&) = default;
    ~xtop_transfer_contract() override = default;

    BEGIN_CONTRACT_API()
    DECLARE_API(xtop_transfer_contract::transfer);
    DECLARE_API(xtop_transfer_contract::deposit);
    DECLARE_API(xtop_transfer_contract::withdraw);
    END_CONTRACT_API

private:
    void transfer(const std::string & token_name, const std::string & amount_256_str);
    void deposit(const std::string & token_name, const std::string & amount_256_str);
    void withdraw(const std::string & token_name, const std::string & amount_256_str);
};
using xtransfer_contract_t = xtop_transfer_contract;

NS_END3
