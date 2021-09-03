
#include "xbase/xobject_ptr.h"
#include "xblockstore/xblockstore_face.h"
#include "xcontract_runtime/xsystem/xsystem_action_runtime.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xcontract_common/xcontract_state.h"
#include "xdata/xblocktool.h"
#include "xdb/xdb_face.h"
#include "xdb/xdb_factory.h"
#include "xstore/xstore_face.h"
#include "xsystem_contracts/xtransfer_contract.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvledger.h"


#include "tests/mock/xvchain_creator.hpp"


#include <gtest/gtest.h>
#include <memory>

#define private public
#include "xcontract_runtime/xsystem_contract_manager.h"

NS_BEG3(top, tests, contract_runtime)

using namespace top::contract_common;
using namespace top::contract_runtime;
std::string const contract_address = "T200024uDhihoPJ24LQL4znxrugPM4eWk8rY42ceS";


class test_system_contract_runtime : public testing::Test {
protected:
    std::unique_ptr<top::contract_runtime::xsystem_contract_manager_t> system_contract_manager_{};
    top::xobject_ptr_t<top::base::xvbstate_t> bstate_{};
    std::shared_ptr<top::contract_common::properties::xproperty_access_control_t> property_access_control_{};
    std::shared_ptr<top::contract_common::xcontract_state_t> contract_state_{};
    std::shared_ptr<top::contract_common::xcontract_execution_context_t> contract_ctx_{};
    std::unique_ptr<top::contract_runtime::system::xsystem_action_runtime_t> contract_runtime_{};
    xobject_ptr_t<base::xvblockstore_t> blockstore_;

protected:
    void SetUp() override;
    void TearDown() override;
};

void test_system_contract_runtime::SetUp()  {
    bstate_.attach(new top::base::xvbstate_t{contract_address, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::contract_common::properties::xproperty_access_control_data_t{});
    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(top::common::xaccount_address_t{contract_address}, top::make_observer(property_access_control_.get()));
    contract_runtime_ = top::make_unique<top::contract_runtime::system::xsystem_action_runtime_t>(top::make_observer(system_contract_manager_.get()));
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

TEST_F(test_system_contract_runtime, deploy_system_contract) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    contract_runtime::xsystem_contract_manager_t manager;
    manager.initialize(blockstore);
    manager.deploy_system_contract(common::xaccount_address_t{contract_address}, xblock_sniff_config_t{}, contract_deploy_type_t::rec, "rec", contract_broadcast_policy_t::invalid);

    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, contract_address);
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    assert(bstate != nullptr);

    contract_common::properties::xproperty_identifier_t propety_identifier("balance", properties::xproperty_type_t::token, properties::xenum_property_category::system);
    EXPECT_TRUE(bstate->find_property(propety_identifier.full_name()));
}

TEST_F(test_system_contract_runtime, init_system_contract) {
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_t>();
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()), top::contract_common::properties::xproperty_access_control_data_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(common::xaccount_address_t{contract_address}, top::make_observer(property_access_control.get()));
    auto contract_ctx= std::make_shared<contract_common::xcontract_execution_context_t>(tx, contract_state);
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>(top::make_observer(contract_ctx.get()));

    contract_common::properties::xproperty_identifier_t propety_identifier("balance", properties::xproperty_type_t::token, properties::xenum_property_category::system);
    transfer_contract->state()->access_control()->property_exist(common::xaccount_address_t{contract_address}, propety_identifier);
}


NS_END3
