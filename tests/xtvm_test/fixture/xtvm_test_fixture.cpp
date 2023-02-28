#include "tests/xtvm_test/fixture/xtvm_test_fixture.h"

#include "xbasic/xhex.h"
#include "xdata/xconsensus_action.h"
#include "xtvm_runtime/xtvm.h"

#include <dirent.h>
#include <stdio.h>

#define EXPECT_TRUE_OR_RETURN_FALSE(expr)                                                                                                                                          \
    {                                                                                                                                                                              \
        EXPECT_TRUE(expr);                                                                                                                                                         \
        if ((expr) == false)                                                                                                                                                       \
            return false;                                                                                                                                                          \
    }

#define EXPECT_EQ_OR_RETURN_FALSE(lhs, rhs)                                                                                                                                        \
    {                                                                                                                                                                              \
        EXPECT_EQ(lhs, rhs);                                                                                                                                                       \
        if (lhs != rhs)                                                                                                                                                            \
            return false;                                                                                                                                                          \
    }

static std::string evm_to_t8_address(std::string const & input) {
    if (input.substr(0, 2) == "0x") {
        return "T80000" + input.substr(2);
    }
    if (input.substr(0, 6) == "T80000") {
        return input;
    }
    return "T80000" + input;
}

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

top::xbytes_t get_contract_bin(std::string const & current_json_file_directory, std::string const & code_file_path) {
    std::ifstream code_file_stream(current_json_file_directory + code_file_path);
    std::string bytecode_hex_string;
    code_file_stream >> bytecode_hex_string;

    std::error_code ec;
    auto code_bytes = top::from_hex(bytecode_hex_string, ec);
    EXPECT_TRUE(!ec);
    return code_bytes;
}



NS_BEG3(top, tvm, tests)

bool xtest_tvm_fixture::execute() {
    if (tvm_tests_argc <= 1) {
        std::cout << "no enough args. Please set test_cases directory!" << std::endl;
        return false;
    }

    std::vector<std::string> res;
    std::cout << "-- find .json files:" << std::endl;
    find_json_files(tvm_tests_argv[1], res);
    std::cout << "-- " << res.size() << " files found" << std::endl;
    m_summary.json_files_num = res.size();
    std::cout << "==================================" << std::endl;
    for (auto p : res) {
        std::cout << "-- [DO TEST] file: " << p << std::endl;
        //     init_env();
        execute_test_case(p);
        std::cout << "-- finish test_file: " << p << std::endl;
        //     clean_env();
        deployed_contract_map.clear();
        static_cast<top::tvm::tests::xmock_statectx *>(statestore.get())->m_mock_bstate.clear();
    }
    std::cout << "==================================" << std::endl;

    m_summary.dump();
    return true;
}

bool xtest_tvm_fixture::execute_test_case(std::string const & json_file_path) {
    std::ifstream i{json_file_path};

    if (json_file_path.find("/") != std::string::npos) {
        current_json_file_directory = json_file_path.substr(0, json_file_path.find_last_of("/") + 1);
    } else {
        current_json_file_directory = "";
    }

    json j;
    i >> j;
    auto pre_data = j["pre_data"];
    auto deploy_list = j["deploy_contract"];
    auto testcases_list = j["test_cases"];
    m_summary.deploy_cases_num += deploy_list.size();
    m_summary.call_cases_num += testcases_list.size();

    // todo pre_data

    std::size_t deploy_case_succ_num = 0;
    std::size_t call_case_succ_num = 0;
    for (auto each_deploy : deploy_list) {
        if (!each_deploy.empty()) {
            try {
                if (do_deploy_test(each_deploy)) {
                    deploy_case_succ_num++;
                } else {
                    std::cout << "\033[1;31m    -- deploy case failed --\033[0m" << std::endl;
                    std::cout << "    case input:\033[1;32m" << each_deploy << "\033[0m\n" << std::endl;
                }
            } catch (nlohmann::detail::type_error & e) {
                std::cout << "    catch json error: " << e.what() << std::endl;
                std::cout << "\033[1;31m    -- deploy case failed --\033[0m" << std::endl;
                std::cout << "    case input:\033[1;32m" << each_deploy << "\033[0m\n" << std::endl;
            } catch (const std::exception & e) {
                std::cout << "    catch exception: " << e.what() << std::endl;
                std::cout << "\033[1;31m    -- deploy case failed --\033[0m" << std::endl;
                std::cout << "    case input:\033[1;32m" << each_deploy << "\033[0m\n" << std::endl;
            } catch (...) {
                std::cout << "\033[1;31m    -- deploy case failed --\033[0m" << std::endl;
                std::cout << "    case input:\033[1;32m" << each_deploy << "\033[0m\n" << std::endl;
            }
        }
    }

    // std::cout << testcases_list << std::endl;
    for (auto each_call : testcases_list) {
        if (!each_call.empty()) {
            try {
                if (do_call_test(each_call)) {
                    call_case_succ_num++;
                } else {
                    std::cout << "\033[1;31m    -- call case failed --\033[0m" << std::endl;
                    std::cout << "    case input:\033[1;32m" << each_call << "\033[0m\n" << std::endl;
                }
            } catch (nlohmann::detail::type_error & e) {
                std::cout << "    catch json error: " << e.what() << std::endl;
                std::cout << "\033[1;31m    -- call case failed --\033[0m" << std::endl;
                std::cout << "    case input:\033[1;32m" << each_call << "\033[0m\n" << std::endl;
            } catch (const std::exception & e) {
                std::cout << "    catch exception: " << e.what() << std::endl;
                std::cout << "\033[1;31m    -- call case failed --\033[0m" << std::endl;
                std::cout << "    case input:\033[1;32m" << each_call << "\033[0m\n" << std::endl;
            } catch (...) {
                std::cout << "\033[1;31m    -- call case failed --\033[0m" << std::endl;
                std::cout << "    case input:\033[1;32m" << each_call << "\033[0m\n" << std::endl;
            }
        }
    }
    std::cout << "  -- succ deploy:" << deploy_case_succ_num << ", succ call:" << call_case_succ_num << std::endl;

    return true;
}

bool xtest_tvm_fixture::do_deploy_test(json const & each_deploy) {
    std::string src_address = each_deploy["src_address"];
    std::string code_file = each_deploy["code_file_path"];
    uint64_t gas_limit = each_deploy["gas_limit"];
    std::string value = each_deploy["value"];
    evm_common::u256 value_256{value};

    auto expected = each_deploy["expected"];
    std::string contract_name_symbol = expected["extra_message"];

    auto tvm_action = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::evm>>(
        common::xaccount_address_t{src_address}, eth_zero_address, value_256, get_contract_bin(current_json_file_directory, code_file), gas_limit);

    top::tvm::xtop_vm tvm{statestore};

    auto action_result = tvm.execute_action(std::move(tvm_action), vm_param);

    auto contract_address = action_result.extra_msg;
    std::cout << "[TEST fixture]: deploy contract: " << contract_name_symbol << ": " << contract_address << std::endl;
    deployed_contract_map[contract_name_symbol] = contract_address;

    // check
    uint32_t expected_result = expected["status"];
    EXPECT_EQ_OR_RETURN_FALSE(action_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    EXPECT_EQ_OR_RETURN_FALSE(action_result.used_gas, expected_gas_used);

    EXPECT_TRUE_OR_RETURN_FALSE(check_logs(action_result.logs, expected["logs"]));

    m_summary.succ_deploy_cases += 1;

    return true;
}

bool xtest_tvm_fixture::do_call_test(json const & each_call) {
    std::string src_address = each_call["src_address"];
    std::string contract_name_symbol = each_call["target_address"];
    EXPECT_TRUE_OR_RETURN_FALSE(deployed_contract_map.find(contract_name_symbol) != deployed_contract_map.end());

    std::string target_address = deployed_contract_map.at(contract_name_symbol);
    uint64_t gas_limit = each_call["gas_limit"];
    std::string value = each_call["value"];
    evm_common::u256 value_256{value};

    std::error_code ec;
    xbytes_t input_data = top::from_hex(each_call["data"], ec);
    EXPECT_TRUE(!ec);

    auto expected = each_call["expected"];

    auto tvm_action = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::evm>>(
        common::xaccount_address_t{src_address}, common::xaccount_address_t{evm_to_t8_address(target_address)}, value_256, input_data, gas_limit);

    top::tvm::xtop_vm tvm{statestore};

    auto action_result = tvm.execute_action(std::move(tvm_action), vm_param);

    // check
    uint32_t expected_result = expected["status"];
    EXPECT_EQ_OR_RETURN_FALSE(action_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    EXPECT_EQ_OR_RETURN_FALSE(action_result.used_gas, expected_gas_used);

    std::string expected_extra_msg = expected["extra_message"];
    if (!expected_extra_msg.empty()) {
        EXPECT_EQ_OR_RETURN_FALSE(action_result.extra_msg, expected_extra_msg);
    }

    EXPECT_TRUE_OR_RETURN_FALSE(check_logs(action_result.logs, expected["logs"]));

    m_summary.succ_call_cases += 1;

    return true;
}

bool xtest_tvm_fixture::check_logs(std::vector<top::evm_common::xevm_log_t> const & result_logs, json const & expected_json) {
    if (expected_json.empty()) {
        EXPECT_TRUE_OR_RETURN_FALSE(result_logs.empty());
    } else {
        EXPECT_EQ_OR_RETURN_FALSE(result_logs.size(), expected_json.size());
        if (result_logs.size() == expected_json.size()) {
            auto index = 0;
            for (auto _log : expected_json) {
                auto res_log = result_logs[index];

                std::string contract_name_symbol = _log["address"];
                EXPECT_TRUE_OR_RETURN_FALSE(deployed_contract_map.find(contract_name_symbol) != deployed_contract_map.end());
                EXPECT_EQ_OR_RETURN_FALSE(res_log.address.to_hex_string(), deployed_contract_map[contract_name_symbol]);

                std::vector<std::string> expected_topics;
                for (auto _each_topic : _log["topics"]) {
                    expected_topics.push_back(_each_topic);
                }
                std::string expected_data = _log["data"];

                EXPECT_EQ_OR_RETURN_FALSE(res_log.topics.size(), expected_topics.size());
                if (res_log.topics.size() == expected_topics.size()) {
                    auto topics_index = 0;
                    for (auto _topics : expected_topics) {
                        auto res_topic = top::to_hex_prefixed(res_log.topics[topics_index].asBytes());
                        EXPECT_EQ_OR_RETURN_FALSE(res_topic, expected_topics[topics_index]);
                    }
                }

                // EXPECT_EQ_OR_RETURN_FALSE(res_log.topics, expected_topics);
                std::string data_hex = res_log.data.size() > 0 ? top::to_hex_prefixed(res_log.data) : "";
                EXPECT_EQ_OR_RETURN_FALSE(data_hex, expected_data);

                index++;
            }
        }
    }
    return true;
}

NS_END3