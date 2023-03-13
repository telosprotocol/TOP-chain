#pragma once
#include "tests/xevm_engine_test/evm_test_fixture/xmock_evm_statectx.h"
#include "tests/xevm_engine_test/evm_test_fixture/xmock_evm_storage.h"
#include "xbasic/xmemory.hpp"
#include "xevm/xevm.h"
#include "xevm_common/xevm_transaction_result.h"
#include "xevm_contract_runtime/xevm_context.h"
#include "xevm_contract_runtime/xevm_logic.h"
#include "xevm_contract_runtime/xevm_storage.h"
#include "xevm_runner/evm_engine_interface.h"
#include "xevm_runner/evm_import_instance.h"
#if defined(XCXX20)
#include "xevm_runner/proto/ubuntu/proto_basic.pb.h"
#include "xevm_runner/proto/ubuntu/proto_parameters.pb.h"
#else
#include "xevm_runner/proto/centos/proto_basic.pb.h"
#include "xevm_runner/proto/centos/proto_parameters.pb.h"
#endif
#include "xtxexecutor/xvm_face.h"

#include <nlohmann/json.hpp>

#include <gtest/gtest.h>

#include <fstream>
#include <iostream>

extern int evm_tests_argc;
extern char ** evm_tests_argv;

NS_BEG4(top, contract_runtime, evm, tests)

using namespace top::evm;
using namespace top::contract_runtime::evm;
using tests::xmock_evm_storage;

using json = nlohmann::json;

class xtest_evm_fixture : public testing::Test {
public:
    xtest_evm_fixture() = default;
    xtest_evm_fixture(xtest_evm_fixture const &) = delete;
    xtest_evm_fixture & operator=(xtest_evm_fixture const &) = delete;
    xtest_evm_fixture(xtest_evm_fixture &&) = default;
    xtest_evm_fixture & operator=(xtest_evm_fixture &&) = default;
    ~xtest_evm_fixture() override = default;

public:
    bool execute();
    bool execute_test_case(std::string const & json_file_path);

private:
    void init_env();
    void clean_env();

    bool do_deploy_test(json const & each_deploy);
    bool do_call_test(json const & each_call);

    xbytes_t get_contract_bin(std::string const & code_file_path);
    bool expected_logs(std::vector<evm_common::xevm_log_t> const & result_logs, json const & expected_json);

    void mock_add_balance(common::xaccount_address_t const & account, std::string token_symbol, evm_common::u256 amount);
    void mock_add_approve(common::xaccount_address_t const & owner, common::xaccount_address_t const & spender, std::string const & symbol, evm_common::u256 amount);

protected:
    void SetUp() override {
        Test::SetUp();
    }

    void TearDown() override {
        Test::TearDown();
    }

private:
    // std::string directory_path{"../tests/xevm_engine_test/test_cases/"};  // todo need to pass from args[1]
    std::string current_json_file_directory{""};

    using account_id = std::string;
    std::map<std::string, account_id> deployed_contract_map;
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0, 0, 0, eth_miner_zero_address};
    std::shared_ptr<top::evm::tests::xmock_evm_statectx> statestore{std::make_shared<top::evm::tests::xmock_evm_statectx>()};

    struct summary_infos {
        std::size_t json_files_num{0};
        std::size_t deploy_cases_num{0};
        std::size_t call_cases_num{0};
        std::size_t succ_deploy_cases{0};
        std::size_t succ_call_cases{0};

        void dump() {
            xkinfo("[evm_fixture dump]: deceted json_file number(%zu)  - Dect number: deploy cases(%zu), call cases(%zu).  - Succ number: deploy cases(%zu), call cases(%zu)",
                   json_files_num,
                   deploy_cases_num,
                   call_cases_num,
                   succ_deploy_cases,
                   succ_call_cases);
            printf("[evm_fixture dump]: deceted json_file number(%zu)\n  - Dect number: deploy cases(%zu), call cases(%zu).\n  - Succ number: deploy cases(%zu), call cases(%zu)\n",
                   json_files_num,
                   deploy_cases_num,
                   call_cases_num,
                   succ_deploy_cases,
                   succ_call_cases);
        }
    };

    summary_infos m_summary;
};

NS_END4
