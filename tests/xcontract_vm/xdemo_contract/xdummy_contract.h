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
    DECLARE_SEND_ONLY_API(xtop_dummy_contract::send_only);
    END_CONTRACT_API

    void setup() {
        m_string_prop.set("9999999");
    }

    void do_nothing() {
        static int i{0};
        m_string_prop.set(std::to_string(i));
    }

    void send_only() {
        deposit(common::xtoken_t{500});
    }

private:
    contract_common::properties::xstring_property_t m_string_prop{"$999", this};
};
using xdummy_contract_t = xtop_dummy_contract;

NS_END3
