// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <vector>
#include <string>
#include <chrono>
#include <map>
#include <mutex>
#include <array>

#include "xkad/routing_table/node_info.h"
#include "xwrouter/multi_routing/net_node.h"


namespace top {

namespace wrouter {
static constexpr uint32_t SingleNetSuperNodeSize = 3;
using SingleNetSuperNodeDeq= std::array<kadmlia::NodeInfoPtr, SingleNetSuperNodeSize>;
using SuperNodeCacheMap = std::map<uint64_t, SingleNetSuperNodeDeq>;

class SuperNode final {
public:
    SuperNode() = default;
    SuperNode(const SuperNode&) = delete;
    SuperNode& operator=(const SuperNode&) = delete;
    SuperNode(SuperNode&&) = delete;
    SuperNode& operator=(SuperNode&&) = delete;

public:
    bool AddNode(
            const uint32_t& xnetwork_id,
            const uint64_t& service_type,
            const kadmlia::NodeInfoPtr& super_node) noexcept;
    bool CheckSuperNode(
            const uint32_t& xnetwork_id,
            const uint64_t& service_type,
            const std::string& super_node_id) const noexcept;
    void SuperRoutingNodes(
            const uint32_t& xnetwork_id,
            std::vector<kadmlia::NodeInfoPtr>& node_vec) noexcept;
    void CheckReplace(
            const uint32_t& xnetwork_id,
            const uint64_t& service_type,
            const std::array<NetNode, SingleNetSuperNodeSize>& node_ary,
            std::set<uint32_t>& exclude_index);
    void DumpSuperNode(
            const uint32_t& xnetwork_id,
            const uint64_t& service_type) const noexcept;
 

private:
    mutable std::mutex super_node_cache_map_mutex_;
    std::map<uint32_t, SuperNodeCacheMap> super_node_cache_map_;
};

}

}
