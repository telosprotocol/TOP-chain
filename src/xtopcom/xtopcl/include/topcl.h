// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.
#pragma once

#include "api_method.h"
#include "task_dispatcher.h"
#include "trans_base.h"
#include "user_info.h"
#include "xbase/xns_macro.h"
#include "xcrypto/xckey.h"

//#include <readline/history.h>
//#include <readline/readline.h>
#include <stdio.h>
#include <stdlib.h>

#include <algorithm>
#include <csignal>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

NS_BEG2(top, xtopcl)

using ParamList = std::vector<std::string>;

class xtopcl final {
public:
    xtopcl();
    int redirect_cli_out(CLI::App & system_app, int argc, char** argv, std::ostringstream & out_str);
    bool parser_command(const std::string & cmd, ParamList & param_list);
    bool is_query_method(const xChainSDK::Command_type result, const std::string & method);

    void input_reader();
    bool do_command(ParamList & param_list, std::string & result);

    xChainSDK::ApiMethod api;

private:
    xChainSDK::Command_type filter_command(ParamList & param_list);
    void update_account(const xChainSDK::Command_type result, const ParamList & param_list, std::ostringstream & out_str);
    std::string trim(std::string s);
    std::string remove_surplus_spaces(const std::string & s);
};

NS_END2
