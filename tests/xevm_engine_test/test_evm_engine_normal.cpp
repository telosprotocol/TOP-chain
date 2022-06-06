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

TEST(evm_engine_normal_test, deploy_and_call_contract_success_0) {
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
        "4051808367ffffffffffffffff1667ffffffffffffffff1681526020018267ffffffffffffffff1667ffffffffffffffff1681526020019250505060405180910390a1505056fea2646970667358221220b81251e6"
        "353d97dcc679b08a746d7d8f00f5b7051bdf6c1793d3f6f3a9e361fa64736f6c63430006040033",
        true};
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0, 0};
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(eth_zero_address.value());  // deploy code
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
    }
}

TEST(evm_engine_normal_test, deploy_and_call_contract_success_1) {
    std::string contract_address;
    // ./solidity_contracts/erc20.sol
    xvariant_bytes contract_code{
        "60806040526040518060400160405280600781526020017f4d794572633230000000000000000000000000000000000000000000000000008152506003908051906020019061004f92919061016a565b50604051"
        "8060400160405280600381526020017f53594d00000000000000000000000000000000000000000000000000000000008152506004908051906020019061009b92919061016a565b506012600560006101000a8154"
        "8160ff021916908360ff1602179055503480156100c457600080fd5b50620186a0600081905550600054600160003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffff"
        "ffffffffff168152602001908152602001600020819055503373ffffffffffffffffffffffffffffffffffffffff167f60226e015dfa2f4684230d052fa02b7297d54471129f02a9585f4f4a81e9e0d26000546040"
        "518082815260200191505060405180910390a261020f565b828054600181600116156101000203166002900490600052602060002090601f016020900481019282601f106101ab57805160ff191683800117855561"
        "01d9565b828001600101855582156101d9579182015b828111156101d85782518255916020019190600101906101bd565b5b5090506101e691906101ea565b5090565b61020c91905b808211156102085760008160"
        "009055506001016101f0565b5090565b90565b610a5b8061021e6000396000f3fe608060405234801561001057600080fd5b506004361061009e5760003560e01c806342966c681161006657806342966c68146102"
        "5457806370a082311461028257806395d89b41146102da578063a9059cbb1461035d578063dd62ed3e146103c35761009e565b806306fdde03146100a3578063095ea7b31461012657806318160ddd1461018c5780"
        "6323b872dd146101aa578063313ce56714610230575b600080fd5b6100ab61043b565b6040518080602001828103825283818151815260200191508051906020019080838360005b838110156100eb578082015181"
        "8401526020810190506100d0565b50505050905090810190601f1680156101185780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b610172600480360360408110"
        "1561013c57600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803590602001909291905050506104d9565b604051808215151515815260200191505060405180"
        "910390f35b6101946105cb565b6040518082815260200191505060405180910390f35b610216600480360360608110156101c057600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16"
        "9060200190929190803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803590602001909291905050506105d1565b604051808215151515815260200191505060405180910390f35b61"
        "0238610767565b604051808260ff1660ff16815260200191505060405180910390f35b6102806004803603602081101561026a57600080fd5b810190808035906020019092919050505061077a565b005b6102c460"
        "04803603602081101561029857600080fd5b81019080803573ffffffffffffffffffffffffffffffffffffffff16906020019092919050505061083f565b6040518082815260200191505060405180910390f35b61"
        "02e2610857565b6040518080602001828103825283818151815260200191508051906020019080838360005b83811015610322578082015181840152602081019050610307565b50505050905090810190601f1680"
        "1561034f5780820380516001836020036101000a031916815260200191505b509250505060405180910390f35b6103a96004803603604081101561037357600080fd5b81019080803573ffffffffffffffffffffff"
        "ffffffffffffffffff169060200190929190803590602001909291905050506108f5565b604051808215151515815260200191505060405180910390f35b610425600480360360408110156103d957600080fd5b81"
        "019080803573ffffffffffffffffffffffffffffffffffffffff169060200190929190803573ffffffffffffffffffffffffffffffffffffffff169060200190929190505050610a00565b60405180828152602001"
        "91505060405180910390f35b60038054600181600116156101000203166002900480601f01602080910402602001604051908101604052809291908181526020018280546001816001161561010002031660029004"
        "80156104d15780601f106104a6576101008083540402835291602001916104d1565b820191906000526020600020905b8154815290600101906020018083116104b457829003601f168201915b505050505081565b"
        "600081600260003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008573ffffffffffffffffffffffffffffff"
        "ffffffffff1673ffffffffffffffffffffffffffffffffffffffff168152602001908152602001600020819055508273ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffff"
        "ffffffffffff167f8c5be1e5ebec7d5bd14f71427d1e84f3dd0314c0f7b2291e5b200ac8c7c3b925846040518082815260200191505060405180910390a36001905092915050565b60005481565b60008160026000"
        "8673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060003373ffffffffffffffffffffffffffffffffffffffff1673"
        "ffffffffffffffffffffffffffffffffffffffff1681526020019081526020016000206000828254039250508190555081600160008673ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffff"
        "ffffffffffffffffffffffff1681526020019081526020016000206000828254039250508190555081600160008573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffff"
        "ffffffff168152602001908152602001600020600082825401925050819055508273ffffffffffffffffffffffffffffffffffffffff168473ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2"
        "c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef846040518082815260200191505060405180910390a3600190509392505050565b600560009054906101000a900460ff1681565b80600160003373"
        "ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260200160002060008282540392505081905550806000808282540392505081905550"
        "600073ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f163c4a11628f55a4df523b3ef8360405180"
        "82815260200191505060405180910390a350565b60016020528060005260406000206000915090505481565b60048054600181600116156101000203166002900480601f0160208091040260200160405190810160"
        "405280929190818152602001828054600181600116156101000203166002900480156108ed5780601f106108c2576101008083540402835291602001916108ed565b820191906000526020600020905b8154815290"
        "600101906020018083116108d057829003601f168201915b505050505081565b600081600160003373ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff1681"
        "526020019081526020016000206000828254039250508190555081600160008573ffffffffffffffffffffffffffffffffffffffff1673ffffffffffffffffffffffffffffffffffffffff16815260200190815260"
        "2001600020600082825401925050819055508273ffffffffffffffffffffffffffffffffffffffff163373ffffffffffffffffffffffffffffffffffffffff167fddf252ad1be2c89b69c2b068fc378daa952ba7f1"
        "63c4a11628f55a4df523b3ef846040518082815260200191505060405180910390a36001905092915050565b600260205281600052604060002060205280600052604060002060009150915050548156fea2646970"
        "667358221220d85b6d67c18cbaefa92cadb028ffbb9d0d410e0960f7466456990c711ab8a77464736f6c63430006040033",
        true};
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0, 0};
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(eth_zero_address.value());  // deploy code
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

    // call contract:
    // erc.totalSupply.getData()
    {
        xvariant_bytes contract_params{"0x18160ddd", true};
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
        ASSERT_EQ(output.m_tx_result.extra_msg, "0x00000000000000000000000000000000000000000000000000000000000186a0");  // 100000 in hex
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // erc.balanceOf.getData("0000000000000000000000000000000000000123")
    {
        xvariant_bytes contract_params{"0x70a08231000000000000000000000000000000000000000000000000000000000000007b", true};
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
        ASSERT_EQ(output.m_tx_result.extra_msg, "0x0000000000000000000000000000000000000000000000000000000000000000");  // 0
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // erc.transfer.getData("0000000000000000000000000000000000000123",123)
    {
        xvariant_bytes contract_params{"0xa9059cbb000000000000000000000000000000000000000000000000000000000000007b000000000000000000000000000000000000000000000000000000000000007b",
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
        ASSERT_EQ(output.m_tx_result.extra_msg, "0x0000000000000000000000000000000000000000000000000000000000000001");  // true
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }

    // erc.balanceOf.getData("0000000000000000000000000000000000000123")
    {
        xvariant_bytes contract_params{"0x70a08231000000000000000000000000000000000000000000000000000000000000007b", true};
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
        ASSERT_EQ(output.m_tx_result.extra_msg, "0x000000000000000000000000000000000000000000000000000000000000007b");  // 123 in hex
        ASSERT_EQ(output.m_vm_error_code, 0);
        ASSERT_EQ(output.m_tx_result.status, evm_common::xevm_transaction_status_t::Success);
    }
}

TEST(evm_engine_normal_test, deploy_and_call_contract_success_2) {
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
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0, 0};
    top::statectx::xstatectx_face_ptr_t statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(eth_zero_address.value());  // deploy code
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
    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(eth_zero_address.value());  // deploy code
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
    // deploy code
    {
        // param
        top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
        tx->set_source_addr("T60004001bdc8251890aafc5841b05620c0eab336e3ebc");
        tx->set_target_addr(eth_zero_address.value());  // deploy code
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
}

NS_END4
#endif
