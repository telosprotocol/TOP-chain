// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xproperty_map.h"
#include "xcontract_common/xproperties/xproperty_string.h"
#include "xsystem_contract_runtime/xsystem_contract_runtime_helper.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xsystem_contracts/xbasic_system_contract.h"

NS_BEG3(top, tests, system_contracts)

class xdemo_contract_b final : public top::system_contracts::xbasic_system_contract_t {
    using xbase_t = top::system_contracts::xbasic_system_contract_t;
public:
    xdemo_contract_b() = default;
    xdemo_contract_b(xdemo_contract_b const &) = delete;
    xdemo_contract_b & operator=(xdemo_contract_b const &) = delete;
    xdemo_contract_b(xdemo_contract_b &&) = default;
    xdemo_contract_b & operator=(xdemo_contract_b &&) = default;
    ~xdemo_contract_b() override = default;

    BEGIN_CONTRACT_API()
        DECLARE_API(xdemo_contract_b::setup);
        DECLARE_API(xdemo_contract_b::test_set_string_property);
        DECLARE_API(xdemo_contract_b::test_set_map_property);
    END_CONTRACT_API

    void setup() {
    }

    void test_set_string_property(std::string const & string) {
        m_string_prop.set(string);
    }

    void test_set_map_property(std::map<std::string, std::string> const & map) {
        for (auto const & item : map) {
            m_map_prop.set(item.first, item.second);
        }
    }

private:
    contract_common::properties::xstring_property_t m_string_prop{data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, this};

    contract_common::properties::xmap_property_t<std::string, std::string> m_map_prop{data::system_contract::XPORPERTY_CONTRACT_REG_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map2_prop{data::system_contract::XPORPERTY_CONTRACT_TICKETS_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map3_prop{data::system_contract::XPORPERTY_CONTRACT_REFUND_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map4_prop{data::system_contract::XPROPERTY_CONTRACT_SLASH_INFO_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map5_prop{data::system_contract::XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY, this};
};
using xcontract_b_t = xdemo_contract_b;


NS_END3
