
#include <dirent.h>

#include <cstdio>
#include <exception>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#define private public

#include "tests/xevm_engine_test/evm_test_fixture/xtest_evm_fixture.h"
#include "xbasic/xhex.h"
#include "xdata/xnative_contract_address.h"

#define MYEXPECT_TRUE(expr)                                                                                                                                                        \
    {                                                                                                                                                                              \
        EXPECT_TRUE(expr);                                                                                                                                                         \
        if ((expr) == false)                                                                                                                                                       \
            return false;                                                                                                                                                          \
    }

#define MYEXPECT_EQ(expra, exprb)                                                                                                                                                  \
    {                                                                                                                                                                              \
        EXPECT_EQ(expra, exprb);                                                                                                                                                   \
        if (expra != exprb) {                                                                                                                                                      \
            return false;                                                                                                                                                          \
        }                                                                                                                                                                          \
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

NS_BEG4(top, contract_runtime, evm, tests)

void xtest_evm_fixture::clean_env() {
    deployed_contract_map.clear();
    static_cast<top::evm::tests::xmock_evm_statectx *>(statestore.get())->m_mock_bstate.clear();
}

void xtest_evm_fixture::init_env() {
    deployed_contract_map.insert({"eth_bridge", "0xff00000000000000000000000000000000000002"});
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
    std::cout << "==================================" << std::endl;
    for (auto p : res) {
        std::cout << "-- [DO TEST] file: " << p << std::endl;
        init_env();
        execute_test_case(p);
        // std::cout << "-- finish test_file: " << p << std::endl;
        clean_env();
    }
    std::cout << "==================================" << std::endl;

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

    auto pre_data = j["pre_data"];
    auto deploy_list = j["deploy_contract"];
    auto testcases_list = j["test_cases"];
    m_summary.deploy_cases_num += deploy_list.size();
    m_summary.call_cases_num += testcases_list.size();

    // std::map<account_id, std::string> pre_data = j.at("pre_data").get<std::map<account_id, std::string>>();
    if (!pre_data.empty()) {
        std::string default_token_type = "ETH";
        auto storage = top::make_unique<evm::xevm_storage>(statestore, default_token_type);
        std::shared_ptr<top::evm::xevm_logic_face_t> logic_ptr = std::make_shared<top::contract_runtime::evm::xevm_logic_t>(
            std::move(storage),
            top::make_observer(statestore.get()),
            nullptr,
            top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance()));
        top::evm::evm_import_instance::instance()->add_evm_logic(logic_ptr);

        for (auto const & each : pre_data) {
            common::xaccount_address_t account_address{each["account"]};
            auto balances = each["balances"].get<std::map<std::string, std::string>>();
            for (auto const & balance : balances) {
                evm_common::u256 mock_value_256{balance.second};
                mock_add_balance(account_address, balance.first, mock_value_256);
            }

            if (each.contains("spenders")) {
                auto const & spenders_data = each["spenders"];
                for (auto const & spender_data : spenders_data) {
                    common::xaccount_address_t spender{spender_data["spender"]};
                    auto approve_amounts = spender_data["approve"].get<std::map<std::string, std::string>>();
                    for (auto const & approve : approve_amounts) {
                        evm_common::u256 value_256{approve.second};
                        mock_add_approve(account_address, spender, approve.first, value_256);
                    }
                }
            }
        }
        top::evm::evm_import_instance::instance()->remove_evm_logic();
    }

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

bool xtest_evm_fixture::do_deploy_test(json const & each_deploy) {
    account_id src_address = each_deploy["src_address"];
    std::string code_file = each_deploy["code_file_path"];
    uint64_t gas_limit = each_deploy["gas_limit"];
    std::string value = each_deploy["value"];
    evm_common::u256 value_256{value};
    // std::cout << "get value_256:" << value_256 << std::endl;

    auto expected = each_deploy["expected"];
    std::string contract_name_symbol = expected["extra_message"];

    auto evm_action = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::evm>>(
        common::xaccount_address_t{src_address}, eth_zero_address, value_256, get_contract_bin(code_file), gas_limit);

    auto contract_manager = top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance());

    top::evm::xtop_evm evm{contract_manager, statestore};
    auto action_result = evm.execute_action(std::move(evm_action), vm_param);

    account_id contract_address = action_result.extra_msg;
    deployed_contract_map[contract_name_symbol] = contract_address;

    uint32_t expected_result = expected["status"];
    MYEXPECT_EQ(action_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    MYEXPECT_EQ(action_result.used_gas, expected_gas_used);

    // std::string expected_extra_msg = expected["extra_message"];
    // EXPECT_EQ(action_result.extra_msg, expected_extra_msg);

    // std::cout << "tx output: " << action_result.dump_info() << std::endl;
    MYEXPECT_TRUE(expected_logs(action_result.logs, expected["logs"]));

    m_summary.succ_deploy_cases += 1;
    return true;
}
bool xtest_evm_fixture::do_call_test(json const & each_call) {
    if (each_call.empty()) {
        return false;
    }

    account_id src_address = each_call["src_address"];

    std::string contract_name_symbol = each_call["target_address"];
    MYEXPECT_TRUE(deployed_contract_map.find(contract_name_symbol) != deployed_contract_map.end());
    account_id target_address = deployed_contract_map[contract_name_symbol];

    uint64_t gas_limit = each_call["gas_limit"];
    std::string value = each_call["value"];
    evm_common::u256 value_256{value};
    // std::cout << "get value_256:" << value_256 << std::endl;
    std::string input_data = each_call["data"];

    std::error_code ec;
    auto contract_code_bytes = top::from_hex(input_data, ec);
    EXPECT_TRUE(!ec);

    auto evm_action = top::make_unique<data::xconsensus_action_t<data::xtop_action_type_t::evm>>(
        common::xaccount_address_t{src_address}, common::xaccount_address_t{evm_to_top_address(target_address)}, value_256, contract_code_bytes, gas_limit);

    auto contract_manager = top::make_observer<contract_runtime::evm::xevm_contract_manager_t>(contract_runtime::evm::xevm_contract_manager_t::instance());

    top::evm::xtop_evm evm{contract_manager, statestore};
    auto action_result = evm.execute_action(std::move(evm_action), vm_param);

    auto expected = each_call["expected"];
    uint32_t expected_result = expected["status"];
    MYEXPECT_EQ(action_result.status, expected_result);

    uint64_t expected_gas_used = expected["gas_used"];
    MYEXPECT_EQ(action_result.used_gas, expected_gas_used);

    std::string expected_extra_msg = expected["extra_message"];
    if (!expected_extra_msg.empty()) {
        EXPECT_EQ(action_result.extra_msg, expected_extra_msg);
    }

    MYEXPECT_TRUE(expected_logs(action_result.logs, expected["logs"]));

    m_summary.succ_call_cases += 1;
    return true;
}

xbytes_t xtest_evm_fixture::get_contract_bin(std::string const & code_file_path) {
    std::ifstream code_file_stream(current_json_file_directory + code_file_path);
    std::string bytecode_hex_string;
    code_file_stream >> bytecode_hex_string;

    std::error_code ec;
    auto code_bytes = top::from_hex(bytecode_hex_string, ec);
    EXPECT_TRUE(!ec);
    return code_bytes;
}

bool xtest_evm_fixture::expected_logs(std::vector<evm_common::xevm_log_t> const & result_logs, json const & expected_json) {
    // std::cout << expected_json << std::endl;
    if (expected_json.empty()) {
        MYEXPECT_TRUE(result_logs.empty());
    } else {
        MYEXPECT_EQ(result_logs.size(), expected_json.size());
        if (result_logs.size() == expected_json.size()) {
            auto index = 0;
            for (auto _log : expected_json) {
                auto res_log = result_logs[index];

                std::string contract_name_symbol = _log["address"];
                MYEXPECT_TRUE(deployed_contract_map.find(contract_name_symbol) != deployed_contract_map.end());
                MYEXPECT_EQ(res_log.address.to_hex_string(), deployed_contract_map[contract_name_symbol]);

                std::vector<std::string> expected_topics;
                for (auto _each_topic : _log["topics"]) {
                    expected_topics.push_back(_each_topic);
                }
                std::string expected_data = _log["data"];

                MYEXPECT_EQ(res_log.topics.size(), expected_topics.size());
                if (res_log.topics.size() == expected_topics.size()) {
                    auto topics_index = 0;
                    for (auto _topics : expected_topics) {
                        auto res_topic = top::to_hex_prefixed(res_log.topics[topics_index].asBytes());
                        MYEXPECT_EQ(res_topic, expected_topics[topics_index]);
                    }
                }

                // MYEXPECT_EQ(res_log.topics, expected_topics);
                std::string data_hex = res_log.data.size() > 0 ? top::to_hex_prefixed(res_log.data) : "";
                MYEXPECT_EQ(data_hex, expected_data);

                index++;
            }
        }
    }
    return true;
}

void xtest_evm_fixture::mock_add_balance(common::xaccount_address_t const & account, std::string token_symbol, evm_common::u256 amount) {
    assert(account.to_string().substr(0, 6) == base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN);
    std::string eth_address = account.to_string().substr(6);
    assert(eth_address.size() == 40);

    auto state = statestore->load_unit_state(account);
    if (token_symbol != top::data::XPROPERTY_ASSET_TOP) {
        state->set_tep_balance(top::common::token_id(common::xsymbol_t{token_symbol}), amount);
    } else {
        state->token_deposit(data::XPROPERTY_BALANCE_AVAILABLE, static_cast<top::base::vtoken_t>(amount.convert_to<uint64_t>()));
    }
}

void xtest_evm_fixture::mock_add_approve(common::xaccount_address_t const & owner,
                                         common::xaccount_address_t const & spender,
                                         std::string const & symbol,
                                         evm_common::u256 amount) {
    assert(owner.to_string().substr(0, 6) == base::ADDRESS_PREFIX_EVM_TYPE_IN_MAIN_CHAIN);
    std::string eth_address = owner.to_string().substr(6);
    assert(eth_address.size() == 40);

    std::error_code ec;
    auto state = statestore->load_unit_state(owner);
    if (symbol != top::data::XPROPERTY_ASSET_TOP) {
        if (symbol == "tUSDT") {
            state->approve(common::xtoken_id_t::usdt, spender, amount, ec);
        } else if (symbol == "tUSDC") {
            state->approve(common::xtoken_id_t::usdc, spender, amount, ec);
        } else if (symbol == "ETH") {
            xerror("ETH is not allowed to approve");
        }
    } else {
        state->approve(common::xtoken_id_t::top, spender, amount, ec);
    }

    assert(!ec);
}

NS_END4
