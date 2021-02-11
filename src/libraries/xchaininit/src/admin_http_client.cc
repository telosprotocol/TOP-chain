#include "xchaininit/admin_http_client.h"

#include <iostream>
#include <fstream>
#include <chrono>
#include <thread>
#include <memory>

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

#include "xpbase/base/line_parser.h"
#include "xpbase/base/top_log.h"

namespace  top {

namespace admin {

AdminHttpClient::AdminHttpClient(const std::string& remote_ip, uint16_t remote_port) {
    std::string remote_host_port = remote_ip + ":" + std::to_string(remote_port);
    cli_ = std::make_shared<HttpClient>(remote_host_port);
    cli_->config.timeout = 40;

}

AdminHttpClient::AdminHttpClient(const std::string& remote_ip, uint16_t remote_port, uint32_t timeout) {
    std::string remote_host_port = remote_ip + ":" + std::to_string(remote_port);
    cli_ = std::make_shared<HttpClient>(remote_host_port);
    cli_->config.timeout = timeout;
}

AdminHttpClient::AdminHttpClient(const std::string& remote_ip, uint16_t remote_port, uint32_t timeout, const std::string& proxy) {
    std::string remote_host_port = remote_ip + ":" + std::to_string(remote_port);
    cli_ = std::make_shared<HttpClient>(remote_host_port);
    cli_->config.timeout = timeout;
    cli_->config.proxy_server = proxy;
}

AdminHttpClient::~AdminHttpClient() {
    cli_->stop();
    cli_ = nullptr;
}

// cmdlien: node help; node osinfo; ...
void AdminHttpClient::Request(const std::string& cmdline, std::string& result) {
    std::string path = "/api/command";

    json payload;
    payload["command"] = cmdline;
    // TODO(smaug) enable someday
    payload["token"] = "sometoken";
    std::string json_payload = payload.dump();

    SimpleWeb::CaseInsensitiveMultimap header;
    header.insert({"Content-Type", "application/json"});
    header.insert({"Connection", "keep-alive"});

    TOP_DEBUG("begin admin http request path:%s payload:%s", path.c_str(), json_payload.c_str());

    try {
        auto r = cli_->request("POST", path, json_payload, header);
        result = r->content.string();
        TOP_DEBUG("begin admin http request path:%s payload:%s response:%s", path.c_str(), json_payload.c_str(), result.c_str());
    } catch(const std::exception& e) {
        TOP_WARN("catch exception:%s", e.what());
        result = "topio not running, please  execute command 'topio node startnode' first.";
    }

    return;
}

} // end namespace admin

} // end namespace top
