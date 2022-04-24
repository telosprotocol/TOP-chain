
#include "tests/xevm_engine_test/evm_test_fixture/xtest_evm_fixture.h"

#include <dirent.h>
#include <stdio.h>

#include <map>
#include <string>
#include <vector>

void find_json_files(const char * name, std::vector<std::string> & res, int indent = 0) {
    DIR * dir;
    struct dirent * entry;

    if (!(dir = opendir(name)))
        return;

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char path[1024];
            if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
                continue;
            snprintf(path, sizeof(path), "%s/%s", name, entry->d_name);
            printf("%*s[%s]\n", indent, "", entry->d_name);
            find_json_files(path, res, indent + 2);
        } else {
            std::string file_name{entry->d_name};
            if (file_name.size() >= 6 && file_name.substr(file_name.size() - 5) == ".json") {
                res.push_back(std::string{name} + "/" + file_name);
                printf("%*s- %s\n", indent, "", entry->d_name);
                // printf("find:%s/%s %s", name, entry->d_name, file_name.c_str());
            }
        }
    }
    closedir(dir);
    return;
}

NS_BEG4(top, contract_runtime, evm, tests)

void xtest_evm_fixture::clean_env() {
    deployed_contract_map.clear();
    static_cast<top::evm::tests::xmock_evm_statectx *>(statestore.get())->m_mock_bstate.clear();
}

bool xtest_evm_fixture::execute() {
    if (evm_tests_argc <= 1) {
        std::cout << "no enough args. Please set test_cases directory!" << std::endl;
        return false;
    }
    // find_json_files(evm_tests_argv[1]);
    std::vector<std::string> res;
    std::cout << "-- find .json files:" << std::endl;
    find_json_files(evm_tests_argv[1], res);
    std::cout << "-- " << res.size() << " files found" << std::endl;
    for (auto p : res) {
        std::cout << "---- begin test_file: " << p << std::endl;
        try {
            execute_test_case(p);
        } catch (...) {
        }
        std::cout << "---- finish test_file: " << p << std::endl;
        clean_env();
    }

    return true;
}

bool xtest_evm_fixture::execute_test_case(std::string const & json_file_path) {
    // std::cout << evm_tests_argc << std::endl;
    // std::cout << evm_tests_argv[1] << std::endl;

    std::ifstream i(json_file_path);
    // std::cout << "json_file_path: " << json_file_path << std::endl;

    if (json_file_path.find("/") != std::string::npos) {
        current_json_file_directory = json_file_path.substr(0, json_file_path.find_last_of("/") + 1);
        // if (!current_json_file_directory.empty()) {
        //     current_json_file_directory = current_json_file_directory + "/";
        // }
    } else {
        current_json_file_directory = "";
    }

    // std::cout << "current_json_file_directory: " << current_json_file_directory << std::endl;

    json j;
    i >> j;
    // std::cout << j << std::endl;

    // auto pre_data = j["pre_data"];
    std::map<account_id, uint64_t> pre_data = j.at("pre_data").get<std::map<account_id, uint64_t>>();
    if (!pre_data.empty()) {
        for (auto _each : pre_data) {
            mock_add_balance(_each.first, _each.second);
        }
    }

    auto deploy_list = j["deploy_contract"];
    // std::cout << deploy_list << std::endl;
    for (auto each_deploy : deploy_list) {
        if (!each_deploy.empty()) {
            do_deploy_test(each_deploy);
        }
    }

    auto testcases_list = j["test_cases"];
    // std::cout << testcases_list << std::endl;
    for (auto each_call : testcases_list) {
        if (!each_call.empty()) {
            do_call_test(each_call);
        }
    }

    return true;
}

bool xtest_evm_fixture::do_deploy_test(json const & each_deploy) {
    std::cout << "[deploy_contract]:" << each_deploy << std::endl;
    account_id src_address = each_deploy["src_address"];
    std::string code_file = each_deploy["code_file_path"];
    uint64_t gas_limit = each_deploy["gas_limit"];
    uint64_t value = each_deploy["value"];

    auto expected = each_deploy["expected"];
    std::string contract_name_symbol = expected["extra_message"];

    vm_param.set_evm_gas_limit(gas_limit);

    top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
    tx->set_source_addr(src_address);
    tx->set_target_addr("T600040000000000000000000000000000000000000000");  // deploy code
    tx->set_ext(get_contract_bin(code_file));
    tx->set_amount(value);
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

    txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{nullptr, statestore};

    auto ret = evm.execute(input, output);
    EXPECT_EQ(ret, txexecutor::enum_exec_success);
    EXPECT_EQ(output.m_vm_error_code, 0);

    account_id contract_address = output.m_tx_result.extra_msg;
    deployed_contract_map[contract_name_symbol] = contract_address;

    uint32_t expected_result = expected["status"];
    EXPECT_EQ(output.m_tx_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    EXPECT_EQ(output.m_tx_result.used_gas, expected_gas_used);

    // std::string expected_extra_msg = expected["extra_message"];
    // EXPECT_EQ(output.m_tx_result.extra_msg, expected_extra_msg);

    // std::cout << "tx output: " << output.m_tx_result.dump_info() << std::endl;
    EXPECT_TRUE(expected_logs(expected["logs"], output.m_tx_result.logs));

    return true;
}
bool xtest_evm_fixture::do_call_test(json const & each_call) {
    std::cout << "[call_contract]:" << each_call << std::endl;
    if (each_call.empty()) {
        return false;
    }

    account_id src_address = each_call["src_address"];

    std::string contract_name_symbol = each_call["target_address"];
    EXPECT_TRUE(deployed_contract_map.find(contract_name_symbol) != deployed_contract_map.end());
    account_id target_address = deployed_contract_map[contract_name_symbol];

    uint64_t gas_limit = each_call["gas_limit"];
    uint64_t value = each_call["value"];
    std::string input_data = each_call["data"];

    vm_param.set_evm_gas_limit(gas_limit);

    top::data::xtransaction_ptr_t tx = top::make_object_ptr<top::data::xtransaction_v2_t>();
    tx->set_source_addr(src_address);
    tx->set_target_addr(evm_to_top_address(target_address));  // deploy code
    tx->set_ext(xvariant_bytes{input_data, true}.to_string());
    tx->set_amount(value);
    auto cons_tx = top::make_object_ptr<top::data::xcons_transaction_t>(tx.get());

    txexecutor::xvm_input_t input{statestore, vm_param, cons_tx};
    txexecutor::xvm_output_t output;
    top::evm::xtop_evm evm{nullptr, statestore};

    auto ret = evm.execute(input, output);
    // std::cout << "tx output: " << output.m_tx_result.dump_info() << std::endl;
    EXPECT_EQ(ret, txexecutor::enum_exec_success);
    EXPECT_EQ(output.m_vm_error_code, 0);

    auto expected = each_call["expected"];
    uint32_t expected_result = expected["status"];
    EXPECT_EQ(output.m_tx_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    EXPECT_EQ(output.m_tx_result.used_gas, expected_gas_used);

    std::string expected_extra_msg = expected["extra_message"];
    if (!expected_extra_msg.empty()) {
        EXPECT_EQ(output.m_tx_result.extra_msg, expected_extra_msg);
    }

    EXPECT_TRUE(expected_logs(expected["logs"], output.m_tx_result.logs));
    return true;
}

std::string xtest_evm_fixture::get_contract_bin(std::string const & code_file_path) {
    std::ifstream code_file_stream(current_json_file_directory + code_file_path);
    std::string bytecode_hex_string;
    code_file_stream >> bytecode_hex_string;

    return xvariant_bytes{bytecode_hex_string, true}.to_string();
}

bool xtest_evm_fixture::expected_logs(json const & expected_json, std::vector<evm_common::xevm_log_t> const & result_logs) {
    // std::cout << expected_json << std::endl;
    if (expected_json.empty()) {
        EXPECT_TRUE(result_logs.empty());
    } else {
        EXPECT_TRUE(result_logs.size() == expected_json.size());
        if (result_logs.size() == expected_json.size()) {
            auto index = 0;
            for (auto _log : expected_json) {
                auto res_log = result_logs[index];

                std::string contract_name_symbol = _log["address"];
                EXPECT_TRUE(deployed_contract_map.find(contract_name_symbol) != deployed_contract_map.end());
                EXPECT_EQ(deployed_contract_map[contract_name_symbol], res_log.address);

                std::vector<std::string> expected_topics;
                for (auto _each_topic : _log["topics"]) {
                    expected_topics.push_back(_each_topic);
                }
                std::string expected_data = _log["data"];

                EXPECT_EQ(expected_topics, res_log.topics);
                EXPECT_EQ(expected_data, res_log.data);

                index++;
            }
        }
    }
    return true;
}

void xtest_evm_fixture::mock_add_balance(std::string account, uint64_t amount) {
    assert(account.substr(0, 6) == "T60004");
    std::string eth_address = account.substr(6);
    assert(eth_address.size() == 40);
    do_mock_add_balance(eth_address.c_str(), eth_address.size(), amount);
}
NS_END4