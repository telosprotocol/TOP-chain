// Copyright (c) 2017-2021 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include "xcontract_common/xproperties/xproperty_map.h"
#include "xcontract_common/xproperties/xproperty_string.h"
#include "xdata/xsystem_contract/xdata_structures.h"
#include "xsystem_contract_runtime/xsystem_contract_runtime_helper.h"
#include "xsystem_contracts/xbasic_system_contract.h"

NS_BEG3(top, tests, system_contracts)

class xdemo_contract_a final : public top::system_contracts::xbasic_system_contract_t {
    using xbase_t = top::system_contracts::xbasic_system_contract_t;
public:
    xdemo_contract_a() = default;
    xdemo_contract_a(xdemo_contract_a const &) = delete;
    xdemo_contract_a & operator=(xdemo_contract_a const &) = delete;
    xdemo_contract_a(xdemo_contract_a &&) = default;
    xdemo_contract_a & operator=(xdemo_contract_a &&) = default;
    ~xdemo_contract_a() override = default;

    BEGIN_CONTRACT_API()
        DECLARE_API(xdemo_contract_a::setup);
        DECLARE_API(xdemo_contract_a::test_set_string_property);
        DECLARE_API(xdemo_contract_a::test_set_map_property);
        DECLARE_API(xdemo_contract_a::test_sync_call);
        DECLARE_API(xdemo_contract_a::test_async_call);
        DECLARE_API(xdemo_contract_a::test_followup_transfer_to_user);
        DECLARE_SEND_ONLY_API(xdemo_contract_a::send_only_api);
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

    void test_sync_call(common::xaccount_address_t const & target_addr, std::string const & method_name, std::string const & method_params) {
        base::xstream_t param_stream(base::xcontext_t::instance());
        param_stream << method_params;
        sync_call(target_addr, method_name, std::string{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())});
        m_string_prop.set("sync_call_a_to_b");
    }

    void test_async_call(common::xaccount_address_t const & target_addr,
                         std::string const & method_name,
                         std::string const & method_params,
                         contract_common::xfollowup_transaction_schedule_type_t type) {
        base::xstream_t param_stream(base::xcontext_t::instance());
        param_stream << method_params;
        call(target_addr, method_name, std::string{reinterpret_cast<char *>(param_stream.data()), static_cast<std::size_t>(param_stream.size())}, type);
        m_string_prop.set("call_a_to_b");
    }

    void test_followup_transfer_to_user(uint64_t amount, contract_common::xfollowup_transaction_schedule_type_t type) {
        auto user = sender();
        transfer(user, amount, type);
    }

    void send_only_api() {
        deposit(common::xtoken_t{500});
        auto token = withdraw(500);
        token.clear();
    }

private:
    contract_common::properties::xstring_property_t m_string_prop{data::system_contract::XPORPERTY_CONTRACT_GENESIS_STAGE_KEY, this};

    contract_common::properties::xmap_property_t<std::string, std::string> m_map_prop{data::system_contract::XPORPERTY_CONTRACT_REG_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map2_prop{data::system_contract::XPORPERTY_CONTRACT_TICKETS_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map3_prop{data::system_contract::XPORPERTY_CONTRACT_REFUND_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map4_prop{data::system_contract::XPROPERTY_CONTRACT_SLASH_INFO_KEY, this};
    contract_common::properties::xmap_property_t<std::string, std::string> m_map5_prop{data::system_contract::XPORPERTY_CONTRACT_VOTE_REPORT_TIME_KEY, this};
};
using xcontract_a_t = xdemo_contract_a;


NS_END3
