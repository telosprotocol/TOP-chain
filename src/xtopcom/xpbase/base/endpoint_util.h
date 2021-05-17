
#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <set>
#include <utility>

namespace top {
namespace base {

void ParseEndpoints(
        const std::string& endpoints,
        std::set<std::pair<std::string, uint16_t>>& set_endpoints);

// endpoints to vector is sorted by ip
void ParseEndpoints(
        const std::string& endpoints,
        std::vector<std::string>& vec_endpoint);

void ParseVecEndpoint(
        const std::vector<std::string>& vec_endpoint,
        std::set<std::pair<std::string, uint16_t>>& set_endpoint);

// endpoints to vector is sorted by ip
void ParseSetEndpoint(
        const std::set<std::pair<std::string, uint16_t>>& set_endpoints,
        std::vector<std::string>& vec_endpoint);

void MergeEndpoints(
        const std::set<std::pair<std::string, uint16_t>>& set_endpoint1,
        std::set<std::pair<std::string, uint16_t>>& set_endpoint2);

std::string FormatVecEndpoint(const std::vector<std::string>& vec_endpoint);
std::string FormatSetEndpoint(const std::set<std::pair<std::string, uint16_t>>& set_endpoints);

}  // namespace base
}  // namespace top
