// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/routing_table_info_manager.h"

#include <cassert>
#include <cinttypes>

namespace top {
namespace wrouter {

void RoutingTableInfoMgr::add_routing_table_info(common::xip2_t group_xip, uint64_t height) {
    xdbg("RoutingTableInfoMgr::add_routing_table_info group_xip: %s %" PRIu64, group_xip.to_string().c_str(), height);
    // version &= (uint64_t)0x1FFFFFULL;  // height only use 21 bits.
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);

    if (routing_table_infos.find(group_xip) == routing_table_infos.end()) {
        routing_table_infos.emplace(group_xip, std::vector<uint64_t>{});
    }
    routing_table_infos[group_xip].push_back(height);

    if (routing_table_infos[group_xip].size() > 2) {
        routing_table_infos[group_xip].erase(routing_table_infos[group_xip].begin());
    }
}
void RoutingTableInfoMgr::delete_routing_table_info(common::xip2_t group_xip, uint64_t version_or_blk_height) {
    // version_or_blk_height &= (uint64_t)0x1FFFFFULL;  // height only use 21 bits.
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);

    if (routing_table_infos.find(group_xip) == routing_table_infos.end()) {
        assert(false);  // should exist. But make no damage if not.
        return;
    }

    auto & group_infos = routing_table_infos[group_xip];
    for (auto iter = group_infos.begin(); iter != group_infos.end(); ++iter) {
        if (version_or_blk_height == *iter) {
            group_infos.erase(iter);
            break;
        }
    }
}

bool RoutingTableInfoMgr::exist_routing_table_info(common::xip2_t group_xip, uint64_t blk_height) const {
    // version_or_blk_height &= (uint64_t)0x1FFFFFULL;  // height only use 21 bits.
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);
    if (routing_table_infos.find(group_xip) != routing_table_infos.end()) {
        auto const & group_infos = routing_table_infos.at(group_xip);
        for (auto iter = group_infos.begin(); iter != group_infos.end(); ++iter) {
            if (blk_height == *iter) {
                return true;
            }
        }
    }
    return false;
}

uint64_t RoutingTableInfoMgr::get_routing_table_info(common::xip2_t group_xip, uint64_t blk_height) const {
    // version_or_blk_height &= (uint64_t)0x1FFFFFULL;  // height only use 21 bits.
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);
    if (routing_table_infos.find(group_xip) == routing_table_infos.end()) {
        return {};
    }
    auto const & group_infos = routing_table_infos.at(group_xip);
    for (auto iter = group_infos.begin(); iter != group_infos.end(); ++iter) {
        if (blk_height == *iter) {
            return *iter;
        }
    }
    // assert(false);
    return {};
}

}  // namespace wrouter
}  // namespace top