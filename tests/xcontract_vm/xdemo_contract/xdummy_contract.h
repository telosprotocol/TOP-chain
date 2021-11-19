// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xsystem_contract_runtime/xsystem_contract_runtime_helper.h"
#include "xsystem_contracts/xbasic_system_contract.h"

NS_BEG3(top, tests, system_contracts)

class xtop_dummy_contract final : public top::system_contracts::xbasic_system_contract_t {
    using xbase_t = top::system_contracts::xbasic_system_contract_t;

public:
    xtop_dummy_contract() = default;
    xtop_dummy_contract(xtop_dummy_contract const &) = delete;
    xtop_dummy_contract & operator=(xtop_dummy_contract const &) = delete;
    xtop_dummy_contract(xtop_dummy_contract &&) = default;
    xtop_dummy_contract & operator=(xtop_dummy_contract &&) = default;
    ~xtop_dummy_contract() override = default;

    BEGIN_CONTRACT_API()
    DECLARE_API(xtop_dummy_contract::setup);
    DECLARE_API(xtop_dummy_contract::do_nothing);
    END_CONTRACT_API

    void setup() {
    }

    void do_nothing() {
    }
};
using xdummy_contract_t = xtop_dummy_contract;

NS_END3
