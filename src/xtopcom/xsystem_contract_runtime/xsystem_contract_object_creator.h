// Copyright (c) 2017-present Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xsystem_contracts/xbasic_system_contract.h"

NS_BEG3(top, contract_runtime, system)

class xtop_contract_object_creator {
public:
    xtop_contract_object_creator() = default;
    xtop_contract_object_creator(xtop_contract_object_creator const &) = delete;
    xtop_contract_object_creator & operator=(xtop_contract_object_creator const &) = delete;
    xtop_contract_object_creator(xtop_contract_object_creator &&) = default;
    xtop_contract_object_creator & operator=(xtop_contract_object_creator &&) = delete;
    virtual ~xtop_contract_object_creator() = default;

    virtual std::unique_ptr<system_contracts::xbasic_system_contract_t> create() = 0;
};
using xcontract_object_creator_t = xtop_contract_object_creator;

template <typename SystemContractT>
class xtop_system_contract_object_creator : public xcontract_object_creator_t {
public:
    std::unique_ptr<system_contracts::xbasic_system_contract_t> create() override {
        auto * system_contract_obj = new SystemContractT{};
        return std::unique_ptr<SystemContractT>{system_contract_obj};
    }
};

template <typename SystemContractT>
using xsystem_contract_object_creator_t = xtop_system_contract_object_creator<SystemContractT>;

NS_END3
