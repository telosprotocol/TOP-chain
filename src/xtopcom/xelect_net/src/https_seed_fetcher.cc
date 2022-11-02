// Copyright (c) 2017-2018 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xelect_net/include/https_seed_fetcher.h"

#include "xbase/xlog.h"

#include <string>
#include <vector>

// nlohmann_json
#include <nlohmann/json.hpp>
using json = nlohmann::json;

namespace top {

namespace elect {

HttpsSeedFetcher::HttpsSeedFetcher(std::string const & url) : base_t{url} {
}

bool HttpsSeedFetcher::GetSeeds(std::vector<std::string> & url_seeds) {
    std::string result;
    try {
        result = request_get();
        xinfo("[seeds] get result:%s", result.c_str());
    } catch (const std::exception & e) {
        xwarn("[seeds] catch exception:%s", e.what());
    }

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
    } catch (json::parse_error & e) {
        xwarn("[seeds] json parse error:%s", e.what());
        return false;
    }
    return true;
}

}  // end namespace elect

}  // end namespace top
