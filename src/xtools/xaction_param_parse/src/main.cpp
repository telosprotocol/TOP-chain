#include "CLI11.hpp"
#include "../xaction_param_parse.h"


int main(int argc, char** argv) {
    xaction_param_parse_tool parse_tool;
    CLI::App parse_app{"parse tool to parse params."};

    // parse action param
    auto parse_param_cmd = parse_app.add_subcommand("parse_action_param", "get the content of param in action");
    std::string action_param_str;
    uint32_t action_type;
    parse_param_cmd->add_option("action_param_str", action_param_str, "the action param str")->required();
    parse_param_cmd->add_option("action_type", action_type, "the action type, default asset in(0) or out(6)")->default_val(0);
    parse_param_cmd->callback(std::bind(&xaction_param_parse_tool::parse_action_param, &parse_tool, std::ref(action_param_str), std::ref(action_type)));

    CLI11_PARSE(parse_app, argc, argv);
    return 0;

}