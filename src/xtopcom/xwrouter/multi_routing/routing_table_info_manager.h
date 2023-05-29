// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "xcommon/xip.h"
#include "xpbase/base/kad_key/kadmlia_key.h"

#include <memory>
#include <mutex>
#include <unordered_map>

namespace top {
namespace wrouter {

class RoutingTableInfoMgr {
private:
    using height_value = uint64_t;
    // using version_value = uint64_t;

    std::unordered_map<common::xip2_t, std::vector<height_value>> routing_table_infos;
    mutable std::mutex routing_table_infos_mutex;

public:
    RoutingTableInfoMgr() = default;
    RoutingTableInfoMgr(RoutingTableInfoMgr const &) = delete;
    RoutingTableInfoMgr(RoutingTableInfoMgr &&) = delete;
    RoutingTableInfoMgr & operator=(RoutingTableInfoMgr const &) = delete;
    RoutingTableInfoMgr & operator=(RoutingTableInfoMgr &&) = delete;
    ~RoutingTableInfoMgr() = default;

public:
    void add_routing_table_info(common::xip2_t group_xip, uint64_t height);

    void delete_routing_table_info(common::xip2_t group_xip, uint64_t blk_height);

    uint64_t get_routing_table_info(common::xip2_t group_xip, uint64_t blk_height) const;

    bool exist_routing_table_info(common::xip2_t group_xip, uint64_t blk_height) const;

private:
};

}  // namespace wrouter
}  // namespace top
