#if 0
#include "evm_test_fixture/xmock_evm_statectx.h"
#include "tests/xevm_engine_test/evm_test_fixture/xmock_evm_storage.h"
#include "xbasic/xmemory.hpp"
#include "xdata/xtransaction_v2.h"
#include "xevm/xevm.h"
#include "xevm_common/xevm_transaction_result.h"
#include "xevm_contract_runtime/xevm_context.h"
#include "xevm_contract_runtime/xevm_logic.h"
#include "xevm_contract_runtime/xevm_type.h"
#include "xevm_contract_runtime/xevm_variant_bytes.h"
#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
#include "xevm_runner/proto/proto_basic.pb.h"
#include "xevm_runner/proto/proto_parameters.pb.h"
#include "xtxexecutor/xvm_face.h"
#include "xdata/xnative_contract_address.h"

#include <gtest/gtest.h>

NS_BEG4(top, contract_runtime, evm, tests)

using namespace top::evm;
using namespace top::contract_runtime::evm;
using tests::xmock_evm_storage;

void mock_add_balance(std::string account, uint64_t amount) {
    assert(account.substr(0, 6) == "T60004");
    std::string eth_address = account.substr(6);
    assert(eth_address.size() == 40);
    do_mock_add_balance(eth_address.c_str(), eth_address.size(), amount);
}

TEST(evm_engine_balance_test, OufOfFund) {
    std::string contract_address;
    xvariant_bytes contract_code{
        "0x608060405234801561001057600080fd5b5061024e806100206000396000f3fe60806040526004361061002d5760003560e01c80632565b1b8146100b3578063ad7a672f146100ee576100ae565b366100ae5734"
        "60008082825401925050819055507fe1fffcc4923d04b559f4d29a8bfc6cda04eb5b0d3c460751c2402c5c5cc9109c3334604051808373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffff"
        "ffffffffffffffffffffffff1681526020018281526020019250505060405180910390a1005b600080fd5b3480156100bf57600080fd5b506100ec600480360360208110156100d657600080fd5b81019080803590"
        "60200190929190505050610119565b005b3480156100fa57600080fd5b50610103610212565b6040518082815260200191505060405180910390f35b80600080828254039250508190555060003373ffffffffffff"
        "ffffffffffffffffffffffffffff1661271083604051806000019050600060405180830381858888f193505050503d806000811461018c576040519150601f19603f3d011682016040523d82523d6000602084013e"
        "610191565b606091505b5050809150507f171a466754afbbdce4dc1ab85f822d6767825c31a83b1113cc18bc97ddbfed2281338460405180841515151581526020018373ffffffffffffffffffffffffffffffffff"
        "ffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001828152602001935050505060405180910390a15050565b6000548156fea26469706673582212201285a1a792cec99fd557c4fb8b1f92"
        "dccf09d34d37da99fe7de2b8526427bf3f64736f6c63430006040033",
        true};
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0};
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(evm_zero_address.value());  // deploy code
        tx->set_ext(contract_code.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        contract_address = evm_to_top_address(output.m_tx_result.extra_msg);
        std::cout << "UT print: "
                  << "contract_address: " << output.m_tx_result.extra_msg << std::endl;
        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // not add balance.
    // mock_add_balance("T60004001bdc8251890aafc5841b05620c0eab336e3ebc", 200);
    // deposit
    {
        // xvariant_bytes contract_params{"0x", true};
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(contract_address);  // call contract
        // tx->set_ext(contract_params.to_string());
        tx->set_amount(100);
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::OutOfFund);
    }
}

TEST(evm_engine_balance_test, success) {
    std::string contract_address;
    xvariant_bytes contract_code{
        "0x608060405234801561001057600080fd5b5061024e806100206000396000f3fe60806040526004361061002d5760003560e01c80632565b1b8146100b3578063ad7a672f146100ee576100ae565b366100ae5734"
        "60008082825401925050819055507fe1fffcc4923d04b559f4d29a8bfc6cda04eb5b0d3c460751c2402c5c5cc9109c3334604051808373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffff"
        "ffffffffffffffffffffffff1681526020018281526020019250505060405180910390a1005b600080fd5b3480156100bf57600080fd5b506100ec600480360360208110156100d657600080fd5b81019080803590"
        "60200190929190505050610119565b005b3480156100fa57600080fd5b50610103610212565b6040518082815260200191505060405180910390f35b80600080828254039250508190555060003373ffffffffffff"
        "ffffffffffffffffffffffffffff1661271083604051806000019050600060405180830381858888f193505050503d806000811461018c576040519150601f19603f3d011682016040523d82523d6000602084013e"
        "610191565b606091505b5050809150507f171a466754afbbdce4dc1ab85f822d6767825c31a83b1113cc18bc97ddbfed2281338460405180841515151581526020018373ffffffffffffffffffffffffffffffffff"
        "ffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001828152602001935050505060405180910390a15050565b6000548156fea26469706673582212201285a1a792cec99fd557c4fb8b1f92"
        "dccf09d34d37da99fe7de2b8526427bf3f64736f6c63430006040033",
        true};
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0};
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(evm_zero_address.value());  // deploy code
        tx->set_ext(contract_code.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        contract_address = evm_to_top_address(output.m_tx_result.extra_msg);
        std::cout << "UT print: "
                  << "contract_address: " << output.m_tx_result.extra_msg << std::endl;
        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    mock_add_balance("T60004001bdc8251890aafc5841b05620c0eab336e3ebc", 2000);
    // deposit
    {
        // xvariant_bytes contract_params{"0x", true};
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(contract_address);  // call contract
        // tx->set_ext(contract_params.to_string());
        tx->set_amount(1000);
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // contract.totalBalance.getData()
    {
        xvariant_bytes contract_params{"0xad7a672f", true};
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(contract_address);  // call contract
        tx->set_ext(contract_params.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_tx_result.extra_msg, "0x00000000000000000000000000000000000000000000000000000000000003e8");  // 1000 in hex
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // contract.withdraw_balance.getData(666)
    {
        xvariant_bytes contract_params{"0x2565b1b8000000000000000000000000000000000000000000000000000000000000029a", true};
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(contract_address);  // call contract
        tx->set_ext(contract_params.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // contract.totalBalance.getData()
    {
        xvariant_bytes contract_params{"0xad7a672f", true};
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(contract_address);  // call contract
        tx->set_ext(contract_params.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_tx_result.extra_msg, "0x000000000000000000000000000000000000000000000000000000000000014e");  // 666 in hex
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }
}

NS_END4
#endif