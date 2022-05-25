#pragma once

#include "xdata/xsystem_contract/xdata_structures.h"

#include <gtest/gtest.h>

#define private public
#include "xevm_contract_runtime/sys_contract/xevm_eth_bridge_contract.h"
#include "xdata/xtable_bstate.h"

namespace top {
namespace tests {

class xcontract_fixture_t : public testing::Test {
public:
    xcontract_fixture_t() {
    }

    void init() {
        auto bstate = make_object_ptr<base::xvbstate_t>(evm_eth_bridge_contract_address.value(), (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
        auto canvas = make_object_ptr<base::xvcanvas_t>();
        bstate->new_string_map_var(data::system_contract::XPROPERTY_ETH_CHAINS_HEADER, canvas.get());
        bstate->new_string_map_var(data::system_contract::XPROPERTY_ETH_CHAINS_HASH, canvas.get());
        bstate->new_string_var(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT, canvas.get());
        auto bytes = evm_common::toBigEndian(evm_common::u256(0));
        bstate->load_string_var(data::system_contract::XPROPERTY_ETH_CHAINS_HEIGHT)->reset({bytes.begin(), bytes.end()}, canvas.get());
        default_bstate = bstate;
        if (contract.m_contract_state == nullptr) {
            contract.m_contract_state = std::make_shared<data::xunit_bstate_t>(bstate.get());
        }

        parent_header.m_number = 100;
        parent_header.m_time = 1653471000;
        header.m_number = 101;
        header.m_extra = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
        header.m_time = 1653471010;
        header.
    }

    void SetUp() override {
        init();
    }

    void TearDown() override {
    }

    xobject_ptr_t<base::xvbstate_t> default_bstate;
    contract_runtime::evm::sys_contract::xevm_eth_bridge_contract_t contract;

    evm_common::eth::xeth_block_header_t parent_header;
    evm_common::eth::xeth_block_header_t header;
};
}
}