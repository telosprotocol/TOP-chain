#include <gtest/gtest.h>

#include <sstream>
#define private public
#define protected public
#include "tests/mock/xvchain_creator.hpp"
#include "xbase/xobject_ptr.h"
#include "xblockstore/xblockstore_face.h"
#include "xcontract_common/xcontract_state.h"
#include "xcontract_vm/xaccount_vm.h"
#include "xdata/xblocktool.h"
#include "xdata/xdata_common.h"
#include "xdata/xnative_contract_address.h"
#include "xdata/xtop_action_generator.h"
#include "xdata/xtransaction_v2.h"
#include "xdb/xdb_face.h"
#include "xdb/xdb_factory.h"

#include "xsystem_contract_runtime/xsystem_action_runtime.h"
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xsystem_contracts/xtransfer_contract.h"
#include "xvledger/xvledger.h"
#include "xvledger/xvstate.h"
#include "xstatestore/xstatestore_face.h"
// #include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract_new.h"
// #include "xvm/xsystem_contracts/xregistration/xrec_registration_contract_new.h"

#include <memory>

NS_BEG3(top, tests, contract_runtime)

using namespace top::contract_common;
using namespace top::contract_runtime;
std::string const contract_address{"T2000138NZjvNJjRNG5iEqVKydpqAqoeNjBuFmNbj@0"};

class test_system_contract_runtime : public testing::Test {
protected:
    std::unique_ptr<top::contract_runtime::system::xsystem_contract_manager_t> system_contract_manager_{};
    top::xobject_ptr_t<top::base::xvbstate_t> bstate_{};
    // std::shared_ptr<top::contract_common::properties::xproperty_access_control_t> property_access_control_{};
    std::shared_ptr<top::state_accessor::xstate_accessor_t> state_accessor_{};
    std::shared_ptr<top::contract_common::xcontract_state_t> contract_state_{};
    std::shared_ptr<top::contract_common::xcontract_execution_context_t> contract_ctx_{};
    std::unique_ptr<top::contract_runtime::system::xsystem_action_runtime_t> contract_runtime_{};
    xobject_ptr_t<base::xvblockstore_t> blockstore_;

protected:
    void SetUp() override;
    void TearDown() override;
};

void test_system_contract_runtime::SetUp() {
    bstate_.attach(new top::base::xvbstate_t{contract_address, (uint64_t)1, (uint64_t)1, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0});
    // property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()),
    // top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    state_accessor_ = std::make_shared<top::state_accessor::xstate_accessor_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{});
    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(
        top::common::xaccount_address_t{contract_address}, top::make_observer(state_accessor_.get()), contract_common::xcontract_execution_param_t{});
    system_contract_manager_ = top::make_unique<top::contract_runtime::system::xsystem_contract_manager_t>();
    contract_runtime_ = top::make_unique<top::contract_runtime::system::xsystem_action_runtime_t>(top::make_observer(system_contract_manager_.get()));
}

void test_system_contract_runtime::TearDown() {
    contract_runtime_.reset();
    contract_state_.reset();
    bstate_.reset();
}

//TEST_F(test_system_contract_runtime, run_system_contract) {
//    mock::xvchain_creator creator;
//    base::xvblockstore_t* blockstore = creator.get_blockstore();
//
//    uint64_t init_value = 100;
//    // system_contract_manager_->initialize(blockstore);
//    // system_contract_manager_->m_blockstore = make_observer(blockstore);
//    system_contract_manager_->deploy_system_contract<system_contracts::xtop_transfer_contract>(
//        common::xaccount_address_t{contract_address}, {}, {}, {}, {}, {}, make_observer(blockstore));
//
//    auto latest_vblock = data::xblocktool_t::get_latest_connectted_state_changed_block(blockstore, contract_address);
//    bstate_ = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
//    assert(bstate_ != nullptr);
//    state_accessor::properties::xproperty_identifier_t propety_identifier("$0", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
//    uint64_t balance = bstate_->load_token_var(propety_identifier.full_name())->get_balance();
//    EXPECT_EQ(balance, init_value);
//
//    auto transfer_tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
//    uint64_t amount = 1000;
//    top::base::xstream_t param_stream(base::xcontext_t::instance());
//    param_stream << amount;
//    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
//    transfer_tx->make_tx_run_contract("transfer", param);
//    transfer_tx->set_different_source_target_address("T00000LS7SABDaqKaKfNDqbsyXB23F8dndquCeEu", contract_address);
//    transfer_tx->set_digest();
//    transfer_tx->set_len();
//
//    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(transfer_tx.get());
//    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
//
//    // property_access_control_ =
//    //     std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
//    state_accessor_ = std::make_shared<top::state_accessor::xstate_accessor_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{});
//    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(
//        top::common::xaccount_address_t{contract_address}, top::make_observer(state_accessor_.get()), contract_common::xcontract_execution_param_t{});
//
//    contract_ctx_ = std::make_shared<top::contract_common::xcontract_execution_context_t>(std::move(action), contract_state_);
//    // contract_ctx_->execution_stage(contract_common::xcontract_execution_stage_t{contract_common::xtop_enum_contract_execution_stage::target_action});
//
//    // before execution
//    // EXPECT_EQ(contract_ctx_->contract_state()->access_control()->balance(common::xaccount_address_t{contract_address}, propety_identifier), 0);
//    // contract_runtime_->execute(top::make_observer(contract_ctx_.get()));
//    // after execution
//    // EXPECT_EQ(contract_ctx_->contract_state()->balance(propety_identifier, common::xsymbol_t{"TOP"}));
//
//}

TEST_F(test_system_contract_runtime, deploy_system_contract) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    // system_contract_manager_->initialize(blockstore);
    // system_contract_manager_->m_blockstore = make_observer(blockstore);
    system_contract_manager_->deploy_system_contract<system_contracts::xtop_transfer_contract>(
        common::xaccount_address_t{contract_address}, {}, {}, {}, {}, {}, make_observer(blockstore));

    data::xunitstate_ptr_t unitstate = statestore::xstatestore_hub_t::instance()->get_unit_latest_connectted_change_state(common::xaccount_address_t{contract_address});
    auto bstate = unitstate->get_bstate();   

    state_accessor::properties::xproperty_identifier_t propety_identifier("$0", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    EXPECT_TRUE(bstate->find_property(propety_identifier.full_name()));
}

TEST_F(test_system_contract_runtime, init_system_contract) {
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->set_different_source_target_address("T00000LS7SABDaqKaKfNDqbsyXB23F8dndquCeEu", contract_address);
    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    // auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()),
    // top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    auto state_accessor = std::make_shared<state_accessor::xstate_accessor_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(
        common::xaccount_address_t{contract_address}, top::make_observer(state_accessor.get()), top::contract_common::xcontract_execution_param_t{});
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx = std::make_shared<contract_common::xcontract_execution_context_t>(std::move(action), contract_state);
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>();
    transfer_contract->reset_execution_context(top::make_observer(contract_ctx.get()));

    state_accessor::properties::xproperty_identifier_t propety_identifier(
        "balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    transfer_contract->contract_state()->property_exist(propety_identifier);
}

TEST_F(test_system_contract_runtime, test_asset_api_normal) {
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>();

    auto tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
    data::xproperty_asset asset_out{data::XPROPERTY_ASSET_TOP, uint64_t{100000}};
    tx->make_tx_run_contract(asset_out, "just_test_asset_api", "");
    tx->set_different_source_target_address("T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks", sys_contract_rec_registration_addr);

    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    // auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()),
    // top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    auto state_accessor = std::make_shared<state_accessor::xstate_accessor_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(
        common::xaccount_address_t{contract_address}, top::make_observer(state_accessor.get()), top::contract_common::xcontract_execution_param_t{});
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx = std::make_shared<contract_common::xcontract_execution_context_t>(std::move(action), contract_state);

    transfer_contract->reset_execution_context(contract_ctx);

    std::error_code err;
    auto token = transfer_contract->last_action_asset(err);
    assert(!err);


    EXPECT_EQ(token.amount(), 0);
    EXPECT_EQ(token.symbol().to_string(), common::SYMBOL_TOP_TOKEN.to_string());
}

TEST_F(test_system_contract_runtime, test_asset_api_invalid) {
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>();

    auto tx = top::make_object_ptr<top::data::xtransaction_v2_t>();

    data::xproperty_asset asset{data::XPROPERTY_ASSET_TOP, uint64_t{100000}};
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address("T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks", sys_contract_rec_registration_addr);

    xobject_ptr_t<base::xvbstate_t> bstate =
        make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    // auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()),
    // top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    auto state_accessor = std::make_shared<state_accessor::xstate_accessor_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(
        common::xaccount_address_t{contract_address}, top::make_observer(state_accessor.get()), top::contract_common::xcontract_execution_param_t{});
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx = std::make_shared<contract_common::xcontract_execution_context_t>(std::move(action), contract_state);

    transfer_contract->reset_execution_context(contract_ctx);

    std::error_code err;
    auto token = transfer_contract->last_action_asset(err);
    assert(!err);
    EXPECT_EQ(token.amount(), 0);
}

NS_END3
