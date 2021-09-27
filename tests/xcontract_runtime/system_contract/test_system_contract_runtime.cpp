
#define private public
#define protected public
#include "xbase/xobject_ptr.h"
#include "xblockstore/xblockstore_face.h"
#include "xsystem_contract_runtime/xsystem_action_runtime.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
#include "xcontract_common/xproperties/xproperty_access_control.h"
#include "xcontract_common/xcontract_state.h"
#include "xdata/xblocktool.h"
#include "xdata/xtransaction_v2.h"
#include "xdb/xdb_face.h"
#include "xdb/xdb_factory.h"
#include "xstore/xstore_face.h"
#include "xsystem_contracts/xtransfer_contract.h"
#include "xvledger/xvstate.h"
#include "xvledger/xvledger.h"
#include "xcontract_runtime/xtop_action_generator.h"


#include "tests/mock/xvchain_creator.hpp"


#include <gtest/gtest.h>
#include <memory>


#define private public
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xcontract_vm/xaccount_vm.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract_new.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract_new.h"
#include "xdata/xdata_common.h"

NS_BEG3(top, tests, contract_runtime)

using namespace top::contract_common;
using namespace top::contract_runtime;
std::string const contract_address = system_contracts::transfer_address;


class test_system_contract_runtime : public testing::Test {
protected:
    std::unique_ptr<top::contract_runtime::system::xsystem_contract_manager_t> system_contract_manager_{};
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
    property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(top::common::xaccount_address_t{contract_address}, top::make_observer(property_access_control_.get()));
    system_contract_manager_ = top::make_unique<top::contract_runtime::system::xsystem_contract_manager_t>();
    contract_runtime_ = top::make_unique<top::contract_runtime::system::xsystem_action_runtime_t>(top::make_observer(system_contract_manager_.get()));
}

void test_system_contract_runtime::TearDown() {
    contract_runtime_.reset();
    contract_state_.reset();
    bstate_.reset();
}


TEST_F(test_system_contract_runtime, run_system_contract) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    uint64_t init_value = 0;
    system_contract_manager_->initialize(blockstore);
    system_contract_manager_->deploy_system_contract<system_contracts::xtop_transfer_contract>(common::xaccount_address_t{contract_address}, {}, {}, {}, {}, {});

    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, contract_address);
    bstate_ = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    assert(bstate_ != nullptr);
    state_accessor::properties::xproperty_identifier_t propety_identifier("balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    uint64_t balance = bstate_->load_token_var(propety_identifier.full_name())->get_balance();
    EXPECT_EQ(balance, init_value);

    auto transfer_tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
    uint64_t amount = 1000;
    top::base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << amount;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    transfer_tx->make_tx_run_contract("transfer", param);
    transfer_tx->set_different_source_target_address("T00000LS7SABDaqKaKfNDqbsyXB23F8dndquCeEu", contract_address);
    transfer_tx->set_digest();
    transfer_tx->set_len();

    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(transfer_tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);

    property_access_control_ =
        std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    contract_state_ =
        std::make_shared<top::contract_common::xcontract_state_t>(top::common::xaccount_address_t{contract_address}, top::make_observer(property_access_control_.get()));

    contract_ctx_ = std::make_shared<top::contract_common::xcontract_execution_context_t>(std::move(action), contract_state_);
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

    system_contract_manager_->initialize(blockstore);
    system_contract_manager_->deploy_system_contract<system_contracts::xtop_transfer_contract>(common::xaccount_address_t{contract_address}, {}, {}, {}, {}, {});

    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, contract_address);
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    assert(bstate != nullptr);

    state_accessor::properties::xproperty_identifier_t propety_identifier("balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    EXPECT_TRUE(bstate->find_property(propety_identifier.full_name()));
}

TEST_F(test_system_contract_runtime, init_system_contract) {
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_v2_t>();
    tx->set_different_source_target_address("T00000LS7SABDaqKaKfNDqbsyXB23F8dndquCeEu", contract_address);
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(common::xaccount_address_t{contract_address}, top::make_observer(property_access_control.get()));
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx= std::make_shared<contract_common::xcontract_execution_context_t>(std::move(action), contract_state);
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>();
    transfer_contract->reset_execution_context(top::make_observer(contract_ctx.get()));

    state_accessor::properties::xproperty_identifier_t propety_identifier("balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    transfer_contract->state()->access_control()->property_exist(common::xaccount_address_t{contract_address}, propety_identifier);
}

TEST_F(test_system_contract_runtime, account_vm) {
    std::string account{"T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks"};
    std::string public_key{"BFqS6Al19LkycuHhrHMuI/E1G6+rZi4NJTQ1w1U55UnMjhBnb8/ey4pj+Mn69lyVB0+r6GR6M6eett9Tv/yoizI="};
    data::xblock_consensus_para_t cs_para;
    {
        cs_para.m_clock = 5796740;
        cs_para.m_total_lock_tgas_token = 0;
        cs_para.m_proposal_height = 7;
        cs_para.m_account = "Ta0001@0";
        cs_para.m_random_seed = base::xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
    }

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << common::xtop_node_id{account};
    param_stream << common::xnetwork_id_t{ static_cast<common::xnetwork_id_t::value_type>(top::config::to_chainid(XGET_CONFIG(chain_name))) };
#if defined XENABLE_MOCK_ZEC_STAKE
    ENUM_SERIALIZE(param_stream, common::xrole_type_t::advance);
    param_stream << public_key;
    param_stream << static_cast<uint64_t>(top::config::to_chainid(XGET_CONFIG(chain_name)));
#endif
    param_stream << std::string("1.1.1");
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    tx->make_tx_run_contract("nodeJoinNetwork2", param);
    tx->set_different_source_target_address(account, sys_contract_rec_standby_pool_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    tx->set_tx_type(xtransaction_type_run_contract_new);
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);

    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();

    contract_runtime::system::xtop_system_contract_manager::instance()->initialize(blockstore);
    contract_runtime::system::xtop_system_contract_manager::instance()->deploy();
    // auto system_contract_manager = make_unique<contract_runtime::system::xsystem_contract_manager_t>();
    auto * system_contract_manager = contract_runtime::system::xsystem_contract_manager_t::instance();
    xassert(system_contract_manager != nullptr);

    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, std::string{sys_contract_rec_standby_pool_addr});
    auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    auto bstate_cmp = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);

    contract_vm::xaccount_vm_t vm(make_observer(system_contract_manager));
    auto result = vm.execute(input_txs, bstate, cs_para);
    EXPECT_EQ(result.status.ec.value(), 0);
    EXPECT_EQ(result.success_tx_assemble.size(), 2);
    EXPECT_EQ(result.failed_tx_assemble.size(), 0);
    EXPECT_EQ(result.delay_tx_assemble.size(), 0);
    EXPECT_EQ(result.success_tx_assemble[0]->get_source_addr(), account);
    EXPECT_EQ(result.success_tx_assemble[0]->get_target_addr(), sys_contract_rec_standby_pool_addr);
    EXPECT_EQ(result.success_tx_assemble[1]->get_source_addr(), sys_contract_rec_standby_pool_addr);
    EXPECT_EQ(result.success_tx_assemble[1]->get_target_addr(), sys_contract_rec_registration_addr);
}

TEST_F(test_system_contract_runtime, test_follow_up_delay) {
    mock::xvchain_creator creator;
    base::xvblockstore_t* blockstore = creator.get_blockstore();
    contract_runtime::system::xtop_system_contract_manager::instance()->initialize(blockstore);
    contract_runtime::system::xtop_system_contract_manager::instance()->deploy_system_contract<system_contracts::xtransfer_contract_t>(
        common::xaccount_address_t{sys_contract_zec_reward_addr}, {}, {}, {}, {}, {});

    xtransaction_ptr_t tx = make_object_ptr<xtransaction_v2_t>();
    base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << uint64_t(1000);
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    tx->make_tx_run_contract("follow_up_delay", param);
    tx->set_different_source_target_address(sys_contract_zec_reward_addr, sys_contract_zec_reward_addr);
    tx->set_fire_and_expire_time(600);
    tx->set_deposit(XGET_ONCHAIN_GOVERNANCE_PARAMETER(min_tx_deposit));
    // tx->set_tx_type(xtransaction_type_run_contract_new);
    xcons_transaction_ptr_t cons_tx = make_object_ptr<xcons_transaction_t>(tx.get());
    cons_tx->set_tx_subtype(enum_transaction_subtype_recv);

    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(cons_tx);
    auto * system_contract_manager = contract_runtime::system::xsystem_contract_manager_t::instance();
    xassert(system_contract_manager != nullptr);

    contract_vm::xaccount_vm_t vm(make_observer(system_contract_manager));

    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, std::string{sys_contract_zec_reward_addr});
    auto bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    data::xblock_consensus_para_t cs_para;
    {
        cs_para.m_clock = 5796740;
        cs_para.m_total_lock_tgas_token = 0;
        cs_para.m_proposal_height = 7;
        cs_para.m_account = "Ta0001@0";
        cs_para.m_random_seed = base::xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
    }
    auto result = vm.execute(input_txs, bstate, cs_para);

    EXPECT_EQ(result.status.ec.value(), 0);
    EXPECT_EQ(result.delay_tx_assemble.size(), enum_vledger_const::enum_vbucket_has_tables_count);

    auto nonce = 1;
    auto amount = 1000;
    for (auto const & tx : result.delay_tx_assemble) {
        EXPECT_EQ(tx->get_source_addr(), sys_contract_zec_reward_addr);
        EXPECT_EQ(tx->get_target_addr(), std::string{sys_contract_sharding_reward_claiming_addr} + "@" + std::to_string(nonce - 1));
        auto tx_nonce = tx->get_tx_nonce();
        EXPECT_EQ(nonce++, tx_nonce);
    }
}

TEST_F(test_system_contract_runtime, test_asset_api_normal) {
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>();

    auto tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
    data::xproperty_asset asset_out{data::XPROPERTY_ASSET_TOP, uint64_t{100000}};
    tx->make_tx_run_contract(asset_out, "just_test_asset_api", "");
    tx->set_different_source_target_address("T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks", sys_contract_rec_registration_addr);

    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(common::xaccount_address_t{contract_address}, top::make_observer(property_access_control.get()));
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx= std::make_shared<contract_common::xcontract_execution_context_t>(std::move(action), contract_state);

    transfer_contract->reset_execution_context(contract_ctx);

    std::error_code err;
    auto amount = transfer_contract->src_action_asset_amount(err);
    auto token_name = transfer_contract->src_action_asset_name(err);
    assert(!err);

    EXPECT_EQ(amount, 100000);
    EXPECT_EQ(token_name, data::XPROPERTY_ASSET_TOP);

}

TEST_F(test_system_contract_runtime, test_asset_api_fail) {
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>();

    auto tx = top::make_object_ptr<top::data::xtransaction_v2_t>();

    data::xproperty_asset asset{data::XPROPERTY_ASSET_TOP, uint64_t{100000}};
    tx->make_tx_transfer(asset);
    tx->set_different_source_target_address("T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks", sys_contract_rec_registration_addr);

    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{}, contract_common::xcontract_execution_param_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(common::xaccount_address_t{contract_address}, top::make_observer(property_access_control.get()));
    data::xcons_transaction_ptr_t cons_tx = make_object_ptr<data::xcons_transaction_t>(tx.get());
    auto action = top::contract_runtime::xaction_generator_t::generate(cons_tx);
    auto contract_ctx= std::make_shared<contract_common::xcontract_execution_context_t>(std::move(action), contract_state);

    transfer_contract->reset_execution_context(contract_ctx);

    std::error_code err;
    auto amount = transfer_contract->src_action_asset_amount(err);
    assert(err); // have error
    EXPECT_EQ(err.value(), (int)contract_common::error::xerrc_t::src_action_asset_not_exist);
    err.clear();
    auto token_name = transfer_contract->src_action_asset_name(err);
    assert(err); // have error
    EXPECT_EQ(err.value(), (int)contract_common::error::xerrc_t::src_action_asset_not_exist);

}


NS_END3
