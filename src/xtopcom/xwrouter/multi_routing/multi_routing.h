// Copyright (c) 2017-2019 Telos Foundation & contributors
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#pragma once

#include <memory>
#include <mutex>
#include <map>

#include "xpbase/base/top_config.h"
#include "xpbase/base/top_timer.h"
#include "xkad/routing_table/routing_utils.h"
#include "xkad/routing_table/routing_table.h"
#include "xwrouter/register_routing_table.h"
#include "xkad/proto/kadmlia.pb.h"

namespace top {

namespace wrouter {

class RootRoutingManager;

class MultiRouting {
public:
    static MultiRouting* Instance();
    kadmlia::RoutingTablePtr GetRoutingTable(const uint64_t& type, bool root = false);
    kadmlia::RoutingTablePtr GetRoutingTable(const std::string& routing_id, bool root = false);
    void AddRoutingTable(uint64_t type, kadmlia::RoutingTablePtr routing_table);
    void RemoveRoutingTable(uint64_t type);


    void SetRootRoutingManager(std::shared_ptr<RootRoutingManager> root_manager_ptr);

private:
    friend std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const uint64_t& type, bool root);
    friend std::shared_ptr<kadmlia::RoutingTable> GetRoutingTable(const std::string& routing_id, bool root);

    friend void GetAllRegisterType(std::vector<uint64_t>& vec_type);
    friend void GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>>& vec_rt);


    friend bool GetServiceBootstrapRootNetwork(
        uint64_t service_type,
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints);
    friend bool SetCacheServiceType(uint64_t service_type);
    
    
    void RemoveAllRoutingTable();
    
    void GetAllRegisterType(std::vector<uint64_t>& vec_type);
    void GetAllRegisterRoutingTable(std::vector<std::shared_ptr<kadmlia::RoutingTable>>& vec_rt);

    kadmlia::RoutingTablePtr GetServiceRoutingTable(const uint64_t& type);
    kadmlia::RoutingTablePtr GetServiceRoutingTable(const std::string& routing_id);

    bool GetServiceBootstrapRootNetwork(
        uint64_t service_type,
        std::set<std::pair<std::string, uint16_t>>& boot_endpoints);
    bool SetCacheServiceType(uint64_t service_type);

    void CheckSingleNodeNetwork();
    void WaitCheckSignal();
    void NotifyCheckSignal();

    MultiRouting();
    ~MultiRouting();

    std::map<uint64_t, kadmlia::RoutingTablePtr> routing_table_map_;
    std::mutex routing_table_map_mutex_;
    std::shared_ptr<RootRoutingManager> root_manager_ptr_;

    // CheckSingleNodeNetwork is a block task, so use thread instead of timer
    std::mutex check_single_network_mutex_;
    std::condition_variable check_single_network_condv_;
    base::TimerRepeated timer_{base::TimerManager::Instance(), "MultiRouting"};
    std::shared_ptr<std::thread> check_single_network_thread_;

    DISALLOW_COPY_AND_ASSIGN(MultiRouting);
};

}  // namespace wrouter

}  // namespace top
