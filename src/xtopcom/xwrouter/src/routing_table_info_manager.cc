// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xwrouter/multi_routing/routing_table_info_manager.h"

#include "assert.h"

namespace top {
namespace wrouter {

void RoutingTableInfoMgr::add_routing_table_info(common::xip2_t group_xip, uint64_t version, uint64_t height) {
    version &= (uint64_t)0x1FFFFFULL;  // height only use 21 bits.
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);

    if (routing_table_infos.find(group_xip) == routing_table_infos.end()) {
        routing_table_infos.insert(std::make_pair(group_xip, std::vector<std::pair<uint64_t, uint64_t>>{}));
    }
    routing_table_infos[group_xip].push_back(std::make_pair(version, height));
}
void RoutingTableInfoMgr::delete_routing_table_info(common::xip2_t group_xip, uint64_t version_or_blk_height) {
    version_or_blk_height &= (uint64_t)0x1FFFFFULL;  // height only use 21 bits.
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);

    if (routing_table_infos.find(group_xip) == routing_table_infos.end()) {
        assert(false);  // should exist. But make no damage if not.
        return;
    }

    auto & group_infos = routing_table_infos[group_xip];
    for (auto iter = group_infos.begin(); iter != group_infos.end(); iter++) {
        if ((base::now_service_type_ver == base::service_type_ver::service_type_height_use_version && version_or_blk_height == iter->first) ||
            (base::now_service_type_ver == base::service_type_ver::service_type_height_use_blk_height && version_or_blk_height == iter->second)) {
            group_infos.erase(iter);
            break;
        }
    }
}

bool RoutingTableInfoMgr::exist_routing_table_info(common::xip2_t group_xip, base::service_type_ver ver, uint64_t version_or_blk_height) {
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);
    if (routing_table_infos.find(group_xip) != routing_table_infos.end()) {
        auto const & group_infos = routing_table_infos[group_xip];
        for (auto iter = group_infos.begin(); iter != group_infos.end(); ++iter) {
            if ((ver == base::service_type_ver::service_type_height_use_version && version_or_blk_height == iter->first) ||
                (ver == base::service_type_ver::service_type_height_use_blk_height && version_or_blk_height == iter->second)) {
                return true;
            }
        }
    }
    return false;
}

std::pair<uint64_t, uint64_t> RoutingTableInfoMgr::get_routing_table_info(common::xip2_t group_xip, base::service_type_ver ver, uint64_t version_or_blk_height) {
    std::unique_lock<std::mutex> lock(routing_table_infos_mutex);
    if (routing_table_infos.find(group_xip) == routing_table_infos.end()) {
        return {};
    }
    auto const & group_infos = routing_table_infos[group_xip];
    for (auto iter = group_infos.begin(); iter != group_infos.end(); ++iter) {
        if ((ver == base::service_type_ver::service_type_height_use_version && version_or_blk_height == iter->first) ||
            (ver == base::service_type_ver::service_type_height_use_blk_height && version_or_blk_height == iter->second)) {
            return *iter;
        }
    }
    // assert(false);
    return {};
}

}  // namespace wrouter
}  // namespace top