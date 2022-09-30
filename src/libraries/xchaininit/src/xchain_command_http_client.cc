// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xchaininit/xchain_command_http_client.h"

#include "xbase/xlog.h"

#include <nlohmann/json.hpp>

using json = nlohmann::json;

NS_BEG2(top, chain_command)

ChainCommandFetcher::ChainCommandFetcher(std::pair<std::string, uint16_t> const & http_ip_port) : base_t{http_ip_port} {
}

// cmdlien: node help; node osinfo; ...
void ChainCommandFetcher::Request(std::string const & cmdline, std::string & result) {
    std::string path = "/api/command";

    json payload;
    payload["command"] = cmdline;
    // TODO(smaug) enable someday
    payload["token"] = "sometoken";
    std::string json_payload = payload.dump();

    xdbg("[ChainCommandFetcher]http request path:%s payload:%s", path.c_str(), json_payload.c_str());

    try {
        result = request_post_json(path, json_payload);
        xdbg("[ChainCommandFetcher]http request path:%s payload:%s response:%s", path.c_str(), json_payload.c_str(), result.c_str());
    } catch (const std::exception & e) {
        xwarn("catch exception:%s", e.what());
        result = "topio not running, please  execute command 'topio node startnode' first.";
    }

    return;
}

NS_END2