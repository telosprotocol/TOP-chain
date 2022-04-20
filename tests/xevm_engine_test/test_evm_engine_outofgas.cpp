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

#include <gtest/gtest.h>

NS_BEG4(top, contract_runtime, evm, tests)

using namespace top::evm;
using namespace top::contract_runtime::evm;
using tests::xmock_evm_storage;

TEST(evm_engine_out_of_gas_test, deploy_failed) {
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
    vm_param.set_evm_gas_limit(100000);
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr("T600040000000000000000000000000000000000000000");  // deploy code
        tx->set_ext(contract_code.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        contract_address = "T60004" + output.m_tx_result.extra_msg;
        std::cout << "UT print: "
                  << "contract_address: " << output.m_tx_result.extra_msg << std::endl;
        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::OutOfGas);
        ASSERT_EQ(output.m_tx_result.used_gas, 100000);
    }

    vm_param.set_evm_gas_limit(150000);
    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr("T600040000000000000000000000000000000000000000");  // deploy code
        tx->set_ext(contract_code.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        contract_address = "T60004" + output.m_tx_result.extra_msg;
        std::cout << "UT print: "
                  << "contract_address: " << output.m_tx_result.extra_msg << std::endl;
        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::OutOfGas);
        ASSERT_EQ(output.m_tx_result.used_gas, 150000);
    }

    vm_param.set_evm_gas_limit(200000);
    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr("T600040000000000000000000000000000000000000000");  // deploy code
        tx->set_ext(contract_code.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        contract_address = "T60004" + output.m_tx_result.extra_msg;
        std::cout << "UT print: "
                  << "contract_address: " << output.m_tx_result.extra_msg << std::endl;
        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
        ASSERT_TRUE(output.m_tx_result.used_gas > 150000 && output.m_tx_result.used_gas < vm_param.get_evm_gas_limit());
    }
}

TEST(evm_engine_out_of_gas_test, deploy_success_call_failed) {
    std::string contract_address;
    xvariant_bytes contract_code{
        "608060405234801561001057600080fd5b50610304806100206000396000f3fe608060405234801561001057600080fd5b506004361061004c5760003560e01c80632fb3c740146100515780636e2c732d1461005b"
        "578063a05f9906146100a7578063fad772db146100d9575b600080fd5b610059610111565b005b6100a56004803603604081101561007157600080fd5b81019080803567ffffffffffffffff169060200190929190"
        "803567ffffffffffffffff1690602001909291905050506101b3565b005b6100af610242565b604051808267ffffffffffffffff1667ffffffffffffffff16815260200191505060405180910390f35b61010f6004"
        "80360360208110156100ef57600080fd5b81019080803567ffffffffffffffff16906020019092919050505061025b565b005b60016000809054906101000a900467ffffffffffffffff16016000806101000a8154"
        "8167ffffffffffffffff021916908367ffffffffffffffff1602179055507f4558e87569f3778927a0c0ab7e6c04d9cc3a08694412ac6b8b8bbd8276c8fb3d6000809054906101000a900467ffffffffffffffff16"
        "604051808267ffffffffffffffff1667ffffffffffffffff16815260200191505060405180910390a1565b600081830190507fba81e10edd752f92a850d20e6ca5897bc0ff54393985bc25b2c2bdc9252186498383"
        "83604051808467ffffffffffffffff1667ffffffffffffffff1681526020018367ffffffffffffffff1667ffffffffffffffff1681526020018267ffffffffffffffff1667ffffffffffffffff1681526020019350"
        "50505060405180910390a1505050565b6000809054906101000a900467ffffffffffffffff1681565b60006001820190507f76b87589c0efe817c6ec312c8fa2ab35ac24bbbd1e5fb8d3e3c3b4b789fdc7d4828260"
        "4051808367ffffffffffffffff1667ffffffffffffffff1681526020018267ffffffffffffffff1667ffffffffffffffff1681526020019250505060405180910390a1505056fea2646970667358221220aa046f63"
        "4f0927440a2dc3e5b0298f8101a60505f1d303bc416a90fcc0db54fa64736f6c63430006040033",
        true};
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0};
    vm_param.set_evm_gas_limit(300000);
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr("T600040000000000000000000000000000000000000000");  // deploy code
        tx->set_ext(contract_code.to_string());
        auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

        txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
        txexecutor::xvm_output_t output;
        top::evm::xtop_evm evm{nullptr, statestore};

        auto ret = evm.execute(input, output);

        contract_address = "T60004" + output.m_tx_result.extra_msg;
        std::cout << "UT print: "
                  << "contract_address: " << output.m_tx_result.extra_msg << std::endl;
        std::cout << "UT print: " << output.m_tx_result.dump_info() << std::endl;

        ASSERT_EQ(ret, txexecutor::enum_exec_success);
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
        ASSERT_TRUE(output.m_tx_result.used_gas < vm_param.get_evm_gas_limit());
    }

    // no enough to call
    vm_param.set_evm_gas_limit(21000);
    // call contract:
    // add(123, 321) => (123,321,444)
    {
        xvariant_bytes contract_params{"0x6e2c732d000000000000000000000000000000000000000000000000000000000000007b0000000000000000000000000000000000000000000000000000000000000141",
                                       true};
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
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::OutOfGas);
        ASSERT_EQ(output.m_tx_result.used_gas, vm_param.get_evm_gas_limit());
    }

    // addOne(12345) => (12346)
    {
        xvariant_bytes contract_params{"0xfad772db0000000000000000000000000000000000000000000000000000000000003039", true};
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
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::OutOfGas);
        ASSERT_EQ(output.m_tx_result.used_gas, vm_param.get_evm_gas_limit());
    }

    // addGlobal => (1)
    {
        xvariant_bytes contract_params{"0x2fb3c740", true};
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
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::OutOfGas);
        ASSERT_EQ(output.m_tx_result.used_gas, vm_param.get_evm_gas_limit());
    }

    // enough to call
    vm_param.set_evm_gas_limit(50000);
    // call contract:
    // add(123, 321) => (123,321,444)
    {
        xvariant_bytes contract_params{"0x6e2c732d000000000000000000000000000000000000000000000000000000000000007b0000000000000000000000000000000000000000000000000000000000000141",
                                       true};
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
        ASSERT_TRUE(output.m_tx_result.used_gas < vm_param.get_evm_gas_limit() && output.m_tx_result.used_gas > 21000);
    }

    // addOne(12345) => (12346)
    {
        xvariant_bytes contract_params{"0xfad772db0000000000000000000000000000000000000000000000000000000000003039", true};
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
        ASSERT_TRUE(output.m_tx_result.used_gas < vm_param.get_evm_gas_limit() && output.m_tx_result.used_gas > 21000);
    }

    // addGlobal => (1)
    {
        xvariant_bytes contract_params{"0x2fb3c740", true};
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
        ASSERT_TRUE(output.m_tx_result.used_gas < vm_param.get_evm_gas_limit() && output.m_tx_result.used_gas > 21000);
    }
}

NS_END4