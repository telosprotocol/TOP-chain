#pragma once
#if 0
#include "xvledger/xvstate.h"
#include "xbase/xobject_ptr.h"
#include "xcontract_runtime/xuser/xuser_action_runtime.h"
//#include "xcontract_common/xproeprties/"

#include <gtest/gtest.h>

#include <memory>

NS_BEG3(top, tests, contract_runtime)

class contract_runtime_fixture : public testing::Test {
protected:
    top::xobject_ptr_t<top::base::xvbstate_t> bstate_{};
    std::shared_ptr<top::contract_common::properties::xproperty_access_control_t> property_access_control_{};
    std::shared_ptr<top::contract_common::xcontract_state_t> contract_state_{};
    std::shared_ptr<top::contract_common::xcontract_execution_context_t> contract_ctx_{};
    std::unique_ptr<top::contract_runtime::user::xuser_action_runtime_t> contract_runtime_{};

protected:
    void SetUp() override;
    void TearDown() override;
};

NS_END3
#endif