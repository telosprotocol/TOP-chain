// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelect_net/include/http_client.h"

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

namespace elect {

SeedHttpClient::SeedHttpClient(const std::string& url) {
    // http://gitee.com/smaugx/temp/raw/master/topseeds.json
    // http://gitee.com
    // http://gitee.com/
    auto pos = url.find("//");
    auto host_path = url.substr(pos + 2); // gitee.com/smaugx/temp/raw/master/topseeds.json
    auto path_pos = host_path.find("/");
    std::string host;
    if (path_pos == std::string::npos) {
        host = host_path;
        path_ = "/";
    } else {
        host = host_path.substr(0, path_pos); // gitee.com
        path_ = host_path.substr(path_pos);  //   path_ is  /smaugx/temp/raw/master/topseeds.json
    }

    TOP_INFO("[seeds] url:%s host:%s path:%s", url.c_str(), host.c_str(), path_.c_str());
    cli_ = std::make_shared<HttpClient>(host);
    cli_->config.timeout = 20;
}

SeedHttpClient::~SeedHttpClient() {
    cli_->stop();
    cli_ = nullptr;
}

void SeedHttpClient::Request(const std::string& path, std::string& result) {
    /*
    {
      "seeds": [
        {
          "ip": "192.168.1.1",
          "port": 9000
        },
        {
          "ip": "192.168.1.2",
          "port": 9000
        }
      ]
    }
    */

    try {
        auto r = cli_->request("GET", path);
        result = r->content.string();
        TOP_INFO("[seeds] get path:%s result:%s", path.c_str(), result.c_str());
    } catch(const std::exception& e) {
        TOP_WARN("[seeds] catch exception:%s", e.what());
    }

    return;
}

bool SeedHttpClient::GetSeeds(std::vector<std::string>& url_seeds) {
    std::string path = path_;
    std::string result;
    Request(path, result);

    if (result.empty()) {
        return false;
    }

    json response;
    try {
        response = json::parse(result);
        auto seeds = response["seeds"];
        for (json::iterator it = seeds.begin(); it != seeds.end(); ++it) {
            auto item = *it;
            auto ip = item["ip"].get<std::string>();
            auto port = item["port"].get<uint16_t>();
            std::string ip_port = ip + ":" + std::to_string(port);
            url_seeds.push_back(ip_port);
        }
    } catch (json::parse_error& e) {
        TOP_WARN("[seeds] json parse error:%s", e.what());
        return false;
    }
    return true;
}

bool SeedHttpClient::GetEdgeSeeds(std::vector<std::string>& url_seeds) {
    std::string path = path_;
    std::string result;
    Request(path, result);

    if (result.empty()) {
        return false;
    }

    json edge_seeds;
    try {
        edge_seeds = json::parse(result);
        for (auto seed : edge_seeds) {
            url_seeds.push_back(seed);
        }
    } catch (json::parse_error& e) {
        TOP_WARN("[edgeseeds] json parse error:%s", e.what());
        return false;
    }
    return true;
}

} // end namespace elect

} // end namespace top
