// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "topcl.h"

#include "global_definition.h"
#include "xcrypto.h"

#include <sstream>

NS_BEG2(top, xtopcl)

using namespace xChainSDK;
using std::cout;
using std::endl;

std::string xtopcl::trim(std::string s) {
    if (s.empty()) {
        return s;
    }
    s.erase(0, s.find_first_not_of(" "));
    s.erase(s.find_last_not_of(" ") + 1);
    return s;
}

std::string xtopcl::remove_surplus_spaces(const std::string & s) {
    std::string src = trim(s);
    std::string result = "";
    for (int i = 0; src[i] != '\0'; i++) {
        if (src[i] != ' ') {
            result.append(1, src[i]);
        } else {
            if (src[i + 1] != ' ')
                result.append(1, src[i]);
        }
    }
    return result;
}

xtopcl::xtopcl() {
    // api.set_userinfo();
    // std::cout << "Command Line Interface to TOP Client.\n";
    // std::cout << "  'help' for more information\n";
}

void xtopcl::input_reader() {
    return;
    /*
#ifndef _WIN32
    char * ch;
    while ((ch = readline(">>>")) != NULL) {
        if (strlen(ch) == 0)
            continue;
        add_history(ch);

        ParamList param_list;
        parser_command(ch, param_list);
        std::string result;
        do_command(param_list, result);
        std::cout << result << '\n';
        free(ch);
    }

#else
    std::string str;
    for (;;) {
        printf(">>>");
        fflush(stdout);
        if (!std::getline(std::cin, str)) {
            return;
        }
        ParamList param_list;
        parser_command(str, param_list);
        std::string result;
        do_command(param_list, result);
        std::cout << result << '\n';
    }
#endif
*/
}

bool xtopcl::parser_command(const std::string & cmd, ParamList & param_list) {
    std::string nc = remove_surplus_spaces(cmd);
    std::string param;
    auto it = nc.begin();
    for (; it != nc.end(); ++it) {
        if (*it != ' ') {
            param.push_back(*it);
        } else {
            param_list.push_back(param);
            param.clear();
        }
    }

    if (!param.empty()) {
        param_list.push_back(param);
    }
    return true;
}

int xtopcl::redirect_cli_out(CLI::App & app, int argc, char ** argv, std::ostringstream & out_str) {
    std::streambuf * const buffer = cout.rdbuf();
    auto osb = out_str.rdbuf();
    cout.rdbuf(osb);
    try {
        (app).parse(argc, argv);
    } catch (const CLI::ParseError & e) {
        (app).exit(e);
        cout.rdbuf(buffer);
        return -1;
    }
    cout.rdbuf(buffer);
    return 0;
}

bool xtopcl::do_command(ParamList & param_list, std::string & res) {
    std::ostringstream out_str;
    if (param_list.empty()) {
        out_str << "ERROR: please input command.\n";
        out_str << "\n";
        out_str << "Try 'help' for more information.\n";
        return false;
    }

    xChainSDK::Command_type result = filter_command(param_list);
    if (xChainSDK::Command_type::toplevel != result && param_list.empty()) {
        out_str << "ERROR: unknown command.\n";
        out_str << "\n";
        out_str << "Try 'help' for more information.\n";
        return false;
    }

    int ret = 0;
    std::string method = param_list.front();
    std::transform(method.begin(), method.end(), method.begin(), ::tolower);

    if (g_userinfo.account.size() == 0 && param_list.back() != COMMAND_HELP_STRING[0] && param_list.back() != COMMAND_HELP_STRING[1] &&
        (result == xChainSDK::Command_type::system || result == xChainSDK::Command_type::sendtransaction || result == xChainSDK::Command_type::get)) {
        std::vector<std::string> files = xChainSDK::xcrypto::scan_key_dir(g_keystore_dir);
        std::vector<std::string> accounts;
        for (auto file : files) {
            top::base::xvaccount_t _vaccount(file);
            if (_vaccount.is_unit_address())
            {
                accounts.push_back(file);
            }
        }
        if (accounts.size() < 1) {
            out_str << "You do not have a TOP account, please create an account." << std::endl;
            return false;
        }
        if (accounts.size() > 1) {
            out_str << "There are multiple accounts in your wallet, please set a default account first." << std::endl;
            return false;
        }
        std::string base64_pri = xChainSDK::xcrypto::import_existing_keystore(" ", g_keystore_dir + "/" + accounts[0]);
        if (base64_pri.size() == 0) {
            out_str << "There is no default account, please set the default account first!" << std::endl;
            return false;
        }
        if (xChainSDK::xcrypto::set_g_userinfo(base64_pri)) {
            out_str << g_userinfo.account << " has been set as the default account." << std::endl;
        } else {
            out_str << "Set default account failed." << std::endl;
        }
    }

    if (result == xChainSDK::Command_type::system || result == xChainSDK::Command_type::sendtransaction) {
        if (g_userinfo.account == "" && param_list.back() != COMMAND_HELP_STRING[0] && param_list.back() != COMMAND_HELP_STRING[1]) {
            if (!is_query_method(result, method)) {
                out_str << "Please create new public-private key pairs or set default account first!!!\n";
                return false;
            }
        }
    }

    auto func = api.get_method(method, out_str, result);
    if (func != nullptr) {
        // std::cout << "Do Command: " << method.c_str() << " ........" << std::endl;
        try {
            // get identity_token
            if (g_userinfo.identity_token.size() == 0) {
                if (result == xChainSDK::Command_type::system || result == xChainSDK::Command_type::sendtransaction || result == xChainSDK::Command_type::get) {
                    if (param_list.back() != COMMAND_HELP_STRING[0] && param_list.back() != COMMAND_HELP_STRING[1]) {
                        api.get_token();
                    }
                }
            }

            if (g_userinfo.account.size() == 0 && is_query_method(result, method)) {
                api_method_imp tmp;
                tmp.make_private_key(g_userinfo.private_key, g_userinfo.account);
            }

            // update account info: nonce, last_hash ...
            if (!is_query_method(result, method)) {
                update_account(result, param_list, out_str);
            }

            ret = func(param_list, out_str);
            api.reset_tx_deposit();
        } catch (std::invalid_argument & e) {
            out_str << "Error! " << e.what() << ": invalid argument" << std::endl;
        } catch (std::exception & e) {
            out_str << "Error! Exception Thrown " << e.what() << std::endl;
        }
    } else {
        out_str << "ERROR: " << method.c_str() << " --- Unknown Command." << std::endl;
        out_str << "Try 'help' for more information." << std::endl;
    }

    res = out_str.str();
    return 0 == ret;
}

bool xtopcl::is_query_method(const xChainSDK::Command_type result, const std::string & method) {
    if (result == xChainSDK::Command_type::get) {
        return true;
    }

    if (method == "querynodeinfo" || method == "querynodereward" || method == "queryvoterdividend" || method == "queryproposal") {
        return true;
    }

    return false;
}

void xtopcl::update_account(const xChainSDK::Command_type result, const ParamList & param_list, std::ostringstream & out_str) {
    if (result == xChainSDK::Command_type::system || result == xChainSDK::Command_type::sendtransaction) {
        if (param_list.back() != COMMAND_HELP_STRING[0] && param_list.back() != COMMAND_HELP_STRING[1]) {
            auto_query = true;
            // api.get_account_info(out_str);
            if (auto_query) {
                auto_query = false;
                // out_str << "Update Account Failed\n";
            } else {
                // out_str << "Update Account Successfully\n";
            }
        }
    }
}

xChainSDK::Command_type xtopcl::filter_command(ParamList & param_list) {
    xChainSDK::Command_type result = xChainSDK::Command_type::toplevel;

    if (param_list.front() == xChainSDK::COMMAND_LEVEL_FILTER[0]) {
        result = xChainSDK::Command_type::get;
    } else if (param_list.front() == xChainSDK::COMMAND_LEVEL_FILTER[1]) {
        result = xChainSDK::Command_type::system;
    } else if (param_list.front() == xChainSDK::COMMAND_LEVEL_FILTER[2]) {
        result = xChainSDK::Command_type::sendtransaction;
    } else if (param_list.front() == xChainSDK::COMMAND_LEVEL_FILTER[3]) {
        result = xChainSDK::Command_type::debug;
    } else if (param_list.front() == xChainSDK::COMMAND_LEVEL_FILTER[4]) {
        result = xChainSDK::Command_type::wallet;
    }

    if (xChainSDK::Command_type::toplevel != result) {
        if (param_list.size() > 1 && (xChainSDK::COMMAND_HELP_STRING[0] == param_list[1] || xChainSDK::COMMAND_HELP_STRING[1] == param_list[1])) {
            result = xChainSDK::Command_type::subcommands;
            return result;
        }

        param_list.erase(param_list.begin());
    }

    return result;
}

NS_END2
