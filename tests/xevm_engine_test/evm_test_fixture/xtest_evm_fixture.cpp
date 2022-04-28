
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
    m_summary.json_files_num = res.size();
    for (auto p : res) {
        std::cout << "---- begin test_file: " << p << std::endl;
        try {
            execute_test_case(p);
        } catch (...) {
        }
        std::cout << "---- finish test_file: " << p << std::endl;
        clean_env();
    }

    m_summary.dump();
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
    auto deploy_list = j["deploy_contract"];
    auto testcases_list = j["test_cases"];
    m_summary.deploy_cases_num += deploy_list.size();
    m_summary.call_cases_num += testcases_list.size();

    std::map<account_id, std::string> pre_data = j.at("pre_data").get<std::map<account_id, std::string>>();
    if (!pre_data.empty()) {
        auto storage = std::make_shared<contract_runtime::evm::xevm_storage>(statestore);
        std::unique_ptr<top::evm::xevm_logic_face_t> logic_ptr = top::make_unique<top::contract_runtime::evm::xevm_logic_t>(
            storage, nullptr, top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance()));
        top::evm::evm_import_instance::instance()->set_evm_logic(std::move(logic_ptr));
        for (auto _each : pre_data) {
            evm_common::u256 mock_value_256{_each.second};
            mock_add_balance(_each.first, mock_value_256);
        }
    }

    // std::cout << deploy_list << std::endl;
    for (auto each_deploy : deploy_list) {
        if (!each_deploy.empty()) {
            do_deploy_test(each_deploy);
        }
    }

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
    std::string value = each_deploy["value"];
    evm_common::u256 value_256{value};
    std::cout << "get value_256:" << value_256 << std::endl;

    auto expected = each_deploy["expected"];
    std::string contract_name_symbol = expected["extra_message"];

    vm_param.set_evm_gas_limit(gas_limit);

    auto evm_action = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::evm>>(
        common::xaccount_address_t{src_address}, common::xaccount_address_t{"T600040000000000000000000000000000000000000000"}, value_256, get_contract_bin(code_file));

    auto contract_manager = top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance());

    top::evm::xtop_evm evm{contract_manager, statestore};
    auto action_result = evm.execute_action(std::move(evm_action), vm_param);

    account_id contract_address = action_result.extra_msg;
    deployed_contract_map[contract_name_symbol] = contract_address;

    uint32_t expected_result = expected["status"];
    EXPECT_EQ(action_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    EXPECT_EQ(action_result.used_gas, expected_gas_used);

    // std::string expected_extra_msg = expected["extra_message"];
    // EXPECT_EQ(action_result.extra_msg, expected_extra_msg);

    // std::cout << "tx output: " << action_result.dump_info() << std::endl;
    EXPECT_TRUE(expected_logs(expected["logs"], action_result.logs));

    m_summary.succ_deploy_cases += 1;
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
    std::string value = each_call["value"];
    evm_common::u256 value_256{value};
    std::cout << "get value_256:" << value_256 << std::endl;
    std::string input_data = each_call["data"];

    vm_param.set_evm_gas_limit(gas_limit);

    auto evm_action = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::evm>>(
        common::xaccount_address_t{src_address}, common::xaccount_address_t{evm_to_top_address(target_address)}, value_256, xvariant_bytes{input_data, true}.to_bytes());

    auto contract_manager = top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance());

    top::evm::xtop_evm evm{contract_manager, statestore};
    auto action_result = evm.execute_action(std::move(evm_action), vm_param);

    auto expected = each_call["expected"];
    uint32_t expected_result = expected["status"];
    EXPECT_EQ(action_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    EXPECT_EQ(action_result.used_gas, expected_gas_used);

    std::string expected_extra_msg = expected["extra_message"];
    if (!expected_extra_msg.empty()) {
        EXPECT_EQ(action_result.extra_msg, expected_extra_msg);
    }

    EXPECT_TRUE(expected_logs(expected["logs"], action_result.logs));

    m_summary.succ_call_cases += 1;
    return true;
}

xbytes_t xtest_evm_fixture::get_contract_bin(std::string const & code_file_path) {
    std::ifstream code_file_stream(current_json_file_directory + code_file_path);
    std::string bytecode_hex_string;
    code_file_stream >> bytecode_hex_string;

    return xvariant_bytes{bytecode_hex_string, true}.to_bytes();
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

void xtest_evm_fixture::mock_add_balance(std::string account, evm_common::u256 amount) {
    assert(account.substr(0, 6) == "T60004");
    std::string eth_address = account.substr(6);
    assert(eth_address.size() == 40);

    auto u64_4 = amount.convert_to<uint64_t>();
    amount >>= 64;
    auto u64_3 = amount.convert_to<uint64_t>();
    amount >>= 64;
    auto u64_2 = amount.convert_to<uint64_t>();
    amount >>= 64;
    auto u64_1 = amount.convert_to<uint64_t>();

    do_mock_add_balance(eth_address.c_str(), eth_address.size(), u64_1, u64_2, u64_3, u64_4);
}
NS_END4