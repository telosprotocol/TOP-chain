#include "xpbase/base/endpoint_util.h"

#include <utility>
#include <string>

#include "xpbase/base/line_parser.h"
#include "xpbase/base/check_cast.h"
#include "xpbase/base/top_log.h"

namespace top {
namespace base {

void ParseEndpoints(
        const std::string& endpoints,
        std::set<std::pair<std::string, uint16_t>>& set_endpoints) {
    base::LineParser line_split(endpoints.c_str(), ',', endpoints.size());
    for (uint32_t i = 0; i < line_split.Count(); ++i) {
        base::LineParser ip_port(line_split[i], ':');
        if (ip_port.Count() != 2) {
            TOP_INFO("<blueshi> invalid endpoint: %s", line_split[i]);
            continue;
        }
        try {
            set_endpoints.insert(std::make_pair(ip_port[0], check_cast<uint16_t>(ip_port[1])));
        } catch (...) {
            TOP_INFO("<blueshi> invalid endpoint: %s", line_split[i]);
        }
    }
}

void ParseEndpoints(
        const std::string& endpoints,
        std::vector<std::string>& vec_endpoint) {
    std::set<std::pair<std::string, uint16_t>> set_endpoints;
    ParseEndpoints(endpoints, set_endpoints);

    ParseSetEndpoint(set_endpoints, vec_endpoint);
}

void ParseVecEndpoint(
        const std::vector<std::string>& vec_endpoint,
        std::set<std::pair<std::string, uint16_t>>& set_endpoint) {
    set_endpoint.clear();
    for (auto& ep_str : vec_endpoint) {
        base::LineParser ip_port(ep_str.c_str(), ':');
        if (ip_port.Count() != 2) {
            TOP_INFO("<blueshi> invalid endpoint: %s", ep_str.c_str());
            continue;
        }

        // TODO(blueshi): check ip/port format!
        try {
            set_endpoint.insert(std::make_pair(ip_port[0], check_cast<uint16_t>(ip_port[1])));
        } catch (...) {
            TOP_INFO("<blueshi> invalid endpoint: %s", ep_str.c_str());
        }
    }
}

void ParseSetEndpoint(
        const std::set<std::pair<std::string, uint16_t>>& set_endpoints,
        std::vector<std::string>& vec_endpoint) {
    vec_endpoint.clear();
    for (auto& ep : set_endpoints) {
        vec_endpoint.push_back(ep.first + ":" + std::to_string(ep.second));
    }
}

void MergeEndpoints(
        const std::set<std::pair<std::string, uint16_t>>& set_endpoint1,
        std::set<std::pair<std::string, uint16_t>>& set_endpoint2) {
    for (auto& ep : set_endpoint1) {
        set_endpoint2.insert(ep);
    }
}

std::string FormatVecEndpoint(const std::vector<std::string>& vec_endpoint) {
    if (vec_endpoint.empty()) {
        return "";
    }

    std::string ret = vec_endpoint[0];
    for (size_t i = 1; i < vec_endpoint.size(); ++i) {
        ret += "," + vec_endpoint[i];
    }
    return ret;
}

std::string FormatSetEndpoint(const std::set<std::pair<std::string, uint16_t>>& set_endpoints) {
    if (set_endpoints.empty()) {
        return "";
    }

    auto it = set_endpoints.begin();
    std::string ret = it->first + ":" + std::to_string(it->second);
    for (++it; it != set_endpoints.end(); ++it) {
        ret += "," + it->first + ":" + std::to_string(it->second);
    }
    return ret;
}

}  // namespace base
}  // namespace top
