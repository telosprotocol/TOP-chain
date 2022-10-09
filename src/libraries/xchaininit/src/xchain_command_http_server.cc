// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchaininit/xchain_command_http_server.h"

#include "xchaininit/xchain_info_query.h"

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

NS_BEG2(top, chain_command)

ChainCommandServer::ChainCommandServer(std::string const & server_ip, uint16_t server_port) : base_t{server_ip, server_port} {
}

void ChainCommandServer::Start() {
    register_json_request_callback("/api/command", xhttp::xhttp_server_method_type_t::POST, [](std::string const & json_req_content, std::string & json_resp_content) -> bool {
        json req_json;
        try {
            req_json = json::parse(json_req_content);
        } catch (json::parse_error & e) {
            std::string error = e.what();
            json res_content;
            res_content["status"] = "";
            res_content["error"] = error;

            json_resp_content = res_content.dump(4);  // dump(4)
            return false;
        }

        auto command = req_json["command"].get<std::string>();

        json res_content;
        std::string result;
        // cmdline: node help; node isjoined; node peers ... etc.
        if (ChainInfo::Instance()->ProcessCommand(command, result)) {
            res_content["status"] = "ok";
            res_content["error"] = "";
            res_content["data"] = json::object();
            res_content["data"][command] = result;
        } else {
            res_content["status"] = "fail";
            if (result.empty()) {
                res_content["error"] = "request failed";
            } else {
                res_content["error"] = result;
            }
        }

        json_resp_content = res_content.dump();  // dump(4)
        return true;
    });

    start_server();
}

NS_END2