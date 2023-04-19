#pragma once

#include "xdata/xsystem_contract/xdata_structures.h"

#include <gtest/gtest.h>

#define private public
#include "test_mock_statectx.h"
#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"
#include "xevm_contract_runtime/sys_contract/xevm_heco_client_contract.h"
#include "xdata/xtable_bstate.h"

namespace top {
namespace tests {

//class xmock_statectx_t : public statectx::xstatectx_face_t {
//public:
//    xmock_statectx_t(data::xunitstate_ptr_t s) {
//        ustate = s;
//    }
//
//    const data::xtablestate_ptr_t & get_table_state() const override {
//        return table_state;
//    }
//
//    data::xunitstate_ptr_t load_unit_state(common::xaccount_address_t const& address) override {
//        return ustate;
//    }
//
//    data::xaccountstate_ptr_t           load_account_state(common::xaccount_address_t const& address) override {
//        xassert(false);
//        return nullptr;
//    }
//
//    bool do_rollback() override {
//        return false;
//    }
//
//    size_t do_snapshot() override {
//        return 0;
//    }
//
//    std::string get_table_address() const override {
//        return table_address;
//    }
//
//    bool is_state_dirty() const override {
//        return true;
//    }
//
//    data::xtablestate_ptr_t table_state{nullptr};
//    data::xunitstate_ptr_t ustate{nullptr};
//    std::string table_address{common::eth_table_base_address.to_string()};
//};

class xcontract_fixture_t : public testing::Test {
public:
    xcontract_fixture_t() {
    }

    void init() {
        auto bstate = make_object_ptr<base::xvbstate_t>(
            evm_eth_bridge_contract_address.to_string(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_HEADERS_SUMMARY, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_ALL_HASHES, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_EFFECTIVE_HASHES, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_LAST_HASH, canvas.get());
        auto bytes = (evm_common::h256(0)).asBytes();
        bstate->load_string_var(data::system_contract::XPROPERTY_LAST_HASH)->reset({bytes.begin(), bytes.end()}, canvas.get());
        contract_state = std::make_shared<data::xunit_bstate_t>(bstate.get(), bstate.get());
        statectx = top::make_unique<xmock_statectx_t>(contract_state);
        statectx_observer = make_observer<statectx::xstatectx_face_t>(statectx.get());
        context.address = common::xtop_eth_address::build_from("ff00000000000000000000000000000000000002");
        context.caller = common::xtop_eth_address::build_from("f8a1e199c49c2ae2682ecc5b4a8838b39bab1a38");
    }

    void SetUp() override {
        init();
    }

    void TearDown() override {
    }

    contract_runtime::evm::sys_contract::xevm_eth_bridge_contract_t contract;
    data::xunitstate_ptr_t contract_state;
    std::unique_ptr<statectx::xstatectx_face_t> statectx;
    observer_ptr<statectx::xstatectx_face_t> statectx_observer;
    contract_runtime::evm::sys_contract_context context;
};

}
}