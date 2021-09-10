
#define private public
#define protected public
#include "xbase/xobject_ptr.h"
#include "xblockstore/xblockstore_face.h"
#include "xsystem_contract_runtime/xsystem_action_runtime.h"
#include "xsystem_contracts/xsystem_contract_addresses.h"
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
#include "xsystem_contract_runtime/xsystem_contract_manager.h"
#include "xcontract_vm/xaccount_vm.h"
#include "xvm/xsystem_contracts/xelection/xrec/xrec_standby_pool_contract_new.h"
#include "xvm/xsystem_contracts/xregistration/xrec_registration_contract_new.h"

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
    property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{});
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

    uint64_t init_value = 1000;
    system_contract_manager_->initialize(blockstore);
    system_contract_manager_->deploy_system_contract<system_contracts::xtop_transfer_contract>(common::xaccount_address_t{contract_address}, xblock_sniff_config_t{}, system::contract_deploy_type_t::rec, common::xnode_type_t::zec, system::contract_broadcast_policy_t::invalid);


    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, contract_address);
    bstate_ = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    assert(bstate_ != nullptr);
    state_accessor::properties::xproperty_identifier_t propety_identifier("balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    uint64_t balance = bstate_->load_token_var(propety_identifier.full_name())->get_balance();
    EXPECT_EQ(balance, init_value);

    auto transfer_tx = top::make_object_ptr<top::data::xtransaction_t>();
    uint64_t amount = 1000;
    top::base::xstream_t param_stream(base::xcontext_t::instance());
    param_stream << amount;
    std::string param(reinterpret_cast<char *>(param_stream.data()), param_stream.size());
    transfer_tx->make_tx_run_contract("transfer", param);

    property_access_control_ = std::make_shared<top::contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate_.get()), top::state_accessor::xstate_access_control_data_t{});
    contract_state_ = std::make_shared<top::contract_common::xcontract_state_t>(top::common::xaccount_address_t{contract_address}, top::make_observer(property_access_control_.get()));
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

    system_contract_manager_->initialize(blockstore);
    system_contract_manager_->deploy_system_contract<system_contracts::xtop_transfer_contract>(common::xaccount_address_t{contract_address}, xblock_sniff_config_t{}, system::contract_deploy_type_t::rec, common::xnode_type_t::zec, system::contract_broadcast_policy_t::invalid);

    auto latest_vblock = data::xblocktool_t::get_latest_committed_lightunit(blockstore, contract_address);
    base::xauto_ptr<base::xvbstate_t> bstate = base::xvchain_t::instance().get_xstatestore()->get_blkstate_store()->get_block_state(latest_vblock.get(), metrics::statestore_access_from_application_isbeacon);
    assert(bstate != nullptr);

    state_accessor::properties::xproperty_identifier_t propety_identifier("balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    EXPECT_TRUE(bstate->find_property(propety_identifier.full_name()));
}

TEST_F(test_system_contract_runtime, init_system_contract) {
    data::xtransaction_ptr_t tx = make_object_ptr<data::xtransaction_t>();
    xobject_ptr_t<base::xvbstate_t> bstate = make_object_ptr<base::xvbstate_t>(contract_address, (uint64_t)0, (uint64_t)0, std::string(), std::string(), (uint64_t)0, (uint32_t)0, (uint16_t)0);
    auto property_access_control = std::make_shared<contract_common::properties::xproperty_access_control_t>(top::make_observer(bstate.get()), top::state_accessor::xstate_access_control_data_t{});
    auto contract_state = std::make_shared<contract_common::xcontract_state_t>(common::xaccount_address_t{contract_address}, top::make_observer(property_access_control.get()));
    auto contract_ctx= std::make_shared<contract_common::xcontract_execution_context_t>(tx, contract_state);
    auto transfer_contract = std::make_shared<system_contracts::xtop_transfer_contract>(top::make_observer(contract_ctx.get()));

    state_accessor::properties::xproperty_identifier_t propety_identifier("balance", state_accessor::properties::xproperty_type_t::token, state_accessor::properties::xenum_property_category::system);
    transfer_contract->state()->access_control()->property_exist(common::xaccount_address_t{contract_address}, propety_identifier);
}

static std::string tx_encode{"YgEAAC8EAAABYwEAAP4BAAADAO4BAAAAAAAAAACghgEAWALTojlhAAAAAAAAAAAAAAAAAAAAAAAAAAD1LGNwXb7p9gAAAAAAAAAAAAAAAAAAAAAAAFMAKAAAAFQwMDAwMExVdXFFaVdpVnNLSFRiQ0pUYzJZcVRlRDZpWlZzcW10a3MAAAAADwAAAAMAAABUT1AAAAAAAAAAAAAAAAAAAAAAAAAAAAUA+AArAAAAVDIwMDAxMzhDUXd5ekZ4YldaNTltTmprcTNlWjNlSDQxdDdiNW1pZG1AMBAAAABub2RlSm9pbk5ldHdvcmsyoQAAACgAAABUMDAwMDBMVXVxRWlXaVZzS0hUYkNKVGMyWXFUZUQ2aVpWc3FtdGtz/wAAAAIAAABYAAAAQkZxUzZBbDE5TGt5Y3VIaHJITXVJL0UxRzYrclppNE5KVFExdzFVNTVVbk1qaEJuYjgvZXk0cGorTW42OWx5VkIwK3I2R1I2TTZlZXR0OVR2L3lvaXpJPf8AAAAAAAAABQAAADEuMi4zAAAAAAAAAABgrFjVIn/LdKPX4VQMr+OLo9/X/Yb/gC+kI50eI2y4rUEAAAAAN9Mjy8K/Fs2FNQ2ymUoej86WcB4FZc1m19QTLkkXHa5Da+JXj8iPcXIPlHb1AwavtV0QasxPrDGZzRwytG2h5AAAAAAAAAAAAWEBAAAnAgAAfSBgrFjVIn/LdKPX4VQMr+OLo9/X/Yb/gC+kI50eI2y4rQJkKFQwMDAwMExVdXFFaVdpVnNLSFRiQ0pUYzJZcVRlRDZpWlZzcW10a3MMcnVuX2NvbnRyYWN0PwYBMAQ1MDAwATIEMTQ4MgE3ATEBOAExATkCNTEBYQQxMDI0YAEAAKEBAABtAQAA1/9AAG0BAADQDfgC8RmS6t6O+sfq++EBAADrz7uDDbr6dRmm7+EC/////w8ABAUA/wAA9gEAAQBJAf8HBBAA/9BJSCBR7n9jFYKiuNxYehtFc6dIPKB1hOiWSviCKEtsgBfocyAJFy72uNhOYXIuWQGM43obE6Jfd23/uKSVR49m65WxNSDfD+EjdhUkTyQYUn8ywP9MkAqLo2BdfXPaOsBDHNgA3yDpoGW9b/SSkOk88W0mo3AbG7X77NvF3TQKg3YnAphS51h0AAAAWAAAACVljNAAAAAAIQACxVss1s0Tmi0Fjkl7J/eewO9gMOvthiG+vB73ORC+XaEgAGpCwqAG5DGfk7SvacFFaVIjo9PPR1RR2qe8mJ16nJ09BAAPWQAA8DkDPx32QAyxz7UF7FKXPTr+5c79hQmls4dw987DIheYxNMgAIeSzZe+L/gzFF4i9wnNY76V4GET+f4IFlUAXAyeogWyBAAHAAABIwAAAAECAQkXLva42E5hci5ZAYzjehsTol93bf+4pJVHj2brlbE1"};
static std::string tx_raw_encode{"YwEAAP4BAAADAO4BAAAAAAAAAACghgEAWALTojlhAAAAAAAAAAAAAAAAAAAAAAAAAAD1LGNwXb7p9gAAAAAAAAAAAAAAAAAAAAAAAFMAKAAAAFQwMDAwMExVdXFFaVdpVnNLSFRiQ0pUYzJZcVRlRDZpWlZzcW10a3MAAAAADwAAAAMAAABUT1AAAAAAAAAAAAAAAAAAAAAAAAAAAAUA+AArAAAAVDIwMDAxMzhDUXd5ekZ4YldaNTltTmprcTNlWjNlSDQxdDdiNW1pZG1AMBAAAABub2RlSm9pbk5ldHdvcmsyoQAAACgAAABUMDAwMDBMVXVxRWlXaVZzS0hUYkNKVGMyWXFUZUQ2aVpWc3FtdGtz/wAAAAIAAABYAAAAQkZxUzZBbDE5TGt5Y3VIaHJITXVJL0UxRzYrclppNE5KVFExdzFVNTVVbk1qaEJuYjgvZXk0cGorTW42OWx5VkIwK3I2R1I2TTZlZXR0OVR2L3lvaXpJPf8AAAAAAAAABQAAADEuMi4zAAAAAAAAAABgrFjVIn/LdKPX4VQMr+OLo9/X/Yb/gC+kI50eI2y4rUEAAAAAN9Mjy8K/Fs2FNQ2ymUoej86WcB4FZc1m19QTLkkXHa5Da+JXj8iPcXIPlHb1AwavtV0QasxPrDGZzRwytG2h5AAAAAAAAAAA"};
static std::string state_encode{"zP9AABMIAACbb+kT///wAAIAAQAABAAAK1QyMDAwMTM4Q1F3eXpGeGJXWjU5bU5qa3EzZVozZUg0MXQ3YjVtaWRtQDAgURwSG4KjdC7Q2gqex8xWArZGJxSx4KnNhYKVF+3FDrsgci9w70uCt981w4VF+8wFtyGxPebwKP9JmNoWaudR9VYDAJ//AAA1AAAAAyQwNj8CATEBMQEyILz0fquEPcC6YIqUpacIRFMuD1l0YgqOsMHsWQiuRexjuP8AABEAAAADJDA3B5Tv4QK3/wAAKgkAAANANDEPmxKRgcz/ko7ZKFQwMDAwMExLZkJZZndUY05uaURTUXFqOGZqNWF0aURxUDhaRUpKdjaV2VhCRkZWbmhlQlMyeUpMd2xiK3E2eEgvREwrUm90YnZSZGQ5WWVKS3VnMXRQK1dwcFRkQjM2S3pNT0h4bUhUc2g1dTlCS2dQRGdYcHBGdnlCZXFZVXhvVFU9hs0CAQDNAgIAzRAAAM0gAADNQAQAzYAAAA+lMS4xLjDD2ShUMDAwMDBMTEo4QXNONGhSRUR0Q3B1S0F4SkZ3cWthOUx3aUFvbjNNldlYQkR1bEpoRTJoY1ZjY1g2aXBpUVE3bGVyVGppaUxPUEhGUlZJaEZxRnBGR0VjZ1FsRUgxbHhNYzJUeGtWT215Y3dQa2RhREpEeWVNQW9FV3hGUmtoQjdvpgAY8BJOaTUzVWI3MjZIY1BYWmZDNHo2ekxnVG81a3M2R3pUVXBMAf9HTlJIZVJHdzRZWm5USGVOR3hZdHVBc3ZTc2xUVjdUSE1zM0E5UkpNKzFWZzYzZ3lRNFhtSzJpOEhXK2YzSWFNN0thdmNIN0pNaFRQRnpLdFdwN0lYVzSmABjwElRTaXA4WGJqdXRydG04UmtRenNIS3F0MjhnOTd4ZFV4Z6YA/0dFVFRnRXY2SEZGdHhUVkNRWkJpb1hjNU0yb1hiNWlQUWdvTzZxbFhsUEV6VFBLNEQyeXV6NHBBZlFxZnh3QUJSdmkwbmYxRVkwQ1Z5OVozSEpmMitDUaYAGPASVXY3ZThSWkxOdG5FMUs5c0VmRTlTWWU3NHJ3WWt6RXVipgD/R0Y3ZTJFdDg2elkzUElKMkJoL3dneGNLVFRkZ3hmZnV2YUhKM0FiUjk5YlFyOWpBZ1VOS0N5RzlxYllEYmdVNzRlVVREWkZjb0t5Y0dXZTdVRjRTY0ZvpgAY8BJWcEw5WFJ0VmRVNVJ3Zm5tckN0Smh2UUZ4SjhUQjQ2Z0KmAP9HUCtzOTZpbHVyaHJhRlU3UkQyVWE2MHJEOENwZ0R4Q2pXY3A2N3lxN0Q1MDBnZjBlajV2Qkdpd3FaMkd3b0VXQWNYRkhxVWxUUVc4SXFJV0hDazVlS2umABjwElhSU0RrenJVc3NlWm1mSkZuU1NCc2dtNzU0WHdWOVNMd6YA/0dETDErdStRQlRmMTUvc3VzUDhKSEFyMGNickhyejhpWFJuTGZaNDdpemFGdGMxWkdoRDJPVHVDRU1VTk8wY1FDMExobnZaNlFoa2FpaVB1UGI2dEM1OKYAGfARcXAxTmtmb29NQXc3QnR5MmlYVHhnVENmc3lnTW54clSmAP9HRnloQTZCUDJtVGJnT3Ntc1FGalEwOXI5aVhuK2YzZm1jZU9iK08xYVltcjZxRG83S3dEdjI1aU9NUlY4bkJPZ3VudjZFVUF0akRLaWR2TUU5WWt1QlGmABjwEmFGbVJBeWJTS1RLakU4VVh5ZjdhdDJXY3c4aW9ka29aOKYA/0dNcG45dDRQRGVIb2RvVWVpYW1paXBzUzNibk5HVDRNYmsveW5HSlkxcG5JdXF2NG5sRWhWT3YxQ1VaNUpiZU5jV1YvVk5UaW4zeHV2bC9zT0tOeDFMVaYAGPASY05mY3FGUEg5dnkzRVlBcGtyY1hMY1FOMmhiMXlnWldFpgD/RkM4MUoyUGxkS1VNMitKamtnem1MV2NIckFiUXk3VzlPWkZZSGRjM215VG9JTWxyWFlIdXJhRXArbmNTZkdFT2t4dzNCWFlaUXRBenA2Z0Q3VUtTaESmABnwEmVYTnFXN21DQ29qMjNMRXN4RW1OY1dLczhtNmtKSDQ0NqYA/0dOOUlRdXgxTlEwQnlCQ1lBQVZkczVTaTUzOGdhekgzZ05JUzVzT0RhZE5SQTJ6dnZLRFRTS2hmd1g1R05XdHZiMG5tb0dIalFwOUo5RWxNeU9Vd0Jra6YAGfARZnpZblZVYXlKU2dlWDNYZEtDZ0I0dms3QlZVb3FzdW2mAP9HUElNeWV2UnlWb0tOb2doYmNkTVp1clNOakhFUzVsdE8wQmhZTUNUb0RPVDRhQmxMQnU0U2xWU2dVR1pkTG9yODBLdVpidTVDeFRsOWNlZmVGTlNFZlWmABjwEmd2N2pMQzNEUTNpM2d1VFZMRVZoR2FTdFI0UmFVSlZ3QaYA/0dNbWx5Y09PL3k4Wi9NRHJDVXc1OThuSVUwR1pseEFnWVgrLzNNRWk2VXZndURmbml2amRVTEhPN0wyeVJrTTloV3kzQ2gzbUtLeXFNdklNRzJXK1B5a6YAGPASaENYVUM1aVFDUkVlZm5SUFJGaHh3REpURWJ1Zmk0MUVMpgD/R0Z5VUJFRy9lTzVTb21hRFFaaWRvZnA3bjBzMGVxLzlzY1JBeFdwOHcrZmJiM0NuT1NmZmROM0NlTkh6SktZZ0JCbUs1YW5YdHZYa2tCWUNtVzcrdGlVpgALUDEuMMPD"};
TEST_F(test_system_contract_runtime, account_vm) {
    data::xblock_consensus_para_t cs_para;
    {
        cs_para.m_clock = 5796740;
        cs_para.m_total_lock_tgas_token = 0;
        cs_para.m_proposal_height = 7;
        cs_para.m_account = "Ta0001@0";
        cs_para.m_random_seed = base::xstring_utl::base64_decode("ODI3OTg4ODkxOTMzOTU3NDk3OA==");
    }
    xobject_ptr_t<xtransaction_t> raw_tx{make_object_ptr<xtransaction_t>()};
    {
        base::xstream_t stream(base::xcontext_t::instance(),(uint8_t*)tx_raw_encode.data(),(int32_t)tx_raw_encode.size());
        raw_tx->serialize_from(stream);
    }
    xobject_ptr_t<xcons_transaction_t> tx{make_object_ptr<xcons_transaction_t>()};
    {
        base::xstream_t stream(base::xcontext_t::instance(),(uint8_t*)tx_encode.data(),(int32_t)tx_encode.size());
        tx->serialize_from(stream);
        tx->m_tx = raw_tx;
        tx->m_tx->m_source_action.m_account_addr = "T00000LUuqEiWiVsKHTbCJTc2YqTeD6iZVsqmtks";
        tx->m_tx->m_target_action.m_account_addr = sys_contract_rec_standby_pool_addr;
        tx->m_tx->m_target_addr = sys_contract_rec_standby_pool_addr;
        tx->m_subtype = enum_transaction_subtype::enum_transaction_subtype_recv;
    }
    base::xvbstate_t state;
    {
        base::xstream_t stream(base::xcontext_t::instance(),(uint8_t*)state_encode.data(),(int32_t)state_encode.size());
        state.serialize_from(stream);
    }

    auto proposal_bstate = make_object_ptr<base::xvbstate_t>(state);
    std::vector<xcons_transaction_ptr_t> input_txs;
    input_txs.emplace_back(tx);
    auto temp_manager = make_unique<system::xsystem_contract_manager_t>();

    contract_vm::xaccount_vm_t vm(temp_manager);
    auto result = vm.execute(input_txs, proposal_bstate, cs_para);
}


NS_END3
