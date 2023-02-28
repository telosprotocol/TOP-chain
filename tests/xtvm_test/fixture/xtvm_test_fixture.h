#pragma once

#include "gtest/gtest.h"
#include "tests/xtvm_test/fixture/xmock_statectx.h"
#include "xtvm_runtime/xtvm.h"

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include <fstream>
#include <iostream>

extern int tvm_tests_argc;
extern char ** tvm_tests_argv;

NS_BEG3(top, tvm, tests)
class xtest_tvm_fixture : public testing::Test {
public:
    xtest_tvm_fixture() = default;
    xtest_tvm_fixture(xtest_tvm_fixture const &) = delete;
    xtest_tvm_fixture & operator=(xtest_tvm_fixture const &) = delete;
    xtest_tvm_fixture(xtest_tvm_fixture &&) = default;
    xtest_tvm_fixture & operator=(xtest_tvm_fixture &&) = default;
    ~xtest_tvm_fixture() override = default;

public:
    bool execute();
    bool execute_test_case(std::string const & json_file_path);

protected:
    void SetUp() override {
        Test::SetUp();
    }
    void TearDown() override {
        Test::TearDown();
    }

private:
    bool do_deploy_test(json const & each_deploy);
    bool do_call_test(json const & each_call);

    bool check_logs(std::vector<top::evm_common::xevm_log_t> const & result_logs, json const & expected_json);

private:
    std::string current_json_file_directory{""};
    using eth_address_str = std::string;
    std::map<std::string, eth_address_str> deployed_contract_map;
    std::shared_ptr<xmock_statectx> statestore{std::make_shared<xmock_statectx>()};
    txexecutor::xvm_para_t vm_param{0, "random_seed", 0, 0, 0, eth_zero_address};

    struct summary_infos {
        std::size_t json_files_num{0};
        std::size_t deploy_cases_num{0};
        std::size_t call_cases_num{0};
        std::size_t succ_deploy_cases{0};
        std::size_t succ_call_cases{0};

        void dump() {
            xkinfo("[tvm_fixture dump]: deceted json_file number(%zu)  - Dect number: deploy cases(%zu), call cases(%zu).  - Succ number: deploy cases(%zu), call cases(%zu)",
                   json_files_num,
                   deploy_cases_num,
                   call_cases_num,
                   succ_deploy_cases,
                   succ_call_cases);
            printf("[tvm_fixture dump]: deceted json_file number(%zu)\n  - Dect number: deploy cases(%zu), call cases(%zu).\n  - Succ number: deploy cases(%zu), call cases(%zu)\n",
                   json_files_num,
                   deploy_cases_num,
                   call_cases_num,
                   succ_deploy_cases,
                   succ_call_cases);
        }
    };

    summary_infos m_summary;
};

NS_END3