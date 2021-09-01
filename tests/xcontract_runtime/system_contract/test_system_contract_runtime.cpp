#include "xvledger/xvstate.h"
#include "xbase/xobject_ptr.h"
#include "xcontract_runtime/xsystem/xsystem_action_runtime.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xdata/xblocktool.h"
#include "xcontract_common/xcontract_state.h"
//#include "xcontract_common/xproeprties/"

#include <gtest/gtest.h>

#include <memory>

NS_BEG3(top, tests, contract_runtime)

using namespace top::contract_common;
std::string const contract_address = "T00000Li18Pb8WuxQUphZPfmN6QCWTL9kwWPTns1";


class test_system_contract_runtime : public testing::Test {
protected:
    top::xobject_ptr_t<top::base::xvbstate_t> bstate_{};
    std::shared_ptr<top::contract_common::properties::xproperty_access_control_t> property_access_control_{};
    std::shared_ptr<top::contract_common::xcontract_state_t> contract_state_{};
    std::shared_ptr<top::contract_common::xcontract_execution_context_t> contract_ctx_{};
    std::unique_ptr<top::contract_runtime::system::xsystem_action_runtime_t> contract_runtime_{};

protected:
    void SetUp() override;
    void TearDown() override;
};

void test_system_contract_runtime::SetUp()  {
    bstate_.attach(new top::base::xvbstate_t{contract_address, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::contract_common::properties::xproperty_access_control_data_t{});
    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(top::common::xaccount_address_t{contract_address}, top::make_observer(property_access_control_.get()));
    contract_runtime_ = top::make_unique<top::contract_runtime::system::xsystem_action_runtime_t>();
}

void test_system_contract_runtime::TearDown() {
    contract_runtime_.reset();
    contract_state_.reset();
    bstate_.reset();
}


TEST_F(test_system_contract_runtime, run_system_contract) {
    contract_common::properties::xproperty_identifier_t propety_identifier("balance", properties::xproperty_type_t::token, properties::xenum_property_category::system);

    auto transfer_tx = top::make_object_ptr<top::data::xtransaction_t>();
    uint64_t amount = 1000;
    top::base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << amount;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    transfer_tx->make_tx_run_contract("transfer", param);

    contract_ctx_ = std::make_shared<top::contract_common::xcontract_execution_context_t>(transfer_tx, contract_state_);
    contract_ctx_->execution_stage(contract_common::xcontract_execution_stage_t{contract_common::xtop_enum_contract_execution_stage::target_action});

    // before execution
    // EXPECT_EQ(contract_ctx_->contract_state()->access_control()->balance(common::xaccount_address_t{contract_address}, propety_identifier), 0);
    contract_runtime_->execute(top::make_observer(contract_ctx_.get()));
    // after execution
    EXPECT_EQ(contract_ctx_->contract_state()->access_control()->balance(common::xaccount_address_t{contract_address}, propety_identifier), amount);

}


NS_END3
