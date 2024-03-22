#pragma once

#include "test_mock_statectx.h"
#include "xdata/xsystem_contract/xdata_structures.h"

#include <gtest/gtest.h>

#define private public
#include "xdata/xtable_bstate.h"
#include "xevm_contract_runtime/sys_contract/xevm_eth2_client_contract.h"

namespace top {
namespace tests {

using namespace top::evm_common;
using namespace top::evm_common::eth2;
using namespace top::contract_runtime::evm::sys_contract;

class xeth2_contract_fixture_t : public testing::Test {
public:
    void SetUp() override {
        auto bstate = make_object_ptr<base::xvbstate_t>(
            evm_eth2_client_contract_address.to_string(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_BLOCKS, canvas.get());
        // bstate->new_string_map_var(data::system_contract::XPROPERTY_UNFINALIZED_HEADERS, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_FINALIZED_BEACON_HEADER, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_FINALIZED_EXECUTION_HEADER, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_CURRENT_SYNC_COMMITTEE, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_NEXT_SYNC_COMMITTEE, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_RESET_FLAG, canvas.get());
        bstate->new_uint64_var(data::system_contract::XPROPERTY_CLIENT_MODE, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_UNFINALIZED_HEAD_EXECUTION_HEADER, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_UNFINALIZED_TAIL_EXECUTION_HEADER, canvas.get());
        m_contract_state = std::make_shared<data::xunit_bstate_t>(bstate.get(), bstate.get());
        m_statectx = top::make_unique<xmock_statectx_t>(m_contract_state);
        m_statectx_observer = make_observer<statectx::xstatectx_face_t>(m_statectx.get());
        m_context.address = common::xtop_eth_address::build_from("ff00000000000000000000000000000000000010");
        m_context.caller = common::xtop_eth_address::build_from("f8a1e199c49c2ae2682ecc5b4a8838b39bab1a38");
        m_contract.m_network = evm_common::eth2::xnetwork_id_t::kiln;
    }

    void TearDown() override {
    }

    xevm_eth2_client_contract_t m_contract;
    contract_runtime::evm::sys_contract_context m_context;
    std::shared_ptr<data::xunit_bstate_t> m_contract_state;
    std::unique_ptr<statectx::xstatectx_face_t> m_statectx;
    top::observer_ptr<statectx::xstatectx_face_t> m_statectx_observer;
};

}  // namespace tests
}  // namespace top